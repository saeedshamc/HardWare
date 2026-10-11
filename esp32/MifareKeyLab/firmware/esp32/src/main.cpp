/**
 * MIFARE KeyLab — ESP32 + RC522 serial backend
 * Line protocol for host CLI (authorized test cards only).
 *
 * Commands:
 *   DETECT
 *   INFO
 *   AUTH <sector> <A|B> <12hex>
 *   READ <block>
 *   HALT
 */
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "config.h"

MFRC522 mfrc(RFID_SS_PIN, RFID_RST_PIN);

static void haltCrypto() {
  mfrc.PICC_HaltA();
  mfrc.PCD_StopCrypto1();
}

static bool parseKey(const String& hex, MFRC522::MIFARE_Key& key) {
  if (hex.length() != 12) return false;
  for (int i = 0; i < 6; ++i) {
    key.keyByte[i] = (uint8_t)strtol(hex.substring(i * 2, i * 2 + 2).c_str(), nullptr, 16);
  }
  return true;
}

static void cmdDetect() {
  if (!mfrc.PICC_IsNewCardPresent() || !mfrc.PICC_ReadCardSerial()) {
    Serial.println(F("ERR no card"));
    return;
  }
  Serial.println(F("OK detected"));
}

static void cmdInfo() {
  byte atqa[2];
  byte sz = sizeof(atqa);
  if (mfrc.PICC_WakeupA(atqa, &sz) != MFRC522::STATUS_OK &&
      mfrc.PICC_RequestA(atqa, &sz) != MFRC522::STATUS_OK) {
    Serial.println(F("ERR no card"));
    return;
  }
  if (!mfrc.PICC_ReadCardSerial()) {
    Serial.println(F("ERR select"));
    return;
  }
  Serial.print(F("UID:"));
  for (byte i = 0; i < mfrc.uid.size; i++) {
    Serial.printf(" %02X", mfrc.uid.uidByte[i]);
  }
  Serial.println();
  Serial.printf("ATQA: %02X %02X\n", atqa[0], atqa[1]);
  Serial.printf("SAK: %02X\n", mfrc.uid.sak);
  Serial.println(F("TYPE: MIFARE Classic 1K"));
  Serial.println(F("OK"));
  haltCrypto();
}

static void cmdAuth(int sector, char type, const String& keyHex) {
  MFRC522::MIFARE_Key key;
  if (!parseKey(keyHex, key)) {
    Serial.println(F("ERR bad key"));
    return;
  }
  if (!mfrc.PICC_IsNewCardPresent()) {
    // try wake
    byte atqa[2];
    byte sz = sizeof(atqa);
    mfrc.PICC_WakeupA(atqa, &sz);
  }
  if (!mfrc.PICC_ReadCardSerial()) {
    Serial.println(F("ERR no card"));
    return;
  }
  byte block = sector * 4 + 3;
  byte cmd = (type == 'B') ? MFRC522::PICC_CMD_MF_AUTH_KEY_B : MFRC522::PICC_CMD_MF_AUTH_KEY_A;
  auto st = mfrc.PCD_Authenticate(cmd, block, &key, &(mfrc.uid));
  if (st == MFRC522::STATUS_OK) {
    Serial.println(F("OK auth"));
  } else {
    Serial.println(F("FAIL auth"));
  }
  haltCrypto();
}

static void cmdRead(int block) {
  byte buffer[18];
  byte size = sizeof(buffer);
  auto st = mfrc.MIFARE_Read(block, buffer, &size);
  if (st != MFRC522::STATUS_OK) {
    Serial.println(F("ERR read"));
    return;
  }
  Serial.print(F("DATA:"));
  for (int i = 0; i < 16; ++i) Serial.printf("%02X", buffer[i]);
  Serial.println();
  Serial.println(F("OK"));
}

void setup() {
  Serial.begin(115200);
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  mfrc.PCD_Init();
  Serial.println(F("KEYLAB_READY"));
}

void loop() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  if (line.equalsIgnoreCase("DETECT")) {
    cmdDetect();
  } else if (line.equalsIgnoreCase("INFO")) {
    cmdInfo();
  } else if (line.equalsIgnoreCase("HALT")) {
    haltCrypto();
    Serial.println(F("OK"));
  } else if (line.startsWith("AUTH ") || line.startsWith("auth ")) {
    // AUTH sector A|B KEY
    int sp1 = line.indexOf(' ');
    int sp2 = line.indexOf(' ', sp1 + 1);
    int sp3 = line.indexOf(' ', sp2 + 1);
    if (sp1 < 0 || sp2 < 0 || sp3 < 0) {
      Serial.println(F("ERR usage AUTH sector A|B key"));
      return;
    }
    int sector = line.substring(sp1 + 1, sp2).toInt();
    char type = toupper(line.charAt(sp2 + 1));
    String key = line.substring(sp3 + 1);
    key.toUpperCase();
    cmdAuth(sector, type, key);
  } else if (line.startsWith("READ ") || line.startsWith("read ")) {
    int block = line.substring(5).toInt();
    cmdRead(block);
  } else {
    Serial.println(F("ERR unknown"));
  }
}
