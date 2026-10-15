#include "ui/display_menu.h"
#include "config.h"

#if USE_DISPLAY

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifndef DISPLAY_SDA
#define DISPLAY_SDA 21
#endif
#ifndef DISPLAY_SCL
#define DISPLAY_SCL 25
#endif
#ifndef DISPLAY_ADDR
#define DISPLAY_ADDR 0x3C
#endif
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH 128
#endif
#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT 64
#endif

static Adafruit_SSD1306* g_disp = nullptr;

bool display_init(void) {
    Wire.begin(DISPLAY_SDA, DISPLAY_SCL);
    g_disp = new Adafruit_SSD1306(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, -1);
    if (!g_disp->begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDR)) {
        delete g_disp;
        g_disp = nullptr;
        return false;
    }
    g_disp->clearDisplay();
    g_disp->setTextColor(SSD1306_WHITE);
    g_disp->setTextSize(1);
    g_disp->setCursor(0, 0);
    g_disp->println(F("MifareBreaker"));
    g_disp->display();
    return true;
}

void display_showBoot(const char* ipOrStatus) {
    if (!g_disp) return;
    g_disp->clearDisplay();
    g_disp->setCursor(0, 0);
    g_disp->println(F("MifareBreaker"));
    g_disp->println(ipOrStatus ? ipOrStatus : "boot");
    g_disp->display();
}

void display_showCard(const CardDump* d) {
    if (!g_disp || !d) return;
    g_disp->clearDisplay();
    g_disp->setCursor(0, 0);
    g_disp->print(F("UID "));
    for (uint8_t i = 0; i < d->uid.len && i < 7; i++) {
        if (d->uid.bytes[i] < 16) g_disp->print('0');
        g_disp->print(d->uid.bytes[i], HEX);
    }
    g_disp->println();
    g_disp->print(F("Sectors "));
    g_disp->println(d->totalSectors);
    g_disp->display();
}

void display_showProgress(const char* attack, uint8_t sector, uint8_t cracked, uint8_t total) {
    if (!g_disp) return;
    g_disp->clearDisplay();
    g_disp->setCursor(0, 0);
    g_disp->println(attack ? attack : "?");
    g_disp->print(F("Sector "));
    g_disp->println(sector);
    g_disp->print(cracked);
    g_disp->print('/');
    g_disp->println(total);
    g_disp->display();
}

void display_showSummary(const CardDump* d, uint32_t elapsedMs) {
    if (!g_disp || !d) return;
    g_disp->clearDisplay();
    g_disp->setCursor(0, 0);
    g_disp->println(F("Done"));
    g_disp->print(ks_countCracked(d));
    g_disp->print('/');
    g_disp->println(d->totalSectors);
    g_disp->print(elapsedMs);
    g_disp->println(F(" ms"));
    g_disp->display();
}

#else

bool display_init(void) { return false; }
void display_showBoot(const char*) {}
void display_showCard(const CardDump*) {}
void display_showProgress(const char*, uint8_t, uint8_t, uint8_t) {}
void display_showSummary(const CardDump*, uint32_t) {}

#endif
