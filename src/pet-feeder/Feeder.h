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
	// FEEDER 동작 상태 정의
	enum FeederState {
		IDLE,         // 대기 상태
		DISPENSING,   // 모터회전상태
		// DISPENSING_PAUSE,   // 배식 중 무게측정
		REVERSING,    // 사료 걸림 감지 시, 반대 회전
		DONE,      	  // 작업 상태
		STOPPED       // 중지 상태
	};

	// 생성자에서 DC 모터 제어 핀과 타임아웃(ms) 설정
	Feeder(uint8_t stepPin, uint8_t dirPin, uint8_t enPin, unsigned long timeoutMs);
	// setup() 안에서 호출
	void begin();
	
	// 논블로킹 함수로 리팩토링

	// 급식 시작시키는 명령 함수 
	void startDispense(float targetgrams, WeightSensor& sensor);

	// mainloop()에서 지속적으로 호출되어야하는 상태 관리 함수, 배식 중
	void update(WeightSensor& sensor);

	// 급식 중지 명령 함수
	void stop();

	// 현재 상태 반환 함수
	FeederState getState() const { return currentState; }

	void resetState(); 

	// 현재 배식된 양 반환 함수
	float getDispensedAmount() const { return dispensedAmount; }

private:
	// uint8_t       stepPin;       // 스텝 제어 핀
	// uint8_t       dirPin;        // 방향 제어 핀
	// uint8_t		  enPin; // 모터 활성화 핀 (필요시 변경)
	unsigned long maxRunMs; // 최대 실행 시간(ms) 설정
	AccelStepper stepper;       // AccelStepper 객체 생성
	TMC2209Stepper tmcDriver; // TMC 드라이버 객체 생성

	// 상태 관리를 위한 멤버 변수
	FeederState currentState; // 현재 상태
	float targetGrams; 
	float startWeight; // 시작 시 무게
	float dispensedAmount; // 현재 배식된 양

	// 타이머 변수
	unsigned long lastWeightCheckTime; // 마지막 무게 측정 시간
	const int weightCheckInterval = 1000; // 무게 측정 간격 (1초)
};

#endif // FEEDER_H