#pragma once
#include "core/key_store.h"

void dictAttack_run(CardDump* d);
void nestedAttack_run(CardDump* d);
void darksideAttack_runSector(CardDump* d, uint8_t sector);
