#pragma once
#include <stdint.h>
#include <stdbool.h>

// ─── Crypto1 48-bit LFSR — MIFARE Classic stream cipher ──────────────────
// Poly: x^48+x^43+x^39+x^38+x^36+x^34+x^33+x^31+x^29+x^24+
//       x^23+x^21+x^19+x^13+x^9+x^7+x^6+x^5+1
// Filter taps (state positions): {9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41}
// fa truth table : 0x9E98   — algebraically verified
// fb truth table : 0xD71B824E — derived from (a|b)^(a&c)^(b&d)^(a&e)^e, balanced

typedef struct { uint64_t state; } Crypto1;  // bits 47:0 used

void     crypto1_init(Crypto1* s, const uint8_t* key6);
uint8_t  crypto1_filter(uint64_t state);
uint8_t  crypto1_bit(Crypto1* s, uint8_t inputBit, bool feedbackEnabled);
uint8_t  crypto1_byte(Crypto1* s, uint8_t in, bool feedbackEnabled);
uint32_t crypto1_word(Crypto1* s, uint32_t in, bool feedbackEnabled);
uint8_t  crypto1_rollback_bit(Crypto1* s, uint8_t inputBit);
uint32_t crypto1_prng_successor(uint32_t x, uint32_t n);
bool     crypto1_verify_nonce(uint64_t key48, uint32_t uid,
                               uint32_t nT, uint32_t nR, uint32_t aR);
