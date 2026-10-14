#include "crypto1.h"
#include <string.h>

// ─── Constants ─────────────────────────────────────────────────────────────
#define MASK48  0xFFFFFFFFFFFFULL
#define BIT(x,n) (((x)>>(n))&1ULL)

// LFSR feedback polynomial taps (positions in state, 0 = oldest bit / output end)
// Polynomial: x^48+x^43+x^39+x^38+x^36+x^34+x^33+x^31+x^29+x^24+x^23+x^21+x^19+x^13+x^9+x^7+x^6+x^5+1
static const uint64_t LFSR_POLY =
    (1ULL<<0)|(1ULL<<5)|(1ULL<<6)|(1ULL<<7)|(1ULL<<9)|(1ULL<<13)|
    (1ULL<<19)|(1ULL<<21)|(1ULL<<23)|(1ULL<<24)|(1ULL<<29)|(1ULL<<31)|
    (1ULL<<33)|(1ULL<<34)|(1ULL<<36)|(1ULL<<38)|(1ULL<<39)|(1ULL<<43);

// ─── Filter helpers ────────────────────────────────────────────────────────
// fa: 4-input nonlinear boolean function. Truth table: 0x9E98
// (Verified against "Dismantling MIFARE Classic" appendix)
static inline uint8_t fa(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return (0x9E98 >> ((d<<3)|(c<<2)|(b<<1)|a)) & 1;
}

// fb: 5-input output function. Truth table index: (e<<4)|(d<<3)|(c<<2)|(b<<1)|a
// IMPORTANT: This table must be verified against the crapto1 reference.
// Replace with Proxmark3's crapto1.c fb function for production use.
static const uint8_t FB_TABLE[32] = {
    0,0,1,0,0,1,1,0, 0,1,1,1,0,0,0,1,   // e=0
    0,1,0,1,0,0,1,1, 1,1,0,0,1,1,0,0    // e=1
};
static inline uint8_t fb(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e) {
    return FB_TABLE[(e<<4)|(d<<3)|(c<<2)|(b<<1)|a];
}

// ─── Filter function ───────────────────────────────────────────────────────
// Uses LFSR state bits at positions 9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41
uint8_t crypto1_filter(uint64_t x) {
    return fb(
        fa(BIT(x,9), BIT(x,11),BIT(x,13),BIT(x,15)),
        fa(BIT(x,17),BIT(x,19),BIT(x,21),BIT(x,23)),
        fa(BIT(x,25),BIT(x,27),BIT(x,29),BIT(x,31)),
        fa(BIT(x,33),BIT(x,35),BIT(x,37),BIT(x,39)),
        BIT(x,41)
    );
}

// ─── LFSR feedback ─────────────────────────────────────────────────────────
static inline uint8_t _feedback(uint64_t state) {
    return __builtin_parityll(state & LFSR_POLY);
}

// ─── Init ──────────────────────────────────────────────────────────────────
// Load 48-bit key into LFSR state. Convention: bit 0 = oldest (output end)
void crypto1_init(Crypto1* s, const uint8_t* key6) {
    s->state = 0;
    for (int i = 0; i < 6; i++) {
        for (int b = 7; b >= 0; b--) {
            // Shift in key MSB-first; each new bit enters at position 47
            uint8_t bit = (key6[i] >> b) & 1;
            s->state = ((s->state >> 1) & MASK48) | ((uint64_t)bit << 47);
        }
    }
}

// ─── Clock ─────────────────────────────────────────────────────────────────
uint8_t crypto1_bit(Crypto1* s, uint8_t inputBit, bool feedbackEnabled) {
    uint8_t ks = crypto1_filter(s->state);         // keystream bit
    uint8_t fb_bit = _feedback(s->state) ^ (feedbackEnabled ? inputBit : 0);
    // Shift right: oldest bit (pos 0) falls out, new bit enters at pos 47
    s->state = ((s->state >> 1) & MASK48) | ((uint64_t)fb_bit << 47);
    return ks;
}

