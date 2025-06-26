#include <WiFi.h>
#include <time.h>        // NTP 시간 동기
#include "HX711.h"

/*  Wi-Fi 접속 정보  (공란 → 핫스팟 사용)  */
const char* SSID     = "SK_WiFiGIGAE498_2.4G";
const char* PASSWORD = "1704017757";

/* ───────────────── HX711 ───────────────── */
const int PIN_DOUT = 19;
const int PIN_SCK  = 18;
HX711 scale;
const float CAL_FACTOR = 3352.05;   // 교정 완료된 값 기입
/* ───────────────── 모터 ─────────────────── */
const int PIN_MOTOR = 25;
const unsigned long MAX_RUN_MS = 20'000; // 20 초 안전 타임아웃

/* ───────────────── 급식 테이블 ───────────── */
struct FeedTask { uint8_t hour, minute; float target_g; };
FeedTask tasks[] = { {8,0,120},{18,30,80} };
const int NUM_TASKS = sizeof(tasks)/sizeof(tasks[0]);

/* 상태 관리 */
bool task_done[NUM_TASKS] = {false};   // 오늘 이미 수행했는가? ..?

void setup() {
  Serial.begin(115200);
  pinMode(PIN_MOTOR, OUTPUT);    //.??
  digitalWrite(PIN_MOTOR, LOW);        // 모터 OFF
  
  /* Wi-Fi & NTP (간단 동기) */
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(200);
  configTime(9*3600, 0, "pool.ntp.org");   // Asia/Seoul = UTC+9
  
  /* HX711 */
  scale.begin(PIN_DOUT, PIN_SCK);
  scale.set_scale(CAL_FACTOR);
  scale.tare();
}

void loop() {
  /* 1) 현재 시각 얻기 */
  struct tm now; if (!getLocalTime(&now)) { delay(1000); return; }

  /* 2) 각 Task 비교 */
  for (int i=0;i<NUM_TASKS;i++) {
    if (task_done[i]) continue;        // 이미 끝난 급식
    if (now.tm_hour==tasks[i].hour && now.tm_min==tasks[i].minute) {
      dispense(tasks[i].target_g);
      task_done[i]=true;
    }
  }

  /* 3) 자정이면 리셋 */
  if (now.tm_hour==0 && now.tm_min==0) memset(task_done,0,sizeof(task_done));

  delay(5000); // 5 초 주기
}

/* ───────────────────── 배출 루틴 ───────────────────── */
void dispense(float target_g)
{
  Serial.printf("\n▶ 급식 시작 : 목표 %.1f g\n", target_g);
  float start_weight = getWeightAvg();
  unsigned long t0 = millis();
  digitalWrite(PIN_MOTOR, HIGH);            // 모터 ON

  while (millis()-t0 < MAX_RUN_MS) {        // 타임아웃 안전장치
    float now_w = getWei.ghtAvg();
    float diff  = now_w - start_weight;     // 배출된 양(+)
    Serial.printf("  진행: %.1f g / %.1f g\r", diff, target_g);

    if (diff >= target_g) break;            // 목표 달성
  }
  digitalWrite(PIN_MOTOR, LOW);             // 모터 OFF
  Serial.println("\n■ 급식 완료");
  delay(1000);
}

/* HX711 평균 래퍼 */
float getWeightAvg(uint8_t n=10) {
  return scale.get_units(n);
}
