#include "hmac_keys.h"

#include <string.h>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <mbedtls/md.h>
#else
// Host/unit-test stub — not used by access_bits tests.
#include <stdio.h>
#endif

static bool hmac_sha256_prefix6(const uint8_t* master, size_t master_len,
                                const uint8_t* msg, size_t msg_len,
                                uint8_t out_key[6]) {
  if (!master || !msg || !out_key || master_len == 0 || msg_len == 0) {
    return false;
  }

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  uint8_t digest[32];
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info) {
    return false;
  }
  int rc = mbedtls_md_hmac(info, master, master_len, msg, msg_len, digest);
  if (rc != 0) {
    return false;
  }
  memcpy(out_key, digest, 6);
  return true;
#else
  (void)master_len;
  (void)msg_len;
  // Deterministic stub for host builds that accidentally link this file.
  for (int i = 0; i < 6; ++i) {
    out_key[i] = (uint8_t)(0xA0 + i);
  }
  return true;
#endif
}

bool derive_key_a(const uint8_t* master, size_t master_len,
                  const uint8_t* uid, size_t uid_len,
                  uint8_t sector, uint8_t out_key[6]) {
  if (!uid || uid_len == 0 || uid_len > 10) {
    return false;
  }
  uint8_t msg[12];
  memcpy(msg, uid, uid_len);
  msg[uid_len] = sector;
  return hmac_sha256_prefix6(master, master_len, msg, uid_len + 1, out_key);
}

bool derive_key_b(const uint8_t* master, size_t master_len,
                  const uint8_t* uid, size_t uid_len,
                  uint8_t sector, uint8_t out_key[6]) {
  if (!uid || uid_len == 0 || uid_len > 10) {
    return false;
  }
  uint8_t msg[13];
  memcpy(msg, uid, uid_len);
  msg[uid_len] = sector;
  msg[uid_len + 1] = 0x42;
  return hmac_sha256_prefix6(master, master_len, msg, uid_len + 2, out_key);
}
