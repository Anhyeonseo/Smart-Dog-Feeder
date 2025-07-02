#pragma once

namespace Config {
    namespace RFID {
        static constexpr uint8_t RFID_SS_PIN    = 5;
        static constexpr uint8_t RFID_RST_PIN   = 22;
        static constexpr uint8_t RFID_SCK_PIN   = 18;
        static constexpr uint8_t RFID_MISO_PIN  = 19;
        static constexpr uint8_t RFID_MOSI_PIN  = 23;
        static constexpr uint8_t RFID_SERVO_PIN = 13;
        static const char* AUTH_TAGS[] = { "6c 1e b2 01" }; // 승인된 tag는 1개
        static constexpr uint8_t AUTH_TAG_COUNT = sizeof(AUTH_TAGS) / sizeof(AUTH_TAGS[0]);
    }

    namespace WEIGHT {
        static constexpr uint8_t WEIGHT_DOUT_PIN   = 26;
        static constexpr uint8_t WEIGHT_SCK_PIN    = 27;
        static constexpr float   WEIGHT_CAL_FACTOR = 3352.05f;
    }

    namespace MOTOR {
        constexpr int FEED_PIN = 25;
    }
}
