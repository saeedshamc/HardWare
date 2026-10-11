#pragma once

#include <Arduino.h>
#include <MFRC522.h>
#include <stdint.h>

struct RfidScanResult {
  bool ok;
  uint8_t uid[10];
  uint8_t uidLen;
  uint32_t user_id;
  uint32_t counter;      // value AFTER increment on card
  char uidHex[21];
};

class RfidReader {
 public:
  RfidReader();

  void begin();
  /** Poll for a new card; returns true when a full read+counter bump succeeded. */
  bool pollScan(RfidScanResult* out);

  /**
   * Enrollment: default key FFFFFFFFFFFF → write diversified keys + data block.
   * Honors ENROLL_DRY_RUN from config.h.
   */
  bool enroll(uint32_t user_id, RfidScanResult* out);

  void setMasterKey(const uint8_t* key, size_t len);

 private:
  MFRC522 mfrc_;
  MFRC522::MIFARE_Key defaultKey_;
  uint8_t master_[64];
  size_t masterLen_;

  void releaseCard();
  bool readUid(RfidScanResult* out);
  bool authWithKey(byte command, const uint8_t key[6], byte blockAddr);
  bool writeTrailerSafe(const uint8_t keyA[6], const uint8_t keyB[6]);
  static void uidToHex(const uint8_t* uid, uint8_t len, char* out, size_t outSz);
};
