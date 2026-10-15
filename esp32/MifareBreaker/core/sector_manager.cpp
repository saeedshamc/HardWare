#include "core/sector_manager.h"

// MIFARE Classic 1K: sectors 0–15 → 4 blocks each
// MIFARE Classic 4K: sectors 0–31 → 4 blocks; sectors 32–39 → 16 blocks
uint8_t sm_sectorToFirstBlock(uint8_t sector) {
    return sector < 32 ? (uint8_t)(sector * 4) : (uint8_t)(128 + (sector - 32) * 16);
}

uint8_t sm_sectorToTrailer(uint8_t sector) {
    return sector < 32 ? (uint8_t)(sector * 4 + 3) : (uint8_t)(128 + (sector - 32) * 16 + 15);
}

uint8_t sm_blocksInSector(uint8_t sector) {
    return sector < 32 ? 4 : 16;
}

bool sm_sectorValid(uint8_t sector, uint8_t totalSectors) {
    return totalSectors > 0 && sector < totalSectors;
}

uint8_t sm_totalSectorsForType(uint8_t cardType) {
    // uid_reader cardType: typically 0=unknown, 1=1K (16 sectors), 2=4K (40)
    if (cardType == 2) return 40;
    if (cardType == 1) return 16;
    return 16;
}
