# 시스템 아키텍처

```text
Application
    │ API
    ▼
Server
    │ MQTT
    ▼
MQTT Broker
    │ MQTT
    ▼
ESP32 Feeder
 ├── RFID Reader
 ├── Door Servo
 ├── Stepper Motor + Auger Screw
 ├── Load Cell + HX711
 ├── Schedule Manager
 └── Feeding State Machine
```

앱은 서버에 반려동물 정보와 급식 스케줄을 요청합니다. 서버는 스케줄과 급식 기록을 관리하고, 급식기에 전달할 내용은 MQTT Broker를 통해 ESP32로 보냅니다. 급식기가 RFID와 무게를 확인해 한 번의 급식을 마치면, 배식량과 잔량은 같은 경로로 서버에 전달되고 앱에서 확인합니다.

## 급식 흐름

```text
앱에서 반려동물·RFID 태그 등록과 스케줄 설정
  → 서버에 저장
  → MQTT로 급식기에 스케줄 전송
  → 급식 시각 도달
  → RFID로 대상 확인
  → 문 열기 및 사료 배출
  → 목표 무게 도달 시 배출 중지
  → RFID 이탈 후 잔량 측정
  → MQTT status 발행
  → 서버에 급식 결과 저장
```

## 상태 머신

급식은 아래 순서로 상태를 옮겨 가며 진행합니다.

```text
IDLE → WAIT_FOR_PET → OPEN_DOOR → DISPENSING
  → EATING → MEASURE_REMAINDER → REPORT_RESULT → IDLE
```

스케줄 시각, 등록 태그 감지, 목표 무게 도달, 태그 이탈, 사료 걸림이 이 흐름을 바꾸는 주요 이벤트입니다. 문제가 생기면 진행 중인 단계와 관계없이 오류 처리로 넘길 수 있게 잡았습니다.

## 동시 작업 모델

RFID, 무게, 모터, MQTT, 스케줄 처리는 한쪽이 오래 멈추지 않도록 짧게 나눠 반복합니다. 그래서 급식 중에도 센서를 읽고 MQTT 메시지를 확인하거나 걸림을 감지할 수 있습니다.
