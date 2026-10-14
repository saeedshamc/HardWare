#pragma once
#include <Arduino.h>
#include "core/key_store.h"

typedef void (*ReporterObserver)(const char* jsonLine);

void reporter_init(void);
void reporter_addObserver(ReporterObserver cb);
void reporter_cardDetected(const CardDump* d);
void reporter_attackStart(const char* attack, uint8_t sector, bool isKeyA);
void reporter_keyFound(uint8_t sector, bool isKeyA, const uint8_t* key6, const char* method);
void reporter_keyFailed(uint8_t sector, bool isKeyA);
void reporter_dataRead(uint8_t sector, uint8_t blk, const uint8_t* data16);
void reporter_summary(const CardDump* d, uint32_t elapsedMs);
void reporter_dumpSaved(const char* path);
void reporter_error(const char* code, const char* msg);
