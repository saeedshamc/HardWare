#include "card_comm.h"
#include <string.h>

static RC522Status _sendRequest(uint8_t cmd, uint8_t* out) {
    rc522_clearBits(REG_COLL, 0x80);
    RC522Frame rx;
    RC522Status s = rc522_transceive(&cmd, 1, &rx, 7, 0, false);
    if (s != RC522_OK || rx.len != 2) return s == RC522_OK ? RC522_ERR_UNKNOWN : s;
    if (out) { out[0] = rx.data[0]; out[1] = rx.data[1]; }
    return RC522_OK;
}
RC522Status picc_requestA(uint8_t* o) { return _sendRequest(PICC_REQA, o); }
RC522Status picc_wakeupA(uint8_t* o)  { return _sendRequest(PICC_WUPA, o); }

RC522Status picc_anticollision(PiccUID* uid) {
    uint8_t sel = PICC_SEL_CL1, idx = 0;
    memset(uid->bytes, 0, UID_MAX_LEN); uid->len = 0;
    rc522_clearBits(REG_COLL, 0x80);
    for (int cas = 0; cas < 3; cas++) {
        uint8_t ac[2] = {sel, PICC_SEL_NVB_AC};
        RC522Frame rx;
        RC522Status s = rc522_transceive(ac, 2, &rx, 0, 0, false);
        if (s != RC522_OK || rx.len != 5) return s == RC522_OK ? RC522_ERR_UNKNOWN : s;
        if ((rx.data[0]^rx.data[1]^rx.data[2]^rx.data[3]) != rx.data[4]) return RC522_ERR_PARITY;
        uint8_t sb[9]; sb[0]=sel; sb[1]=PICC_SEL_NVB_SE;
        memcpy(&sb[2], rx.data, 5);
        uint8_t crc[2]; rc522_calcCRC(sb, 7, crc); sb[7]=crc[0]; sb[8]=crc[1];
        RC522Frame rs;
        s = rc522_transceive(sb, 9, &rs, 0, 0, true);
        if (s != RC522_OK || rs.len < 3) return s == RC522_OK ? RC522_ERR_UNKNOWN : s;
        uid->sak = rs.data[0];
        bool more = (rx.data[0] == PICC_CT);
        if (more) { memcpy(&uid->bytes[idx], &rx.data[1], 3); idx+=3; sel = sel==PICC_SEL_CL1?PICC_SEL_CL2:PICC_SEL_CL3; }
        else       { memcpy(&uid->bytes[idx], rx.data, 4);    idx+=4; uid->len=idx; return RC522_OK; }
    }
    return RC522_ERR_UNKNOWN;
}
RC522Status picc_halt(void) {
    uint8_t b[4]; b[0]=PICC_HLTA; b[1]=0;
    uint8_t crc[2]; rc522_calcCRC(b,2,crc); b[2]=crc[0]; b[3]=crc[1];
    RC522Status s = rc522_transceive(b,4,nullptr,0,0,false);
    return s==RC522_ERR_TIMEOUT ? RC522_OK : RC522_ERR_UNKNOWN;
}
bool picc_isPresent(void) { uint8_t a[2]; return picc_requestA(a)==RC522_OK; }

RC522Status picc_mfAuth(uint8_t cmd, uint8_t blk, const uint8_t* key, const PiccUID* uid) {
    uint8_t b[12]; b[0]=cmd; b[1]=blk;
    memcpy(&b[2], key, 6); memcpy(&b[8], uid->bytes, 4);
    rc522_writeReg(REG_COMM_I_EN, 0x77|0x80);
    rc522_clearBits(REG_COMM_IRQ, 0x80);
    rc522_setBits(REG_FIFO_LEVEL, 0x80);
    rc522_writeReg(REG_COMMAND, CMD_IDLE);
    for (int i=0;i<12;i++) rc522_writeReg(REG_FIFO_DATA,b[i]);
    rc522_writeReg(REG_COMMAND, CMD_MF_AUTH);
    uint32_t dl = millis()+100; bool done=false;
    while(millis()<dl){ uint8_t irq=rc522_readReg(REG_COMM_IRQ); if(irq&0x10){done=true;break;} if(irq&0x01)break; }
    if(!done||!(rc522_readReg(REG_STATUS2)&0x08)){ picc_mfDeauth(); return RC522_ERR_AUTH; }
    return RC522_OK;
}
RC522Status picc_mfRead(uint8_t blk, uint8_t* out) {
    uint8_t cmd[4]; cmd[0]=MF_READ; cmd[1]=blk;
    uint8_t crc[2]; rc522_calcCRC(cmd,2,crc); cmd[2]=crc[0]; cmd[3]=crc[1];
    RC522Frame rx;
    RC522Status s = rc522_transceive(cmd,4,&rx,0,0,true);
    if(s!=RC522_OK||rx.len<16) return s==RC522_OK?RC522_ERR_UNKNOWN:s;
    memcpy(out,rx.data,16); return RC522_OK;
}
void picc_mfDeauth(void) { rc522_clearBits(REG_STATUS2, 0x08); }
