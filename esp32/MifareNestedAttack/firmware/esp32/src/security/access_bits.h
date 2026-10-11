#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool access_bits_validate(const uint8_t access[3]);
void access_bits_print_decode(const uint8_t access[3]);

#ifdef __cplusplus
}
#endif
