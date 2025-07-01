// ─────────────────────────────────────────────────────────────────────────
// File: Feeder.cpp
// ─────────────────────────────────────────────────────────────────────────
#include "Feeder.h"

Feeder::Feeder(uint8_t motorPin, unsigned long timeoutMs)
    : motorPin(motorPin), maxRunMs(timeoutMs) {}

void Feeder::begin() {
    pinMode(motorPin, OUTPUT);
    digitalWrite(motorPin, LOW);
}

void Feeder::dispense(float targetGrams, WeightSensor& sensor) {
    Serial.printf("\n▶ 급식 시작 : 목표 %.1f g\n", targetGrams);
    float startWeight = sensor.getWeightAvg();
    unsigned long startTime = millis();
    digitalWrite(motorPin, HIGH);

    while (millis() - startTime < maxRunMs) {
        float currentWeight = sensor.getWeightAvg();
        float dispensed = currentWeight - startWeight;
        Serial.printf("  진행: %.1f g / %.1f g\r", dispensed, targetGrams);
        if (dispensed >= targetGrams) break;
    }

    digitalWrite(motorPin, LOW);
    Serial.println("\n■ 급식 완료");
    delay(1000);
}