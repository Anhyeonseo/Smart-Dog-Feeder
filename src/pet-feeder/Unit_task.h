// Unit_task.h
// 이 파일은 자동 급식기의 핵심 동작(급식, 센서 감지, 상태 관리)을 담당하는
// UnitTask 클래스의 설계도(정의)입니다.

#ifndef UNIT_TASK_H
#define UNIT_TASK_H

// UnitTask가 사용하는 다른 하드웨어 제어 클래스들을 포함합니다.
#include "Feeder.h"
#include "Rfid.h"
#include "WeightSensor.h"
#include <Arduino.h>
#include <ArduinoJson.h>   // JSON 파싱을 위한 라이브러리
#include <time.h>

struct FeedTask {
    long id;             // 스케쥴 고유 ID
    uint8_t hour;        // 시간
    uint8_t minute;      // 분
    float target_g;      // 목표 무게 (그램 단위)
};

class UnitTask {
public:
    enum UnitState {
        IDLE,         // 대기 상태(스케줄 대기)
        DISPENSING,   // 배식 중(스케줄 실행 중)
        MONITORING    // RFID 인식 대기/식사 감지 중
    };

    UnitTask(
        uint8_t doutPin, uint8_t sckPin, float calFactor,
        uint8_t stepPin, uint8_t dirPin, uint8_t enPin, unsigned long timeoutMs,
        uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID,
        uint8_t buttonPin = 255 // 255면 비활성화
    );

    ~UnitTask();

    void begin();
    void update(tm& current_Time);

    void startTestDispense(float amount);
    void setTasksFromJson(String json);
    void resetTasks(tm& current_Time);

    bool  isMealCompleted();
    long  getCompletedTaskId();
    float getRemainingWeight();

private:
    Feeder       feeder;
    RFID         rfid;
    WeightSensor scale;

    UnitState currentState = IDLE;

    FeedTask* tasks = nullptr;
    bool*     taskDone = nullptr;
    uint8_t   taskCount = 0;
    int       currentTaskIndex = -1;

    unsigned long monitoringStartTime = 0;
    const unsigned long MONITORING_TIMEOUT_MS = 3600000; // 1시간

    bool  mealCompletedFlag = false;
    long  completedTaskId   = 0;
    float remainingWeight   = 0.0;

    // --- 수동 버튼(토글) 관련 ---
    uint8_t buttonPin_ = 255;
    int     btnStable_ = HIGH;
    unsigned long btnLastChange_ = 0;
    static constexpr unsigned long debounceMs_ = 40;

    // 헬퍼들(작게 쪼개기)
    void handleButton_();
    void handleIdle_(tm& now);
    void handleDispensing_();
    void handleMonitoring_();
};
#endif // UNIT_TASK_H