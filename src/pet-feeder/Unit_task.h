#ifndef UNIT_TASK_H
#define UNIT_TASK_H
#include "Feeder.h"
#include "Rfid.h"
#include "WeightSensor.h"
#include <Arduino.h>
#include <config.h>
// 여러개의 배식 그릇을 제어하기 위해 Feeder ,Rfid, weightSensor 기능을 포함하는 UnitTask 클래스를 정의
// 하나의 UnitTask 객체는 하나의 배식 그릇을 제어.     
// pet-feeder.ino에서 여러개의 UnitTask 객체를 생성하여 사용
// 배식통이 연결되면(어떤걸로 연결된걸 파악할지는 미정), unitTask 객체를 생성하고 
// unittask 생성자가 받아야하는 인자는 
// Feeder(uint8_t motorPin, unsigned long timeoutMs)
// Rfid(uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID)
// WeightSensor(uint8_t doutPin, uint8_t sckPin, float calFactor)

// unitTask 객체의 begin() 함수를 호출하여 초기화. begin()에서는 먼저 각 기능 class의 객체 생성 및 연결된 배식통의 핀 번호를 설정하고, 
// WIfi, RFID리더, 서보 모터, 로드셀, DC모터등을 초기화
// unitTask 객체 멤버함수로 각 기능 통합
// 1. 앱에서 데이터 가져와서 config.h에 저장. 서버에 데이터 변경될때마다 동기화 해줘야함.
// 2. 예약 시간에 배식 기능 수행 if 사료가 남아 있는 경우 해당 무게 특정 하여 배식량 조절
// 3. 리더기 활성화 및 RFID 태그 인식시 서보모터로 배식통 열기
// 4. 문이 닫힌 경우, 로드셀로 잔량 확인 if) 잔량 = 0 서버에 <식사 완료> 알림 발송, loop 탈출
// 4-1.                                if) 잔량 != 0 서버에 <잔량 및 식사 중단> 알림 발송, 3단계로 다시 continue 
// 5. 리더기 종료 및 특정 시간 될 때까지 대기 / 00::00시가 되면 task초기화;

// 파라미터가 너무 많아 구조체로 생성하여 할당하는게 좋을 듯 추후 개발 


class UnitTask {
public:
    UnitTask(
        uint8_t doutPin, uint8_t sckPin, float calFactor,
        uint8_t motorPin, unsigned long timeoutMs,
        uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID       
    )
    :   scale(doutPin, sckPin, calFactor),
        feeder(motorPin, timeoutMs),
        rfid(ssPin, rstPin, servoPin, AUTH_ID) {}
    
    ~UnitTask() {
        delete[] tasks; // 동적 할당된 메모리 해제
        delete[] taskDone; // 동적 할당된 메모리 해제
    }

    // Weightsensor, Feeder, RFID 객체 초기화
    void begin() {
        scale.WeightSensor::begin();        // 로드셀 초기화
        feeder.Feeder::begin();       // 배식 기능 초기화
        rfid.RFID::begin();       // RFID 리더 초기화
    }
    
    // main.ino에서 급식 일정과 배식량을 가져져와 FeedTask, tasks, task_done 배열을 초기화
    // 추후 JSON 파일로 가져온 데이터들을 파라미터로 받아서 초기화하는 방식으로 변경 예정
    void setTasks(const FeedTask* newTasks, uint8_t count) {
        // 변경이 없으면 아무것도 하지 않음 일단 구현해놓음 필요하면 사용
        // if (taskCount == count) {
        //     bool same = true;
        //     for (size_t i = 0; i < count; ++i) {
        //         if (tasks[i].hour != newTasks[i].hour ||
        //             tasks[i].minute != newTasks[i].minute ||
        //             tasks[i].target_g != newTasks[i].target_g) {
        //             same = false;
        //             break;
        //         }
        //     }
        //     if (same) return; // 완전히 동일하면 재할당하지 않음
        // }
        
        delete[] tasks; // 기존 할당된 메모리 해제
        delete[] taskDone; // 기존 할당된 메모리 해제
        tasks = new FeedTask[count]; // tasks 동적 할당
        taskDone = new bool[count]; // taskDone 동적 할당
        for (uint8_t i = 0; i < count; i++) {
            tasks[i] = newTasks[i]; // 새로운 작업으로 초기화
            taskDone[i] = false; // 모든 작업 완료 상태 초기화
        }
        taskCount = count;
    }

    void run(tm& current_Time) {
        for (int i = 0; i < taskCount; i++) {
            if (taskDone[i]) continue; // 이미 완료된 작업은 건너뜀 
            if (current_Time.tm_hour == tasks[i].hour && current_Time.tm_min == tasks[i].minute) {
                feeder.dispense(tasks[i].target_g, scale); // 목표 사료량 배식
                
                while (scale.getWeightAvg() <= 0.5) {
                    // 로드셀로 무게 확인, 0.5g 이하일 때까지 대기
                    rfid.scan(); // RFID 리더 활성화 5초 이상 tag가 인식되지 않으면 자동으로 종료
                    delay(50); // 50ms 대기 후 다시 확인
                    // 여기서 잔량 확인 및 서버 알림 로직 추가하면 될듯, 아직 잔량 반영하는 로직은 적용안함       
                }
                taskDone[i] = true; // 작업 완료 표시
            }
        }
    }

void resetTasks(tm& current_Time) {
    // 현재 시간이 00:00시가 되면 모든 작업을 초기화
    if (current_Time.tm_hour == 0 && current_Time.tm_min == 0) {
        for (uint8_t i = 0; i < taskCount; ++i) {
            taskDone[i] = false; // 모든 작업 완료 상태 초기화
        }
        // lastFeedTime = millis(); // 마지막 배식 시간 갱신
    }
}

private:
    Feeder      feeder;        // 배식 기능
    RFID         rfid;          // RFID 리더 기능
    WeightSensor scale;         // 로드셀 기능

    // unsigned long lastFeedTime = 0; // 마지막 배식 시간 
   // 추후 배식 로그 관리시 필요할 수 있음 혹은 중복 배식 방지를 위해 마지막 배식 시간을 기록해 
   // 다음 배식 시간과 비교해 일정 시간이 지났을때만 배식하도록.. 사용
    
    FeedTask* tasks = nullptr; // 추후 setTask() 함수에서 초기화 / 런타임 동적 할당 위해 포인터
    bool* taskDone = nullptr;          // 작업 완료 여부/ 런타임 동적 할당 위해 포인터
    uint8_t taskCount = 0;
};

#endif // UNIT_TASK_H