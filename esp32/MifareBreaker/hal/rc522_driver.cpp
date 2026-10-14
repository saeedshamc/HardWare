#include "rc522_driver.h"

static inline void _cs_lo() { digitalWrite(PIN_SS, LOW); }
static inline void _cs_hi() { digitalWrite(PIN_SS, HIGH); }

uint8_t rc522_readReg(uint8_t reg) {
    _cs_lo();
    SPI.transfer(0x80 | (reg << 1));
    uint8_t v = SPI.transfer(0x00);
    _cs_hi();
    return v;
}
void rc522_writeReg(uint8_t reg, uint8_t val) {
    _cs_lo();
    SPI.transfer((reg << 1) & 0xFE);
    SPI.transfer(val);
    _cs_hi();
}
void rc522_setBits(uint8_t reg, uint8_t mask)   { rc522_writeReg(reg, rc522_readReg(reg) | mask); }
void rc522_clearBits(uint8_t reg, uint8_t mask) { rc522_writeReg(reg, rc522_readReg(reg) & ~mask); }

bool rc522_init(void) {
    pinMode(PIN_SS, OUTPUT); pinMode(PIN_RST, OUTPUT); _cs_hi();
    digitalWrite(PIN_RST, LOW); delayMicroseconds(2);
    digitalWrite(PIN_RST, HIGH); delay(50);
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SS);
    SPI.beginTransaction(SPISettings(SPI_FREQ, MSBFIRST, SPI_MODE0));
    rc522_reset();
    // Timer: TAuto=1, TPreScaler=169 → ~40kHz → 25µs/tick, TReload=1000 → 25ms timeout
    rc522_writeReg(REG_T_MODE,      0x80);
    rc522_writeReg(REG_T_PRESCALER, 0xA9);
    rc522_writeReg(REG_T_RELOAD_H,  0x03);
    rc522_writeReg(REG_T_RELOAD_L,  0xE8);
    rc522_writeReg(REG_TX_ASK, 0x40);  // Force100ASK
    rc522_writeReg(REG_MODE,   0x3D);  // CRCPreset=6363h
    rc522_antennaOn();
    uint8_t v = rc522_version();
    return (v == 0x91 || v == 0x92);
}
void rc522_reset(void) {
    rc522_writeReg(REG_COMMAND, CMD_SOFT_RESET); delay(50);
    uint32_t t = millis();
    while ((rc522_readReg(REG_COMMAND) & 0x10) && (millis()-t < 1000)) delay(1);
}
uint8_t rc522_version(void) { return rc522_readReg(REG_VERSION); }
void rc522_antennaOn(void) {
    if ((rc522_readReg(REG_TX_CONTROL) & 0x03) != 0x03)
        rc522_setBits(REG_TX_CONTROL, 0x03);
}
void rc522_antennaOff(void) { rc522_clearBits(REG_TX_CONTROL, 0x03); }

RC522Status rc522_calcCRC(const uint8_t* data, uint8_t len, uint8_t* out) {
    rc522_writeReg(REG_COMMAND, CMD_IDLE);
    rc522_writeReg(REG_DIV_IRQ, 0x04);
    rc522_setBits(REG_FIFO_LEVEL, 0x80);
    for (uint8_t i = 0; i < len; i++) rc522_writeReg(REG_FIFO_DATA, data[i]);
    rc522_writeReg(REG_COMMAND, CMD_CALC_CRC);
    uint32_t dl = millis() + 89;
    while (millis() < dl) {
        if (rc522_readReg(REG_DIV_IRQ) & 0x04) {
            rc522_writeReg(REG_COMMAND, CMD_IDLE);
            out[0] = rc522_readReg(REG_CRC_RESULT_L);
            out[1] = rc522_readReg(REG_CRC_RESULT_H);
            return RC522_OK;
        }
    }
    return RC522_ERR_TIMEOUT;
}

RC522Status rc522_transceive(const uint8_t* txBuf, uint8_t txLen,
    RC522Frame* rxFrame, uint8_t txLastBits, uint8_t rxAlign, bool checkCRC)
{
    rc522_writeReg(REG_COMM_I_EN, 0x77 | 0x80);
    rc522_clearBits(REG_COMM_IRQ, 0x80);
    rc522_setBits(REG_FIFO_LEVEL, 0x80);
    rc522_writeReg(REG_COMMAND, CMD_IDLE);
    for (uint8_t i = 0; i < txLen; i++) rc522_writeReg(REG_FIFO_DATA, txBuf[i]);
    rc522_writeReg(REG_BIT_FRAMING, (rxAlign << 4) | txLastBits);
    rc522_writeReg(REG_COMMAND, CMD_TRANSCEIVE);
    rc522_setBits(REG_BIT_FRAMING, 0x80);

    bool ok = false;
    uint32_t dl = millis() + 36;
    while (millis() < dl) {
        uint8_t irq = rc522_readReg(REG_COMM_IRQ);
        if (irq & 0x30) { ok = true; break; }
        if (irq & 0x01) break;
    }
    rc522_clearBits(REG_BIT_FRAMING, 0x80);
    if (!ok) return RC522_ERR_TIMEOUT;

    uint8_t err = rc522_readReg(REG_ERROR);
    if (err & 0x10) return RC522_ERR_OVERFLOW;
    if (err & 0x08) return RC522_ERR_COLLISION;
    if (err & 0x02) return RC522_ERR_PARITY;
    if (err & 0x01) return RC522_ERR_UNKNOWN;

    uint8_t n = rc522_readReg(REG_FIFO_LEVEL);
    uint8_t lb = rc522_readReg(REG_CONTROL) & 0x07;
    if (rxFrame) {
        rxFrame->len      = n > sizeof(rxFrame->data) ? sizeof(rxFrame->data) : n;
        rxFrame->lastBits = lb;
        for (uint8_t i = 0; i < rxFrame->len; i++)
            rxFrame->data[i] = rc522_readReg(REG_FIFO_DATA);
        if (checkCRC && rxFrame->len >= 2) {
            uint8_t crc[2];
            RC522Status cs = rc522_calcCRC(rxFrame->data, rxFrame->len-2, crc);
            if (cs != RC522_OK) return cs;
            if (crc[0] != rxFrame->data[rxFrame->len-2] ||
                crc[1] != rxFrame->data[rxFrame->len-1]) return RC522_ERR_CRC;
        }
    }
    return RC522_OK;
}
