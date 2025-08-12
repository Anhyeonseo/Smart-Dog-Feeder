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
	// 샘플 수 지정하여 평균 무게(그램) 반환
	float getWeightAvg(uint8_t samples = 5);

private:
	HX711   scale;
	uint8_t pinDout;
	uint8_t pinSck;
	float   calibrationFactor;
};

#endif // WEIGHT_SENSOR_H