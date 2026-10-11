#pragma once

#include <stdint.h>
#include <stddef.h>

// CRC-CCITT (poly 0x1021, init 0xFFFF)
uint16_t crc16_ccitt(const uint8_t* data, size_t len);
