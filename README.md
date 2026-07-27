# ILLO

다묘·다견 가정에서 반려동물별 식사량을 관리하기 위한 개체 인식 자동 급식기다.

반려동물의 목걸이에 부착된 RFID 태그로 개체를 식별하고, 설정된 스케줄에 따라 사료를 배식한다. Load Cell을 이용해 실제 배식량과 식사 후 잔량을 측정하며, 급식 기록은 MQTT를 통해 애플리케이션과 동기화된다.

## 프로젝트 배경

여러 마리의 반려동물을 함께 키우는 가정에서는 밥그릇을 따로 사용하더라도 각 개체가 실제로 자기 몫을 먹었는지 확인하기 어렵다.

성격이 온순한 반려동물은 다른 개체에게 사료를 빼앗길 수 있고, 체중 관리가 필요한 반려동물은 보호자가 직접 지켜보지 않으면 식사량을 조절하기 어렵다. 보호자가 외출 중인 경우에는 어떤 개체가 식사했는지, 실제로 얼마나 먹었는지 확인하기도 어렵다.

기존 자동 급식기는 정해진 시간에 설정된 양을 배식할 수 있지만, 다음 정보까지 확인하기는 어렵다.

* 어떤 반려동물이 먹었는지
* 다른 개체가 대신 먹지는 않았는지
* 배식된 사료 중 실제로 얼마나 먹었는지
* 이전 식사에서 사료를 얼마나 남겼는지

ILLO는 개체 인식, 정량 배식, 잔량 측정, 급식 기록을 하나의 시스템으로 통합해 이러한 문제를 해결하는 것을 목표로 한다.

## 주요 기능

### RFID 기반 개체 식별

반려동물의 목걸이에 부착된 RFID 태그를 이용해 급식기에 접근한 개체를 확인한다.

등록된 태그가 감지되면 해당 반려동물의 급식 스케줄과 설정된 급식량을 확인한다. 등록되지 않은 태그이거나 급식 대상이 아닌 경우에는 급식 공간을 열지 않는다.

### 개체별 급식 스케줄

애플리케이션에서 반려동물별로 다음 정보를 설정할 수 있다.

* 급식 시간
* 급식량
* 반복 요일
* RFID 태그 정보
* 스케줄 활성화 여부

설정된 스케줄은 MQTT를 통해 급식기로 전달된다.

### Load Cell 기반 정량 배식

ILLO는 Stepper Motor의 회전 시간만으로 배식량을 추정하지 않는다.

사료를 배출하는 동안 Load Cell로 밥그릇의 무게를 측정하고, 목표 무게에 도달하면 모터를 정지한다. 이를 통해 사료의 크기나 배출 속도 변화가 있더라도 실제 무게를 기준으로 배식량을 제어할 수 있다.

### 잔량 및 실제 섭취량 측정

식사가 끝난 뒤 밥그릇에 남아 있는 사료의 무게를 다시 측정한다.

```text
실제 섭취량 = 배식량 - 잔량
```

배식량, 잔량, 실제 섭취량은 애플리케이션에 저장되며, 개체별 식사 기록을 확인하는 데 사용된다.

### 다음 배식량 보정

이전 식사에서 남긴 사료가 있는 경우 다음 배식 시 잔량을 고려해 추가로 배식할 양을 계산한다.

예를 들어 목표 급식량이 80g이고 밥그릇에 15g이 남아 있다면, 다음 배식에서는 기존 잔량을 고려해 추가 배식량을 조절할 수 있다.

이 기능은 중복 배식과 사료 낭비를 줄이기 위한 것이다.

### 사료 걸림 감지 및 복구

Auger Screw 내부에 사료가 걸리면 모터가 회전하더라도 Load Cell의 측정값이 증가하지 않는다.

ILLO는 일정 시간 동안 무게 변화가 없으면 사료 걸림 가능성이 있다고 판단하고 다음 순서로 복구를 시도한다.

```text
정회전
→ 무게 변화 확인
→ 걸림 감지
→ 역회전
→ 다시 정회전
→ 정상 배식 재개
```

