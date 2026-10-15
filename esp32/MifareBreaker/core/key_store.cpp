#include "key_store.h"
#include "core/sector_manager.h"
#include <string.h>

void ks_init(CardDump* d) {
    memset(d, 0, sizeof(CardDump));
    for (uint8_t i=0; i<MAX_SECTORS; i++) {
        d->sectors[i].statusA = KEY_UNKNOWN;
        d->sectors[i].statusB = KEY_UNKNOWN;
    }
}
void ks_setKeyA(CardDump* d, uint8_t s, const uint8_t* k) { memcpy(d->sectors[s].keyA,k,KEY_SIZE); d->sectors[s].statusA=KEY_FOUND; }
void ks_setKeyB(CardDump* d, uint8_t s, const uint8_t* k) { memcpy(d->sectors[s].keyB,k,KEY_SIZE); d->sectors[s].statusB=KEY_FOUND; }
void ks_markFailed(CardDump* d, uint8_t s, bool isA) { if(isA) d->sectors[s].statusA=KEY_FAILED; else d->sectors[s].statusB=KEY_FAILED; }
void ks_storeBlockData(CardDump* d, uint8_t s, uint8_t b, const uint8_t* data) {
    if(b<BLOCKS_PER_SECTOR) { memcpy(&d->sectors[s].data[b*BLOCK_SIZE],data,BLOCK_SIZE); d->sectors[s].dataRead=true; }
}
bool    ks_hasKeyA(const CardDump* d, uint8_t s) { return d->sectors[s].statusA==KEY_FOUND; }
bool    ks_hasKeyB(const CardDump* d, uint8_t s) { return d->sectors[s].statusB==KEY_FOUND; }
bool    ks_isSectorKnown(const CardDump* d, uint8_t s) { return ks_hasKeyA(d,s)||ks_hasKeyB(d,s); }
int8_t  ks_firstKnownSector(const CardDump* d) {
    for(uint8_t i=0;i<d->totalSectors;i++) if(ks_isSectorKnown(d,i)) return (int8_t)i;
    return -1;
}
uint8_t ks_countCracked(const CardDump* d) {
    uint8_t n=0;
    for(uint8_t i=0;i<d->totalSectors;i++) if(ks_isSectorKnown(d,i)) n++;
    return n;
}
uint8_t ks_sectorToFirstBlock(uint8_t s) { return sm_sectorToFirstBlock(s); }
uint8_t ks_sectorToTrailer(uint8_t s)    { return sm_sectorToTrailer(s); }
