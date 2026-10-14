#pragma once
#include "core/key_store.h"
// Call once from setup() AFTER reporter_init()
// Connects to WiFi, mounts SPIFFS, starts AsyncWebServer + WebSocket
void webServer_init(void);
// Update the shared dump pointer so /api/status reflects current state
void webServer_setDump(const CardDump* dump);
// ReporterObserver-compatible callback — register with reporter_addObserver()
void webServer_broadcastEvent(const char* jsonLine);
