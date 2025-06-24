// Project: RFID-Based Automatic Pet Feeder
// │   ├── WeightSensor.h
// │   ├── WeightSensor.cpp
// │   ├── DoorControl.h
// │   ├── DoorControl.cpp
// │   ├── AppInterface.h
// │   └── AppInterface.cpp
// ├── lib/           // Third-party or custom libraries
// └── platformio.ini // or Arduino project file

//----------------------------------------
// src/main.ino
//----------------------------------------
#include "Config.h"
#include "RFIDHandler.h"
#include "Feeder.h"
#include "WeightSensor.h"
#include "DoorControl.h"
#include "AppInterface.h"

RFIDHandler rfid;
Feeder feeder;
WeightSensor scale;
DoorControl door;
AppInterface app;

void setup() {
  Serial.begin(115200);
  rfid.begin();        // Initialize RFID reader
  scale.begin();       // Initialize HX711
  feeder.begin();      // Initialize motor/servo
  door.begin();        // Initialize servo for door
  app.begin();         // Initialize Wi-Fi and app data fetch
}

void loop() {
  app.update();        // Fetch latest feeding schedules & commands
  String tag = rfid.readTag();
  if (tag.length() > 0 && app.isFeedingTime(tag)) {
    door.open();       // Open only if correct pet
    feeder.dispense(app.getPortion(tag));
    // Monitor weight
    while (!scale.isTargetReached(app.getPortion(tag))) {
      delay(100);
    }
    feeder.stop();
    door.close();
    app.sendReport(tag, scale.getRemaining());
  }
  delay(200);
}