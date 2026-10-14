#pragma once

#include <stdint.h>

// ─── WiFi ───────────────────────────────────────────────────────────
#define WIFI_SSID               "your_ssid"
#define WIFI_PASS               "your_password"

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

// ─── Storage ─────────────────────────────────────────────────────────
#define DICT_PATH               "/sdcard/keys.txt"
#define DUMP_PATH_PREFIX        "/sdcard/"

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
#define USE_DISPLAY             false
#define SPI_FREQ                1000000

// ─── Debug ──────────────────────────────────────────────────────────
#define DEBUG_MODE              false
#define DBG(x) do { if(DEBUG_MODE){ Serial.print("// [DBG] "); Serial.println(x); } } while(0)