복구를 반복해도 무게 변화가 나타나지 않으면 배식을 중단하고 오류 상태로 전환한다.

### 식사 기록 시각화

급식 결과는 애플리케이션의 Local DB에 저장한다.

보호자는 기간별 식사 기록을 통해 다음과 같은 변화를 확인할 수 있다.

* 특정 시간대에 사료를 자주 남기는지
* 평소보다 섭취량이 감소했는지
* 특정 개체가 반복적으로 과식하는지
* 급식량을 조정할 필요가 있는지

### 적응 훈련

급식기에는 보호자가 직접 문을 열 수 있는 수동 개폐 기능을 포함했다.

보호자가 수동으로 문을 열고 간식을 제공하면 반려동물이 급식기를 낯선 장치가 아니라 보상을 제공하는 대상으로 인식하도록 훈련할 수 있다.

## System Architecture

ILLO는 애플리케이션, MQTT Broker, ESP32 기반 급식기로 구성된다.

```text
Application
     │
     │ MQTT
     ▼
MQTT Broker
     │
     ▼
ESP32 Feeder
├── RFID Reader
├── Door Servo
├── Stepper Motor
├── Load Cell
├── Feeding State Machine
├── Schedule Manager
└── MQTT Client
```

애플리케이션에서 생성한 급식 스케줄은 MQTT Broker를 거쳐 ESP32로 전달된다.

ESP32는 스케줄과 RFID 정보를 기준으로 급식 동작을 수행하며, 급식이 끝난 뒤 배식량과 잔량을 애플리케이션으로 전송한다.

## 전체 동작 흐름

```text
애플리케이션에서 반려동물과 RFID 태그 등록
        │
        ▼
개체별 급식 스케줄과 급식량 설정
        │
        ▼
MQTT command topic으로 스케줄 전송
        │
        ▼
ESP32가 스케줄 저장 후 response 전송
        │
        ▼
급식 시간 도달
        │
        ▼
RFID로 접근한 반려동물 확인
        │
        ▼
등록된 개체이면 급식 공간 개방
        │
        ▼
Stepper Motor와 Auger Screw로 사료 배출
        │
        ▼
Load Cell로 배식량 측정
        │
        ▼
목표 무게 도달 시 배식 종료
        │
        ▼
RFID 이탈 감지 후 식사 종료 판단
        │
        ▼
Load Cell로 잔량 측정
        │
        ▼
급식 결과를 MQTT status topic으로 전송
        │
        ▼
애플리케이션에서 기록 저장 및 시각화
```

## Hardware

### ESP32 DevKit V4

ESP32는 급식기의 Main Controller로 사용한다.

주요 역할은 다음과 같다.

* RFID 태그 인식
* Servo Motor 제어
* Stepper Motor 제어
* Load Cell 측정
* 급식 스케줄 관리
* Finite State Machine 실행
* Wi-Fi 연결
* MQTT 메시지 송수신

ESP32는 Wi-Fi를 기본 지원하고 여러 주변장치를 제어할 수 있어 초기 prototype의 controller로 선정했다.

### RFID

사용 부품:

* RC522 RFID Module
* RFID Tag

RFID 태그는 반려동물의 목걸이에 부착하며, RC522는 급식기 앞에 접근한 태그의 UID를 읽는다.

실제 테스트에서 RC522의 안정적인 인식 거리는 약 2~3cm로 확인됐다. 초기 prototype에서 기능을 검증하기에는 사용할 수 있지만, 반려동물이 자연스럽게 접근하는 실제 환경에서는 인식 범위가 부족하다.

향후에는 134.2kHz RFID Reader와 외부 안테나를 적용하는 방안을 고려한다.

### Door Servo

사용 부품:

* SG90 Servo Motor

Servo Motor는 RFID 인식 후 급식 공간의 문을 여닫는 데 사용한다.

등록된 개체가 접근하면 문을 열고, 식사가 끝나거나 오류가 발생하면 문을 닫는다.

현재 prototype에서는 SG90을 사용하지만, 실제 제품에서는 문의 무게와 반복 동작 횟수를 고려해 더 높은 torque와 내구성을 가진 actuator를 검토할 필요가 있다.

