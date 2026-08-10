# 펌웨어

지금 쓰는 통합 펌웨어는 [`../src/pet-feeder/`](../src/pet-feeder/)에 있습니다. 시작점은 [`../src/pet-feeder/pet-feeder.ino`](../src/pet-feeder/pet-feeder.ino)입니다.

## 파일 역할

| 파일 | 역할 |
| --- | --- |
| `pet-feeder.ino` | 초기화와 메인 루프 |
| `config.h` | 핀·네트워크 등 공통 설정 |
| `Feeder.*` | 배식 및 모터 제어 |
| `WeightSensor.*` | HX711/Load Cell 측정 |
| `Rfid.*` | RFID 태그 읽기와 판별 |
| `MqttHandler.*` | MQTT 연결과 메시지 처리 |
| `Unit_task.*` | 작업 단위와 급식 흐름 조정 |

## 동작 원칙

- RFID, 무게 측정, 모터, MQTT, 스케줄을 짧은 작업으로 나누어 반복 처리합니다.
- 배출 중 무게 변화가 일정 시간 동안 없으면 사료 걸림 가능성을 판단합니다.
- 걸림이 의심되면 역회전 후 재배출을 시도하고, 반복 실패하면 급식을 중단하고 오류 상태를 보고합니다.

부품별로 먼저 돌려 본 Arduino 스케치는 [`../experiments/README.md`](../experiments/README.md)에 남겨 뒀습니다.
