#include "authentication.h"

#include "config.h"
#include "../rfid/mifare.h"

#include <string.h>

AuthenticationDemo::AuthenticationDemo(RfidReader& reader) : reader_(reader) {}

bool AuthenticationDemo::authenticate(uint8_t sector, KeyType keyType, const uint8_t key[6]) {
  const uint8_t block = MifareHelper::trailerBlockOfSector(sector);
  Serial.println();
  Serial.printf("[*] Authenticating Sector %u\n", sector);
  Serial.printf("[*] Block: %u\n", block);
  Serial.printf("[*] Authentication: %s\n", keyType == KeyType::KeyA ? "KEY_A" : "KEY_B");

  MFRC522::MIFARE_Key k;
  memcpy(k.keyByte, key, 6);
  const byte cmd = (keyType == KeyType::KeyA) ? MFRC522::PICC_CMD_MF_AUTH_KEY_A
                                              : MFRC522::PICC_CMD_MF_AUTH_KEY_B;

  MFRC522::StatusCode st =
      reader_.picc().PCD_Authenticate(cmd, block, &k, &(reader_.picc().uid));

  if (st == MFRC522::STATUS_OK) {
    Serial.println(F("\n[+] Authentication SUCCESS"));
    Serial.println(F("[*] Secure communication state established"));
    return true;
  }

  Serial.println(F("\n[-] Authentication FAILURE"));
  Serial.printf("    Status: %s\n", reader_.picc().GetStatusCodeName(st));
  return false;
}

bool AuthenticationDemo::authenticateDefaultKeyA(uint8_t sector) {
  return authenticate(sector, KeyType::KeyA, LAB_DEFAULT_KEY);
}
