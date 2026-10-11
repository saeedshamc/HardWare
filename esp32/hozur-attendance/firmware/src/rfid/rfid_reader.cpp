#include "rfid_reader.h"

#include "config.h"
#include "../security/access_bits.h"
#include "../security/card_block.h"
#include "../security/hmac_keys.h"

#include <SPI.h>
#include <string.h>

RfidReader::RfidReader()
    : mfrc_(PIN_RC522_SS, PIN_RC522_RST), masterLen_(0) {
  memset(master_, 0, sizeof(master_));
  for (byte i = 0; i < 6; ++i) {
    defaultKey_.keyByte[i] = 0xFF;
  }
}

void RfidReader::begin() {
  SPI.begin(PIN_RC522_SCK, PIN_RC522_MISO, PIN_RC522_MOSI, PIN_RC522_SS);
  mfrc_.PCD_Init();
  delay(50);
  Serial.print(F("[RFID] "));
  mfrc_.PCD_DumpVersionToSerial();
}

void RfidReader::setMasterKey(const uint8_t* key, size_t len) {
  if (!key || len == 0) {
    masterLen_ = 0;
    return;
  }
  if (len > sizeof(master_)) {
    len = sizeof(master_);
  }
  memcpy(master_, key, len);
  masterLen_ = len;
}

void RfidReader::uidToHex(const uint8_t* uid, uint8_t len, char* out, size_t outSz) {
  if (!out || outSz < 3) {
    return;
  }
  size_t pos = 0;
  for (uint8_t i = 0; i < len && pos + 2 < outSz; ++i) {
    snprintf(out + pos, outSz - pos, "%02X", uid[i]);
    pos += 2;
  }
  out[pos] = '\0';
}

void RfidReader::releaseCard() {
  mfrc_.PICC_HaltA();
  mfrc_.PCD_StopCrypto1();
}

bool RfidReader::readUid(RfidScanResult* out) {
  if (!out) return false;
  memset(out, 0, sizeof(*out));

  if (!mfrc_.PICC_IsNewCardPresent() || !mfrc_.PICC_ReadCardSerial()) {
    return false;
  }

  out->uidLen = mfrc_.uid.size;
  if (out->uidLen > sizeof(out->uid)) {
    out->uidLen = sizeof(out->uid);
  }
  memcpy(out->uid, mfrc_.uid.uidByte, out->uidLen);
  uidToHex(out->uid, out->uidLen, out->uidHex, sizeof(out->uidHex));
  return true;
}

bool RfidReader::authWithKey(byte command, const uint8_t key[6], byte blockAddr) {
  MFRC522::MIFARE_Key k;
  memcpy(k.keyByte, key, 6);
  MFRC522::StatusCode st =
      mfrc_.PCD_Authenticate(command, blockAddr, &k, &(mfrc_.uid));
  return st == MFRC522::STATUS_OK;
}

bool RfidReader::writeTrailerSafe(const uint8_t keyA[6], const uint8_t keyB[6]) {
  uint8_t access[3];
  if (!access_bits_hozour_default(access)) {
    Serial.println(F("[RFID] Access bits encode failed"));
    return false;
  }
  if (!access_bits_validate(access)) {
    Serial.println(F("[RFID] Access bits validation failed — abort write"));
    return false;
  }

  uint8_t trailer[16];
  memcpy(trailer + 0, keyA, 6);
  memcpy(trailer + 6, access, 3);
  trailer[9] = 0x00;  // GPB
  memcpy(trailer + 10, keyB, 6);

  Serial.printf("[RFID] Trailer access bytes: %02X %02X %02X\n",
                access[0], access[1], access[2]);

#if ENROLL_DRY_RUN
  Serial.println(F("[RFID] ENROLL_DRY_RUN=1 — skipping trailer write"));
  return true;
#else
  MFRC522::StatusCode st =
      mfrc_.MIFARE_Write(CARD_TRAILER_BLOCK, trailer, 16);
  if (st != MFRC522::STATUS_OK) {
    Serial.printf("[RFID] Trailer write failed: %s\n",
                  mfrc_.GetStatusCodeName(st));
    return false;
  }
  return true;
#endif
}

