#include "Rfid.h"

RFID::RFID(uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID)
    : reader_(ssPin, rstPin),
      ssPin_(ssPin), rstPin_(rstPin), servoPin_(servoPin),
      AUTH_TAG(AUTH_ID) {}

void RFID::begin() {
    // 1) SS/RST 안정화
    pinMode(ssPin_, OUTPUT);
    digitalWrite(ssPin_, HIGH);   // 부팅시 SS 부동 방지
    pinMode(rstPin_, OUTPUT);
    digitalWrite(rstPin_, HIGH);  // 다수 보드에서 RST 풀업 필요

    // 2) SPI 시작 (배선과 정확히 일치해야 함)
    SPI.begin(
        Config::RFID::RFID_SCK_PIN,
        Config::RFID::RFID_MISO_PIN,
        Config::RFID::RFID_MOSI_PIN,
        ssPin_
    );
    delay(50);

    // 3) MFRC522 초기화 + 안테나 구동 + 수신 이득 최대
    reader_.PCD_Init();
    reader_.PCD_AntennaOn();
    reader_.PCD_SetAntennaGain(MFRC522::RxGain_max);

    // 4) 칩 응답 확인(디버그) — 여기서 멈추면 SPI/핀매핑 문제
    reader_.PCD_DumpVersionToSerial();

    // 5) 서보 초기 세팅
    doorServo_.attach(servoPin_);
    doorServo_.write(0);
    doorState_ = CLOSED;
    doorAngle_ = 0;
    lastStepTime_ = millis();
    closeDeadlineMs_ = 0;
    justClosed_ = false;

    Serial.println("[RFID] init complete (SS HIGH, RST HIGH, Antenna ON, RxGain MAX)");
}

bool RFID::IsInList(const String& id) {
    return (id == AUTH_TAG);
}

String RFID::convertUidToString() {
    String uidStr = "";
    for (byte i = 0; i < reader_.uid.size; i++) {
        if (reader_.uid.uidByte[i] < 0x10) uidStr += "0";
        uidStr += String(reader_.uid.uidByte[i], HEX);
        if (i != reader_.uid.size - 1) uidStr += " ";
    }
    return uidStr;
}

void RFID::setManualOverride(bool active) {
    manualOverrideActive_ = active;
    unsigned long now = millis();
    if (active) {
        beginOpening_(now);             // 수동으로 누르면 천천히 열고
        closeDeadlineMs_ = 0;           // 자동 닫힘 타이머 무시
    } else {
        beginClosing_(now);             // 다시 누르면 천천히 닫기
    }
}

// --- 내부 유틸리티 --------------------------------------------------------
void RFID::beginOpening_(unsigned long now) {
    doorState_ = OPENING;
    lastStepTime_ = now;
    closeLogArmed_ = true;
}

void RFID::beginClosing_(unsigned long now) {
    doorState_ = CLOSING;
    lastStepTime_ = now;
}

void RFID::updateMotion_(unsigned long now) {
    if (doorState_ == OPENING) {
        if (now - lastStepTime_ >= stepIntervalMs_) {
            lastStepTime_ = now;
            doorAngle_ += angleStep_;
            if (doorAngle_ > 66) doorAngle_ = 66;
            doorServo_.write(doorAngle_);
            if (doorAngle_ >= 66) {
                doorState_ = OPEN;
            }
        }
    } else if (doorState_ == CLOSING) {
        if (now - lastStepTime_ >= stepIntervalMsDown_) {
            lastStepTime_ = now;
            doorAngle_ -= angleStep_;
            if (doorAngle_ < 0) doorAngle_ = 0;
            doorServo_.write(doorAngle_);
            if (doorAngle_ == 0) {
                doorState_ = CLOSED;
                justClosed_ = true;
            }
        }
    }
}

void RFID::beginMonitoring() {
    justClosed_ = false;
    closeDeadlineMs_ = 0;
    lastScanTryMs_ = 0;
    closeLogArmed_ = true;
}

void RFID::tickMotionOnly() {
    // RFID 읽기 없이 서보 애니메이션만 진행
    updateMotion_(millis());
    // 여기선 justClosed_를 소비하지 않음 (MONITORING에서만 의미있게 true 반환)
}

void RFID::resetRFIDSession() {
    // 남아 있을 수 있는 카드 통신 상태 정리
    reader_.PICC_HaltA();
    reader_.PCD_StopCrypto1();
    reader_.PCD_AntennaOn();     // 혹시라도 꺼져있다면 재가동
    // 다음 스캔을 바로 하도록 스로틀/타이머 재장전
    lastScanTryMs_   = 0;
    closeDeadlineMs_ = 0;
    justClosed_      = false;
    closeLogArmed_   = true;
}

// --- 메인 루프용 ----------------------------------------------------------
bool RFID::scan() {
    const unsigned long now = millis();

    // 1) 문 애니메이션은 항상 갱신
    updateMotion_(now);

    // 2) 수동 오버라이드면 RFID는 읽지 않고 자동 닫힘도 안 함
    if (manualOverrideActive_) {
        if (justClosed_) { justClosed_ = false; return true; }  
        return false;
    }

    // 3) 스캔 주기 스로틀
    if (now - lastScanTryMs_ < scanIntervalMs_) {
        if (justClosed_) { justClosed_ = false; return true;}
        return false;
    }
    lastScanTryMs_ = now;
    // 4) "새 카드" 존재 여부부터 확인 (첫 읽기 안정성 ↑)
    bool detected = false;
    
    // 4-1) 권장 플로우: 새 카드 감지 후 읽기
    if (reader_.PICC_IsNewCardPresent() && reader_.PICC_ReadCardSerial()) {
        detected = true;
    }
    // 4-2) 폴백: 새 카드 감지가 False라도, 필드 안의 태그를 직접 읽어본다
    else if (reader_.PICC_ReadCardSerial()) {
        detected = true;
    }

    if (detected) {
        String tagID = convertUidToString();
        // (세션 정리 필수)
        //if (doorState_ != OPEN && doorState_ != OPENING) {
        //    reader_.PICC_HaltA();
        //    reader_.PCD_StopCrypto1();
        //}

        if (IsInList(tagID)) {
            // 마지막 감지 기준 10초 연장
            closeDeadlineMs_ = now + holdAfterLastDetectMs_;
            closeLogArmed_ = true;

            Serial.print("[RFID] AUTH 유지 연장, next close at +");
            Serial.print(holdAfterLastDetectMs_); Serial.println(" ms");

            // 닫힘/닫는중이었다면 천천히 열기 시작
            if (doorState_ == CLOSED || doorState_ == CLOSING) {
                Serial.println("[RFID] 인증 태그 감지 → 문을 천천히 엽니다");
                beginOpening_(now);
            }
        } 
    } else {
        // 태그를 못 읽었을 때: 열린 상태면 10초 타임아웃만 체크
        if (doorState_ == OPEN && closeDeadlineMs_ > 0 && now > closeDeadlineMs_) {
            if (closeLogArmed_) {
                Serial.println("[RFID] 연속 미감지 10초 → 문을 닫습니다");
                closeLogArmed_ = false;
            }
            reader_.PICC_HaltA();
            reader_.PCD_StopCrypto1();
            beginClosing_(now);
        }
    }
    if (justClosed_) { justClosed_ = false; return true; }
    return false;
}