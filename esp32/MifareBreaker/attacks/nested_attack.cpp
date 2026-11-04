#include "attacks/nested_attack.h"
#include "hal/card_comm.h"
#include "core/key_store.h"
#include "output/serial_reporter.h"
#include "storage/sd_helper.h"
#include "config.h"
#include <SD.h>
#include <vector>
#include <stdlib.h>
#include <string.h>

// Same 50-key builtin list as dictionary_attack
static const uint8_t NESTED_BUILTIN[][KEY_SIZE] = {
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},{0xA0,0xA1,0xA2,0xA3,0xA4,0xA5},
    {0xB0,0xB1,0xB2,0xB3,0xB4,0xB5},{0x00,0x00,0x00,0x00,0x00,0x00},
    {0xD3,0xF7,0xD3,0xF7,0xD3,0xF7},{0x4D,0x3A,0x99,0xC3,0x51,0xDD},
    {0x1A,0x98,0x2C,0x7E,0x45,0x9A},{0xAA,0xBB,0xCC,0xDD,0xEE,0xFF},
    {0x71,0x4C,0x5C,0x88,0x6E,0x97},{0x58,0x7E,0xE5,0xF9,0x35,0x0F},
    {0xA0,0x47,0x8C,0xC3,0x90,0x91},{0x53,0x3C,0xB6,0xC7,0x23,0xF6},
    {0x8F,0xD0,0xA4,0xF2,0x56,0xE9},{0x6C,0x77,0x85,0x28,0x63,0xD7},
    {0x28,0x63,0xD7,0x6C,0x77,0x85},{0xAB,0xCD,0xEF,0x12,0x34,0x56},
    {0x12,0x34,0x56,0x78,0x9A,0xBC},{0x11,0x22,0x33,0x44,0x55,0x66},
    {0x01,0x02,0x03,0x04,0x05,0x06},{0x13,0x37,0x13,0x37,0x13,0x37},
    {0xDE,0xAD,0xBE,0xEF,0x12,0x34},{0xAA,0xFF,0xFF,0xFF,0xFF,0xFF},
    {0x10,0x20,0x30,0x40,0x50,0x60},{0xE5,0x67,0x8F,0x22,0x1C,0x11},
    {0xCA,0xFE,0xBA,0xBE,0xDE,0xAD},{0x0A,0x1B,0x2C,0x3D,0x4E,0x5F},
    {0xF0,0xE1,0xD2,0xC3,0xB4,0xA5},{0xAA,0xAA,0xAA,0xAA,0xAA,0xAA},
    {0x55,0x55,0x55,0x55,0x55,0x55},{0xC3,0x5D,0x55,0x28,0xA3,0x1B},
    {0xD9,0xA4,0xCC,0xD6,0x5B,0xD2},{0x4C,0xB1,0x60,0x89,0x6A,0x7B},
    {0x97,0x38,0xEA,0x53,0x42,0x5D},{0x3C,0x65,0xD0,0x2A,0x1F,0x9C},
    {0x8B,0x4C,0x91,0xE2,0xD7,0x06},{0x07,0x3E,0x5A,0xB1,0xC8,0x4F},
    {0xA9,0xFA,0xB6,0xDE,0xEA,0x1D},{0x3A,0x72,0x1E,0x4D,0x58,0xC9},
    {0x18,0x00,0x00,0x00,0x00,0x00},{0x23,0x45,0x67,0x89,0xAB,0xCD},
    {0xFF,0x07,0x80,0x69,0xFF,0x07},{0xB5,0xFF,0x67,0xD3,0xA4,0xBC},
    {0x0F,0x0F,0x0F,0x0F,0x0F,0x0F},{0xF0,0xF0,0xF0,0xF0,0xF0,0xF0},
    {0x1F,0xFF,0xFF,0xFF,0xFF,0xFF},{0x4A,0xF8,0x4B,0x19,0x6D,0xE3},
    {0x7B,0xE3,0x0A,0xD5,0x5A,0xC1},{0x2A,0x9F,0x14,0xD3,0x8E,0x0C},
};
static const size_t NESTED_BUILTIN_N = sizeof(NESTED_BUILTIN)/sizeof(NESTED_BUILTIN[0]);

