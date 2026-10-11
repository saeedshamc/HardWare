#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t user_id;
  uint32_t counter;
} CardPayload;

/** Pack user_id + counter + CRC16 into a 16-byte MIFARE block. */
void card_block_pack(uint32_t user_id, uint32_t counter, uint8_t block[16]);

/** Verify CRC and extract fields. */
bool card_block_unpack(const uint8_t block[16], CardPayload* out);

#ifdef __cplusplus
}
#endif