### Feeding Mechanism

사용 부품:

* NEMA 17HS4401S Stepper Motor
* TMC2209 Stepper Motor Driver
* Auger Screw

사료 배출에는 Stepper Motor와 Auger Screw 구조를 적용했다.

Auger Screw는 나선형 구조가 회전하면서 사료를 배출구 방향으로 밀어내는 방식이다. 모터 회전량을 제어하기 쉽고, 사료가 걸렸을 때 역회전을 적용할 수 있다는 장점이 있다.

TMC2209는 NEMA 17 Stepper Motor를 구동하며, ESP32에서 Step과 Direction 신호를 전달받는다.

### Load Cell

사용 부품:

* Load Cell
* HX711 Amplifier Module

Load Cell은 다음 두 시점에서 사료 무게를 측정한다.

* 사료를 배출하는 동안 목표 급식량 확인
* 식사가 끝난 뒤 밥그릇의 잔량 확인

HX711은 Load Cell의 작은 전압 변화를 증폭하고 digital value로 변환해 ESP32에 전달한다.

정확한 측정을 위해 영점 보정과 calibration factor 설정이 필요하다.

### Food Container

사료 저장통의 용량은 약 4.5L로 설계했다.

다묘·다견 가정에서 여러 개체가 함께 사용하는 상황을 고려해 일반적인 소형 급식기보다 큰 저장 용량을 적용했다.

저장 가능 기간은 사료의 밀도와 반려동물별 급식량에 따라 달라진다.

### Bowl Height

밥그릇 높이는 약 12cm로 설계했다.

초소형견을 제외한 소형견과 일부 중형견이 사용할 수 있는 높이를 기준으로 정했으며, 개체별 체형 차이는 받침대 또는 교체형 구조로 보완할 수 있다.

## Power Architecture

각 부품의 동작 전압이 다르기 때문에 하나의 12V 입력을 여러 voltage rail로 분리했다.

```text
12V 5A Adapter
├── 12V Motor Rail
│   └── Stepper Motor Driver
│
└── 5V Buck Converter
    ├── Servo Motor
    └── 3.3V Regulator
        ├── RFID Module
        ├── HX711
        └── Sensor Rail
```

주요 구성은 다음과 같다.

* 외부 입력: 12V 5A Adapter
* Stepper Motor: 12V
* Servo 및 일부 주변장치: 5V
* RFID와 Sensor Rail: 3.3V

모터와 센서를 동일한 전원 라인에서 직접 구동하면 모터의 전류 변화가 센서 측정값에 영향을 줄 수 있다. 이를 줄이기 위해 motor rail과 sensor rail을 분리하고, regulator와 capacitor를 적용했다.

### Noise Management

Load Cell과 RFID는 전원 노이즈에 민감하다.

이를 고려해 다음 요소를 적용했다.

* Motor와 Sensor 전원 경로 분리
* HX711과 RC522 전원 안정화
* 10µF Bulk Capacitor
* 100nF Bypass Capacitor
* 공통 Ground 구성
* Stepper Motor 전류 경로와 Sensor signal 배선 분리
* 12V 고전류 구간에 20AWG 이상의 전선 사용

## Circuit and PCB

초기 단계에서는 Breadboard와 Jumper Wire를 사용해 각 부품을 개별적으로 검증했다.

검증 대상은 다음과 같다.

* RFID UID 인식
* Servo 각도 제어
* Stepper Motor 정회전 및 역회전
* Load Cell 측정
* HX711 calibration
* Wi-Fi 및 MQTT 연결
* 전원 변환 회로

기능별 검증 후 배선 정리와 회로 안정성을 높이기 위해 PCB를 설계했다.

### Hardware Prototype

![회로 테스트 1](./images/hardware_test_1.png)

![회로 테스트 2](./images/hardware_test_2.png)

### PCB Design

![PCB 설계](./images/pcb_design.png)

## Firmware

펌웨어는 ESP32, Arduino Framework, C++을 기반으로 구현했다.

```text
MCU       ESP32 DevKit V4
Framework Arduino Framework
Language  C++
```

