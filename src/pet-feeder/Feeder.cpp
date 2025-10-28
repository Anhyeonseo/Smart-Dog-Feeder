// ─────────────────────────────────────────────────────────────────────────
// File: Feeder.cpp
// ─────────────────────────────────────────────────────────────────────────
#include "Feeder.h"


Feeder::Feeder(uint8_t stepPin, uint8_t dirPin, uint8_t enPin, unsigned long timeoutMs)
    : stepPin(stepPin), dirPin(dirPin), enPin(enPin), maxRunMs(timeoutMs),
      stepper(AccelStepper::DRIVER, stepPin, dirPin),
      tmcDriver(&Serial1, Config::FEEDER::R_SENSE, 0) {
        stepper.setEnablePin(enPin);
        stepper.setPinsInverted(false, false, true); // 스텝 모터 핀 반전 설정
     }
    
void Feeder::begin() {
    Serial1.begin(115200);
    tmcDriver.begin();
  
      // 1. 토크: 1000mA (1.0A)로 설정. 최대 1200mA까지 가능.
    tmcDriver.rms_current(1000); 

    // 2. 마이크로스텝: 16으로 유지 (부드러움)
    tmcDriver.microsteps(16);
    
    // 3. 모드: SpreadCycle로 설정하여 토크 극대화 (약간의 소음 증가)
    tmcDriver.en_spreadCycle(false);
    
    // 4. 속도/가속도: 중간 값에서 시작
    stepper.setMaxSpeed(2000); 
    stepper.setAcceleration(1000);

    stepper.disableOutputs(); // 모터 출력을 비활성화하여 초기 상태로 설정
    // stepper.enableOutputs(); // 모터 출력을 활성화
    currentState = IDLE; // 초기 상태를 IDLE로 설정

    Serial.println("스텝모터 준비 완료");

}

// 사료 미배출 시 감지하여 사료 막힘, 사료 부족 탐지 추가기능 -> TMC 드라이버의 stallGuard 기능 사용
// 스위치 눌러서 스텝모터 반대회전 기능 추가 쉽게가능
// 훈련모드 및 수동 배급을 위한 스위치 추가

/*
급식을 시작시키는 명령함수. 논블로킹을 위해 State만 변경하고 실제 배식은 update()에서 처리
*/
void Feeder::startDispense(float targetgrams, WeightSensor& sensor) {
    bool canStart = (currentState == IDLE) ||
                    (currentState == DONE) ||
                    (currentState == STOPPED);
    if (!canStart) {
        Serial.println("Feeder: 현재 다른 작업 중이라 새 급식을 시작할 수 없습니다.");
        return;
    }

    Serial.printf("급식 시작: 목표 %.1f g\n", targetgrams);
    
    // startweight 잔량 무게
    // this->startWeight = sensor.getWeightAvg(5); // getWeightAvg() 블로킹함수이기에 시작 시 한번만 호출
    // 실제 배식해야하는 무게
    // this->targetGrams = targetgrams - startWeight;
    targetGrams = targetgrams;
    // delay(50); // 센서 안정화 대기
    sensor.setCurrentWeight(); // 현재 실제 무게로 초기화

    // 다시 한번 초기화
    this->dispensedAmount = 0; // 무게 저장 변수 초기화

    this->isPotentiallyJammed = false; 

    stepper.enableOutputs(); // 모터 출력을 활성화
    stepper.move(999999); // 아주 먼 목표 설정 (계속 회전하도록)
    currentState = DISPENSING;  
}

