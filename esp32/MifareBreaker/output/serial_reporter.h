#pragma once

#include "config.h"
#include "core/key_store.h"

void reporter_init(void);
void reporter_register_cb(void (*cb)(const char* json));
void reporter_error(const char* code, const char* msg);
void reporter_cardDetected(const CardDump* d);
void reporter_attackStart(const char* attack, uint8_t sector, const char* key_type);
void reporter_keyFound(uint8_t sector, const char* key_type, const uint8_t* key6, const char* method);
void reporter_keyFailed(uint8_t sector, const char* key_type);
void reporter_dataRead(uint8_t sector, uint8_t block, const uint8_t* data16);
void reporter_summary(const CardDump* d, uint32_t elapsed_ms);
void reporter_dumpSaved(const char* path);
