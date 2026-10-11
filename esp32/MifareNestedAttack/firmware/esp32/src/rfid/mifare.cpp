#include "mifare.h"

#include "config.h"

#include <string.h>

MifareHelper::MifareHelper(RfidReader& reader) : reader_(reader) {}

uint8_t MifareHelper::sectorOfBlock(uint8_t block) {
  return block / 4;
}

uint8_t MifareHelper::firstBlockOfSector(uint8_t sector) {
  return sector * 4;
}

uint8_t MifareHelper::trailerBlockOfSector(uint8_t sector) {
  return sector * 4 + 3;
}

bool MifareHelper::isClassic1K(const CardIdentity& id) {
  return id.sak == 0x08 || strstr(id.typeName, "1K") != nullptr;
}

void MifareHelper::printCardInfo(const CardIdentity& id) {
  Serial.println(F("Card Information"));
  Serial.println(F("---------------------------"));
  Serial.printf("UID        : %s\n", id.uidHex);
  Serial.printf("ATQA       : %02X %02X\n", id.atqa & 0xFF, (id.atqa >> 8) & 0xFF);
  Serial.printf("SAK        : %02X\n", id.sak);
  Serial.printf("Card Type  : %s\n", id.typeName);
  if (isClassic1K(id)) {
    Serial.println(F("Memory     : 1024 bytes"));
    Serial.printf("Sectors    : %u\n", MIFARE_1K_SECTORS);
    Serial.printf("Blocks     : %u\n", MIFARE_1K_BLOCKS);
    Serial.printf("Block size : %u bytes\n", MIFARE_BLOCK_SIZE);
  } else {
    Serial.println(F("Memory     : (see card type)"));
  }
}

void MifareHelper::printSectorMap(uint8_t sector) {
  if (sector >= MIFARE_1K_SECTORS) {
    Serial.println(F("Invalid sector"));
    return;
  }
  const uint8_t base = firstBlockOfSector(sector);
  Serial.printf("Sector %02u\n", sector);
  Serial.println(F("────────────────────────────────"));
  for (uint8_t i = 0; i < 4; ++i) {
    const uint8_t block = base + i;
    Serial.printf("\nBlock %02u\n", block);
    if (i < 3) {
      Serial.println(F("DATA"));
    } else {
      Serial.println(F("SECTOR TRAILER"));
      Serial.println(F("Key A       : [REDACTED]"));
      Serial.println(F("Access Bits : (read trailer after auth)"));
      Serial.println(F("Key B       : [REDACTED]"));
    }
  }
}

bool MifareHelper::readBlock(uint8_t block, uint8_t out[16]) {
  byte buffer[18];
  byte size = sizeof(buffer);
  MFRC522::StatusCode st = reader_.picc().MIFARE_Read(block, buffer, &size);
  if (st != MFRC522::STATUS_OK) {
    Serial.printf("Read failed: %s\n", reader_.picc().GetStatusCodeName(st));
    return false;
  }
  memcpy(out, buffer, 16);
  return true;
}

void MifareHelper::printBlockHex(const uint8_t data[16]) {
  for (int i = 0; i < 16; ++i) {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();
}