주요 제어 대상은 다음과 같다.

* RC522 RFID Module
* SG90 Servo Motor
* NEMA 17 Stepper Motor
* TMC2209 Stepper Motor Driver
* Load Cell
* HX711
* MQTT Client

## Finite State Machine

급식 동작은 Event-driven Finite State Machine으로 관리한다.

```text
IDLE
  │
  │ 스케줄 시간 도달
  ▼
WAIT_FOR_PET
  │
  │ 등록된 RFID 감지
  ▼
OPEN_DOOR
  │
  ▼
DISPENSING
  │
  │ 목표 무게 도달
  ▼
EATING
  │
  │ RFID 이탈
  ▼
MEASURE_REMAINDER
  │
  ▼
REPORT_RESULT
  │
  ▼
IDLE
```

오류가 발생하면 현재 상태와 관계없이 오류 처리 상태로 전환할 수 있다.

주요 Event는 다음과 같다.

* 급식 스케줄 시간 도달
* 등록된 RFID 태그 인식
* 문 개방 완료
* 목표 배식량 도달
* RFID 태그 이탈
* Load Cell 측정 완료
* 사료 걸림 감지
* Motor 복구 실패
* MQTT 명령 수신

Finite State Machine을 사용함으로써 현재 시스템이 어떤 단계에 있는지 명확하게 구분하고, 동일한 동작이 중복 실행되는 문제를 줄였다.

## Cooperative Multitasking

ILLO는 별도의 RTOS Task 대신 Arduino main loop 안에서 여러 기능을 짧게 반복 실행하는 Cooperative Multitasking 구조를 사용했다.

주요 Task는 다음과 같다.

* `rfidTask`
* `weightTask`
* `motorTask`
* `mqttTask`
* `scheduleTask`
* `stateMachineTask`

각 Task는 긴 시간 동안 CPU를 점유하지 않으며, 현재 상태에서 필요한 작업만 수행한다.

배식 중에도 다음 기능을 함께 처리할 수 있도록 긴 `delay()` 호출을 줄였다.

* MQTT 명령 수신
* RFID 상태 확인
* Load Cell 측정
* 사료 걸림 감지
* 스케줄 변경
* 오류 감지

## Jam Detection

사료 걸림은 Stepper Motor가 동작 중인데도 Load Cell 값이 일정 시간 동안 증가하지 않는 경우로 판단한다.

```text
배식 시작
   │
   ▼
Stepper Motor 정회전
   │
   ▼
Load Cell 변화 확인
   │
   ├── 무게 증가 → 배식 계속
   │
   └── 변화 없음 → Jam 판단
                    │
                    ▼
                 역회전
                    │
                    ▼
                 정회전 재시도
                    │
                    ├── 복구 성공 → 배식 계속
                    └── 반복 실패 → ERROR
```

잘못된 감지를 줄이기 위해 다음 요소를 함께 고려해야 한다.

* 최소 무게 변화량
* 판단 시간
* Load Cell filtering
* 모터 회전 속도
* 최대 복구 횟수

## Networking

애플리케이션과 급식기 사이의 통신에는 MQTT를 사용한다.

```text
Application <-> MQTT Broker <-> ESP32 Feeder
```

애플리케이션과 ESP32는 MQTT Broker에 연결되고, topic을 통해 데이터를 주고받는다.

MQTT를 선택한 이유는 다음과 같다.

* ESP32에서 비교적 가볍게 동작
* Publish/Subscribe 구조 제공
* 급식 완료 및 오류 이벤트 전달에 적합
* 장치와 애플리케이션의 직접 연결이 필요하지 않음
* Topic을 기준으로 command와 status를 분리 가능

## MQTT Topics

```text
illo/{deviceId}/command
illo/{deviceId}/response
illo/{deviceId}/status
```

### command

애플리케이션에서 급식기로 스케줄 또는 제어 명령을 전달한다.

```json
{
  "tempId": "tmp_001",
  "petId": "dog_001",
  "amount": 80,
  "time": "08:00",
  "repeat": ["MON", "TUE", "WED", "THU", "FRI"]
}
```

