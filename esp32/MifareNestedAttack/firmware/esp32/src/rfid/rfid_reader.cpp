#include "rfid_reader.h"

#include "config.h"

#include <SPI.h>
#include <string.h>

RfidReader::RfidReader() : mfrc_(RFID_SS_PIN, RFID_RST_PIN) {}

void RfidReader::begin() {
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  mfrc_.PCD_Init();
  delay(40);
  Serial.println(F("[+] Initializing SPI..."));
  Serial.println(F("[+] Initializing MFRC522..."));
  byte v = mfrc_.PCD_ReadRegister(mfrc_.VersionReg);
  if (v == 0x00 || v == 0xFF) {
    Serial.println(F("[!] Reader not detected — check wiring / 3.3V supply"));
  } else {
    Serial.println(F("[+] Reader detected."));
    mfrc_.PCD_DumpVersionToSerial();
  }
}

const char* RfidReader::describeType(MFRC522::PICC_Type t) {
  switch (t) {
    case MFRC522::PICC_TYPE_MIFARE_MINI: return "MIFARE Mini";
    case MFRC522::PICC_TYPE_MIFARE_1K: return "MIFARE Classic 1K";
    case MFRC522::PICC_TYPE_MIFARE_4K: return "MIFARE Classic 4K";
    case MFRC522::PICC_TYPE_MIFARE_UL: return "MIFARE Ultralight";
    case MFRC522::PICC_TYPE_MIFARE_PLUS: return "MIFARE Plus";
    case MFRC522::PICC_TYPE_MIFARE_DESFIRE: return "MIFARE DESFire";
    default: return "Unknown / other ISO14443-A";
  }
}

void RfidReader::fillIdentity(CardIdentity* out) {
  memset(out, 0, sizeof(*out));
  out->present = true;
  out->uidSize = mfrc_.uid.size;
  if (out->uidSize > sizeof(out->uid)) out->uidSize = sizeof(out->uid);
  memcpy(out->uid, mfrc_.uid.uidByte, out->uidSize);
  out->sak = mfrc_.uid.sak;
  // ATQA is not retained by all library versions after select; approximate from sak path
  out->atqa = 0;
  size_t pos = 0;
  for (uint8_t i = 0; i < out->uidSize && pos + 3 < sizeof(out->uidHex); ++i) {
    pos += snprintf(out->uidHex + pos, sizeof(out->uidHex) - pos, "%s%02X",
                    i ? " " : "", out->uid[i]);
  }
  out->typeName = describeType(mfrc_.PICC_GetType(out->sak));
}

bool RfidReader::detect(CardIdentity* out) {
  if (!out) return false;
  if (!mfrc_.PICC_IsNewCardPresent()) return false;
  if (!mfrc_.PICC_ReadCardSerial()) return false;
  fillIdentity(out);

  // Capture ATQA via Wakeup if possible after halt/reselect is hard; try ReqA on next pass.
  // Best-effort: store last ATQA from a dedicated ReqA in selectKnown.
  return true;
}

bool RfidReader::selectKnown(CardIdentity* out) {
  if (!out) return false;
  byte bufferATQA[2];
  byte bufferSize = sizeof(bufferATQA);
  MFRC522::StatusCode st = mfrc_.PICC_WakeupA(bufferATQA, &bufferSize);
  if (st != MFRC522::STATUS_OK) {
    st = mfrc_.PICC_RequestA(bufferATQA, &bufferSize);
  }
  if (st != MFRC522::STATUS_OK) return false;
  if (!mfrc_.PICC_ReadCardSerial()) return false;
  fillIdentity(out);
  out->atqa = (uint16_t)bufferATQA[0] | ((uint16_t)bufferATQA[1] << 8);
  return true;
}

void RfidReader::halt() {
  mfrc_.PICC_HaltA();
  mfrc_.PCD_StopCrypto1();
}
