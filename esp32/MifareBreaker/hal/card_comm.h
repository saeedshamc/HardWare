#pragma once
#include "rc522_driver.h"
#define UID_MAX_LEN 10
typedef struct {
    uint8_t bytes[UID_MAX_LEN];
    uint8_t len;
    uint8_t sak;
    uint8_t atqa[2];
} PiccUID;
RC522Status picc_requestA(uint8_t* atqaOut);
RC522Status picc_wakeupA(uint8_t* atqaOut);
RC522Status picc_anticollision(PiccUID* uid);
RC522Status picc_halt(void);
bool        picc_isPresent(void);
RC522Status picc_mfAuth(uint8_t cmd, uint8_t blockAddr, const uint8_t* key6, const PiccUID* uid);
RC522Status picc_mfRead(uint8_t blockAddr, uint8_t* out16);
void        picc_mfDeauth(void);