### response

급식기가 command 처리 결과를 애플리케이션에 전달한다.

```json
{
  "tempId": "tmp_001",
  "scheduleId": "sch_1024",
  "status": "created"
}
```

### status

급식 완료 후 배식량과 잔량을 애플리케이션에 전달한다.

```json
{
  "scheduleId": "sch_1024",
  "petId": "dog_001",
  "dispensedAmount": 80,
  "remainingAmount": 15,
  "eatenAmount": 65,
  "timestamp": "2026-06-27T08:15:00"
}
```

## Schedule Synchronization

애플리케이션은 새로운 스케줄을 생성할 때 임시 ID를 함께 전송한다.

급식기는 스케줄을 저장한 뒤 자체 schedule ID를 생성하고, 임시 ID와 함께 결과를 반환한다.

```text
Application
    │
    │ tempId와 스케줄 전송
    ▼
ESP32 Feeder
    │
    │ 스케줄 저장
    │ scheduleId 생성
    ▼
Application
    │
    │ response 확인
    ▼
Local DB 최종 저장
```

애플리케이션은 급식기의 응답을 받은 뒤에만 Local DB에 스케줄을 최종 저장한다.

이를 통해 애플리케이션에는 스케줄이 있지만 급식기에는 저장되지 않은 상태를 줄인다.

## Feeding Result Flow

급식 결과는 다음 흐름으로 전달된다.

```text
스케줄 시간 도달
→ RFID 개체 확인
→ 사료 배식
→ RFID 이탈 확인
→ 잔량 측정
→ status topic 전송
→ Local DB 저장
→ 그래프 업데이트
```

식사 종료는 RFID 태그가 인식 범위를 벗어난 시점을 기준으로 판단한다.

## Application

애플리케이션은 반려동물 정보, 급식 스케줄과 식사 기록을 관리한다.

주요 기능은 다음과 같다.

* 반려동물 등록
* RFID 태그 등록
* 개체별 급식 스케줄 생성
* 급식량 설정
* 반복 요일 설정
* 스케줄 수정 및 삭제
* 급식 결과 확인
* 잔량 기록 저장
* 실제 섭취량 계산
* 기간별 식사 기록 시각화

애플리케이션은 MQTT를 통해 급식기와 데이터를 교환하고, 급식 결과를 Local DB에 저장한다.

## Data Visualization

급식 후 전달받은 데이터를 바탕으로 개체별 식사 기록을 그래프로 표시한다.

저장되는 주요 데이터는 다음과 같다.

* 급식 예정 시간
* 실제 급식 시간
* 설정된 급식량
* 실제 배식량
* 식사 후 잔량
* 계산된 섭취량
* 반려동물 ID

기간별 데이터를 비교하면 다음과 같은 변화를 확인할 수 있다.

* 특정 시간대의 잔량 증가
* 평소보다 낮은 섭취량
* 반복적인 과식
* 급식 스케줄별 섭취 차이

이 데이터는 질병을 진단하기 위한 정보가 아니라 보호자가 식습관 변화를 확인하기 위한 참고 자료로 사용한다.

## Design and Modeling

ILLO의 외형은 반려동물이 사용하는 장치라는 점을 고려해 안정적이고 위협적이지 않은 형태를 목표로 설계했다.

### Auger Screw

사료 배출에는 Auger Screw 구조를 적용했다.

Stepper Motor와 연결된 나선형 스크류가 회전하면서 사료를 배출구 방향으로 밀어낸다.

주요 특징은 다음과 같다.

* 회전량 기반 제어 가능
* 정회전 및 역회전 가능
* 사료 걸림 복구 로직 적용 가능
* 구조가 비교적 단순함
* 분해와 유지보수가 용이함

사료 크기와 형태에 따라 걸림이 발생할 수 있으므로 스크류 간격, 배출구 크기와 내부 경사를 함께 고려해야 한다.

### Form

상단에서는 직선을 줄이고 곡면을 적용해 부드러운 인상을 주도록 했다.

측면에는 일부 사선 구조를 사용해 외형이 지나치게 둔해 보이지 않도록 구성했다.

