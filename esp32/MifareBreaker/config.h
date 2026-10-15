#pragma once

#include <stdint.h>

// ─── WiFi (edit before flash; do not commit real passwords) ─────────
#ifndef WIFI_SSID
#define WIFI_SSID               "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS               "YOUR_WIFI_PASSWORD"
#endif

// ─── Serial ─────────────────────────────────────────────────────────
#define SERIAL_BAUD             115200

// ─── RC522 SPI Pins (ESP32 VSPI) ─────────────────────────────────────
#define PIN_SS                  5
#define PIN_RST                 22
#define PIN_MOSI                23
#define PIN_MISO                19
#define PIN_SCK                 18

// SD card CS pin (use a separate CS for SD to avoid conflict with RC522)
#define SD_CS_PIN               4

// ─── Storage (Arduino SD paths are root-relative, e.g. "/keys.txt") ──
#define DICT_PATH               "/keys.txt"
#define DUMP_PATH_PREFIX        "/"

// ─── Attack Tuning ───────────────────────────────────────────────────
#define ATTACK_TIMEOUT_MS       30000
#define NESTED_NONCE_COUNT      64
#define DICT_MAX_KEYS           600

// ─── Card Constants ──────────────────────────────────────────────────
#define KEY_SIZE                6
#define MAX_SECTORS             40
#define BLOCKS_PER_SECTOR       4
#define BLOCK_SIZE              16

// ─── Hardware ───────────────────────────────────────────────────────
// Set true and wire SSD1306 on I2C (default SDA=21, SCL=25 — avoid RC522 RST on 22)
#define USE_DISPLAY             false
#ifndef DISPLAY_SDA
#define DISPLAY_SDA             21
#endif
#ifndef DISPLAY_SCL
#define DISPLAY_SCL             25
#endif
#ifndef DISPLAY_ADDR
#define DISPLAY_ADDR            0x3C
#endif
#define SPI_FREQ                1000000

// NVS namespace for runtime WiFi overrides (POST /api/wifi)
#define WIFI_NVS_NAMESPACE      "mfb_wifi"

// ─── Debug ──────────────────────────────────────────────────────────
#define DEBUG_MODE              false
#define DBG(x) do { if(DEBUG_MODE){ Serial.print("// [DBG] "); Serial.println(x); } } while(0)
