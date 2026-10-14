#pragma once
#include "core/key_store.h"
// Tries built-in dictionary + SD card keys.txt against all sectors.
// Returns: total keys found.
uint8_t dictAttack_run(CardDump* dump);
