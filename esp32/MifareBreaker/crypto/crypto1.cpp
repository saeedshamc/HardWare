#include "crypto/crypto1.h"
#include <stdint.h>

// A development-only 48-bit LFSR keystream generator.
// NOTE: This is intentionally lightweight and not a real Crypto1 implementation.
static uint64_t s_state = 0;

void crypto1_init(const uint8_t key[6]) {
    s_state = 0;
    for (int i = 0; i < 6; i++) {
        s_state |= ((uint64_t)key[i]) << (8*i);
    }
}

void crypto1_reset(void) { s_state = 0; }

static inline uint8_t _clock_bit(void) {
    uint8_t bit = ((s_state >> 0) ^ (s_state >> 5) ^ (s_state >> 9) ^ (s_state >> 14)) & 1;
    s_state = (s_state >> 1) | ((uint64_t)bit << 47);
    return (uint8_t)((s_state >> 0) & 1);
}

void crypto1_keystream(uint8_t* out, int len) {
    for (int i = 0; i < len; i++) {
        uint8_t v = 0;
        for (int b = 0; b < 8; b++) {
            uint8_t bit = _clock_bit();
            v |= (bit << b);
        }
        out[i] = v;
    }
}
