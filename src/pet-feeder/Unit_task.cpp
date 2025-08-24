// Unit_task.cpp

#include "Unit_task.h"

// --- 생성자, 소멸자, begin 함수 (기존과 거의 동일) ---
UnitTask::UnitTask(
    uint8_t doutPin, uint8_t sckPin, float calFactor,
    uint8_t stepPin, uint8_t dirPin, uint8_t enPin, unsigned long timeoutMs,
    uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID       
) : scale(doutPin, sckPin, calFactor),
    feeder(stepPin, dirPin, enPin, timeoutMs),
    rfid(ssPin, rstPin, servoPin, AUTH_ID) {}

UnitTask::~UnitTask() {
    delete[] tasks;
    delete[] taskDone;
}

void UnitTask::begin() {
    scale.begin();
    feeder.begin();
    rfid.begin();
    currentState = IDLE; // 초기 상태 설정
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
        
        // "HH:MM" 형식의 시간을 파싱하여 hour, minute으로 변환
        String timeStr = task["time"];
        tasks[i].hour = timeStr.substring(0, 2).toInt();
        tasks[i].minute = timeStr.substring(3, 5).toInt();
        
        tasks[i].target_g = task["amount"];
        taskDone[i] = false;
    }
    Serial.printf("%d개의 스케줄이 성공적으로 설정되었습니다.\n", taskCount);
}

/**
 * @brief 지정된 양만큼 즉시 배식하고, RFID 문 개폐까지 테스트하는 함수.
 * 실제 급식 사이클(배식 -> 모니터링 -> 완료)을 시뮬레이션합니다.
 * @param amount 배식할 사료의 양 (g)
 */
void UnitTask::startTestDispense(float amount) {
    if (currentState != IDLE) {
        Serial.println("오류: 다른 작업이 진행 중일 때는 테스트를 시작할 수 없습니다.");
        return;
    }
    Serial.printf("\n--- 즉시 급식 테스트 시작 (목표: %.1fg) ---\n", amount);
    feeder.startDispense(amount, scale); // Feeder에게 배식 '시작'만 지시
    currentState = DISPENSING;      // UnitTask의 상태를 '배식 중'으로 변경
}

/**
 * @brief 메인 실행 함수. 상태에 따라 급식, 모니터링, 완료 처리를 수행합니다.
 */
void UnitTask::update(tm& current_Time) {

    feeder.update(scale);

    switch (currentState) {
        case IDLE:
            // 스케줄 시간이 되었는지 확인
            for (int i = 0; i < taskCount; i++) {
                if (!taskDone[i] && current_Time.tm_hour == tasks[i].hour && current_Time.tm_min == tasks[i].minute) {
                    Serial.printf("스케줄 %d번 (ID: %ld) 실행 시작.\n", i, tasks[i].id);
                    currentTaskIndex = i;
                    feeder.startDispense(tasks[i].target_g, scale);
                    currentState = DISPENSING;
                    return; 
                }
            }

            break;
        case DISPENSING:
            // Feeder가 배식을 다했는지 '상태'만 확인
            if (feeder.getState() == Feeder::DONE || feeder.getState() == Feeder::STOPPED) {
                feeder.resetState();
                Serial.println("UnitTask: 배식 완료/중지 감지. 식사 모니터링 시작.");
                currentState = MONITORING;
                monitoringStartTime = millis(); // 모니터링 타임아웃 타이머 시작
            }
            break;

        case MONITORING:
            // RFID 모듈이 문을 닫았는지 확인 (scan()이 true를 반환하면 닫힌 것)
            bool mealFinished = rfid.scan(); 

            if (mealFinished) { 
                Serial.println("UnitTask: 식사 완료 감지 (문 닫힘).");
                remainingWeight = scale.getWeightAvg();
                if (currentTaskIndex != -1) { // testdispense를 위해 예외 상황 처리
                    completedTaskId = tasks[currentTaskIndex].id;
                    taskDone[currentTaskIndex] = true;
                }
                mealCompletedFlag = true;
                currentState = IDLE;
                currentTaskIndex = -1; // 현재 작업 인덱스 초기화
                break;
            }

            // 모니터링 타임아웃 확인
            if (millis() - monitoringStartTime > MONITORING_TIMEOUT_MS) {
                Serial.println("경고: 식사 시간 초과! 작업을 강제로 종료합니다.");
                // 문을 닫는 로직이 RFID 클래스 내부에 있으므로,
                // RFID scan()이 계속 호출되면 알아서 닫힐 것입니다.
                // 혹은 rfid.closeDoor() 같은 강제 닫기 함수를 만들 수도 있습니다.
                currentState = IDLE;
                currentTaskIndex = -1;
            }
            break;
    }
}

// --- 식사 완료 상태를 외부로 전달하는 함수들 ---
bool UnitTask::isMealCompleted() {
    if (mealCompletedFlag) {
        mealCompletedFlag = false; // 플래그를 확인했으니 다시 내림
        return true;
    }
    return false;
}

long UnitTask::getCompletedTaskId() {
    return completedTaskId;
}

float UnitTask::getRemainingWeight() {
    return remainingWeight;
}


void UnitTask::resetTasks(tm& current_Time) {
    if (current_Time.tm_hour == 0 && current_Time.tm_min == 0) {
        for (uint8_t i = 0; i < taskCount; ++i) {
            taskDone[i] = false;
        }
        Serial.println("자정이 되어 모든 급식 작업을 초기화했습니다.");
    }
}
