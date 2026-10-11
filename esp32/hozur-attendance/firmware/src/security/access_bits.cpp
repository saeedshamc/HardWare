#include "access_bits.h"

// Layout matches MFRC522::MIFARE_SetAccessBits:
//   access[0] = (~c2 << 4) | (~c1)
//   access[1] = ( c1 << 4) | (~c3)
//   access[2] = ( c3 << 4) | ( c2)
// where bit i of each nibble is block i (0..2 data, 3 trailer).

bool access_bits_validate(const uint8_t trailer_access[3]) {
  if (!trailer_access) {
    return false;
  }

  const uint8_t b0 = trailer_access[0];
  const uint8_t b1 = trailer_access[1];
  const uint8_t b2 = trailer_access[2];

  const uint8_t c1_inv = b0 & 0x0F;
  const uint8_t c2_inv = (b0 >> 4) & 0x0F;
  const uint8_t c3_inv = b1 & 0x0F;
  const uint8_t c1     = (b1 >> 4) & 0x0F;
  const uint8_t c2     = b2 & 0x0F;
  const uint8_t c3     = (b2 >> 4) & 0x0F;

  if ((uint8_t)(c1 ^ c1_inv) != 0x0F) return false;
  if ((uint8_t)(c2 ^ c2_inv) != 0x0F) return false;
  if ((uint8_t)(c3 ^ c3_inv) != 0x0F) return false;
  return true;
}

bool access_bits_encode(const AccessBitsConditions* cond, uint8_t out_access[3]) {
  if (!cond || !out_access) {
    return false;
  }

  uint8_t c1 = 0, c2 = 0, c3 = 0;
  for (int i = 0; i < 4; ++i) {
    if (cond->c1[i] > 1 || cond->c2[i] > 1 || cond->c3[i] > 1) {
      return false;
    }
    c1 |= (uint8_t)((cond->c1[i] & 1) << i);
    c2 |= (uint8_t)((cond->c2[i] & 1) << i);
    c3 |= (uint8_t)((cond->c3[i] & 1) << i);
  }

  out_access[0] = (uint8_t)(((~c2 & 0x0F) << 4) | (~c1 & 0x0F));
  out_access[1] = (uint8_t)((( c1 & 0x0F) << 4) | (~c3 & 0x0F));
  out_access[2] = (uint8_t)((( c3 & 0x0F) << 4) | ( c2 & 0x0F));

  return access_bits_validate(out_access);
}

bool access_bits_decode(const uint8_t trailer_access[3], AccessBitsConditions* out) {
  if (!out || !access_bits_validate(trailer_access)) {
    return false;
  }

  const uint8_t c1 = (trailer_access[1] >> 4) & 0x0F;
  const uint8_t c2 = trailer_access[2] & 0x0F;
  const uint8_t c3 = (trailer_access[2] >> 4) & 0x0F;

  for (int i = 0; i < 4; ++i) {
    out->c1[i] = (uint8_t)((c1 >> i) & 1);
    out->c2[i] = (uint8_t)((c2 >> i) & 1);
    out->c3[i] = (uint8_t)((c3 >> i) & 1);
  }
  return true;
}

bool access_bits_hozour_default(uint8_t out_access[3]) {
  // Data 0..2: C1 C2 C3 = 0 1 0 → KeyA read, KeyB write
  // Trailer 3:  C1 C2 C3 = 0 1 1 → KeyB can change keys/access
  AccessBitsConditions cond = {};
  for (int i = 0; i < 3; ++i) {
    cond.c1[i] = 0;
    cond.c2[i] = 1;
    cond.c3[i] = 0;
  }
  cond.c1[3] = 0;
  cond.c2[3] = 1;
  cond.c3[3] = 1;
  return access_bits_encode(&cond, out_access);
}
