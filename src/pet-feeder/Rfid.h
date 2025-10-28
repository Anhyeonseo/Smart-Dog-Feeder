#ifndef RFID_H
#define RFID_H
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include "config.h"

/**
 * RFID + 서보 문 제어 클래스
 * - 태그가 인식되면 문을 '천천히' 연다.
 * - 마지막 인식 이후 10초 동안 연속 미인식이면 '천천히' 닫는다.
 * - 수동 토글(버튼)용 수동 오버라이드 지원: 언제든 열기/닫기 애니메이션.
 * - 내부에서 스캔 주기를 스로틀링 하여 과도 폴링 방지.
 */
class RFID {
public:
    RFID(uint8_t ssPin, uint8_t rstPin, uint8_t servoPin, const String& AUTH_ID);
    void begin();

    // 메인 루프에서 호출. 문 모션 업데이트 + (오버라이드 아닐 때만) RFID 스캔
    // 반환값: 이번 호출에서 '완전히 닫힘'이 완료되면 true (그 외 false)
    bool scan();

    // 허용 태그인지 여부
    bool IsInList(const String& id);
    // UID를 사람이 읽기 쉬운 문자열로
    String convertUidToString();

    // 수동 오버라이드 토글/설정
    void setManualOverride(bool active);
    bool isManualOverrideActive() const { return manualOverrideActive_; }
    void beginMonitoring();
    void tickMotionOnly();
    void resetRFIDSession();

private:
    // --- 하드웨어
    MFRC522 reader_;
    Servo   doorServo_;
    uint8_t ssPin_, rstPin_, servoPin_;
    const String AUTH_TAG;

    // --- 문(서보) 상태 머신
    enum Doorstate { CLOSED, OPENING, OPEN, CLOSING };
    Doorstate doorState_ = CLOSED;
    int doorAngle_ = 0; // 0=닫힘, 90=완전 개방
    bool justClosed_ = false;
    bool closeLogArmed_ = true;
    
    // --- 타이밍
    unsigned long lastStepTime_ = 0;
    static constexpr unsigned long stepIntervalMs_ = 20; // 서보 각도 1스텝 간격
    static constexpr unsigned long stepIntervalMsDown_ = 40;
    static constexpr int angleStep_ = 3;                 // 한 번에 움직일 각도
    unsigned long closeDeadlineMs_ = 0;                  // 이 시각이 지나면 닫기 시작
    static constexpr unsigned long holdAfterLastDetectMs_ = 10000; // 10초 유지
    unsigned long lastScanTryMs_ = 0;
    static constexpr unsigned long scanIntervalMs_ = 60; // RFID 스캔 텀(스로틀)

    // --- 모드 
    bool manualOverrideActive_ = false;

    // --- 내부 유틸
    void updateMotion_(unsigned long now);
    void beginClosing_(unsigned long now);
    void beginOpening_(unsigned long now);
};

#endif
