#pragma once

#include <stdint.h>

// RC522 on VSPI (3.3V only)
#ifndef RFID_SS_PIN
#define RFID_SS_PIN   5
#endif
#ifndef RFID_RST_PIN
#define RFID_RST_PIN  22
#endif
#ifndef RFID_SCK_PIN
#define RFID_SCK_PIN  18
#endif
#ifndef RFID_MISO_PIN
#define RFID_MISO_PIN 19
#endif
#ifndef RFID_MOSI_PIN
#define RFID_MOSI_PIN 23
#endif

// Laboratory mode: when 1, sector analyzer may show keys used for auth demos.
// Keep 0 for classroom demos unless instructor enables it.
#ifndef LAB_SHOW_KEYS
#define LAB_SHOW_KEYS 0
#endif

// Default factory transport key for educational demos on test cards only.
static const uint8_t LAB_DEFAULT_KEY[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const uint8_t MIFARE_1K_SECTORS = 16;
static const uint8_t MIFARE_1K_BLOCKS  = 64;
static const uint8_t MIFARE_BLOCK_SIZE = 16;
