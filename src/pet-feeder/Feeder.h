// ─────────────────────────────────────────────────────────────────────────
// File: Feeder.h
// ─────────────────────────────────────────────────────────────────────────
#ifndef FEEDER_H
#define FEEDER_H

#include <Arduino.h>
#include "WeightSensor.h"
#include <AccelStepper.h>
#include <TMCStepper.h> // 마이크로스텝 설정용
#include "config.h" // TMC 내부 저항 값 가져오기 위해 추가

class Feeder {
public:
	// 생성자에서 DC 모터 제어 핀과 타임아웃(ms) 설정
	Feeder(uint8_t stepPin, uint8_t dirPin, uint8_t enPin, unsigned long timeoutMs);
	// setup() 안에서 호출
	void begin();
	// 목표 사료량(g)과 센서 참조를 넘겨서 배출
	void dispense(float targetGrams, WeightSensor& sensor);

private:
	uint8_t       stepPin;       // 스텝 제어 핀
	uint8_t       dirPin;        // 방향 제어 핀
	uint8_t		  enPin; // 모터 활성화 핀 (필요시 변경)
	unsigned long maxRunMs; // 최대 실행 시간(ms) 설정
	AccelStepper stepper;       // AccelStepper 객체 생성
	TMC2209Stepper tmcDriver; // TMC 드라이버 객체 생성
};

#endif // FEEDER_H