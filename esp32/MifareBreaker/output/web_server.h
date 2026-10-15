#pragma once
#include <Arduino.h>
#include "core/key_store.h"

void webServer_init(void);
void webServer_setDump(const CardDump* d);
String webServer_ipOrStatus(void);