제품의 하부는 반려동물이 접근하거나 밀었을 때 쉽게 넘어지지 않도록 넓고 안정적인 형태를 목표로 했다.

### Color

메인 색상은 White, Point Color는 Dark Navy로 설정했다.

* White: 위생적이고 정돈된 인상
* Dark Navy: 하부의 안정감과 시각적 무게감

### Modeling

![제품 모델링](./images/modeling_1.png)

![제품 렌더링](./images/rendering.png)

## 현재 한계

### RFID 인식 거리

RC522의 실제 인식 거리는 약 2~3cm로 확인됐다.

태그가 Reader에 매우 가까워야 하므로, 반려동물이 자연스럽게 접근하는 상황에서 안정적으로 인식하기 어렵다.

향후에는 134.2kHz RFID Reader와 외부 안테나를 적용해 인식 범위를 개선할 필요가 있다.

### 개체 접근 제어

RFID로 등록된 개체를 확인할 수는 있지만, 문이 열린 뒤 다른 반려동물이 함께 접근하는 상황을 완전히 차단하지는 못한다.

급식 공간을 한 개체만 들어갈 수 있는 형태로 설계하거나, 추가 거리 센서와 카메라를 사용해 점유 상태를 확인하는 방식이 필요하다.

### 식사 종료 판단

현재는 RFID 태그가 인식 범위를 벗어나면 식사가 끝난 것으로 판단한다.

반려동물이 식사 중 고개를 움직이거나 잠시 뒤로 물러나는 경우 식사 종료로 잘못 판단할 수 있다.

향후에는 다음 정보를 함께 사용해 판단 정확도를 높일 수 있다.

* RFID 인식 상태
* Load Cell 무게 변화
* 거리 센서
* 카메라 영상
* 일정 시간 동안의 움직임

### 사료 형태에 따른 배식 편차

Auger Screw는 사료의 크기, 형태, 표면과 밀도에 따라 한 회전당 배식량이 달라질 수 있다.

Load Cell feedback으로 최종 무게를 보정할 수 있지만, 사료 종류에 따라 배식 속도와 걸림 빈도가 달라질 수 있다.

### Servo Motor 내구성

Prototype에서 사용한 SG90은 소형 Servo Motor이기 때문에 실제 제품 수준의 반복 동작과 충격을 장기간 견디기 어렵다.

문 구조와 필요한 torque를 다시 측정한 뒤 actuator를 선정할 필요가 있다.

## 개선 방향

* 134.2kHz RFID Reader와 외부 안테나 적용
* 카메라 기반 개체 확인
* 거리 센서를 이용한 급식 공간 점유 판단
* RFID, 무게와 영상 정보를 함께 이용한 식사 종료 판단
* 급식 시간 호출음 또는 LED 추가
* 식사량 변화 알림
* 여러 급식기 간 스케줄 동기화
* Cloud 기반 장기 기록 저장
* 개체별 급식량 추천
* Servo Motor와 문 구조 개선
* 다양한 사료 크기에 대응하는 교체형 Auger Screw 설계

## Team

| Role              | Members       |
| ----------------- | ------------- |
| Hardware          | 김기범, 안현서, 허재원 |
| Firmware          | 김기범, 안현서      |
| Design & Modeling | 하채연, 허재원      |
| Networking        | 박상현           |
| Application       | 고시온, 박상현      |

## Tech Stack

| Area           | Technologies                                      |
| -------------- | ------------------------------------------------- |
| Controller     | ESP32 DevKit V4                                   |
| Firmware       | C++, Arduino Framework                            |
| Architecture   | Finite State Machine, Cooperative Multitasking    |
| Identification | RC522, RFID                                       |
| Motor Control  | NEMA 17, TMC2209, SG90                            |
| Measurement    | Load Cell, HX711                                  |
| Networking     | Wi-Fi, MQTT                                       |
| Application    | Local DB, Schedule Management, Data Visualization |
| Design         | 3D Modeling, Auger Screw                          |
| Power          | 12V Adapter, Buck Converter, Regulator            |
