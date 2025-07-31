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
 * @brief 메인 실행 함수. 상태에 따라 급식, 모니터링, 완료 처리를 수행합니다.
 */
void UnitTask::run(tm& current_Time) {
    // 1. [IDLE 상태]: 평소 상태. 급식 시간이 되었는지 확인합니다.
    if (currentState == IDLE) {
        for (int i = 0; i < taskCount; i++) {
            // 시간이 맞고, 아직 완료되지 않은 작업이라면
            if (!taskDone[i] && current_Time.tm_hour == tasks[i].hour && current_Time.tm_min == tasks[i].minute) {
                Serial.printf("스케줄 %d번 (ID: %ld) 실행 시작.\n", i, tasks[i].id);
                currentTaskIndex = i;
                currentState = DISPENSING; // 상태를 '배식 중'으로 변경
                return; // 한 번에 하나의 작업만 처리
            }
        }
    }

    // 2. [DISPENSING 상태]: 사료를 배식합니다.
    if (currentState == DISPENSING) {
        feeder.dispense(tasks[currentTaskIndex].target_g, scale);
        Serial.println("배식 완료. 식사 모니터링을 시작합니다.");
        currentState = MONITORING;
    }

    // 3. [MONITORING 상태]: 반려동물의 식사를 감지하고 완료 여부를 판단합니다.
    if (currentState == MONITORING) {
        // rfid.scan() 함수는 문이 방금 닫혔을 때만 true를 반환합니다.
        bool mealFinished = rfid.scan(); 

        if (mealFinished) {
            Serial.println("식사 완료 감지.");
            // --- 식사 완료 후 알림 전송을 위한 데이터 설정 ---
            remainingWeight = scale.getWeightAvg(); // 최종 잔량 측정
            completedTaskId = tasks[currentTaskIndex].id; // 완료된 스케줄의 ID 저장
            mealCompletedFlag = true; // 메인 .ino 파일에 알리기 위한 플래그 설정
            taskDone[currentTaskIndex] = true; // 현재 작업을 '완료'로 표시
            currentState = IDLE;
        }
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
