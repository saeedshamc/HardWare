#include "dump_manager.h"
#include "output/serial_reporter.h"
#include "config.h"
#include <SD.h>
#include <SPIFFS.h>
#include <stdio.h>
#include <string.h>

// Proxmark3-compatible .mfd format:
// 1K → 1024 bytes (64 blocks × 16 bytes, raw sector data)
// 4K → 4096 bytes (256 blocks × 16 bytes)
// Keys are embedded in the sector trailer blocks (block 3 of each sector):
//   bytes 0-5: Key A
//   bytes 6-9: Access bits
//   bytes 10-15: Key B

static char _lastPath[64];

const char* dump_save(const CardDump* dump) {
    // Build filename from UID
    char uidHex[21] = "unknown";
    if (dump->uid.len > 0) {
        const char* H = "0123456789ABCDEF";
        for (uint8_t i = 0; i < dump->uid.len && i < 10; i++) {
            uidHex[i*2]   = H[(dump->uid.bytes[i]>>4)&0xF];
            uidHex[i*2+1] = H[ dump->uid.bytes[i]    &0xF];
        }
        uidHex[dump->uid.len*2] = '\0';
    }
    snprintf(_lastPath, sizeof(_lastPath), "%s%s.mfd", DUMP_PATH_PREFIX, uidHex);

    // Determine total blocks
    uint16_t totalBlocks = 0;
    for (uint8_t s = 0; s < dump->totalSectors; s++) {
        uint8_t blocksInSector = (s < 32) ? 4 : 16;
        totalBlocks += blocksInSector;
    }

    // Write to SD card
    File f = SD.open(_lastPath, FILE_WRITE);
    if (!f) {
        reporter_error("DUMP_FAIL", "Cannot open dump file on SD");
        return nullptr;
    }

    for (uint8_t s = 0; s < dump->totalSectors; s++) {
        uint8_t blocksInSector = (s < 32) ? 4 : 16;
        for (uint8_t b = 0; b < blocksInSector; b++) {
            if (dump->sectors[s].dataRead && b < BLOCKS_PER_SECTOR) {
                f.write(&dump->sectors[s].data[b * BLOCK_SIZE], BLOCK_SIZE);
            } else {
                // Write zeros for unread blocks
                uint8_t zeros[BLOCK_SIZE] = {};

                // For sector trailer: embed known keys
                if (b == blocksInSector - 1) {
                    if (dump->sectors[s].statusA == KEY_FOUND)
                        memcpy(zeros,    dump->sectors[s].keyA, KEY_SIZE);
                    if (dump->sectors[s].statusB == KEY_FOUND)
                        memcpy(zeros+10, dump->sectors[s].keyB, KEY_SIZE);
                    // Default access bits: FF 07 80 69
                    zeros[6]=0xFF; zeros[7]=0x07; zeros[8]=0x80; zeros[9]=0x69;
                }
                f.write(zeros, BLOCK_SIZE);
            }
        }
    }
    f.close();

    // Also copy to SPIFFS for web dashboard download
    File spf = SPIFFS.open("/dump_latest.mfd", "w");
    if (spf) {
        File src = SD.open(_lastPath, FILE_READ);
        if (src) {
            uint8_t buf[64];
            while (src.available()) {
                int n = src.read(buf, sizeof(buf));
                if (n > 0) spf.write(buf, n);
            }
            src.close();
        }
        spf.close();
    }

    reporter_dumpSaved(_lastPath);
    return _lastPath;
}

bool dump_load(const char* path, CardDump* dump) {
    File f = SD.open(path, FILE_READ);
    if (!f) { reporter_error("DUMP_LOAD_FAIL", "File not found"); return false; }

    ks_init(dump);
    uint8_t sector = 0, blockInSector = 0;

    while (f.available() && sector < MAX_SECTORS) {
        uint8_t buf[BLOCK_SIZE];
        if (f.read(buf, BLOCK_SIZE) != BLOCK_SIZE) break;

        uint8_t blocksInSector = (sector < 32) ? 4 : 16;
        ks_storeBlockData(dump, sector, blockInSector, buf);

        // Extract keys from sector trailer
        if (blockInSector == blocksInSector - 1) {
            ks_setKeyA(dump, sector, buf);        // bytes 0-5
            ks_setKeyB(dump, sector, buf + 10);   // bytes 10-15
            sector++;
            blockInSector = 0;
        } else {
            blockInSector++;
        }
    }
    f.close();
    return true;
}

void dump_print(const CardDump* dump) {
    const char* H = "0123456789ABCDEF";
    for (uint8_t s = 0; s < dump->totalSectors; s++) {
        char keyA[13] = "????????????", keyB[13] = "????????????";
        if (dump->sectors[s].statusA == KEY_FOUND) {
            for (int i=0;i<6;i++){
                keyA[i*2]  =H[(dump->sectors[s].keyA[i]>>4)&0xF];
                keyA[i*2+1]=H[ dump->sectors[s].keyA[i]    &0xF];
            } keyA[12]='\0';
        }
        if (dump->sectors[s].statusB == KEY_FOUND) {
            for (int i=0;i<6;i++){
                keyB[i*2]  =H[(dump->sectors[s].keyB[i]>>4)&0xF];
                keyB[i*2+1]=H[ dump->sectors[s].keyB[i]    &0xF];
            } keyB[12]='\0';
        }
        char line[128];
        snprintf(line,sizeof(line),
            "{\"event\":\"dump_sector\",\"data\":{\"sector\":%d,\"keyA\":\"%s\",\"keyB\":\"%s\"}}",
            s, keyA, keyB);
        Serial.println(line);

        if (dump->sectors[s].dataRead) {
            uint8_t first = ks_sectorToFirstBlock(s);
            for (uint8_t b=0; b<BLOCKS_PER_SECTOR; b++) {
                char hex[33];
                for(int j=0;j<16;j++){
                    hex[j*2]  =H[(dump->sectors[s].data[b*16+j]>>4)&0xF];
                    hex[j*2+1]=H[ dump->sectors[s].data[b*16+j]    &0xF];
                } hex[32]='\0';
                snprintf(line,sizeof(line),
                    "{\"event\":\"dump_block\",\"data\":{\"block\":%d,\"hex\":\"%s\"}}",
                    first+b, hex);
                Serial.println(line);
            }
        }
    }
}
