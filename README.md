# ILLO

RFID로 반려동물을 구분하고, 먹은 양까지 기록하는 자동 급식기입니다. 여러 마리를 함께 키울 때 “누가 얼마나 먹었는지” 확인하기 어려운 점에서 출발했습니다.

![ILLO 제품 렌더링](docs/assets/images/rendering.png)

RFID 태그를 읽어 급식 대상을 확인하고, 정해진 시간에 사료를 배출합니다. Load Cell로 배식량과 식사 뒤 남은 양을 재며, 결과는 MQTT로 앱에 전달합니다. 사료가 걸렸다고 판단되면 모터를 잠시 역회전해 다시 배출을 시도합니다.

## 먼저 볼 곳

- [프로젝트와 기능](docs/overview.md)
- [동작 구조](docs/architecture.md)
- [하드웨어와 전원](docs/hardware.md)
- [펌웨어 구성](docs/firmware.md)
- [MQTT 메시지](docs/mqtt.md)
- [제품 디자인](docs/design.md)
- [현재 한계](docs/limitations.md)

## 폴더 안내

```text
src/pet-feeder/       통합 ESP32 펌웨어
experiments/          부품별·기능별 테스트 스케치
docs/                 설계 문서와 이미지
```

통합 펌웨어는 [`src/pet-feeder/pet-feeder.ino`](src/pet-feeder/pet-feeder.ino)에서 시작합니다. 예전에 따로 확인했던 RFID, Load Cell, 스피커 실험은 [`experiments/`](experiments/README.md)에 남겨 두었습니다.

## 사용한 것들

ESP32 DevKit V4, Arduino, C++, RC522 RFID, HX711 Load Cell, NEMA 17, TMC2209, SG90, Wi-Fi, MQTT

## 팀

| 역할 | 구성원 |
| --- | --- |
| Hardware | 김기범, 안현서, 허재원 |
| Firmware | 김기범, 안현서 |
| Design & Modeling | 허재원, 하채연 |
| Networking | 박상현 |
| Application | 고시온 |
