# 실험 스케치

통합 코드로 합치기 전에, 부품과 기능을 따로 확인하며 만든 Arduino 스케치입니다. Arduino IDE에서 열기 편하도록 스케치 폴더와 `.ino` 파일 이름은 예전 그대로 뒀습니다.

| 경로 | 검증 대상 |
| --- | --- |
| `rfid/RFID_ver_01/` | RC522 태그 인식과 서보 연동 |
| `load-cell/loadcell_ver_01/` | HX711 보정, Load Cell·서보·배식 실험 |
| `load-cell/loadcell_mqtt_test/` | Load Cell 측정값의 MQTT 전송 |
| `audio/Speaker_ver_01/` | SPIFFS 기반 WAV 재생 웹 인터페이스 |
| `integrated/main_structure/` | 초기 통합 구조 초안 |

현재 급식기 로직은 여기보다 [`../src/pet-feeder/`](../src/pet-feeder/)에 있습니다.

## Speaker 실험 메모

WAV는 16,000 Hz, Mono 형식으로 준비했습니다. 업로드·재생 주소는 해당 폴더의 `README.txt`를 참고하면 됩니다.
