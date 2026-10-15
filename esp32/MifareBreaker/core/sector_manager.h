#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Classic 1K/4K sector → absolute block addressing (Proxmark-compatible). */
uint8_t sm_sectorToFirstBlock(uint8_t sector);
uint8_t sm_sectorToTrailer(uint8_t sector);
uint8_t sm_blocksInSector(uint8_t sector);
bool    sm_sectorValid(uint8_t sector, uint8_t totalSectors);
uint8_t sm_totalSectorsForType(uint8_t cardType);

#ifdef __cplusplus
}
#endif
