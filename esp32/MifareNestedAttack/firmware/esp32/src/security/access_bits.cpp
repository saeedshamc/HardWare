#include "access_bits.h"

#include <Arduino.h>

// Layout matches MFRC522::MIFARE_SetAccessBits / NXP MF1S50.

bool access_bits_validate(const uint8_t access[3]) {
  if (!access) return false;
  const uint8_t c1_inv = access[0] & 0x0F;
  const uint8_t c2_inv = (access[0] >> 4) & 0x0F;
  const uint8_t c3_inv = access[1] & 0x0F;
  const uint8_t c1 = (access[1] >> 4) & 0x0F;
  const uint8_t c2 = access[2] & 0x0F;
  const uint8_t c3 = (access[2] >> 4) & 0x0F;
  return ((c1 ^ c1_inv) == 0x0F) && ((c2 ^ c2_inv) == 0x0F) && ((c3 ^ c3_inv) == 0x0F);
}

static void bits_for_block(const uint8_t access[3], int blockIndex, int* c1, int* c2, int* c3) {
  const uint8_t c1n = (access[1] >> 4) & 0x0F;
  const uint8_t c2n = access[2] & 0x0F;
  const uint8_t c3n = (access[2] >> 4) & 0x0F;
  *c1 = (c1n >> blockIndex) & 1;
  *c2 = (c2n >> blockIndex) & 1;
  *c3 = (c3n >> blockIndex) & 1;
}

static const char* data_access_label(int c1, int c2, int c3) {
  const int code = (c1 << 2) | (c2 << 1) | c3;
  switch (code) {
    case 0: return "transport: KeyA/B read/write";
    case 2: return "KeyA read; KeyB write";
    case 4: return "KeyA/B read; KeyB write";
    case 6: return "KeyA/B read; never write";
    case 1: return "KeyA/B read; KeyA write";
    case 3: return "KeyB read/write";
    case 5: return "KeyB read; never write";
    case 7: return "never read/write";
    default: return "unknown";
  }
}

static const char* trailer_access_label(int c1, int c2, int c3) {
  const int code = (c1 << 2) | (c2 << 1) | c3;
  switch (code) {
    case 0: return "KeyA writable; access bits readable with KeyA/B (transport-like)";
    case 2: return "KeyB can write keys/access; KeyA auth for access-bits read";
    case 4: return "access bits readable; keys not readable";
    case 6: return "KeyB write access bits; keys never readable";
    case 1: return "KeyA write; access bits readable";
    case 3: return "KeyB write; never read keys";
    case 5: return "never write; access bits readable";
    case 7: return "never write; access bits readable with KeyA/B";
    default: return "unknown";
  }
}

void access_bits_print_decode(const uint8_t access[3]) {
  Serial.println(F("Sector Trailer"));
  Serial.println();
  Serial.println(F("Access Bits:"));
  Serial.printf("%02X %02X %02X\n", access[0], access[1], access[2]);

  if (!access_bits_validate(access)) {
    Serial.println(F("\n[!] Access bits FAIL validation (bit vs complement mismatch)"));
    Serial.println(F("    Writing a wrong trailer can permanently lock a sector."));
    return;
  }
  Serial.println(F("\n[+] Access bits internally consistent"));

  for (int i = 0; i < 3; ++i) {
    int c1, c2, c3;
    bits_for_block(access, i, &c1, &c2, &c3);
    Serial.printf("\nBlock %d:\n", i);
    Serial.printf("  C1 C2 C3 : %d %d %d\n", c1, c2, c3);
    Serial.printf("  Policy   : %s\n", data_access_label(c1, c2, c3));
  }

  int c1, c2, c3;
  bits_for_block(access, 3, &c1, &c2, &c3);
  Serial.println(F("\nTrailer:"));
  Serial.printf("  C1 C2 C3 : %d %d %d\n", c1, c2, c3);
  Serial.printf("  Policy   : %s\n", trailer_access_label(c1, c2, c3));
  Serial.println(F("\nLesson: authentication alone is not the entire security model."));
}
