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

    // ❶ 태그가 감지되고 UID 읽기에 성공하면…
    if (reader_.PICC_IsNewCardPresent() && reader_.PICC_ReadCardSerial()) {
        String tagID = convertUidToString();
        
        // (UID 문자열 빌드 + isInList 검사)
        if (IsInList(tagID) && doorState_ == CLOSED) {
            Serial.println("인증된 태그입니다: " + tagID);
            doorServo_.write(90);
            doorState_ = OPEN;
            doorAngle_ = 90;
            closetime_ = now + 5000;  // 5초 후에 닫기
            reader_.PICC_HaltA();
            return false; // 문이 열렸지만, 닫힌 것은 아니므로 false 반환
        }
    }

    // ❹ (위의 return을 만나지 않은 경우만) doorOpen == true, 닫기 시간 지나면 “닫기” 수행
    if (doorState_ == OPEN && now > closetime_) {
        Serial.println("문을 닫습니다...");
        doorState_ = CLOSING;
        lastStepTime_ = now;
    }

    if (doorState_ == CLOSING) {
        // 90도에서 0도로 천천히 닫기
        if (now - lastStepTime_ >= stepInterval_) {
            lastStepTime_ = now;
            doorAngle_ -= 10;
            if (doorAngle_ < 0) {
                doorAngle_ = 0;
            }
            doorServo_.write(doorAngle_);
            if (doorAngle_ == 0) {
                Serial.println("문이 닫혔습니다...");
                doorState_ = CLOSED;
                return true;
            }
        }
    }
    return false; // 그 외의 경우는 모두 false 반환
}

// refactoring : static 지역 변수 대신 멤버 변수로 doorOpen closetime을 사용하여
//               객체 상태를 유지하도록 변경