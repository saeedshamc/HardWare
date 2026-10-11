#include "card_block.h"
#include "crc16.h"

#include <string.h>

void card_block_pack(uint32_t user_id, uint32_t counter, uint8_t block[16]) {
  memset(block, 0, 16);
  block[0] = (uint8_t)(user_id & 0xFF);
  block[1] = (uint8_t)((user_id >> 8) & 0xFF);
  block[2] = (uint8_t)((user_id >> 16) & 0xFF);
  block[3] = (uint8_t)((user_id >> 24) & 0xFF);
  block[4] = (uint8_t)(counter & 0xFF);
  block[5] = (uint8_t)((counter >> 8) & 0xFF);
  block[6] = (uint8_t)((counter >> 16) & 0xFF);
  block[7] = (uint8_t)((counter >> 24) & 0xFF);
  const uint16_t crc = crc16_ccitt(block, 8);
  block[8] = (uint8_t)(crc & 0xFF);
  block[9] = (uint8_t)((crc >> 8) & 0xFF);
}

bool card_block_unpack(const uint8_t block[16], CardPayload* out) {
  if (!block || !out) {
    return false;
  }
  const uint16_t expect = crc16_ccitt(block, 8);
  const uint16_t got = (uint16_t)(block[8] | ((uint16_t)block[9] << 8));
  if (expect != got) {
    return false;
  }
  out->user_id =
      (uint32_t)block[0] | ((uint32_t)block[1] << 8) |
      ((uint32_t)block[2] << 16) | ((uint32_t)block[3] << 24);
  out->counter =
      (uint32_t)block[4] | ((uint32_t)block[5] << 8) |
      ((uint32_t)block[6] << 16) | ((uint32_t)block[7] << 24);
  return true;
}
