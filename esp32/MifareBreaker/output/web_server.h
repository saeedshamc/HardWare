#pragma once
#include "core/key_store.h"
#include <Arduino.h>

void    webServer_init(void);
void    webServer_setDump(const CardDump* dump);
String  webServer_ipOrStatus(void);
bool    webServer_isAttackRunning(void);   // برای جلوگیری از race condition در loop()
