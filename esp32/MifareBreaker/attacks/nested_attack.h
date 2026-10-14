#pragma once
#include "core/key_store.h"
// Nested Authentication Attack (Garcia et al.)
// Requires: at least one known valid (sector, key) pair in dump
// Algorithm:
//   1. Auth with known key → Crypto1 session established
//   2. Immediately re-auth to target sector → nonce nT' is encrypted with live keystream
//   3. Collect NESTED_NONCE_COUNT {nt_enc, par} pairs
//   4. Use weak PRNG to predict candidate nT' values
//   5. Recover Crypto1 state → recover unknown key
// Returns: number of additional keys found
uint8_t nestedAttack_run(CardDump* dump);
