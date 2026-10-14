#pragma once
#include "core/key_store.h"
// Darkside / Courtois Attack (Courtois 2009)
// Works with ZERO prior knowledge — no key needed.
// Algorithm:
//   1. Send AUTH command to target sector
//   2. Card responds with plaintext nonce nT
//   3. Reader sends partial/malformed nR → card sends encrypted NAK (4 bits keystream)
//   4. After ~256 attempts: reconstruct first keystream byte
//   5. Search LFSR state space → recover key
// Slower than nested: expect 5-30 min per sector depending on card.
// Returns: true if key found for this sector
bool darksideAttack_runSector(CardDump* dump, uint8_t sector);
