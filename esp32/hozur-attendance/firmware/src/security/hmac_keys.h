#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** KeyA = HMAC-SHA256(master, uid || sector)[0:6] */
bool derive_key_a(const uint8_t* master, size_t master_len,
                  const uint8_t* uid, size_t uid_len,
                  uint8_t sector, uint8_t out_key[6]);

/** KeyB = HMAC-SHA256(master, uid || sector || 0x42)[0:6] */
bool derive_key_b(const uint8_t* master, size_t master_len,
                  const uint8_t* uid, size_t uid_len,
                  uint8_t sector, uint8_t out_key[6]);

#ifdef __cplusplus
}
#endif
