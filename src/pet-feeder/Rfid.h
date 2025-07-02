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
    void begin();
    bool IsInList(const String& id);
    String convertUidToString();
    void scan();

private:
    MFRC522 reader_;
    Servo   doorServo_;
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