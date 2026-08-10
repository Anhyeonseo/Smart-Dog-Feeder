#include <WiFi.h>
#include <time.h>
#include "WeightSensor.h"
#include "Feeder.h"

const char* SSID = "SK_WiFiGIGA";
const char* PASSWORD = "123456";

const int PIN_DOUT = 19;
const int PIN_SCK = 18;
const float CAL_FACTOR = 3352.05;

const int PIN_MOTOR = 25;
const unsigned long MAX_RUN_MS = 20'000;

// 급식시간 설정
struct FeedTask { uint8_t hour, minute; float target_g; };
FeedTask tasks[] = { {8,0,120},{18,30,80} };
bool task_done[sizeof(tasks) / sizeof(tasks[0])] = { false };

WeightSensor scale(PIN_DOUT, PIN_SCK, CAL_FACTOR);
Feeder      feeder(PIN_MOTOR, MAX_RUN_MS);

void setup() {
    Serial.begin(115200);
    scale.begin();
    feeder.begin();

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
            task_done[i] = true;
        }
    }
    if (now.tm_hour == 0 && now.tm_min == 0) {
        memset(task_done, 0, sizeof(task_done));
    }

    delay(5000);
}
