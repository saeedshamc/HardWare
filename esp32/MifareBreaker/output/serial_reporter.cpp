#include "serial_reporter.h"
#include <stdio.h>
#include <string.h>

#define MAX_OBS 4
#define BSIZ    300

static ReporterObserver _obs[MAX_OBS];
static uint8_t _cnt = 0;

static void _hex(const uint8_t* b, uint8_t len, char* out) {
    static const char H[] = "0123456789ABCDEF";
    for(uint8_t i=0;i<len;i++){ out[i*2]=H[(b[i]>>4)&0xF]; out[i*2+1]=H[b[i]&0xF]; }
    out[len*2]='\0';
}
static void _emit(const char* json) {
    Serial.println(json);
    for(uint8_t i=0;i<_cnt;i++) if(_obs[i]) _obs[i](json);
}
void reporter_init(void) { _cnt=0; memset(_obs,0,sizeof(_obs)); }
void reporter_addObserver(ReporterObserver cb) { if(_cnt<MAX_OBS) _obs[_cnt++]=cb; }

void reporter_cardDetected(const CardDump* d) {
    char uid[21], atqa[5]; _hex(d->uid.bytes,d->uid.len,uid); _hex(d->uid.atqa,2,atqa);
    char buf[BSIZ];
    snprintf(buf,BSIZ,"{\"event\":\"card_detected\",\"data\":{\"uid\":\"%s\",\"atqa\":\"%s\",\"sak\":%d,\"type\":\"%s\",\"sectors\":%d}}",
        uid, atqa, d->uid.sak, d->cardType==4?"MIFARE_4K":"MIFARE_1K", d->totalSectors);
    _emit(buf);
}
void reporter_attackStart(const char* atk, uint8_t s, bool isA) {
    char buf[BSIZ];
    snprintf(buf,BSIZ,"{\"event\":\"attack_start\",\"data\":{\"attack\":\"%s\",\"sector\":%d,\"key_type\":\"%s\"}}",atk,s,isA?"A":"B");
    _emit(buf);
}
void reporter_keyFound(uint8_t s, bool isA, const uint8_t* k, const char* m) {
    char key[13]; _hex(k,KEY_SIZE,key); char buf[BSIZ];
    snprintf(buf,BSIZ,"{\"event\":\"key_found\",\"data\":{\"sector\":%d,\"key_type\":\"%s\",\"key\":\"%s\",\"method\":\"%s\"}}",s,isA?"A":"B",key,m);
    _emit(buf);
}
void reporter_keyFailed(uint8_t s, bool isA) {
    char buf[BSIZ];
    snprintf(buf,BSIZ,"{\"event\":\"key_failed\",\"data\":{\"sector\":%d,\"key_type\":\"%s\"}}",s,isA?"A":"B");
    _emit(buf);
}
void reporter_dataRead(uint8_t s, uint8_t b, const uint8_t* d) {
    char hex[33]; _hex(d,16,hex); char buf[BSIZ];
    snprintf(buf,BSIZ,"{\"event\":\"data_read\",\"data\":{\"sector\":%d,\"block\":%d,\"hex\":\"%s\"}}",s,b,hex);
    _emit(buf);
}
void reporter_summary(const CardDump* d, uint32_t ms) {
    uint8_t c=ks_countCracked(d); char buf[BSIZ];
    snprintf(buf,BSIZ,"{\"event\":\"attack_summary\",\"data\":{\"total_sectors\":%d,\"cracked\":%d,\"failed\":%d,\"elapsed_ms\":%lu}}",
        d->totalSectors,c,d->totalSectors-c,(unsigned long)ms);
    _emit(buf);
}
void reporter_dumpSaved(const char* p) {
    char buf[BSIZ]; snprintf(buf,BSIZ,"{\"event\":\"dump_saved\",\"data\":{\"path\":\"%s\"}}",p); _emit(buf);
}
void reporter_error(const char* code, const char* msg) {
    char buf[BSIZ]; snprintf(buf,BSIZ,"{\"event\":\"error\",\"data\":{\"code\":\"%s\",\"msg\":\"%s\"}}",code,msg); _emit(buf);
}
