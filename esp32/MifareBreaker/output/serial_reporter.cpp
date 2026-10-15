#include "output/serial_reporter.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

static void (*g_cb)(const char* json) = NULL;
static char _tmp[640];
static char _esc[256];

void reporter_init(void) { g_cb = NULL; }

void reporter_register_cb(void (*cb)(const char* json)) { g_cb = cb; }

static void _emit(const char* json) {
    Serial.println(json);
    if (g_cb) g_cb(json);
}

/** Escape string for JSON (quotes, backslash, control chars). */
static const char* _jsonEscape(const char* in, char* out, size_t outCap) {
    if (!in || !out || outCap < 2) {
        if (out && outCap) out[0] = '\0';
        return out ? out : "";
    }
    size_t o = 0;
    for (size_t i = 0; in[i] != '\0' && o + 2 < outCap; ++i) {
        const unsigned char c = (unsigned char)in[i];
        if (c == '"' || c == '\\') {
            if (o + 3 >= outCap) break;
            out[o++] = '\\';
            out[o++] = (char)c;
        } else if (c < 0x20) {
            // skip other control characters to keep one-line JSON
            continue;
        } else {
            out[o++] = (char)c;
        }
    }
    out[o] = '\0';
    return out;
}

void reporter_error(const char* code, const char* msg) {
    char codeEsc[64];
    _jsonEscape(code ? code : "", codeEsc, sizeof(codeEsc));
    _jsonEscape(msg ? msg : "", _esc, sizeof(_esc));
    snprintf(_tmp, sizeof(_tmp),
             "{\"event\":\"error\",\"data\":{\"code\":\"%s\",\"msg\":\"%s\"}}",
             codeEsc, _esc);
    _emit(_tmp);
}

void reporter_cardDetected(const CardDump* d) {
    char uidhex[32] = {0};
    for (uint8_t i = 0; i < d->uid.len && (i * 2 + 2) < sizeof(uidhex); i++) {
        snprintf(uidhex + 2 * i, 3, "%02X", d->uid.bytes[i]);
    }
    snprintf(_tmp, sizeof(_tmp),
             "{\"event\":\"card_detected\",\"data\":{\"uid\":\"%s\",\"type\":\"MIFARE_%s\",\"sectors\":%u}}",
             uidhex, d->cardType == 4 ? "4K" : "1K", d->totalSectors);
    _emit(_tmp);
}

void reporter_attackStart(const char* attack, uint8_t sector, const char* key_type) {
    _jsonEscape(attack ? attack : "", _esc, sizeof(_esc));
    char kt[16];
    _jsonEscape(key_type ? key_type : "", kt, sizeof(kt));
    snprintf(_tmp, sizeof(_tmp),
             "{\"event\":\"attack_start\",\"data\":{\"attack\":\"%s\",\"sector\":%u,\"key_type\":\"%s\"}}",
             _esc, sector, kt);
    _emit(_tmp);
}

void reporter_keyFound(uint8_t sector, const char* key_type, const uint8_t* key6, const char* method) {
    char khex[13] = {0};
    for (int i = 0; i < 6; i++) snprintf(khex + 2 * i, 3, "%02X", key6[i]);
    char kt[8];
    char meth[32];
    _jsonEscape(key_type ? key_type : "", kt, sizeof(kt));
    _jsonEscape(method ? method : "", meth, sizeof(meth));
    snprintf(_tmp, sizeof(_tmp),
             "{\"event\":\"key_found\",\"data\":{\"sector\":%u,\"key_type\":\"%s\",\"key\":\"%s\",\"method\":\"%s\"}}",
             sector, kt, khex, meth);
    _emit(_tmp);
}

void reporter_keyFailed(uint8_t sector, const char* key_type) {
    char kt[8];
    _jsonEscape(key_type ? key_type : "", kt, sizeof(kt));
    snprintf(_tmp, sizeof(_tmp),
             "{\"event\":\"key_failed\",\"data\":{\"sector\":%u,\"key_type\":\"%s\"}}",
             sector, kt);
    _emit(_tmp);
}

void reporter_dataRead(uint8_t sector, uint8_t block, const uint8_t* data16) {
    char hex[BLOCK_SIZE * 2 + 1];
    for (int i = 0; i < BLOCK_SIZE; i++) snprintf(hex + 2 * i, 3, "%02X", data16[i]);
    snprintf(_tmp, sizeof(_tmp),
             "{\"event\":\"data_read\",\"data\":{\"sector\":%u,\"block\":%u,\"hex\":\"%s\"}}",
             sector, block, hex);
    _emit(_tmp);
}

void reporter_summary(const CardDump* d, uint32_t elapsed_ms) {
    uint32_t cracked = ks_countCracked(d);
    snprintf(_tmp, sizeof(_tmp),
             "{\"event\":\"attack_summary\",\"data\":{\"total_sectors\":%u,\"cracked\":%u,\"failed\":%u,\"elapsed_ms\":%u}}",
             (unsigned)d->totalSectors, (unsigned)cracked,
             (unsigned)(d->totalSectors - cracked), (unsigned)elapsed_ms);
    _emit(_tmp);
}

void reporter_dumpSaved(const char* path) {
    _jsonEscape(path ? path : "", _esc, sizeof(_esc));
    snprintf(_tmp, sizeof(_tmp), "{\"event\":\"dump_saved\",\"data\":{\"path\":\"%s\"}}", _esc);
    _emit(_tmp);
}
