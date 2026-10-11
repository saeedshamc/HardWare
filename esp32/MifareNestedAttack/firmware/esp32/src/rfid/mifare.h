#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "rfid_reader.h"

class MifareHelper {
 public:
  explicit MifareHelper(RfidReader& reader);

  static uint8_t sectorOfBlock(uint8_t block);
  static uint8_t firstBlockOfSector(uint8_t sector);
  static uint8_t trailerBlockOfSector(uint8_t sector);
  static bool isClassic1K(const CardIdentity& id);

  void printCardInfo(const CardIdentity& id);
  void printSectorMap(uint8_t sector);
  bool readBlock(uint8_t block, uint8_t out[16]);
  void printBlockHex(const uint8_t data[16]);

 private:
  RfidReader& reader_;
};
