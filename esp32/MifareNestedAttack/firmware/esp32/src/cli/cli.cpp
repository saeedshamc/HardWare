#include "cli.h"

#include "config.h"
#include "../security/access_bits.h"
#include "../security/crypto_demo.h"
#include "../security/nested_sim.h"
#include "../security/defensive.h"

#include <string.h>

LabCli::LabCli(RfidReader& reader, MifareHelper& mifare, AuthenticationDemo& auth)
    : reader_(reader), mifare_(mifare), auth_(auth) {}

void LabCli::begin() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("      MIFARE SECURITY LAB"));
  Serial.println(F("========================================"));
  Serial.println();
  Serial.println(F("Educational use only — own/test cards."));
  Serial.println(F("Waiting for commands..."));
  printMenu();
}

void LabCli::printMenu() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" MIFARE CLASSIC SECURITY LAB"));
  Serial.println(F("========================================"));
  Serial.println(F("1. Detect Card"));
  Serial.println(F("2. Card Information"));
  Serial.println(F("3. Authenticate"));
  Serial.println(F("4. Read Authorized Block"));
  Serial.println(F("5. Sector Analyzer"));
  Serial.println(F("6. Access Bits Decoder"));
  Serial.println(F("7. Crypto1 Demonstration"));
  Serial.println(F("8. Nested Attack Simulator"));
  Serial.println(F("9. Defensive Analysis"));
  Serial.println(F("0. Exit (reprint menu)"));
  Serial.print(F("\nSelect: "));
}

int LabCli::readChoice() {
  while (!Serial.available()) {
    delay(10);
  }
  const int v = Serial.parseInt();
  while (Serial.available()) {
    Serial.read();
  }
  return v;
}

bool LabCli::ensureCard(bool withAtqa) {
  Serial.println(F("\nPlace a MIFARE Classic *test* card on the reader..."));
  const uint32_t start = millis();
  while (millis() - start < 15000) {
    bool ok = withAtqa ? reader_.selectKnown(&last_) : reader_.detect(&last_);
    if (ok) {
      Serial.println(F("[+] Card selected"));
      return true;
    }
    delay(50);
  }
  Serial.println(F("[-] No card detected (timeout)"));
  return false;
}

void LabCli::cmdDetect() {
  if (!ensureCard(true)) return;
  Serial.println(F("\n[+] Card detected\n"));
  Serial.printf("UID  : %s\n", last_.uidHex);
  Serial.printf("ATQA : %02X %02X\n", last_.atqa & 0xFF, (last_.atqa >> 8) & 0xFF);
  Serial.printf("SAK  : %02X\n", last_.sak);
  Serial.println(F("\nCard:"));
  Serial.println(last_.typeName);
  reader_.halt();
}

void LabCli::cmdCardInfo() {
  if (!ensureCard(true)) return;
  Serial.println();
  mifare_.printCardInfo(last_);
  reader_.halt();
}

void LabCli::cmdAuthenticate() {
  if (!ensureCard(true)) return;
  Serial.println(F("Sector number (0-15), default 1:"));
  int sector = readChoice();
  if (sector < 0 || sector > 15) sector = 1;
  Serial.println(F("Using laboratory default KeyA = FFFFFFFFFFFF"));
  Serial.println(F("(Only for dedicated test cards you own.)"));
  auth_.authenticateDefaultKeyA((uint8_t)sector);
  reader_.halt();
}

void LabCli::cmdReadBlock() {
  if (!ensureCard(true)) return;
  Serial.println(F("Sector to authenticate (0-15), default 1:"));
  int sector = readChoice();
  if (sector < 0 || sector > 15) sector = 1;
  if (!auth_.authenticateDefaultKeyA((uint8_t)sector)) {
    reader_.halt();
    return;
  }
  const uint8_t block = MifareHelper::firstBlockOfSector((uint8_t)sector);
  Serial.printf("\n[*] Reading block %u\n", block);
  uint8_t data[16];
  if (mifare_.readBlock(block, data)) {
    Serial.print(F("[+] Data: "));
    mifare_.printBlockHex(data);
  }
  reader_.halt();
}

void LabCli::cmdSectorAnalyzer() {
  Serial.println(F("Sector (0-15), default 1:"));
  int sector = readChoice();
  if (sector < 0 || sector > 15) sector = 1;

  Serial.println();
  mifare_.printSectorMap((uint8_t)sector);

  if (!ensureCard(true)) return;
  if (!auth_.authenticateDefaultKeyA((uint8_t)sector)) {
    reader_.halt();
    return;
  }

  const uint8_t trailer = MifareHelper::trailerBlockOfSector((uint8_t)sector);
  uint8_t data[16];
  if (mifare_.readBlock(trailer, data)) {
    Serial.println(F("\nTrailer bytes (keys redacted by default):"));
#if LAB_SHOW_KEYS
    Serial.print(F("Key A       : "));
    for (int i = 0; i < 6; ++i) Serial.printf("%02X ", data[i]);
    Serial.println();
#else
    Serial.println(F("Key A       : [REDACTED]"));
#endif
    Serial.printf("Access Bits : %02X %02X %02X  (GPB %02X)\n",
                  data[6], data[7], data[8], data[9]);
#if LAB_SHOW_KEYS
    Serial.print(F("Key B       : "));
    for (int i = 10; i < 16; ++i) Serial.printf("%02X ", data[i]);
    Serial.println();
#else
    Serial.println(F("Key B       : [REDACTED]"));
#endif
  }
  reader_.halt();
}

void LabCli::cmdAccessBits() {
  if (!ensureCard(true)) return;
  Serial.println(F("Sector (0-15), default 1:"));
  int sector = readChoice();
  if (sector < 0 || sector > 15) sector = 1;
  if (!auth_.authenticateDefaultKeyA((uint8_t)sector)) {
    reader_.halt();
    return;
  }
  uint8_t data[16];
  const uint8_t trailer = MifareHelper::trailerBlockOfSector((uint8_t)sector);
  if (!mifare_.readBlock(trailer, data)) {
    reader_.halt();
    return;
  }
  Serial.println();
  uint8_t access[3] = {data[6], data[7], data[8]};
  access_bits_print_decode(access);
  reader_.halt();
}

void LabCli::loop() {
  if (!Serial.available()) {
    return;
  }
  const int choice = readChoice();
  switch (choice) {
    case 1: cmdDetect(); break;
    case 2: cmdCardInfo(); break;
    case 3: cmdAuthenticate(); break;
    case 4: cmdReadBlock(); break;
    case 5: cmdSectorAnalyzer(); break;
    case 6: cmdAccessBits(); break;
    case 7: crypto1_run_demo(); break;
    case 8: nested_simulator_menu(); break;
    case 9: defensive_analysis_print(); break;
    case 0:
    default:
      break;
  }
  printMenu();
}
