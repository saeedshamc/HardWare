#include "storage/dump_manager.h"
#include "storage/sd_helper.h"
#include "core/sector_manager.h"
#include <SD.h>
#include <Arduino.h>
#include <stdio.h>
#include "output/serial_reporter.h"

static String _lastPath;

// Proxmark3-compatible .mfd binary format:
//   1K  → 1024 bytes (64 blocks × 16 bytes, raw)
//   4K  → 4096 bytes (256 blocks × 16 bytes, raw)
// Sector trailer (last block) layout:
//   bytes  0-5  : Key A (0x00 if unknown)
//   bytes  6-8  : Access bits (0xFF 0x07 0x80 = default)
//   byte   9    : GPB (0x69 = default)
//   bytes 10-15 : Key B (0x00 if unknown)
void dump_save(const CardDump* d) {
    if (!sd_is_available()) {
        reporter_error("DUMP_SKIP", "No SD card — dump not saved");
        return;
    }

    // Build filename from UID hex
    char uidhex[21] = {};
    for (uint8_t i = 0; i < d->uid.len && i < 10; i++)
        snprintf(uidhex + i*2, 3, "%02X", d->uid.bytes[i]);

    String path = String(DUMP_PATH_PREFIX) + "dump_" + uidhex + ".mfd";

    File f = SD.open(path.c_str(), FILE_WRITE);
    if (!f) {
        reporter_error("DUMP_ERR", "Cannot open file for writing");
        return;
    }

    uint8_t block[BLOCK_SIZE];

    for (uint8_t s = 0; s < d->totalSectors; s++) {
        uint8_t nBlocks  = sm_blocksInSector(s);
        uint8_t first    = sm_sectorToFirstBlock(s);

        for (uint8_t b = 0; b < nBlocks; b++) {
            bool isTrailer = (b == nBlocks - 1);

            if (d->sectors[s].dataRead && b < BLOCKS_PER_SECTOR) {
                // Data came from the card directly
                memcpy(block, &d->sectors[s].data[b * BLOCK_SIZE], BLOCK_SIZE);
            } else {
                memset(block, 0x00, BLOCK_SIZE);
            }

            // Overlay trailer with known keys (even if data wasn't read)
            if (isTrailer) {
                if (d->sectors[s].statusA == KEY_FOUND)
                    memcpy(block,     d->sectors[s].keyA, KEY_SIZE);
                // Access bits: use data from card if available, else defaults
                if (!d->sectors[s].dataRead) {
                    block[6] = 0xFF; block[7] = 0x07;
                    block[8] = 0x80; block[9] = 0x69;
                }
                if (d->sectors[s].statusB == KEY_FOUND)
                    memcpy(block+10, d->sectors[s].keyB, KEY_SIZE);
            }
            f.write(block, BLOCK_SIZE);
        }
    }
    f.close();

    _lastPath = path;
    reporter_dumpSaved(path.c_str());
}

const char* dump_lastPath(void) {
    return _lastPath.length() ? _lastPath.c_str() : nullptr;
}
