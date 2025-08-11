#include <WiFi.h>
#include <time.h>
#include "Unit_task.h"
#include "config.h"
#include "MqttHandler.h" // MQTT 핸들러 포함

// 급식시간 설정
// struct FeedTask { uint8_t hour, minute; float target_g; };
// FeedTask tasks[] = { {8,0,120},{18,30,80} };
// bool task_done[sizeof(tasks) / sizeof(tasks[0])] = { false };


UnitTask Unit(
    FirstUnit::WEIGHT_DOUT_PIN, FirstUnit::WEIGHT_SCK_PIN, FirstUnit::WEIGHT_CAL_FACTOR,
    FirstUnit::FEEDER_MOTOR_STEP, FirstUnit::FEEDER_MOTOR_DIR, FirstUnit::FEEDER_MOTOR_EN, FirstUnit::FEEDER_MAX_RUN_MS,
    FirstUnit::RFID_SS_PIN, FirstUnit::RFID_RST_PIN, FirstUnit::RFID_SERVO_PIN, FirstUnit::AUTH_TAG
);

void setup() {
    Serial.begin(115200); // 디버깅을 위한 시리얼 통신 초기화

    // preferences.begin("feeder_settings", false);
    // preferences.clear(); // "feeder_settings" 공간의 모든 데이터를 삭제합니다.
    // preferences.end();
    // Serial.println("\n!!! 내부 메모리(Preferences)가 초기화되었습니다. !!!\n");

    Unit.begin(); // UnitTask 객체 초기화

    WiFi.begin(Config::WIFI::SSID, Config::WIFI::PASSWORD); // WiFi 연결
    
    Serial.print("WiFi 연결 중...");
    
    while (WiFi.status() != WL_CONNECTED) delay(200); // WiFi 연결 대기
    
    Serial.println("\nWiFi 연결 완료!");

    configTime(9 * 3600, 0, "pool.ntp.org"); // NTP 서버 설정

    setupMqtt(); // MQTT 핸들러 초기화

    // 부팅 시 Preferences에 저장된 스케줄로 UnitTask 설정
    Unit.setTasksFromJson(getSchedulesJson());
}

void loop() {
    loopMqtt(); // MQTT 연결 유지 및 메시지 수신

    // 앱에서 스케줄을 변경했는지 확인
    if (isScheduleUpdated()) {
        Serial.println("MQTT로 스케줄 업데이트 감지. UnitTask에 적용합니다.");
        Unit.setTasksFromJson(getSchedulesJson());
    }

    struct tm now;
    if (!getLocalTime(&now)) { delay(1000); return; }
    // 현재 시간 출력
    // Serial.printf("현재 시간: %02d:%02d:%02d\n", now.tm_hour, now.tm_min, now.tm_sec);

    Unit.run(now);

    // UnitTask가 급식을 완료했는지 확인하고 서버에 보고
    if (Unit.isMealCompleted()) {
        long id = Unit.getCompletedTaskId();
        float weight = Unit.getRemainingWeight();
        Serial.printf("급식 완료 감지 (ID: %ld). 서버에 상태 보고...\n", id);
        
        sendMealStatus(id, weight);
    }

    Unit.resetTasks(now); // 하루가 지나면 작업 초기화
    delay(1000); // 1초 대기 후 다시 실행
}