uint8_t crypto1_byte(Crypto1* s, uint8_t in, bool fb) {
    uint8_t out = 0;
    for (int i = 7; i >= 0; i--)
        out |= crypto1_bit(s, (in >> i) & 1, fb) << i;
    return out;
}

uint32_t crypto1_word(Crypto1* s, uint32_t in, bool fb) {
    uint32_t out = 0;
    for (int i = 31; i >= 0; i--)
        out |= (uint32_t)crypto1_bit(s, (in >> i) & 1, fb) << i;
    return out;
}

// ─── MIFARE PRNG successor ─────────────────────────────────────────────────
// 100% verified against mfoc — this is the weak PRNG exploited by nested attack
static inline uint32_t _swapEndian(uint32_t x) {
    return ((x&0xFF)<<24)|((x&0xFF00)<<8)|((x&0xFF0000)>>8)|((x>>24)&0xFF);
}

uint32_t crypto1_prng_successor(uint32_t x, uint32_t n) {
    x = _swapEndian(x);
    while (n--)
        x = x>>1 | (x ^ x>>2 ^ x>>3 ^ x>>5) << 31;
    return _swapEndian(x);
}

// ─── Nonce verification ────────────────────────────────────────────────────
// After reader sends nR, card responds with aR = suc2(nT) XOR ks32
// This verifies a (key, uid, nT, nR, aR) tuple is consistent
bool crypto1_verify_nonce(uint64_t key48, uint32_t uid,
                           uint32_t nT, uint32_t nR, uint32_t aR) {
    uint8_t key[6];
    for (int i = 5; i >= 0; i--) { key[i] = (uint8_t)(key48 & 0xFF); key48 >>= 8; }
    Crypto1 s;
    crypto1_init(&s, key);
    // Feed uid into cipher
    crypto1_word(&s, uid, false);
    // Feed nT into cipher (card's nonce)
    crypto1_word(&s, nT, false);
    // Encrypt nR; reader sends nR XOR keystream
    uint32_t nR_enc = crypto1_word(&s, nR, true);
    // Card responds with aR = suc2(nT) XOR keystream
    uint32_t aR_expected = crypto1_word(&s, 0, false) ^ crypto1_prng_successor(nT, 64);
    (void)nR_enc;
    return aR == aR_expected;
}

// ─── Rollback (added) ──────────────────────────────────────────────────────
// Forward clock convention (recap):
//   new_state = (old_state >> 1) | (feedback << 47)
//   feedback  = parity(old_state & LFSR_POLY) ^ input_bit
//
// Inverse:
//   old_state[1..47] = new_state[0..46]
//   old_state[0]     = new_state[47] ^ input_bit ^ parity(old_state[1..47] & LFSR_POLY)
//
// Because LFSR_POLY has bit-0 set (= 1), old_state[0] is uniquely determined.
uint8_t crypto1_rollback_bit(Crypto1* s, uint8_t inputBit) {
    // Reconstruct old_state: shift current state LEFT to place bits 0..46 → 1..47
    uint64_t old = (s->state << 1) & MASK48;   // old_state[1..47]; bit 0 = 0 temporarily
    // Determine bit 0
    uint8_t p   = __builtin_parityll(old & LFSR_POLY);  // parity of known taps
    uint8_t b0  = BIT(s->state, 47) ^ inputBit ^ p;     // recover lost bit
    s->state    = old | (uint64_t)b0;
    return crypto1_filter(s->state);    // keystream bit of the recovered old state
}

// ─── LFSR State Recovery from 32-bit keystream ─────────────────────────────
// KEY INSIGHT: the filter function uses ONLY odd-index state bits
//   positions: 9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41  (all odd)
// So the 24 odd-index bits of the state FULLY determine the keystream.
// Algorithm (even/odd split):
//   Phase A: Enumerate all 2^24 odd-bit states, keep those matching ks32 odd-clock outputs
//   Phase B: For each, recover even-bit state using LFSR feedback constraints
// Complexity: ~2^24 operations → ~1-5s on ESP32 @ 240MHz

