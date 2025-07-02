// ─────────────────────────────────────────────────────────────────────────
// File: Feeder.h
// ─────────────────────────────────────────────────────────────────────────
#ifndef FEEDER_H
#define FEEDER_H

#include <Arduino.h>
#include "WeightSensor.h"

class Feeder {
public:
	// 생성자에서 DC 모터 제어 핀과 타임아웃(ms) 설정
	Feeder(uint8_t motorPin, unsigned long timeoutMs);
	// setup() 안에서 호출
	void begin();
	// 목표 사료량(g)과 센서 참조를 넘겨서 배출
	void dispense(float targetGrams, WeightSensor& sensor);

private:
	uint8_t       motorPin;
	unsigned long maxRunMs;
};

#endif // FEEDER_H