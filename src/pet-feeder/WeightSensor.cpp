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
	scale.tare(20);
}

// 새로운 논블로킹 업데이트 함수
void WeightSensor::update() {
    if (scale.is_ready()) {
        currentWeight = scale.get_units(1);
    }
}

// 새로운 논블로킹 값 반환 함수
float WeightSensor::getWeight() {
    return currentWeight;
}

void WeightSensor::setCurrentWeight() {
	if(!scale.is_ready()) {
		Serial.println("HX711 not found.");
		return;
	}
	currentWeight = scale.get_units(5);
}

// 샘플 수 지정하여 평균 무게(그램) 반환
float WeightSensor::getWeightAvg(uint8_t samples) {
	return scale.get_units(samples);
}