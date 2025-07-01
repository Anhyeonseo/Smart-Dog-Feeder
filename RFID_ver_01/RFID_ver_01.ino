#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// 핀 정의
#define SS_PIN     5       // RC522 SDA
#define RST_PIN    22      // RC522 RST
#define SERVO_PIN  13      // 서보모터

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo doorServo;

// 등록된 RFID 태그 ID
const String list[] = {"6c 1e b2 01" }

bool isInlist(String id) {
  for (int i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
    if (id == list[i]) return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Serial Start"); 

  SPI.begin(18, 19, 23, SS_PIN); 
  mfrc522.PCD_Init();
  Serial.println("RC522 Initialize");

  doorServo.attach(SERVO_PIN);
  doorServo.write(0); 
  Serial.println("Servo Moter Initialize");

  Serial.println("System Ready");
}

void loop() {
  // 카드가 없으면 루프 중지
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  // UID 문자열로 변환
  String tagID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) tagID += "0";
    tagID += String(mfrc522.uid.uidByte[i], HEX);
    if (i != mfrc522.uid.size - 1) tagID += " ";
  }

  Serial.print("Recognized Tag: ");
  Serial.println(tagID);

  // 리스트 확인
  if (isInlist(tagID)) {
    Serial.println("Registered tag. Open the door.");
    doorServo.write(90); // 도어 열림
    delay(5000);         // 5초간 유지
    doorServo.write(0);  // 도어 닫힘
    Serial.println("Door Close.");
  } else {
    Serial.println("Unrecognized Tag.");
  }

  delay(1000); // 중복 인식 방지
}
