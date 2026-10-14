#include "output/serial_reporter.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

static void (*g_cb)(const char* json) = NULL;
static char _tmp[512];

void reporter_init(void) {
    Serial.begin(SERIAL_BAUD);
}
void reporter_register_cb(void (*cb)(const char* json)) { g_cb = cb; }

static void _emit(const char* json) {
    Serial.println(json);
    if (g_cb) g_cb(json);
}

void reporter_error(const char* code, const char* msg) {
    snprintf(_tmp, sizeof(_tmp), "{\"event\":\"error\",\"data\":{\"code\":\"%s\",\"msg\":\"%s\"}}", code, msg);
    _emit(_tmp);
}
void reporter_cardDetected(const CardDump* d) {
    char uidhex[32]={0};
    for (uint8_t i=0;i<d->uid.len;i++) snprintf(uidhex+2*i,3, "%02X", d->uid.bytes[i]);
    snprintf(_tmp, sizeof(_tmp), "{\"event\":\"card_detected\",\"data\":{\"uid\":\"%s\",\"type\":\"MIFARE_%s\",\"sectors\":%u}}",
        uidhex, d->cardType==4?"4K":"1K", d->totalSectors);
    _emit(_tmp);
}
void reporter_attackStart(const char* attack, uint8_t sector, const char* key_type) {
    snprintf(_tmp, sizeof(_tmp), "{\"event\":\"attack_start\",\"data\":{\"attack\":\"%s\",\"sector\":%u,\"key_type\":\"%s\"}}", attack, sector, key_type);
    _emit(_tmp);
}
void reporter_keyFound(uint8_t sector, const char* key_type, const uint8_t* key6, const char* method) {
    char khex[13]={0}; for (int i=0;i<6;i++) snprintf(khex+2*i,3, "%02X", key6[i]);
    snprintf(_tmp, sizeof(_tmp), "{\"event\":\"key_found\",\"data\":{\"sector\":%u,\"key_type\":\"%s\",\"key\":\"%s\",\"method\":\"%s\"}}", sector, key_type, khex, method);
    _emit(_tmp);
}
void reporter_keyFailed(uint8_t sector, const char* key_type) {
    snprintf(_tmp, sizeof(_tmp), "{\"event\":\"key_failed\",\"data\":{\"sector\":%u,\"key_type\":\"%s\"}}", sector, key_type);
    _emit(_tmp);
}
void reporter_dataRead(uint8_t sector, uint8_t block, const uint8_t* data16) {
    char hex[BLOCK_SIZE*2+1]; for (int i=0;i<BLOCK_SIZE;i++) snprintf(hex+2*i,3, "%02X", data16[i]);
    snprintf(_tmp, sizeof(_tmp), "{\"event\":\"data_read\",\"data\":{\"sector\":%u,\"block\":%u,\"hex\":\"%s\"}}", sector, block, hex);
    _emit(_tmp);
}
void reporter_summary(const CardDump* d, uint32_t elapsed_ms) {
    snprintf(_tmp, sizeof(_tmp), "{\"event\":\"attack_summary\",\"data\":{\"total_sectors\":%u,\"cracked\":%u,\"failed\":%u,\"elapsed_ms\":%u}}",
        d->totalSectors, ks_countCracked(d), d->totalSectors-ks_countCracked(d), (unsigned)elapsed_ms);
    _emit(_tmp);
}
void reporter_dumpSaved(const char* path) {
    snprintf(_tmp, sizeof(_tmp), "{\"event\":\"dump_saved\",\"data\":{\"path\":\"%s\"}}", path);
    _emit(_tmp);
}
