#include "WeightSensor.h"
// setup() 안에서 호출
WeightSensor::WeightSensor(uint8_t doutPin, uint8_t sckPin, float calFactor)
	: pinDout(doutPin), pinSck(sckPin), calibrationFactor(calFactor) {}
void WeightSensor::begin() {
	// HX711 객체 초기화
	scale.begin(pinDout, pinSck);
	// 교정 인자 설정
	scale.set_scale(calibrationFactor);
	// 0점 조정
	scale.tare();
}

// 샘플 수 지정하여 평균 무게(그램) 반환
float WeightSensor::getWeightAvg(uint8_t samples) {
	return scale.get_units(samples);
}