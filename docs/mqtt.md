# MQTT 프로토콜

애플리케이션과 급식기는 MQTT Broker를 통해 통신합니다. 토픽은 장치 ID별로 분리합니다.

```text
illo/{deviceId}/command
illo/{deviceId}/response
illo/{deviceId}/status
```

## `command`

애플리케이션이 급식기에 스케줄 또는 제어 명령을 전달합니다.

```json
{
  "tempId": "tmp_001",
  "petId": "dog_001",
  "amount": 80,
  "time": "08:00",
  "repeat": ["MON", "TUE", "WED", "THU", "FRI"]
}
```

## `response`

급식기는 명령 처리 결과와 생성된 스케줄 ID를 돌려줍니다.

```json
{
  "tempId": "tmp_001",
  "scheduleId": "sch_1024",
  "status": "created"
}
```

애플리케이션은 이 응답을 확인한 뒤에만 로컬 DB의 스케줄을 최종 상태로 반영합니다. `tempId`는 전송 전 임시 식별자와 응답을 연결하는 데 사용합니다.

## `status`

급식이 끝나면 배식·잔량·섭취 결과를 발행합니다.

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

