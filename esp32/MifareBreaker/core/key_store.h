#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "hal/card_comm.h"

typedef enum { KEY_UNKNOWN=0, KEY_FOUND=1, KEY_FAILED=2 } KeyStatus;

typedef struct {
    uint8_t   keyA[KEY_SIZE], keyB[KEY_SIZE];
    KeyStatus statusA, statusB;
    bool      dataRead;
    uint8_t   data[BLOCKS_PER_SECTOR * BLOCK_SIZE];
} SectorInfo;

typedef struct {
    PiccUID    uid;
    uint8_t    cardType;
    uint8_t    totalSectors;
    SectorInfo sectors[MAX_SECTORS];
} CardDump;

void    ks_init(CardDump* d);
void    ks_setKeyA(CardDump* d, uint8_t s, const uint8_t* k);
void    ks_setKeyB(CardDump* d, uint8_t s, const uint8_t* k);
void    ks_markFailed(CardDump* d, uint8_t s, bool isA);
void    ks_storeBlockData(CardDump* d, uint8_t s, uint8_t b, const uint8_t* data16);
bool    ks_hasKeyA(const CardDump* d, uint8_t s);
bool    ks_hasKeyB(const CardDump* d, uint8_t s);
bool    ks_isSectorKnown(const CardDump* d, uint8_t s);
int8_t  ks_firstKnownSector(const CardDump* d);
uint8_t ks_countCracked(const CardDump* d);
uint8_t ks_sectorToFirstBlock(uint8_t s);
uint8_t ks_sectorToTrailer(uint8_t s);
