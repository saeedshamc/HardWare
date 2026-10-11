#pragma once

// ---------------------------------------------------------------------------
// Hozur — pin map & runtime constants (single source of truth)
// ---------------------------------------------------------------------------

// RC522 on VSPI
static const int PIN_RC522_SS   = 5;
static const int PIN_RC522_SCK  = 18;
static const int PIN_RC522_MOSI = 23;
static const int PIN_RC522_MISO = 19;
static const int PIN_RC522_RST  = 22;

// TFT on HSPI (also mirrored in platformio.ini for TFT_eSPI)
static const int PIN_TFT_SCK  = 14;
static const int PIN_TFT_MOSI = 13;
static const int PIN_TFT_MISO = 12;
static const int PIN_TFT_CS   = 15;
static const int PIN_TFT_DC   = 2;
static const int PIN_TFT_RST  = 4;

// Peripherals
static const int PIN_BUZZER    = 27;
static const int PIN_STATUS_LED = 25;
static const int PIN_MODE_BTN  = 32;

// Optional DS3231 (SCL moved off GPIO22 which is RC522 RST)
static const int PIN_RTC_SDA = 21;
static const int PIN_RTC_SCL = 26;
static const bool ENABLE_DS3231 = false;

// Card memory layout
static const uint8_t CARD_SECTOR       = 1;
static const uint8_t CARD_DATA_BLOCK   = 4;  // sector 1, block 0
static const uint8_t CARD_TRAILER_BLOCK = 7; // sector 1 trailer

// Scan / UI
static const uint32_t CARD_COOLDOWN_MS     = 5000;
static const uint32_t UI_RESULT_HOLD_MS    = 4000;
static const uint32_t HTTP_TIMEOUT_MS      = 8000;
static const uint32_t WATCHDOG_TIMEOUT_S   = 30;
static const uint32_t OFFLINE_SYNC_EVERY_MS = 30000;

// Timezone Asia/Tehran = UTC+3:30, no DST
static const long GMT_OFFSET_SEC  = 3 * 3600 + 30 * 60;
static const int  DAYLIGHT_OFFSET_SEC = 0;
static const char* NTP_SERVER_1 = "pool.ntp.org";
static const char* NTP_SERVER_2 = "time.google.com";

// Enrollment safety: 1 = compute & log only, never write trailer/data
#ifndef ENROLL_DRY_RUN
#define ENROLL_DRY_RUN 0
#endif

// Display
static const uint16_t TFT_W = 240;
static const uint16_t TFT_H = 320;
