#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "core/key_store.h"

/** Optional SSD1306 progress UI. No-ops when USE_DISPLAY is false. */
bool display_init(void);
void display_showBoot(const char* ipOrStatus);
void display_showCard(const CardDump* d);
void display_showProgress(const char* attack, uint8_t sector, uint8_t cracked, uint8_t total);
void display_showSummary(const CardDump* d, uint32_t elapsedMs);
