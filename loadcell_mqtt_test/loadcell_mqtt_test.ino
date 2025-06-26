#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HX711.h>

// 1. Wi-Fi 설정
const char* ssid          = "SK_WiFiGIGAE498";
const char* wifi_password = ""; // 설정해주세요

// 2. HiveMQ Cloud 접속 정보
const char* mqtt_server = "58870451efe34c9f83dece69cbf72f38.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;               // TLS 포트
const char* mqtt_user   = "feeder01";         // Access Management에서 만든 ID
const char* mqtt_pass   = "";     // 비밀번호 카톡 참고

// 3. 로드셀 핀 및 객체
const int PIN_DOUT = 19;   // DT
const int PIN_SCK  = 18;   // SCK
HX711 scale;
float calibration_factor = 1.0;   // 자동 계산될 값

// 4. 토픽 & 디바이스 ID
const char* device_id      = "ESP_FEEDER_01";
String      topic_status   = String("feeder/") + device_id + "/status";
String      topic_command  = String("feeder/") + device_id + "/command";

// 5. 퍼블리시 간격
const unsigned long PUBLISH_INTERVAL = 10000;

WiFiClientSecure secureClient;
PubSubClient      client(secureClient);

void waitForEnter() {
  while (Serial.read() >= 0) ;          
  while (!Serial.available()) ;         
  while (Serial.available()) Serial.read();
}

float readFloatBlocking() {
  String str = "";
  while (true) {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (str.length()) return str.toFloat();
      } else {
        str += c;
      }
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.printf("수신[%s]: %s\n", topic, msg.c_str());
  if (String(topic) == topic_command && msg == "feed_now") {
    Serial.println("급식 트리거!");
    // ▶ 실제 모터 구동 코드 여기에 추가
  }
}

void connectWiFi() {
  Serial.print("WiFi 연결 중");
  WiFi.begin(ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" 연결 완료!");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("MQTT 재접속 시도...");
    if (client.connect(device_id, mqtt_user, mqtt_pass)) {
      Serial.println(" 성공!");
      client.subscribe(topic_command.c_str());
    } else {
      Serial.printf(" 실패 rc=%d\n", client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // --- HX711 교정 단계 ---
  scale.begin(PIN_DOUT, PIN_SCK);
  while (!scale.is_ready()) {
    Serial.println("Waiting for HX711...");
    delay(200);
  }

  Serial.println("\n=== STEP 1/2: TARE ===");
  Serial.println("로드셀 위에 아무것도 올리지 말고 <Enter> 키를 눌러 주세요.");
  waitForEnter();
  scale.set_scale();
  scale.tare();
  Serial.println(" > 0점(tare) 완료!\n");

  Serial.println("=== STEP 2/2: CALIBRATION ===");
  Serial.println("정확한 무게(예: 100g) 추 올리고 숫자 입력 후 <Enter>");
  float known_weight = readFloatBlocking();
  Serial.printf("입력된 무게 = %.2f g\n", known_weight);

  long raw = scale.get_value(10);
  Serial.printf("RAW reading = %ld\n", raw);

  calibration_factor = raw / known_weight;
  Serial.printf("계산된 calibration factor = %.2f\n", calibration_factor);

  Serial.println("\n추를 내려놓고 <Enter> 누르세요.");
  waitForEnter();
  scale.set_scale(calibration_factor);
  scale.tare();
  Serial.println("★ 교정 완료! ★\n");

  // --- 네트워크 & MQTT 초기화 ---
  connectWiFi();

  secureClient.setInsecure();  // 개발단계만. 실서비스는 setCACert() 사용
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  static unsigned long lastPub = 0;
  unsigned long now = millis();
  if (now - lastPub > PUBLISH_INTERVAL) {
    lastPub = now;
    float weight = scale.get_units(10);
    char buf[16];
    dtostrf(weight, 6, 2, buf);
    client.publish(topic_status.c_str(), buf);
    Serial.printf("Published[%s]: %s\n", topic_status.c_str(), buf);
  }
}
