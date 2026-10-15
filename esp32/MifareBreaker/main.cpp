#include <Arduino.h>
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
#include "ui/display_menu.h"

static CardDump g_dump;

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial) delay(10);

    reporter_init();

    if (!rc522_init()) {
        reporter_error("RC522_FAIL", "RC522 not responding — check SPI wiring");
        while (true) delay(1000);
    }

    if (!sd_init())
        reporter_error("SD_WARN", "SD card not found — using built-in dict only");

    webServer_init();
    webServer_setDump(&g_dump);

    if (display_init()) {
        display_showBoot(webServer_ipOrStatus().c_str());
    }

    reporter_error("BOOT_OK", "MifareBreaker ready — waiting for card");
}

void loop() {
    ks_init(&g_dump);

    if (!uid_detect(&g_dump)) { delay(200); return; }
    reporter_cardDetected(&g_dump);
    webServer_setDump(&g_dump);
    display_showCard(&g_dump);

    uint32_t t0 = millis();

    display_showProgress("dictionary", 0, 0, g_dump.totalSectors);
    dictAttack_run(&g_dump);
    display_showProgress("dictionary", 0, ks_countCracked(&g_dump), g_dump.totalSectors);

    if (ks_firstKnownSector(&g_dump) >= 0) {
        display_showProgress("nested", 0, ks_countCracked(&g_dump), g_dump.totalSectors);
        nestedAttack_run(&g_dump);
    }

    for (uint8_t i = 0; i < g_dump.totalSectors; i++) {
        if (!ks_isSectorKnown(&g_dump, i)) {
            display_showProgress("darkside", i, ks_countCracked(&g_dump), g_dump.totalSectors);
            darksideAttack_runSector(&g_dump, i);
        }
    }

    uint32_t elapsed = millis() - t0;
    reporter_summary(&g_dump, elapsed);
    dump_save(&g_dump);
    display_showSummary(&g_dump, elapsed);

    picc_halt();
    while (picc_isPresent()) delay(150);
    delay(500);
}
