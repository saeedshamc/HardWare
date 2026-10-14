#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include "hal/rc522_driver.h"
#include "core/uid_reader.h"
#include "core/key_store.h"
#include "output/serial_reporter.h"
#include "output/web_server.h"
#include "attacks/dictionary_attack.h"
#include "attacks/nested_attack.h"
#include "attacks/darkside_attack.h"
#include "storage/dump_manager.h"
#include "storage/sd_helper.h"

static CardDump g_dump;

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial) delay(10);

    reporter_init();

    // RC522
    if (!rc522_init()) {
        reporter_error("RC522_FAIL", "RC522 not responding — check SPI wiring");
        while (true) delay(1000);
    }

    // SD Card (optional — dictionary and dumps disabled if absent)
    if (!sd_init())
        reporter_error("SD_WARN", "SD card not found — using built-in dict only");

    // WiFi + Web Dashboard
    webServer_init();
    webServer_setDump(&g_dump);

    reporter_error("BOOT_OK", "MifareBreaker ready — waiting for card");
}

void loop() {
    ks_init(&g_dump);

    // ── 1. Wait for card ─────────────────────────────────────────────────────
    if (!uid_detect(&g_dump)) { delay(200); return; }
    reporter_cardDetected(&g_dump);
    webServer_setDump(&g_dump);

    uint32_t t0 = millis();

    // ── 2. Dictionary Attack ─────────────────────────────────────────────────
    dictAttack_run(&g_dump);

    // ── 3. Nested Attack (needs >= 1 known key) ──────────────────────────────
    if (ks_firstKnownSector(&g_dump) >= 0)
        nestedAttack_run(&g_dump);

    // ── 4. Darkside (for any sector still completely unknown) ─────────────────
    for (uint8_t i = 0; i < g_dump.totalSectors; i++) {
        if (!ks_isSectorKnown(&g_dump, i))
            darksideAttack_runSector(&g_dump, i);
    }

    // ── 5. Summary + save ────────────────────────────────────────────────────
    reporter_summary(&g_dump, millis() - t0);
    dump_save(&g_dump);

    // ── 6. Wait for card removal ─────────────────────────────────────────────
    picc_halt();
    while (picc_isPresent()) delay(150);
    delay(500);
}
