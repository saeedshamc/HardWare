#include "storage/dump_manager.h"
#include "storage/sd_helper.h"
#include <SD.h>
#include <Arduino.h>
#include <stdio.h>
#include "core/key_store.h"
#include "output/serial_reporter.h"

static String _lastDumpPath = String("");

void dump_save(const CardDump* d) {
    if (!sd_is_available()) {
        reporter_error("SD_MISS", "SD absent — cannot save dump");
        return;
    }

    char uidhex[32] = {0};
    for (uint8_t i = 0; i < d->uid.len && (i * 2 + 2) < sizeof(uidhex); i++) {
        snprintf(uidhex + 2 * i, 3, "%02X", d->uid.bytes[i]);
    }
    String filename = String("dump_") + uidhex + ".mfd";
    String path = String(DUMP_PATH_PREFIX) + filename;

    File f = SD.open(path.c_str(), FILE_WRITE);
    if (!f) {
        reporter_error("SD_WRITE", "Failed to open dump file");
        return;
    }
    // Simple binary dump: uid + sector count + sector records
    f.write(d->uid.bytes, d->uid.len);
    f.write(&d->totalSectors, 1);
    for (uint8_t s = 0; s < d->totalSectors; s++) {
        f.write((const uint8_t*)&d->sectors[s], sizeof(d->sectors[s]));
    }
    f.close();
    _lastDumpPath = path;
    reporter_dumpSaved(path.c_str());
}

const char* dump_lastPath(void) {
    return _lastDumpPath.length() ? _lastDumpPath.c_str() : NULL;
}
