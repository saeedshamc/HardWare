#include "uid_reader.h"
#include "hal/card_comm.h"

bool uid_detect(CardDump* dump) {
    uint8_t atqa[2];
    if (picc_requestA(atqa) != RC522_OK) return false;
    PiccUID uid;
    if (picc_anticollision(&uid) != RC522_OK) return false;
    dump->uid = uid;
    dump->uid.atqa[0] = atqa[0]; dump->uid.atqa[1] = atqa[1];
    uint8_t sak = uid.sak & ~0x02;
    switch (sak) {
        case 0x09: dump->cardType=1; dump->totalSectors=5;  break;
        case 0x08: dump->cardType=1; dump->totalSectors=16; break;
        case 0x18: dump->cardType=4; dump->totalSectors=40; break;
        case 0x28: dump->cardType=1; dump->totalSectors=16; break;
        default:   dump->cardType=1; dump->totalSectors=16; break;
    }
    return true;
}
