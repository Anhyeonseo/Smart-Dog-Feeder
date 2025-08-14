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
        IDLE,         // 대기 상태
        DISPENSING,   // 배식 중
        MONITORING    // 식사 감지 중
    };

    /**
     * UnitTask 객체를 생성
     * 필요한 모든 하드웨어의 핀 번호와 설정값을 전달받아 내부 객체들을 초기화
     */
    UnitTask(
        uint8_t doutPin, uint8_t sckPin, float calFactor,
        uint8_t stepPin, uint8_t dirPin, uint8_t enPin, unsigned long timeoutMs,
        uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID       
    );

    ~UnitTask();

    /**
     * 모든 하드웨어(모터, 센서 등)를 실제로 초기화하고 작동 준비
     */
    void begin();

    /**
     * @brief UnitTask의 핵심 update 함수입니다. 메인 loop()에서 계속 호출됩니다.
     * 모든 자식 모듈(Feeder, RFID)의 상태를 업데이트하고,
     * UnitTask 자신의 상태를 논블로킹 방식으로 관리
     */
    void update(tm& current_Time);

    /**
     * 테스트용 함수로, 지정된 양만큼 즉시 배식하고 RFID 문 개폐를 시뮬레이션
     * 실제 급식 사이클(배식 -> 모니터링 -> 완료)을 테스트할 때 사용
     */
    void startTestDispense(float amount);
    
    /**
     * @brief MQTT 핸들러로부터 받은 JSON 형식의 스케줄 목록 문자열을 해석합니다.
     * 해석된 데이터를 바탕으로 내부 급식 작업(tasks) 목록을 설정합니다.
     * @param json 스케줄 목록이 담긴 JSON 배열 문자열 (예: "[{\"id\":1,...},{\"id\":2,...}]")
     */
    void setTasksFromJson(String json);

    /**
     * @brief 자정(00:00)이 되면 모든 작업의 완료 상태(taskDone)를 초기화합니다.
     * 이를 통해 다음 날 같은 스케줄이 다시 실행될 수 있도록 합니다.
     * @param current_Time 현재 시간 정보가 담긴 구조체
     */
    void resetTasks(tm& current_Time);

    // --- 메인 파일(.ino)에서 식사 완료 상태를 확인하기 위한 함수들 ---

    /**
     * @brief 반려동물의 식사가 완료되었는지 여부를 반환합니다.
     * run() 함수에서 식사 완료를 감지하면 내부 플래그(mealCompletedFlag)가 true가 됩니다.
     * @return 식사가 완료되었으면 true, 아니면 false
     */
    bool isMealCompleted();

    /**
     * @brief 완료된 급식 스케줄의 고유 ID를 반환합니다.
     * @return 완료된 스케줄의 ID (long)
     */
    long getCompletedTaskId();

    /**
     * @brief 식사 완료 후 측정한 최종 사료 잔량을 반환합니다.
     * @return 최종 잔량 (float, 그램 단위)
     */
    float getRemainingWeight();


private:
    // UnitTask가 제어하는 하드웨어 객체들
    Feeder      feeder; // 사료를 배출하는 모터 제어 객체
    RFID         rfid;   // RFID 태그를 읽고 서보모터를 제어하는 객체
    WeightSensor scale;  // 사료 무게를 측정하는 로드셀 객체

    UnitState currentState = IDLE;       // 현재 상태를 저장하는 변수, 초기 상태는 IDLE(대기)
    
    // 스케줄 관리 변수
    FeedTask* tasks = nullptr;    // FeedTask 구조체 배열을 가리키는 포인터
    bool* taskDone = nullptr;     // 각 작업의 완료 여부를 저장하는 bool 배열을 가리키는 포인터
    uint8_t taskCount = 0;        // 현재 설정된 스케줄의 총 개수
    int currentTaskIndex = -1;       // 현재 실행 중인 스케줄의 배열 인덱스

    unsigned long monitoringStartTime = 0; // 식사 감지(모니터링)를 시작한 시간
    const unsigned long MONITORING_TIMEOUT_MS = 3600000; // 1시간(3600초 * 1000)을 타임아웃으로 설정

    // [추가] 메인 파일(.ino)에 식사 완료 정보를 전달하기 위한 변수들 (플래그)
    bool mealCompletedFlag = false;  // 식사가 완료되면 true로 설정됨
    long completedTaskId = 0;        // 완료된 스케줄의 ID를 임시 저장
    float remainingWeight = 0.0;     // 측정된 최종 잔량을 임시 저장
};

#endif // UNIT_TASK_H
