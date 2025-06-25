/***************************************************************************
 * ESP32 DevKitC + HX711 + 500 g 로드셀
 * 1) 전원 켬  → 시리얼창 지시에 따라 0점(tare)
 * 2) 표준추 얹기 → 실제 무게(예: 100) 입력
 * 3) 자동으로 calibration factor 계산 후
 *    0.5 초마다 Weight(g) 출력
 *
 * ─ 핀 배치 (케이블 정리용 변경) ─
 *   HX711 DT  → GPIO 19
 *   HX711 SCK → GPIO 18
 ***************************************************************************/
#include "HX711.h"

const int PIN_DOUT = 19;   // DT
const int PIN_SCK  = 18;   // SCK

HX711 scale; // scale 객체 생성
float calibration_factor = 1.0;   // 첫 실행 땐 1.0 (자동 계산됨)

void setup() {
  Serial.begin(115200);
  delay(100);

  // 1. HX711 시작
  scale.begin(PIN_DOUT, PIN_SCK);
  while (!scale.is_ready()) {
    Serial.println("Waiting for HX711...");
    delay(200);
  }

  // 2. 0점 잡기
  Serial.println("\n=== STEP 1 / 2  :  TARE ===");
  Serial.println("로드셀 위에 아무것도 올리지 말고 <Enter> 키를 눌러 주세요.");
  waitForEnter();
  scale.set_scale();  // factor = 1
  scale.tare();       // 오프셋 저장
  Serial.println(" > 0점(tare) 완료!\n");

  // 3. 교정용 추 올리고 실제 무게 입력
  Serial.println("=== STEP 2 / 2  :  CALIBRATION ===");
  Serial.println("로드셀 상판에 '정확히 몇 g 인지 아는' 추를 올린 뒤\n"
                 "예) 100  → 100 입력 후 <Enter>");
  float known_weight = readFloatBlocking();
  Serial.print("입력한 실제 무게 = ");
  Serial.print(known_weight);
  Serial.println(" g");

  // 4. RAW 값 읽어 factor 계산
  long raw = scale.get_value(10);               // 10회 평균
  Serial.print("RAW reading = ");
  Serial.println(raw);

  calibration_factor = raw / known_weight; // 부호 보정(압축 = 음수)
  Serial.print("계산된 calibration factor = ");
  Serial.println(calibration_factor, 2);

  // ☆ 추를 내려놓고 Enter 키 기다리기
  Serial.println("\n추(무게)를 내려놓고 <Enter>를 누르세요.");
  waitForEnter();

  scale.set_scale(calibration_factor);
  scale.tare();
  Serial.println("\n★ 교정 완료! 0.5초마다 무게를 표시합니다 ★");
}

void loop() {
  float weight = scale.get_units(10);   // 10회 평균
  Serial.print("Weight: ");
  Serial.print(weight, 2);
  Serial.println(" g");
  delay(500);
}

/* ───────────────────────────── UTILITY ────────────────────────────── */
void waitForEnter() {
  while (Serial.read() >= 0) ;          // 입력 버퍼 비우기
  while (!Serial.available()) ;         // 입력 대기
  while (Serial.available()) Serial.read();  // <Enter> 제거
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