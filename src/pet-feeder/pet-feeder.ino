#include <WiFi.h>
#include <time.h>

// libraries for RFID
#include "Rfid.h"

// libraries for Feeder and WeightSensor
#include "WeightSensor.h"
#include "Feeder.h"


#include "Unit_task.h"
#include "config.h"

// 급식시간 설정
// struct FeedTask { uint8_t hour, minute; float target_g; };
// FeedTask tasks[] = { {8,0,120},{18,30,80} };
// bool task_done[sizeof(tasks) / sizeof(tasks[0])] = { false };


UnitTask first_Unit(
    FirstUnit::WEIGHT_DOUT_PIN, FirstUnit::WEIGHT_SCK_PIN, FirstUnit::WEIGHT_CAL_FACTOR,
    FirstUnit::FEEDER_MOTOR_PIN, FirstUnit::FEEDER_MAX_RUN_MS,
    FirstUnit::RFID_SS_PIN, FirstUnit::RFID_RST_PIN, FirstUnit::RFID_SERVO_PIN, FirstUnit::AUTH_TAG
);

void setup() {
    Serial.begin(115200); // 디버깅을 위한 시리얼 통신 초기화
    first_Unit.begin(); // UnitTask 객체 초기화

    first_Unit.setTasks(FirstUnit::TASKS, sizeof(FirstUnit::TASKS) / sizeof(FirstUnit::TASKS[0])); // 급식 시간 설정

    WiFi.begin(Config::WIFI::SSID, Config::WIFI::PASSWORD); // WiFi 연결
    while (WiFi.status() != WL_CONNECTED) delay(200); // WiFi 연결 대기
    configTime(9 * 3600, 0, "pool.ntp.org"); // NTP 서버 설정 ??
}

void loop() {
    struct tm now;
    if (!getLocalTime(&now)) { delay(1000); return; }
    
    first_Unit.run(now);

    first_Unit.resetTasks(now); // 하루가 지나면 작업 초기화
    delay(1000); // 1초 대기 후 다시 실행
}