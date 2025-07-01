#include <WiFi.h>
#include <time.h>

// libraries for RFID
#include "Rfid.h"

// libraries for Feeder and WeightSensor
#include "WeightSensor.h"
#include "Feeder.h"

const char* SSID = "SK_WiFiGIGA";
const char* PASSWORD = "123456";

// RFID function configuration
const int PIN_SS = 5;       // RC522 SDA
const int PIN_RST = 22;      // RC522 RST
const int PIN_SCK_RFID = 18;   // RC522 SCK
const int PIN_MOSI_RFID = 19;  // RC522 MOSI
const int PIN_MISO_RFID = 23;  // RC522 MISO
const int PIN_SERVO_DOOR = 13;      // 서보모터

const String list[] = {"6c 1e b2 01" }; // 파란색 키링

// feeding function configuration
const int PIN_DOUT = 26;
const int PIN_SCK = 27;
const float CAL_FACTOR = 3352.05;

const int PIN_MOTOR_FEED = 25;
const unsigned long MAX_RUN_MS = 20'000;

// 급식시간 설정
struct FeedTask { uint8_t hour, minute; float target_g; };
FeedTask tasks[] = { {8,0,120},{18,30,80} };
bool task_done[sizeof(tasks) / sizeof(tasks[0])] = { false };

WeightSensor scale(PIN_DOUT, PIN_SCK, CAL_FACTOR);
Feeder       feeder(PIN_MOTOR_FEED, MAX_RUN_MS);
RFID         rfid;

void setup() {
    Serial.begin(115200);
    scale.begin();
    feeder.begin();
    rfid.begin(); // RFID 리더 초기화

    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(200);
    configTime(9 * 3600, 0, "pool.ntp.org");
}

void loop() {
    struct tm now;
    if (!getLocalTime(&now)) { delay(1000); return; }

    for (int i = 0; i < (int)(sizeof(tasks) / sizeof(tasks[0])); i++) {
		if (task_done[i]) continue; // 이미 수행한 작업은 건너뜀
        if (now.tm_hour == tasks[i].hour && now.tm_min == tasks[i].minute) {
            feeder.dispense(tasks[i].target_g, scale);
            
            // RFID 태그 인식 대기 인식되면 도어 오픈
            while (scale.getWeightAvg() <= 0.5) {
                rfid.scan();
                delay(50);
            }
            task_done[i] = true;
        }
    }

    if (now.tm_hour == 0 && now.tm_min == 0) {
        memset(task_done, 0, sizeof(task_done));
    }

    delay(5000);
}