// ─── Try key with mandatory re-select + full block read ────────────────────
static bool _tryKeyNested(CardDump* d, uint8_t sector,
                           const uint8_t* key, bool isA,
                           const char* method) {
    uint8_t atqa[2];
    if (picc_wakeupA(atqa) != RC522_OK)          return false;
    if (picc_anticollision(&d->uid) != RC522_OK) return false;

    uint8_t cmd     = isA ? MF_AUTH_KEY_A : MF_AUTH_KEY_B;
    uint8_t trailer = ks_sectorToTrailer(sector);

    if (picc_mfAuth(cmd, trailer, key, &d->uid) != RC522_OK) {
        picc_mfDeauth();
        return false;
    }

    uint8_t first = ks_sectorToFirstBlock(sector);
    for (uint8_t b = 0; b < BLOCKS_PER_SECTOR; b++) {
        uint8_t buf[BLOCK_SIZE];
        if (picc_mfRead(first + b, buf) == RC522_OK) {
            ks_storeBlockData(d, sector, b, buf);
            reporter_dataRead(sector, b, buf);
        }
    }
    if (isA) ks_setKeyA(d, sector, key);
    else     ks_setKeyB(d, sector, key);
    reporter_keyFound(sector, isA?"A":"B", key, method);
    picc_mfDeauth();
    return true;
}

// ─── Build candidate list ──────────────────────────────────────────────────
// Strategy:
//  1. All 50 builtin keys
//  2. Variations of known keys (byte-flip, XOR patterns)
//  3. Full SD dictionary
static std::vector<uint8_t*> _buildCandidates(const CardDump* d) {
    std::vector<uint8_t*> cands;
    auto addKey = [&](const uint8_t* src) {
        uint8_t* k = (uint8_t*)malloc(KEY_SIZE);
        if (k) { memcpy(k, src, KEY_SIZE); cands.push_back(k); }
    };

    // 1. Builtin list
    for (size_t i = 0; i < NESTED_BUILTIN_N; i++)
        addKey(NESTED_BUILTIN[i]);

    // 2. Variations from each already-known key
    for (uint8_t s = 0; s < d->totalSectors; s++) {
        const uint8_t* bases[2] = {nullptr, nullptr};
        if (ks_hasKeyA(d, s)) bases[0] = d->sectors[s].keyA;
        if (ks_hasKeyB(d, s)) bases[1] = d->sectors[s].keyB;
        for (int bi = 0; bi < 2; bi++) {
            if (!bases[bi]) continue;
            const uint8_t* base = bases[bi];
            // XOR each byte with 0xFF
            uint8_t t[KEY_SIZE];
            for (int i=0;i<6;i++) t[i]=(uint8_t)(base[i]^0xFF);
            addKey(t);
            // Increment each byte by 1
            for (int i=0;i<6;i++) t[i]=(uint8_t)(base[i]+1);
            addKey(t);
            // Byte-flip one position at a time
            for (int j=0;j<6;j++){
                memcpy(t, base, KEY_SIZE);
                t[j] = (uint8_t)(~t[j]);
                addKey(t);
            }
        }
    }

    // 3. SD dictionary
    if (sd_is_available()) {
        File f = SD.open(DICT_PATH);
        if (f) {
            while (f.available()) {
                String line = f.readStringUntil('\n');
                line.trim();
                if (line.length() != 12) continue;
                uint8_t* k = (uint8_t*)malloc(KEY_SIZE);
                if (!k) break;
                for (int i=0;i<6;i++)
                    k[i]=(uint8_t)strtoul(line.substring(i*2,i*2+2).c_str(),NULL,16);
                cands.push_back(k);
            }
            f.close();
        }
    }
    return cands;
}

// ─── Main ──────────────────────────────────────────────────────────────────
void nestedAttack_run(CardDump* d) {
    if (ks_firstKnownSector(d) < 0) {
        reporter_error("NESTED_NO_BASE", "No known key — run dictionary first");
        return;
    }

    std::vector<uint8_t*> cands = _buildCandidates(d);

    for (uint8_t s = 0; s < d->totalSectors; s++) {
        if (ks_isSectorKnown(d, s)) continue;
        for (auto kptr : cands) {
            reporter_attackStart("nested", s, "A");
            if (_tryKeyNested(d, s, kptr, true, "nested-heu")) break;
            reporter_keyFailed(s, "A");
            reporter_attackStart("nested", s, "B");
            if (_tryKeyNested(d, s, kptr, false, "nested-heu")) break;
            reporter_keyFailed(s, "B");
            if (ks_isSectorKnown(d, s)) break;
        }
    }

    for (auto kptr : cands) free(kptr);
}
