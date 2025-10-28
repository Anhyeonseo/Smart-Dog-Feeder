// ─────────────────────────────────────────────────────────────────────────
// File: WeightSensor.h
// ─────────────────────────────────────────────────────────────────────────
#ifndef WEIGHT_SENSOR_H
#define WEIGHT_SENSOR_H

#include <Arduino.h>
#include <HX711.h>

class WeightSensor {
public:
	// 생성자에서 핀과 교정 인자 설정
	WeightSensor(uint8_t doutPin, uint8_t sckPin, float calFactor);
	// setup() 안에서 호출
	void begin();

	// --- 새로운 논블로킹 함수들 (Feeder가 사용) ---
    void update();      // 루프에서 항상 호출, 준비될 때만 무게 갱신
    float getWeight();  // 갱신된 무게를 즉시 반환

	// 샘플 수 지정하여 평균 무게(그램) 반환
	float getWeightAvg(uint8_t samples = 5);
	
	void setCurrentWeight();	

private:
	HX711   scale;
	uint8_t pinDout;
	uint8_t pinSck;
	float   calibrationFactor;
	float currentWeight = 0.0; // 논블로킹용 최신 무게 저장 변수
};

#endif // WEIGHT_SENSOR_H