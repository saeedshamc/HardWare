#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Host-side copy of addressing rules — keep in sync with core/sector_manager.cpp */
static uint8_t sm_sectorToFirstBlock(uint8_t sector) {
    return sector < 32 ? (uint8_t)(sector * 4) : (uint8_t)(128 + (sector - 32) * 16);
}
static uint8_t sm_sectorToTrailer(uint8_t sector) {
    return sector < 32 ? (uint8_t)(sector * 4 + 3) : (uint8_t)(128 + (sector - 32) * 16 + 15);
}
static uint8_t sm_blocksInSector(uint8_t sector) {
    return sector < 32 ? 4 : 16;
}

static int fails = 0;

static void expect_u8(const char* name, uint8_t got, uint8_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %u want %u\n", name, got, want);
        fails++;
    }
}

int main(void) {
    expect_u8("s0 first", sm_sectorToFirstBlock(0), 0);
    expect_u8("s0 trail", sm_sectorToTrailer(0), 3);
    expect_u8("s15 first", sm_sectorToFirstBlock(15), 60);
    expect_u8("s15 trail", sm_sectorToTrailer(15), 63);
    expect_u8("s31 first", sm_sectorToFirstBlock(31), 124);
    expect_u8("s32 first", sm_sectorToFirstBlock(32), 128);
    expect_u8("s32 trail", sm_sectorToTrailer(32), 143);
    expect_u8("s39 trail", sm_sectorToTrailer(39), 255);
    expect_u8("blocks s0", sm_blocksInSector(0), 4);
    expect_u8("blocks s32", sm_blocksInSector(32), 16);

    if (fails) {
        fprintf(stderr, "%d failure(s)\n", fails);
        return 1;
    }
    puts("ok");
    return 0;
}
