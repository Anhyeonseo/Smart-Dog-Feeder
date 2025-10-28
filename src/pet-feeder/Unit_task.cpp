// Unit_task.cpp
#include "Unit_task.h"

// --- 생성자, 소멸자, begin 함수 ---
UnitTask::UnitTask(
    uint8_t doutPin, uint8_t sckPin, float calFactor,
    uint8_t stepPin, uint8_t dirPin, uint8_t enPin, unsigned long timeoutMs,
    uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID,
    uint8_t buttonPin
) : scale(doutPin, sckPin, calFactor),
    feeder(stepPin, dirPin, enPin, timeoutMs),
    rfid(ssPin, rstPin, servoPin, AUTH_ID),
    buttonPin_(buttonPin) {}

UnitTask::~UnitTask() {
    delete[] tasks;
    delete[] taskDone;
}

void UnitTask::begin() {
    scale.begin();
    feeder.begin();
    rfid.begin();
    unitstate_ = IDLE;

    if (buttonPin_ != 255) {
        pinMode(buttonPin_, INPUT_PULLUP);
        btnStable_ = digitalRead(buttonPin_);
        btnLastChange_ = millis();
    }
}

// --- MQTT로부터 받은 JSON으로 스케줄을 설정하는 함수 ---
void UnitTask::setTasksFromJson(String json) {
    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, json).code() != DeserializationError::Ok) {
        Serial.println("setTasksFromJson: JSON 파싱 실패");
        return;
    }

    JsonArray array = doc.as<JsonArray>();
    delete[] tasks;
    delete[] taskDone;
    taskCount = array.size();
    if (taskCount == 0) return;

    tasks = new FeedTask[taskCount];
    taskDone = new bool[taskCount];

    for (int i = 0; i < taskCount; i++) {
        JsonObject task = array[i];
        tasks[i].id = task["id"];

        String timeStr = task["time"]; // "HH:MM"
        tasks[i].hour = timeStr.substring(0, 2).toInt();
        tasks[i].minute = timeStr.substring(3, 5).toInt();

        tasks[i].target_g = task["amount"];
        taskDone[i] = false;
    }
    Serial.printf("%d개의 스케줄이 성공적으로 설정되었습니다.\n", taskCount);
}

/**
 * @brief 즉시 테스트 배식을 시작하는 함수
 */
void UnitTask::startTestDispense(float amount) {
    if (unitstate_ != IDLE) {
        Serial.println("오류: 다른 작업이 진행 중일 때는 테스트를 시작할 수 없습니다.");
        return;
    }
    Serial.printf("\n--- 즉시 급식 테스트 시작 (목표: %.1fg) ---\n", amount);
    feeder.startDispense(amount, scale);
    unitstate_ = DISPENSING;
}

// --- 메인 실행 함수 -------------------------------------------------------
void UnitTask::update(tm& current_Time) {
    // 0) 항상 모터/센서를 먼저 업데이트
    feeder.update(scale);

    // 1) 버튼 처리 (상태에 따라 허용/무시)
    handleButton_();

    // 2) 상태별 처리
    switch (unitstate_) {
        case IDLE:        handleIdle_(current_Time);       break;
        case DISPENSING:  handleDispensing_();             break;
        case MONITORING:  handleMonitoring_();             break;
    }
}

// --- 상태별 함수들 --------------------------------------------------------
void UnitTask::handleIdle_(tm& current_Time) {
    // 스케줄 시간이 되었는지 확인
    for (int i = 0; i < taskCount; i++) {
        if (!taskDone[i] &&
            current_Time.tm_hour == tasks[i].hour &&
            current_Time.tm_min  == tasks[i].minute) {

            Serial.printf("스케줄 %d번 (ID: %ld) 실행 시작.\n", i, tasks[i].id);

            // 스케줄 진입 시 수동 오버라이드 강제 해제 & 닫기 시작
            if (rfid.isManualOverrideActive()) {
                rfid.setManualOverride(false);
            }
            currentTaskIndex = i;
            rfid.resetRFIDSession();
            feeder.startDispense(tasks[i].target_g, scale);
            unitstate_ = DISPENSING;
            return;
        }
    }
    if (rfid.isManualOverrideActive()) {
        rfid.scan(); // 오버라이드 중에는 RFID 읽지 않고 모션만 업데이트
    } else {
        rfid.tickMotionOnly();
    }
}

void UnitTask::handleDispensing_() {
    // 버튼은 DISPENSING에서 무시됨 (handleButton_에서 필터링)
    if (feeder.getState() == Feeder::DONE || feeder.getState() == Feeder::STOPPED) {
        feeder.resetState(scale);
        Serial.println("UnitTask: 배식 완료/중지 감지. 식사 모니터링 시작.");
        rfid.beginMonitoring();
        unitstate_ = MONITORING;
        monitoringStartTime = millis();
    }
}

void UnitTask::handleMonitoring_() {
    // 수동 오버라이드가 켜져 있어도 scan()은 내부에서 오버라이드 시 RFID를 읽지 않고
    // 애니메이션만 진행한다. (닫힘 완료 보고는 오버라이드가 꺼진 상태에서만 true)
    bool mealFinished = rfid.scan();

    if (mealFinished) {
        Serial.println("UnitTask: 식사 완료 감지 (문 닫힘).");
        remainingWeight = scale.getWeightAvg();
        if (currentTaskIndex != -1) {
            completedTaskId = tasks[currentTaskIndex].id;
            taskDone[currentTaskIndex] = true;
        }
        mealCompletedFlag = true;
        unitstate_ = IDLE;
        currentTaskIndex = -1;
        return;
    }

    // 타임아웃
    if (millis() - monitoringStartTime > MONITORING_TIMEOUT_MS) {
        Serial.println("경고: 식사 시간 초과! 작업을 강제로 종료합니다.");
        unitstate_ = IDLE;
        currentTaskIndex = -1;
    }
}

// --- 버튼 처리 -----------------------------------------------------------
void UnitTask::handleButton_() {
    if (buttonPin_ == 255) return;   // 버튼 미사용
    // DISPENSING(배식 중)에는 버튼 동작 '무시'
    if (unitstate_ != IDLE) return;

    int reading = digitalRead(buttonPin_);
    unsigned long now = millis();

    if (reading != btnStable_ && (now - btnLastChange_) >= debounceMs_) {
        // 토글 발생 (풀업 기준 LOW가 눌림)
        btnLastChange_ = now;
        btnStable_ = reading;

        if (reading == LOW) {
            // 버튼 눌림 시: 수동 오버라이드 토글
            bool next = !rfid.isManualOverrideActive();
            rfid.setManualOverride(next);
            if (next) {
                Serial.println("[BUTTON] 수동 열기(홀드) 활성화");
            } else {
                Serial.println("[BUTTON] 수동 해제 → 닫기 시작");
            }
        }
    }
}

// --- 외부 쿼리 -----------------------------------------------------------
bool UnitTask::isMealCompleted() {
    if (mealCompletedFlag) {
        mealCompletedFlag = false;
        return true;
    }
    return false;
}
long UnitTask::getCompletedTaskId() { return completedTaskId; }
float UnitTask::getRemainingWeight() { return remainingWeight; }

void UnitTask::resetTasks(tm& current_Time) {
    if (current_Time.tm_hour == 0 && current_Time.tm_min == 0) {
        for (uint8_t i = 0; i < taskCount; ++i) {
            taskDone[i] = false;
        }
        Serial.println("자정이 되어 모든 급식 작업을 초기화했습니다.");
    }
}