/*
상태에 따라 동작 처리(멀티테스킹 핵심)
*/
void Feeder::update(WeightSensor& sensor) {
    // 1. 무게 센서를 논블로킹 방식으로 항상 업데이트합니다.
    
    if (currentState == IDLE || currentState == STOPPED || currentState == DONE ) { 
        return;
    } 

    // sensor.update();
    stepper.run();
    
    switch (currentState) {
        case DISPENSING: {
 
            if (millis() - lastWeightCheckTime >= weightCheckInterval) {
                 lastWeightCheckTime = millis();


               // 2. 무게 체크 (블로킹)
                float beforeDispensed = dispensedAmount;
                // dispensedAmount = sensor.getWeight() - startWeight;
                dispensedAmount = sensor.getWeightAvg(1);
                Serial.printf("  진행: %.1f g / %.1f g\n", dispensedAmount, targetGrams);

//     while (millis() - startTime < maxRunMs) { //maxRunMs은 최대 실행 시간을 설정한거임
//         // 배치 크기 결정 (가까워질수록 작게)
//         int stepBatch = (targetGrams - dispensed > threshold) ? baseStepSize : 200;


                // 목표 도달 시
                if (dispensedAmount >= targetGrams) {
                    stepper.stop(); // 모터 정지
                    stepper.disableOutputs(); // 발열관리를 위해 모터 출력을 비활성화
                    currentState = DONE;
                    Serial.printf("\n■ Feeder: 급식 완료! 최종: %.1f g\n", dispensedAmount);
                    break;
                }

                // // 1. 부하 체크를 통해 사료 걸림 감지
                // uint16_t load = tmcDriver.SG_RESULT();
                // Serial.printf("Motor Load: %d | ", load); // 튜닝 시 값 확인용

                // if (load < 100) { // 임계값 예시
                // Serial.printf("\nFeeder: StallGuard 감지! (값: %d) 반대 회전 실행...\n", load);
                // currentState = REVERSING;
                // stepper.stop();
        
                // // 2. 현재 위치를 0으로 리셋하여 기존 목표를 잊게 만듦
                // stepper.setCurrentPosition(0);
                // stepper.move(-800); // 반대 방향으로 이동
                // break; // 걸림이 감지됐으니, 더 이상 무게를 잴 필요 없이 switch문 탈출
                // }
             
                // 2. 사료 걸림 감지 로직 (무게 변화량 기반)
                // dispensedAmount가 0보다 클 때만 (초반 오류 방지)
                bool weightStalled = (dispensedAmount > 0.5 && abs(dispensedAmount - beforeDispensed) < 0.1);

                if (weightStalled) {
                    // 2. 이전에 의심 상태가 아니었다면, 타이머 시작
                    if (!isPotentiallyJammed) {
                        isPotentiallyJammed = true;
                        jamDetectStartTime = millis();
                        Serial.println("\nFeeder: 사료 걸림 의심. 1.5초 카운트 시작...");
                    }
                    // 3. 이미 의심 상태였다면, 시간이 1.5초를 넘었는지 확인
                    else {
                        if (millis() - jamDetectStartTime >= JAM_DETECT_DURATION_MS) {
                            Serial.println("\nFeeder: 사료 걸림 확정! 반대 회전 실행...");
                            
                            stepper.stop();
                            stepper.setCurrentPosition(0);
                            stepper.move(-1500);
                            
                            currentState = Feeder::REVERSING;
                            isPotentiallyJammed = false; // 상태 전환 후 리셋
                            break;
                        }
                    }
                } 
                // 4. 무게가 다시 변하기 시작했다면 (걸림이 아니었음), 의심 상태 해제
                else {
                    if (isPotentiallyJammed) {
                        Serial.println("\nFeeder: 사료 걸림 의심 해제. 정상 작동.");
                    }
                    isPotentiallyJammed = false;
                }
            }
            break;
        }

        case REVERSING: {
            // 이 상태의 유일한 목적은 loop 한 바퀴를 그냥 보내는 것입니다.
            // 이 루프의 시작점에서 stepper.run()이 호출되면서
            // 모터가 확실히 움직이기 시작하고 isRunning()이 true가 됩니다.
            Serial.println("[N+1] Feeder: 모터 실행 보장. 완료 확인 단계로 전환.");

            // 3. '완료 확인' 상태로 전환
            currentState = REVERSING_DONE;
            break;
        }
        
        case REVERSING_DONE: {
            // 사료 걸림 감지 시 반대 회전
            
            if (!(stepper.isRunning())) {
                Serial.println("Feeder: 사료 걸림 감지, 반대 회전 완료");
                currentState = DISPENSING; // 다시 배식 상태로 변경
                stepper.move(999999); // 다시 정방향 
            }
            break;
        }
    }
}
void Feeder::stop() {
    stepper.stop(); 
    stepper.disableOutputs(); 
    currentState = STOPPED; // 상태를 중지로 변경
    Serial.println("Feeder: 급식 중지");
}

void Feeder::resetState(WeightSensor& sensor) {
    currentState = IDLE;
    dispensedAmount = 0;
}