bool RfidReader::enroll(uint32_t user_id, RfidScanResult* out) {
  if (masterLen_ == 0) {
    Serial.println(F("[RFID] MASTER_KEY not set"));
    return false;
  }
  if (!readUid(out)) {
    return false;
  }

  Serial.printf("[RFID] Enroll UID=%s user_id=%lu dry_run=%d\n",
                out->uidHex, (unsigned long)user_id, ENROLL_DRY_RUN);

  // Authenticate with factory default KeyA
  if (!authWithKey(MFRC522::PICC_CMD_MF_AUTH_KEY_A, defaultKey_.keyByte,
                   CARD_TRAILER_BLOCK)) {
    Serial.println(F("[RFID] Default-key auth failed (card already enrolled?)"));
    releaseCard();
    return false;
  }

  uint8_t keyA[6], keyB[6];
  if (!derive_key_a(master_, masterLen_, out->uid, out->uidLen, CARD_SECTOR, keyA) ||
      !derive_key_b(master_, masterLen_, out->uid, out->uidLen, CARD_SECTOR, keyB)) {
    Serial.println(F("[RFID] Key derivation failed"));
    releaseCard();
    return false;
  }

  if (!writeTrailerSafe(keyA, keyB)) {
    releaseCard();
    return false;
  }

  uint8_t block[16];
  card_block_pack(user_id, 1, block);

#if ENROLL_DRY_RUN
  Serial.println(F("[RFID] ENROLL_DRY_RUN=1 — skipping data block write"));
#else
  // After trailer change, re-select and auth with new KeyB to write data.
  releaseCard();
  delay(50);
  if (!mfrc_.PICC_IsNewCardPresent() || !mfrc_.PICC_ReadCardSerial()) {
    // Card may still be present; try wake
    byte atqa[2];
    mfrc_.PICC_WakeupA(atqa, sizeof(atqa));
    if (!mfrc_.PICC_ReadCardSerial()) {
      Serial.println(F("[RFID] Card lost after trailer write"));
      return false;
    }
  }
  memcpy(out->uid, mfrc_.uid.uidByte, mfrc_.uid.size);
  out->uidLen = mfrc_.uid.size;
  uidToHex(out->uid, out->uidLen, out->uidHex, sizeof(out->uidHex));

  if (!authWithKey(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keyB, CARD_DATA_BLOCK)) {
    Serial.println(F("[RFID] KeyB auth for data write failed"));
    releaseCard();
    return false;
  }

  MFRC522::StatusCode st = mfrc_.MIFARE_Write(CARD_DATA_BLOCK, block, 16);
  if (st != MFRC522::STATUS_OK) {
    Serial.printf("[RFID] Data write failed: %s\n", mfrc_.GetStatusCodeName(st));
    releaseCard();
    return false;
  }
#endif

  out->user_id = user_id;
  out->counter = 1;
  out->ok = true;
  releaseCard();
  Serial.println(F("[RFID] Enroll OK — report UID+user_id to server"));
  return true;
}

bool RfidReader::pollScan(RfidScanResult* out) {
  if (masterLen_ == 0) {
    return false;
  }
  if (!readUid(out)) {
    return false;
  }

  uint8_t keyA[6];
  if (!derive_key_a(master_, masterLen_, out->uid, out->uidLen, CARD_SECTOR, keyA)) {
    releaseCard();
    return false;
  }

  if (!authWithKey(MFRC522::PICC_CMD_MF_AUTH_KEY_A, keyA, CARD_DATA_BLOCK)) {
    Serial.printf("[RFID] Auth failed for UID=%s\n", out->uidHex);
    releaseCard();
    return false;
  }

  uint8_t block[18];
  byte size = sizeof(block);
  MFRC522::StatusCode st = mfrc_.MIFARE_Read(CARD_DATA_BLOCK, block, &size);
  if (st != MFRC522::STATUS_OK) {
    Serial.printf("[RFID] Read failed: %s\n", mfrc_.GetStatusCodeName(st));
    releaseCard();
    return false;
  }

  CardPayload payload;
  if (!card_block_unpack(block, &payload)) {
    Serial.println(F("[RFID] CRC mismatch"));
    releaseCard();
    return false;
  }

  const uint32_t nextCounter = payload.counter + 1;
  uint8_t newBlock[16];
  card_block_pack(payload.user_id, nextCounter, newBlock);

  // Need KeyB to write under default Hozur access bits
  uint8_t keyB[6];
  if (!derive_key_b(master_, masterLen_, out->uid, out->uidLen, CARD_SECTOR, keyB)) {
    releaseCard();
    return false;
  }

  // Re-auth with KeyB for write (crypto session was KeyA)
  mfrc_.PCD_StopCrypto1();
  if (!authWithKey(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keyB, CARD_DATA_BLOCK)) {
    Serial.println(F("[RFID] KeyB auth for counter write failed"));
    releaseCard();
    return false;
  }

  st = mfrc_.MIFARE_Write(CARD_DATA_BLOCK, newBlock, 16);
  if (st != MFRC522::STATUS_OK) {
    Serial.printf("[RFID] Counter write failed: %s\n", mfrc_.GetStatusCodeName(st));
    releaseCard();
    return false;
  }

  out->user_id = payload.user_id;
  out->counter = nextCounter;
  out->ok = true;
  releaseCard();
  return true;
}
