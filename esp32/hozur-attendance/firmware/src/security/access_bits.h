#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MIFARE Classic sector trailer Access Bits (bytes 6..8).
 * Byte 9 (GPB) is ignored by validation.
 *
 * Conditions per block (0..2 = data, 3 = trailer): C1, C2, C3.
 * Trailer stores each bit and its complement; a mismatch bricks/locks oddly.
 */

typedef struct {
  uint8_t c1[4];
  uint8_t c2[4];
  uint8_t c3[4];
} AccessBitsConditions;

/** Returns true if bytes[0..2] are internally consistent (bit vs ~bit). */
bool access_bits_validate(const uint8_t trailer_access[3]);

/**
 * Pack conditions into trailer access bytes 6..8.
 * Each c1/c2/c3[i] must be 0 or 1.
 */
bool access_bits_encode(const AccessBitsConditions* cond, uint8_t out_access[3]);

/** Decode validated access bytes into conditions. Returns false if invalid. */
bool access_bits_decode(const uint8_t trailer_access[3], AccessBitsConditions* out);

/**
 * Recommended Hozur trailer policy for sector data blocks + trailer:
 * - Data blocks 0..2: KeyA read, KeyB write  (C1 C2 C3 = 0 1 0)
 * - Trailer: KeyA auth for transport read of access bits; KeyB can write keys
 *   (typical: C1 C2 C3 = 0 1 1) — never use transport default forever after enroll.
 *
 * Writes into out_access[3]; always validate before PICC_Write.
 */
bool access_bits_hozour_default(uint8_t out_access[3]);

#ifdef __cplusplus
}
#endif
