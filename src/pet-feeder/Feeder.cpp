// ─────────────────────────────────────────────────────────────────────────
// File: Feeder.cpp
// ─────────────────────────────────────────────────────────────────────────
#include "Feeder.h"


Feeder::Feeder(uint8_t stepPin, uint8_t dirPin, uint8_t enPin, unsigned long timeoutMs)
    : stepPin(stepPin), dirPin(dirPin), enPin(enPin), maxRunMs(timeoutMs),
      stepper(AccelStepper::DRIVER, stepPin, dirPin), tmcDriver(&Serial1, Config::FEEDER::R_SENSE, 0) {}

void Feeder::begin() {
    Serial1.begin(115200);
    tmcDriver.begin();
    tmcDriver.microsteps(1); // 풀스텝 설정

    stepper.setMaxSpeed(1000);
    stepper.setAcceleration(500);
    pinMode(enPin, OUTPUT);
    digitalWrite(enPin, LOW); // 모터 활성화
    pinMode(dirPin, OUTPUT);
    digitalWrite(dirPin, HIGH); // 회전 방향 설정 (필요 시 LOW로 바꿔보세요)
    Serial.println("스텝모터 준비 완료");

}

// 사료 미배출 시 감지하여 사료 막힘, 사료 부족 탐지 추가기능
// 스위치 눌러서 스텝모터 반대회전 기능
// 훈련모드 및 수동 배급을 위한 스위치 추가

void Feeder::dispense(float targetGrams, WeightSensor& sensor) {
    Serial.printf("\n▶ 급식 시작 : 목표 %.1f g\n", targetGrams);
    
    float startWeight = sensor.getWeightAvg();
    unsigned long startTime = millis();
    float currentWeight = startWeight;
    float dispensed = 0;
    const float stopMargin = 0.5; // ±0.5g 허용

    const int baseStepSize = 600;     // 기본 배치 스텝 수
    const float threshold = 10;     // 목표까지 1g 이하로 남으면 정밀 모드


    while (millis() - startTime < maxRunMs) { //maxRunMs은 최대 실행 시간을 설정한거임
        // 배치 크기 결정 (가까워질수록 작게)
        int stepBatch = (targetGrams - dispensed > threshold) ? baseStepSize : 200;

        float beforeWeight = sensor.getWeightAvg();

        stepper.move(stepBatch);
        stepper.runToPosition(); // batch만큼 회전

        // 현재 무게 측정
        currentWeight = sensor.getWeightAvg();
        dispensed = currentWeight - startWeight; // 배출된 무게 계산
        
        // 사료 걸림 감지 (한 번 회전 후 변화량이 0.3g 이하일 때 반대 회전)
        if (abs(currentWeight - beforeWeight) < 0.3) {
            Serial.println("사료 걸림 감지! 반대 회전 중...");
            stepper.move(-stepBatch); // 반대 방향으로 회전
            stepper.runToPosition();
            // 필요시 continue; // 다음 루프로
        }
        
        Serial.printf("  진행: %.1f g / %.1f g\r \n", dispensed, targetGrams);
        if (dispensed >= targetGrams) break;
    }

    Serial.println("\n■ 급식 완료");
    delay(1000);
}