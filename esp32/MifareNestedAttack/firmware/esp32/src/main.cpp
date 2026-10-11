/**
 * MIFARE Classic Security Lab — ESP32 + MFRC522
 * Educational RFID laboratory (test cards you own only).
 */
#include <Arduino.h>

#include "rfid/rfid_reader.h"
#include "rfid/mifare.h"
#include "security/authentication.h"
#include "cli/cli.h"

static RfidReader gReader;
static MifareHelper gMifare(gReader);
static AuthenticationDemo gAuth(gReader);
static LabCli gCli(gReader, gMifare, gAuth);

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("      MIFARE SECURITY LAB"));
  Serial.println(F("========================================"));
  Serial.println();

  gReader.begin();
  Serial.println();
  Serial.println(F("Waiting for card / serial menu..."));
  gCli.begin();
}

void loop() {
  gCli.loop();
  delay(20);
}
