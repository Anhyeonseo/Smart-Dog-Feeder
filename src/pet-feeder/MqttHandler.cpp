// MqttHandler.cpp (디버깅 로그 추가 버전)

#include "MqttHandler.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

WiFiClientSecure secureClient;
PubSubClient     client(secureClient);
Preferences preferences;

// extern Preferences preferences; // ino파일에서 정의된 Preferences 객체 가져오기 (더미 데이터 삭제 시)

volatile bool newMqttMsgFlag = false; // 새 스케줄 도착 알리는 플래그
String mqttMessagePayload;            // 도착한 메시지를 보관하는 변수
volatile bool scheduleUpdateFlag = false; 

// 논블로킹 재연결을 위한 타이머
unsigned long lastReconnectAttempt = 0;
const long reconnectInterval = 5000; // 5초

// MQTT 클라이언트 초기 설정 함수
void setupMqtt() {
  Serial.println("[MQTT] 핸들러 설정 시작...");
  secureClient.setInsecure();
  client.setServer(Config::MQTT::SERVER, Config::MQTT::PORT);
  client.setCallback(callback);
  Serial.println("[MQTT] 핸들러 설정 완료.");
}

// 메시지 수신 시 실행될 콜백 함수
/*
콜백 함수가 너무 무거움. 수신, 처리 로직 별도 분리
*/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("\n--- [MQTT] 메시지 수신 (토픽: %s) ---\n", topic);
    
  // 1. 메시지를 복사(보관)
  mqttMessagePayload = "";
  for (int i = 0; i < length; i++) {
      mqttMessagePayload += (char)payload[i];
  }
    
  // 도착 플래그
  newMqttMsgFlag = true;
}

// MQTT 연결 유지 함수
void loopMqtt() {
    if (!client.connected()) {
        // 마지막 재시도 후 5초가 지났다면
        if (millis() - lastReconnectAttempt >= reconnectInterval) {
            lastReconnectAttempt = millis();
            Serial.print("[MQTT] 서버 접속 시도... ");
            if (client.connect("ESP32_PetFeeder_Client", Config::MQTT::USER, Config::MQTT::PASSWORD)) {
                Serial.println("성공!");
                client.subscribe(Config::MQTT::TOPIC_COMMAND);
            } else {
                Serial.printf("실패, rc=%d. %ldms 후 재시도\n", client.state(), reconnectInterval);
            }
        }
    } else {
        // 연결이 되어 있을 때만 메시지 수신을 확인합니다. (매우 빠른 함수)
        client.loop();
    }
}

