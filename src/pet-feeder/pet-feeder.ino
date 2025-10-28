#include <WiFi.h>
#include <time.h>
#include "Unit_task.h"
#include "config.h"
#include "MqttHandler.h" // MQTT 핸들러 포함
// #include <Preferences.h> // 더미 데이터 삭제시 객체 생성 위해

// 급식시간 설정
// struct FeedTask { uint8_t hour, minute; float target_g; };
// FeedTask tasks[] = { {8,0,120},{18,30,80} };
// bool task_done[sizeof(tasks) / sizeof(tasks[0])] = { false };

// Preferences      preferences; // 더미 데이터 삭제시 객체 생성 위해

UnitTask Unit(
    FirstUnit::WEIGHT_DOUT_PIN, FirstUnit::WEIGHT_SCK_PIN, FirstUnit::WEIGHT_CAL_FACTOR,
    FirstUnit::FEEDER_MOTOR_STEP, FirstUnit::FEEDER_MOTOR_DIR, FirstUnit::FEEDER_MOTOR_EN, FirstUnit::FEEDER_MAX_RUN_MS,
    FirstUnit::RFID_SS_PIN, FirstUnit::RFID_RST_PIN, FirstUnit::RFID_SERVO_PIN, FirstUnit::AUTH_TAG, FirstUnit::BUTTON_PIN
);

unsigned long lastSecondTaskTime = 0;

void setup() {
    Serial.begin(115200); // 디버깅을 위한 시리얼 통신 초기화

//    preferences.begin("feeder_settings", false);
//    preferences.clear(); // "feeder_settings" 공간의 모든 데이터를 삭제합니다.
//    preferences.end();
//    Serial.println("\n!!! 내부 메모리(Preferences)가 초기화되었습니다. !!!\n");

    WiFi.begin(Config::WIFI::SSID, Config::WIFI::PASSWORD); // WiFi 연결
    
    Serial.print("WiFi 연결 중...");
    
    while (WiFi.status() != WL_CONNECTED) delay(200); // WiFi 연결 대기
    
    Serial.println("\nWiFi 연결 완료!");

    configTime(9 * 3600, 0, "pool.ntp.org"); // NTP 서버 설정

    Unit.begin(); // UnitTask 객체 초기화
    setupMqtt(); // MQTT 핸들러 초기화

    // 부팅 시 Preferences에 저장된 스케줄로 UnitTask 설정
    Unit.setTasksFromJson(getSchedulesJson());
}

void loop() {
    //  매 루프마다 항상, 그리고 최대한 빠르게 실행되어야 하는 함수들
    
    // 1. mQTT 연결 유지 및 메시지 수신 
    loopMqtt(); // MQTT 연결 유지 및 메시지 수신

    // 2. 수신된 MQTT 메시지 처리 (필요시에만 동작)
    handleMqttMessages();

    // 3. unitTask의 상태 업데이트
    struct tm now;
    if (getLocalTime(&now)) {
        Unit.update(now);
    }

    // 주기적으로 실행되어야하는 함수들 

    if (millis() - lastSecondTaskTime >= 1000) {
        lastSecondTaskTime = millis();

        // 1. MQTT로 스케줄 업데이트가 있었는지 확인
        if (isScheduleUpdated()) {
            Serial.println("MQTT로 스케줄 업데이트 감지. UnitTask에 적용합니다.");
            Unit.setTasksFromJson(getSchedulesJson());
        }

        // 2. 식사 완료 보고
        if (Unit.isMealCompleted()) {
            long id = Unit.getCompletedTaskId();
            float weight = Unit.getRemainingWeight();
            Serial.printf("급식 완료 감지 (ID: %ld). 서버에 상태 보고...\n", id);
            sendMealStatus(id, weight);
        }

        // 3. 자정 작업 초기화
        Unit.resetTasks(now);
    }

    // --- 시리얼 테스트 입력 처리 ---
    handleSerialTest();
}


/**
 * @brief 시리얼 모니터 입력을 받아 즉시 급식 테스트를 수행하는 함수
 */
void handleSerialTest() {
    static String inputAmountStr = "";
    static bool waitingForAmount = false;

    if (Serial.available() > 0) {
        char c = Serial.read();
        
        if (waitingForAmount) {
            if (c == '\n' || c == '\r') {
                float amount = inputAmountStr.toFloat();
                if (amount > 0) {
                    Unit.startTestDispense(amount); // 논블로킹 테스트 시작
                } else {
                    Serial.println("[오류] 잘못된 입력입니다.");
                }
                inputAmountStr = "";
                waitingForAmount = false;
            } else {
                inputAmountStr += c;
            }
        } else {
            if (c == 't' || c == 'T') {
                Serial.println("\n▶ 즉시 급식 테스트 모드. 급식할 양(g)을 입력하고 Enter: ");
                waitingForAmount = true;
                inputAmountStr = "";
            }
        }
    }
}