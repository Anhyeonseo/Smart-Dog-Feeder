// MqttHandler.h

#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include <Arduino.h>

// MQTT 클라이언트 초기 설정
void setupMqtt();

// MQTT 연결 유지 (메인 loop에서 항상 호출)
void loopMqtt();

void sendMealStatus(long id, float remainingWeight); // 함수 이름 변경
bool isScheduleUpdated(); // 스케줄 변경 여부 플래그
String getSchedulesJson(); // 변경된 스케줄 목록을 JSON 문자열로 반환

// // 로드셀로 측정한 무게(잔량)를 앱으로 보내는 함수
// void sendWeightData(long id, String time, float weight);

// // 새 스케줄 등록 시 앱으로 응답 전송 (temp_id → id 매핑)
// void publish_schedule_added_response(long assignedId, const String& tempId, const String& time, int amount);

#endif
