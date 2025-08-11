#ifndef RFID_H
#define RFID_H
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include "config.h"

class RFID {
public:
    RFID();
    RFID(uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID); // 이놈들은 핀번호 인스턴스마다 다르게 해야함
    void begin();
    bool IsInList(const String& id);
    String convertUidToString();
    bool scan();

private:
    MFRC522 reader_;
    Servo   doorServo_;
    uint8_t ssPin_, rstPin_, servoPin_; 
    const String AUTH_TAG; // 승인된 RFID 태그 ID
};
#endif

/*
추후 사용 예시
void loop() {
  for (auto &u : units) {
    u.rfid.scan();
  }
  delay(50);
}
*/