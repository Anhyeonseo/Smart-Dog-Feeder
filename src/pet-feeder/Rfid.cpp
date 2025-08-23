#include "Rfid.h"

RFID::RFID(uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID) 
    : reader_(ssPin, rstPin), doorServo_(), ssPin_(ssPin), rstPin_(rstPin), servoPin_(servoPin), AUTH_TAG(AUTH_ID), 
    doorState_(CLOSED), doorAngle_(0), lastStepTime_(0), closetime_(0) {}

void RFID::begin() {
    SPI.begin(                 
        Config::RFID::RFID_SCK_PIN,
        Config::RFID::RFID_MISO_PIN,
        Config::RFID::RFID_MOSI_PIN,
        ssPin_
    );
    reader_.PCD_Init();
    doorServo_.attach(servoPin_);
    doorServo_.write(0);  // 서보 초기 위치
} 

bool RFID::IsInList(const String& id) {
        if (id == AUTH_TAG) return true;
        return false;
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
  
bool RFID::scan() {
    unsigned long now = millis();
    // 1) 같은 태그가 계속 있는 동안에도 주기적으로 true가 나오도록 ReadCardSerial()만 사용
    if (reader_.PICC_ReadCardSerial()) {
        String tagID = convertUidToString();

        if (IsInList(tagID)) {
            closetime_ = now + 5000;  // 필요하면 보유시간(ms) 조절

            // 닫히는 중이거나 닫혀 있으면 즉시 연 상태로 전환
            if (doorState_ != OPEN) {
                Serial.println("인증된 태그 감지 → 문을 엽니다");
                doorServo_.write(90);
                doorAngle_ = 90;
                doorState_ = OPEN;
            }
            return false; // 열린 상태 유지 중
        }
    }

    // 2) 태그가 떠난 뒤 보유시간이 지나면 닫기 시작
    if (doorState_ == OPEN && now > closetime_) {
        Serial.println("문을 닫습니다...");
        doorState_ = CLOSING;
        lastStepTime_ = now;
    }

    // 3) 점진적으로 닫기
    if (doorState_ == CLOSING) {
        if (now - lastStepTime_ >= stepInterval_) {
            lastStepTime_ = now;
            doorAngle_ -= 10;
            if (doorAngle_ < 0) doorAngle_ = 0;
            doorServo_.write(doorAngle_);

            if (doorAngle_ == 0) {
                Serial.println("문이 닫혔습니다...");
                doorState_ = CLOSED;
                return true;   // 닫힘
            }
        }
    }

    return false; // 그 외의 경우
}


// refactoring : static 지역 변수 대신 멤버 변수로 doorOpen closetime을 사용하여
//               객체 상태를 유지하도록 변경
