#ifndef CONFIG_H
#define CONFIG_H

struct FeedTask {
    uint8_t hour;        // 시간
    uint8_t minute;      // 분
    float target_g;      // 목표 무게 (그램 단위)
};

namespace Config { //공통 사용 설정
    namespace WIFI {
        static const char* SSID     = "your-ssid";       // WiFi SSID
        static const char* PASSWORD = "your-password";   // WiFi 비밀번호
    }
   
    namespace RFID {
        static constexpr uint8_t RFID_SCK_PIN   = 18;
        static constexpr uint8_t RFID_MISO_PIN  = 19;
        static constexpr uint8_t RFID_MOSI_PIN  = 23;
    }
}

namespace FirstUnit {
            
    static constexpr FeedTask TASKS[] = {
        {8, 0, 120},   // 오전 8시, 120g
        {18, 30, 80}   // 오후 6시 30분, 80g
    };
 

    static constexpr uint8_t WEIGHT_DOUT_PIN   = 26;
    static constexpr uint8_t WEIGHT_SCK_PIN    = 27;
    static constexpr float   WEIGHT_CAL_FACTOR = 3352.05;

    static constexpr uint8_t RFID_SS_PIN    = 5;
    static constexpr uint8_t RFID_RST_PIN   = 22;
    static constexpr uint8_t RFID_SERVO_PIN = 13;
 

    static const char* AUTH_TAG = { "6c 1e b2 01" }; // 승인된 tag는 1개
    // static constexpr uint8_t AUTH_TAG_COUNT = sizeof(AUTH_TAGS) / sizeof(AUTH_TAGS[0]);
  

    constexpr int FEEDER_MOTOR_PIN = 25;
    constexpr unsigned long FEEDER_MAX_RUN_MS = 1000000; // 최대 실행 시간 (1000초)

}

#endif