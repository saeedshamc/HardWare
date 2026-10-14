#pragma once

#include <stdint.h>

void crypto1_init(const uint8_t key[6]);
void crypto1_reset(void);
void crypto1_keystream(uint8_t* out, int len);
