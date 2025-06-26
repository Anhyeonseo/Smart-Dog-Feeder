/**********************************************************************
 *  ESP32 DevKitC  +  HX711 + 500 g Load Cell  +  MG995 Servo
 *  ─ 핀 배치  ──────────────────────────────────────────────
 *    HX711 DT      → GPIO 19
 *         SCK      → GPIO 18
 *    Servo Signal  → GPIO 26        (MG995 갈색=GND,  빨강=5.5 V)
 *    모든 GND       → 공통(브레드보드 GND 레일)
 *  ─ 전원 ────────────────────────────────────────────────
 *    ESP32 : USB-5 V → 3.3 V LDO
 *    Servo : AA × 6 → MP1584EN ↓ 5.5 V → 콘덴서 1000 µF → MG995
 *********************************************************************/
 #include <ESP32Servo.h>
#include "HX711.h"

/* ───── 서보 파라미터 ───── */
const int   SERVO_PIN    = 26;
const int   CLOSE_ANGLE  = 0;    // 닫힘
const int   OPEN_ANGLE   = 90;   // 열림
Servo feeder;

/* ───── HX711 파라미터 ──── */
const int   HX_DOUT_PIN  = 19;
const int   HX_SCK_PIN   = 18;
HX711 scale;
const float CAL_FACTOR   = 3362.9;   // ← 본인 로드셀 교정값

/* ───── 유틸 ───── */
float getWeight(uint8_t n = 10) { return scale.get_units(n); }

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== 자동 급식기 하드웨어 셀프-테스트 시작 ===");

  /* 1) 서보 초기화 */
  feeder.setPeriodHertz(50);      // MG995: 20 ms(50 Hz)
  feeder.attach(SERVO_PIN);
  feeder.write(CLOSE_ANGLE);
  Serial.println("Servo READY (닫힘 각도)");

  /* 2) HX711 초기화 */
  scale.begin(HX_DOUT_PIN, HX_SCK_PIN);
  scale.set_scale(CAL_FACTOR);    // g 단위로 변환
  scale.tare();                   // 0 점
  Serial.println("HX711 READY (tare 완료)");

  /* 3) 자동 동작 데모 */
  Serial.println("\n--- 데모: 서보 열기 → 3 초 → 닫기 ---");
  feeder.write(OPEN_ANGLE);
  delay(3000);
  feeder.write(CLOSE_ANGLE);
  Serial.println("데모 완료. 명령 대기 중…\n"
                 "  [o] 열기  [c] 닫기  [w] 무게 읽기  [h] 도움말");
}

void loop() {
  /* 시리얼 문자 명령 처리 */
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'o':  // open
        feeder.write(OPEN_ANGLE);
        Serial.println("→ 서보: OPEN");
        break;
      case 'c':  // close
        feeder.write(CLOSE_ANGLE);
        Serial.println("→ 서보: CLOSE");
        break;
      case 'w':  // weight
        Serial.printf("현재 무게 = %.2f g\n", getWeight());
        break;
      case 'h':
      default:
        Serial.println("[o] 열기  [c] 닫기  [w] 무게읽기");
        break;
    }
  }

  /* 1 초마다 상태 로그 (무게 + 서보 각도) */
  static unsigned long tPrev = 0;
  if (millis() - tPrev >= 1000) {
    tPrev = millis();
    Serial.printf("상태 | 무게: %.2f g  | 서보각: %d°\n",
                  getWeight(), feeder.read());
  }
}