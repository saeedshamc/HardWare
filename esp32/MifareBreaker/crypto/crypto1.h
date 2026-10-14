#pragma once
#include <stdint.h>
#include <stdbool.h>

// ─── Crypto1 48-bit LFSR — MIFARE Classic stream cipher ───────────────────
// Based on: "Dismantling MIFARE Classic" (Nohl et al., 2008)
// NOTE: For production use, integrate crapto1.c from Proxmark3 firmware
//       (MIT licensed): github.com/RfidResearchGroup/proxmark3

typedef struct {
    uint64_t state;  // 48-bit LFSR state (only bits 47:0 valid)
} Crypto1;

// Initialize LFSR with 48-bit key (6 bytes, loaded MSB-to-LSB)
void     crypto1_init(Crypto1* s, const uint8_t* key6);

// Filter function: nonlinear, applied to 9 specific state bits
uint8_t  crypto1_filter(uint64_t state);

// Clock one bit; inputBit is XORed into feedback; returns keystream bit
uint8_t  crypto1_bit(Crypto1* s, uint8_t inputBit, bool feedbackEnabled);

// Generate 8-bit keystream; in = plaintext byte (if feedbackEnabled)
uint8_t  crypto1_byte(Crypto1* s, uint8_t in, bool feedbackEnabled);

// Generate 32-bit keystream word (MSB first)
uint32_t crypto1_word(Crypto1* s, uint32_t in, bool feedbackEnabled);

// MIFARE PRNG successor — 100% verified against mfoc implementation
// Advances the weak 32-bit PRNG by n steps
uint32_t crypto1_prng_successor(uint32_t x, uint32_t n);

// Nonce validation: verifies nT+nR+aR triplet against known UID
bool crypto1_verify_nonce(uint64_t key48, uint32_t uid,
                           uint32_t nT, uint32_t nR, uint32_t aR);

// ─── LFSR Rollback ─────────────────────────────────────────────────────────
// Run one step BACKWARD. Returns keystream bit of old state.
// inputBit: the input bit that was used in that forward clock step.
uint8_t  crypto1_rollback_bit(Crypto1* s, uint8_t inputBit);

// Recover all 48-bit Crypto1 states that produce ks32 in next 32 clocks
// Writes into 'out' array (caller must provide >= 256 slots)
// Returns: number of candidates found
// NOTE: filter uses ONLY odd-index bits → enumerate 2^24 odd halves, verify 32-bit ks
uint32_t crypto1_lfsr_recover(uint32_t ks32, uint64_t* out, uint32_t maxOut);
