#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "config.h"

// SPI address byte: [7:RnW | 6:A5..1:A0 | 0:0]
// Read  → 0x80 | (reg << 1)
// Write → (reg << 1)

#define REG_COMMAND         0x01
#define REG_COMM_I_EN       0x02
#define REG_DIV_I_EN        0x03
#define REG_COMM_IRQ        0x04
#define REG_DIV_IRQ         0x05
#define REG_ERROR           0x06
#define REG_STATUS1         0x07
#define REG_STATUS2         0x08
#define REG_FIFO_DATA       0x09
#define REG_FIFO_LEVEL      0x0A
#define REG_WATER_LEVEL     0x0B
#define REG_CONTROL         0x0C
#define REG_BIT_FRAMING     0x0D
#define REG_COLL            0x0E
#define REG_MODE            0x11
#define REG_TX_MODE         0x12
#define REG_RX_MODE         0x13
#define REG_TX_CONTROL      0x14
#define REG_TX_ASK          0x15
#define REG_RX_SEL          0x17
#define REG_RX_THRESHOLD    0x18
#define REG_DEMOD           0x19
#define REG_CRC_RESULT_H    0x21
#define REG_CRC_RESULT_L    0x22
#define REG_MOD_WIDTH       0x24
#define REG_RF_CFG          0x26
#define REG_T_MODE          0x2A
#define REG_T_PRESCALER     0x2B
#define REG_T_RELOAD_H      0x2C
#define REG_T_RELOAD_L      0x2D
#define REG_T_COUNTER_H     0x2E
#define REG_T_COUNTER_L     0x2F
#define REG_VERSION         0x37

#define CMD_IDLE            0x00
#define CMD_MEM             0x01
#define CMD_CALC_CRC        0x03
#define CMD_TRANSMIT        0x04
#define CMD_RECEIVE         0x08
#define CMD_TRANSCEIVE      0x0C
#define CMD_MF_AUTH         0x0E
#define CMD_SOFT_RESET      0x0F

#define PICC_REQA           0x26
#define PICC_WUPA           0x52
#define PICC_CT             0x88
#define PICC_SEL_CL1        0x93
#define PICC_SEL_CL2        0x95
#define PICC_SEL_CL3        0x97
#define PICC_SEL_NVB_AC     0x20
#define PICC_SEL_NVB_SE     0x70
#define PICC_HLTA           0x50
#define MF_AUTH_KEY_A       0x60
#define MF_AUTH_KEY_B       0x61
#define MF_READ             0x30
#define MF_WRITE            0xA0

typedef enum {
    RC522_OK = 0, RC522_ERR_NOTAG, RC522_ERR_PARITY, RC522_ERR_CRC,
    RC522_ERR_COLLISION, RC522_ERR_TIMEOUT, RC522_ERR_OVERFLOW,
    RC522_ERR_NAK, RC522_ERR_AUTH, RC522_ERR_UNKNOWN
} RC522Status;

typedef struct {
    uint8_t data[64];
    uint8_t len;
    uint8_t lastBits;
} RC522Frame;

bool        rc522_init(void);
void        rc522_reset(void);
uint8_t     rc522_version(void);
uint8_t     rc522_readReg(uint8_t reg);
void        rc522_writeReg(uint8_t reg, uint8_t val);
void        rc522_setBits(uint8_t reg, uint8_t mask);
void        rc522_clearBits(uint8_t reg, uint8_t mask);
void        rc522_antennaOn(void);
void        rc522_antennaOff(void);
RC522Status rc522_transceive(const uint8_t* txBuf, uint8_t txLen,
    RC522Frame* rxFrame, uint8_t txLastBits, uint8_t rxAlign, bool checkCRC);
RC522Status rc522_calcCRC(const uint8_t* data, uint8_t len, uint8_t* outCRC2);