void handleMqttMessages() {
  if (!newMqttMsgFlag) return; // 새 메시지가 없으면 바로 리턴

  Serial.println("--- [MQTT] 메시지 처리 시작 ---");
  newMqttMsgFlag = false; // 깃발을 바로 내립니다 (처리 시작을 알림)
  String message = mqttMessagePayload; // 편지함에서 메시지를 꺼냅니다.
  
  Serial.printf("[MQTT] 내용: %s\n", message.c_str());

  // 1. 수신된 메시지를 파싱합니다.
  StaticJsonDocument<256> receivedDoc;
  DeserializationError error = deserializeJson(receivedDoc, message);
  if (error) {
    Serial.print("[MQTT] 오류: 수신된 메시지 JSON 파싱 실패: ");
    Serial.println(error.c_str());
    return;
  }

  const char* action = receivedDoc["action"];
  if (!action) {
    Serial.println("[MQTT] 오류: 'action' 필드가 없습니다.");
    return;
  }
  
  // 2. Preferences에서 기존 스케줄 목록을 불러옵니다.
  preferences.begin("feeder_settings", false);
  String scheduleListJson = preferences.getString("schedules", "[]");
  
  // 3. 불러온 스케줄 목록을 파싱합니다.
  StaticJsonDocument<1024> scheduleDoc;
  error = deserializeJson(scheduleDoc, scheduleListJson);
  if (error) {
    Serial.print("[MQTT] 오류: 저장된 스케줄 목록 파싱 실패: ");
    Serial.println(error.c_str());
    preferences.end();
    return;
  }
  JsonArray scheduleArray = scheduleDoc.as<JsonArray>();

  // 4. action에 따라 스케줄 목록(scheduleArray)을 수정합니다.
  if (strcmp(action, "set_schedule") == 0) {
    if (receivedDoc.containsKey("id")) {
      // [수정 로직]
      long existingId = receivedDoc["id"];
      bool updated = false;
      for (JsonObject schedule : scheduleArray) {
        if (schedule["id"] == existingId) {
          schedule["time"] = receivedDoc["time"].as<String>();
          schedule["amount"] = receivedDoc["amount"].as<int>();
          updated = true;
          break;
        }
      }
      if(updated) Serial.printf("[MQTT] 처리: ID %ld 스케줄 수정 완료.\n", existingId);
      else Serial.printf("[MQTT] 경고: 수정할 ID %ld를 찾지 못했습니다.\n", existingId);

    } else if (receivedDoc.containsKey("temp_id")) {
      // [추가 로직]
      long newRealId = millis();
      JsonObject newSchedule = scheduleArray.createNestedObject();
      newSchedule["id"] = newRealId;
      newSchedule["time"] = receivedDoc["time"].as<String>();
      newSchedule["amount"] = receivedDoc["amount"].as<int>();
      Serial.printf("[MQTT] 처리: 새 스케줄 추가 완료 (실제 ID: %ld)\n", newRealId);

      // [응답 전송]
      StaticJsonDocument<200> responseDoc;
      responseDoc["action"] = "schedule_added";
      responseDoc["temp_id"] = receivedDoc["temp_id"].as<String>();
      responseDoc["id"] = newRealId;
      responseDoc["time"] = newSchedule["time"];
      responseDoc["amount"] = newSchedule["amount"];
      char jsonBuffer[256];
      serializeJson(responseDoc, jsonBuffer);
      client.publish(Config::MQTT::TOPIC_RESPONSE, jsonBuffer);
    }
  } else if (strcmp(action, "delete_schedule") == 0) {
    // [삭제 로직]
    long idToDelete = receivedDoc["id"];
    for (int i = 0; i < scheduleArray.size(); i++) {
        if (scheduleArray[i]["id"] == idToDelete) {
            scheduleArray.remove(i);
            Serial.printf("[MQTT] 처리: ID %ld 스케줄 삭제 완료.\n", idToDelete);
            break;
        }
    }
  }

  // 5. 수정된 최종 목록을 다시 문자열로 변환하여 저장합니다.
  String newScheduleListJson;
  serializeJson(scheduleDoc, newScheduleListJson); // scheduleDoc을 직접 사용
  preferences.putString("schedules", newScheduleListJson);
  preferences.end();
  
  Serial.printf("[MQTT] 최종 저장된 스케줄: %s\n", newScheduleListJson.c_str());
  Serial.println("--------------------------");
  scheduleUpdateFlag = true;
}

// 상태 보고 함수
void sendMealStatus(long id, float remainingWeight) {
    if (!client.connected()) {
        Serial.println("[MQTT] 상태 보고 실패: 연결되지 않음.");
        return;
    }
    StaticJsonDocument<200> doc;
    struct tm now;
    getLocalTime(&now);
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", now.tm_hour, now.tm_min);

    doc["id"] = id;
    doc["time"] = timeStr;
    doc["weight"] = remainingWeight;
    char jsonBuffer[128];
    serializeJson(doc, jsonBuffer);
    
    Serial.printf("[MQTT] 상태 보고 전송 (ID: %ld, 잔량: %.1fg)\n", id, remainingWeight);
    client.publish(Config::MQTT::TOPIC_STATUS, jsonBuffer);
}

// 스케줄 변경 여부 확인 함수
bool isScheduleUpdated() {
  if (scheduleUpdateFlag) {
    scheduleUpdateFlag = false;
    return true;
  }
  return false;
}

// 저장된 스케줄을 JSON 문자열로 반환하는 함수
String getSchedulesJson() {
    preferences.begin("feeder_settings", true);
    String json = preferences.getString("schedules", "[]");
    preferences.end();
    return json;
}