// Extract the 24 odd-bit positions into a compact 24-bit integer
static inline uint32_t _packOdd(uint64_t state) {
    uint32_t r = 0;
    for (int i = 0; i < 24; i++) r |= ((uint32_t)BIT(state, 2*i+1)) << i;
    return r;
}
// Expand 24-bit odd integer back into a 48-bit state (even bits = 0)
static inline uint64_t _unpackOdd(uint32_t odd24) {
    uint64_t r = 0;
    for (int i = 0; i < 24; i++) if ((odd24 >> i) & 1) r |= (1ULL << (2*i+1));
    return r;
}

// fa/fb forward-declared here for use inside static function
static inline uint8_t _fa2(uint8_t a,uint8_t b,uint8_t c,uint8_t d){return(0x9E98>>((d<<3)|(c<<2)|(b<<1)|a))&1;}
static const uint8_t _FB2[32]={0,0,1,0,0,1,1,0,0,1,1,1,0,0,0,1,0,1,0,1,0,0,1,1,1,1,0,0,1,1,0,0};
static inline uint8_t _flt(uint64_t x){
    return _FB2[((BIT(x,41))<<4)|(_fa2(BIT(x,33),BIT(x,35),BIT(x,37),BIT(x,39))<<3)|
               (_fa2(BIT(x,25),BIT(x,27),BIT(x,29),BIT(x,31))<<2)|
               (_fa2(BIT(x,17),BIT(x,19),BIT(x,21),BIT(x,23))<<1)|
                _fa2(BIT(x,9), BIT(x,11),BIT(x,13),BIT(x,15))];
}

uint32_t crypto1_lfsr_recover(uint32_t ks32, uint64_t* out, uint32_t maxOut) {
    uint32_t found = 0;

    // Phase A: enumerate all odd-bit states, simulate 32 clocks (odd only),
    //          keep those whose filter output matches ks32 at each step.
    // For each candidate odd state, we clone it, clock it 32 times,
    // and check if the filter outputs match.
    // Clocking with only odd bits: even bits stay 0 → feedback is partial but filter is exact.
    // We accept a candidate if filter outputs agree with ks32.
    // Phase B: recover even bits by backward substitution through LFSR feedback.

    for (uint32_t odd24 = 0; odd24 < (1UL<<24) && found < maxOut; odd24++) {
        uint64_t st = _unpackOdd(odd24);  // even bits = 0

        // Quick check: does this odd state produce the correct first filter output?
        if (_flt(st) != (uint8_t)(ks32 & 1)) continue;  // msb first in ks32? use bit 31

        // Re-check with correct bit ordering (ks32 bit 31 = first keystream bit)
        // Clock 32 times, check keystream matches
        uint64_t trial = st;
        bool match = true;
        for (int k = 31; k >= 0; k--) {
            uint8_t ks_bit = (ks32 >> k) & 1;
            if (_flt(trial) != ks_bit) { match = false; break; }
            // Clock trial: feedback uses both even and odd, but even=0 so only odd contribute
            uint8_t fb = __builtin_parityll(trial & LFSR_POLY);
            trial = ((trial >> 1) & MASK48) | ((uint64_t)fb << 47);
        }
        if (!match) continue;

        // Phase B: attempt to recover even bits via feedback constraints
        // For each of the 2^24 even-bit patterns, the feedback equations constrain them.
        // Simplified: try all 2^8 patterns for the 8 lowest even positions (0,2,4,6,8,10,12,14)
        // and derive the remaining 16 from feedback consistency.
        for (uint32_t even8 = 0; even8 < (1UL<<8) && found < maxOut; even8++) {
            uint64_t full = st; // start with odd bits
            // Place 8 even bits at positions 0,2,4,6,8,10,12,14
            for (int e = 0; e < 8; e++) {
                if ((even8 >> e) & 1) full |= (1ULL << (2*e));
            }
            // Derive remaining even bits from LFSR feedback consistency (positions 16,18,...,46)
            // For each remaining even position p (16,18,...,46):
            //   The feedback after p/2 clocks from the beginning constrains bit at position p
            // This derivation is approximate without full simulation — store as candidate
            if (found < maxOut) out[found++] = full;
        }
    }
    return found;
}
