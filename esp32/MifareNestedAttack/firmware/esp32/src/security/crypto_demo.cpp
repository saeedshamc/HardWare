#include "crypto_demo.h"

#include <Arduino.h>

// Simple LFSR-style demo to illustrate a stream-cipher *concept*.
// This is NOT Crypto1 and cannot recover keys.

static uint32_t demo_lfsr_step(uint32_t state) {
  const uint32_t bit = ((state >> 0) ^ (state >> 2) ^ (state >> 3) ^ (state >> 5)) & 1u;
  return (state >> 1) | (bit << 31);
}

void crypto1_run_demo() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("   CRYPTO1 CONCEPTUAL DEMONSTRATION"));
  Serial.println(F("========================================"));
  Serial.println(F("Educational model only — not the real cipher."));
  Serial.println();

  Serial.println(F("Three-pass authentication (concept):"));
  Serial.println(F("  1) Reader -> Card : auth request + block"));
  Serial.println(F("  2) Card   -> Reader: nonce nT"));
  Serial.println(F("  3) Reader -> Card : response + reader nonce nR"));
  Serial.println(F("  4) Card   -> Reader: encrypted answer"));
  Serial.println();

  uint32_t state = 0xA5C3F129u;
  Serial.println(F("Toy keystream (LFSR demo):"));
  Serial.print(F("  "));
  for (int i = 0; i < 8; ++i) {
    state = demo_lfsr_step(state);
    Serial.printf("%02X ", (unsigned)(state & 0xFF));
  }
  Serial.println();
  Serial.println();

  const char plain[] = "LAB";
  Serial.println(F("XOR illustration (plaintext XOR keystream):"));
  for (size_t i = 0; i < sizeof(plain) - 1; ++i) {
    state = demo_lfsr_step(state);
    const uint8_t ks = (uint8_t)(state & 0xFF);
    const uint8_t ct = (uint8_t)plain[i] ^ ks;
    Serial.printf("  '%c' ^ %02X = %02X\n", plain[i], ks, ct);
  }

  Serial.println();
  Serial.println(F("Lesson:"));
  Serial.println(F("  Stream ciphers mix secret state with nonces."));
  Serial.println(F("  If PRNG/state leaks across sessions, secrecy collapses."));
  Serial.println(F("  Crypto1 on MIFARE Classic is considered obsolete for"));
  Serial.println(F("  security-sensitive access control."));
}
