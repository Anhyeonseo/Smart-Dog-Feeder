#ifndef CONFIG_H
#define CONFIG_H

namespace Config { //공통 사용 설정
    namespace WIFI {
        static const char* SSID     = "Ddong";       // WiFi SSID
        static const char* PASSWORD = "hihello1";   // WiFi 비밀번호
    }

    // --- MQTT 설정 추가 ---
    namespace MQTT {
        static const char* SERVER   = "58870451efe34c9f83dece69cbf72f38.s1.eu.hivemq.cloud";
        static constexpr int PORT   = 8883;
        static const char* USER     = "feeder01";
        static const char* PASSWORD = "Zxasqwgg248~"; // 비밀번호 확인

        static const char* TOPIC_COMMAND  = "feeder/ESP_FEEDER_01/command";
        static const char* TOPIC_STATUS   = "feeder/ESP_FEEDER_01/status";
        static const char* TOPIC_RESPONSE = "feeder/ESP_FEEDER_01/response";
    }
   
    namespace RFID {
        static constexpr uint8_t RFID_SCK_PIN   = 18;
        static constexpr uint8_t RFID_MISO_PIN  = 19;
        static constexpr uint8_t RFID_MOSI_PIN  = 23;
    }

    namespace FEEDER {
        static constexpr float R_SENSE = 0.11f; // TMC2209 PCB 내부 저항
    }
}

namespace FirstUnit { 

    static constexpr uint8_t WEIGHT_DOUT_PIN   = 4;
    static constexpr uint8_t WEIGHT_SCK_PIN    = 15;
    static constexpr float   WEIGHT_CAL_FACTOR = -3384;

    static constexpr uint8_t RFID_SS_PIN    = 5;
    static constexpr uint8_t RFID_RST_PIN   = 17;
    static constexpr uint8_t RFID_SERVO_PIN = 16;
 

    static const char* AUTH_TAG = { "6c 1e b2 01" }; // 승인된 tag는 1개
    static constexpr uint8_t BUTTON_PIN = 21;
    // static constexpr uint8_t AUTH_TAG_COUNT = sizeof(AUTH_TAGS) / sizeof(AUTH_TAGS[0]);
  

    constexpr int FEEDER_MOTOR_STEP = 12;
    constexpr int FEEDER_MOTOR_DIR = 13; // 스텝 모터 제어 핀
    constexpr int FEEDER_MOTOR_EN = 14; // 모터 활성화 핀 (필요시 변경)
    constexpr unsigned long FEEDER_MAX_RUN_MS = 1000000; // 최대 실행 시간 (1000초)

}

#endif
