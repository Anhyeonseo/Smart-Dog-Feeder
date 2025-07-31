#include "Rfid.h"

RFID::RFID(uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID) 
    : reader_(ssPin, rstPin), ssPin_(ssPin), rstPin_(rstPin), servoPin_(servoPin), AUTH_TAG(AUTH_ID) {}

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
  
void RFID::scan() {
    static bool doorOpen = false;
    static unsigned long closetime = 0;
    // ❶ 태그가 감지되고 UID 읽기에 성공하면…
    if (reader_.PICC_IsNewCardPresent() && reader_.PICC_ReadCardSerial()) {
        
        Serial.print("RFID 태그 감지: ");
        
        String tagID = convertUidToString();  // UID 문자열로 변환 추후 메모리 부족 시 byte 배열 비교로 전환
        // (UID 문자열 빌드 + isInList 검사)
        if (IsInList(tagID)) {
            Serial.println("인증된 태그입니다: " + tagID);
            // ❷ doorOpen == false 면 처음 한 번만 “열기” 수행
            if (!doorOpen) {
                Serial.println("문을 엽니다...");
                // doorSerco.attach(Config::RFID::SERVO_PIN);
                doorServo_.write(90);
                doorOpen = true;
            }
            // ❸ 여기서 return; 하므로 밑의 “닫기” 코드는 절대 실행 안 됨
            closetime = millis() + 5000;  // 5초 후에 닫기
            reader_.PICC_HaltA();
            return;
        }
    }

    // ❹ (위의 return을 만나지 않은 경우만) doorOpen == true, 닫기 시간 지나면 “닫기” 수행
    if (doorOpen && millis() > closetime) {
        Serial.println("문을 닫습니다...");
        doorServo_.write(0);
        // doorServo.detach();
        doorOpen = false;
    }
}

// refactoring : static 지역 변수 대신 멤버 변수로 doorOpen과 closetime을 사용하여
//               객체 상태를 유지하도록 변경