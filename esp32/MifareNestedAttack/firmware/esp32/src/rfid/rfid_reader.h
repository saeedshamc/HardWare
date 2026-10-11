#pragma once

#include <Arduino.h>
#include <MFRC522.h>

struct CardIdentity {
  bool present;
  uint8_t uid[10];
  uint8_t uidSize;
  uint16_t atqa;
  uint8_t sak;
  char uidHex[28];
  const char* typeName;
};

class RfidReader {
 public:
  RfidReader();
  void begin();
  bool detect(CardIdentity* out);
  bool selectKnown(CardIdentity* out);
  void halt();
  MFRC522& picc() { return mfrc_; }

 private:
  MFRC522 mfrc_;
  void fillIdentity(CardIdentity* out);
  static const char* describeType(MFRC522::PICC_Type t);
};
