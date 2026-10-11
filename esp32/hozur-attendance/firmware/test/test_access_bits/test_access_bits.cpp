#include <unity.h>
#include <string.h>

#include "../../src/security/access_bits.h"
#include "../../src/security/access_bits.cpp"
#include "../../src/security/crc16.h"
#include "../../src/security/crc16.cpp"
#include "../../src/security/card_block.h"
#include "../../src/security/card_block.cpp"

void test_hozour_default_validates(void) {
  uint8_t access[3];
  TEST_ASSERT_TRUE(access_bits_hozour_default(access));
  TEST_ASSERT_TRUE(access_bits_validate(access));
}

void test_corrupt_access_bits_rejected(void) {
  uint8_t access[3];
  TEST_ASSERT_TRUE(access_bits_hozour_default(access));
  access[0] ^= 0x01;  // break complement
  TEST_ASSERT_FALSE(access_bits_validate(access));
}

void test_roundtrip_encode_decode(void) {
  uint8_t access[3];
  AccessBitsConditions cond = {};
  for (int i = 0; i < 3; ++i) {
    cond.c1[i] = 0;
    cond.c2[i] = 1;
    cond.c3[i] = 0;
  }
  cond.c1[3] = 0;
  cond.c2[3] = 1;
  cond.c3[3] = 1;
  TEST_ASSERT_TRUE(access_bits_encode(&cond, access));

  AccessBitsConditions got;
  TEST_ASSERT_TRUE(access_bits_decode(access, &got));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(cond.c1, got.c1, 4);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(cond.c2, got.c2, 4);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(cond.c3, got.c3, 4);
}

void test_card_block_crc(void) {
  uint8_t block[16];
  card_block_pack(42, 7, block);
  CardPayload p;
  TEST_ASSERT_TRUE(card_block_unpack(block, &p));
  TEST_ASSERT_EQUAL_UINT32(42, p.user_id);
  TEST_ASSERT_EQUAL_UINT32(7, p.counter);

  block[0] ^= 0xFF;
  TEST_ASSERT_FALSE(card_block_unpack(block, &p));
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_hozour_default_validates);
  RUN_TEST(test_corrupt_access_bits_rejected);
  RUN_TEST(test_roundtrip_encode_decode);
  RUN_TEST(test_card_block_crc);
  return UNITY_END();
}
