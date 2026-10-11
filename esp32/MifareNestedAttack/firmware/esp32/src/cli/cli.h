#pragma once

#include "../rfid/rfid_reader.h"
#include "../rfid/mifare.h"
#include "../security/authentication.h"

class LabCli {
 public:
  LabCli(RfidReader& reader, MifareHelper& mifare, AuthenticationDemo& auth);
  void begin();
  void loop();

 private:
  RfidReader& reader_;
  MifareHelper& mifare_;
  AuthenticationDemo& auth_;
  CardIdentity last_{};

  void printMenu();
  int readChoice();
  bool ensureCard(bool withAtqa);
  void cmdDetect();
  void cmdCardInfo();
  void cmdAuthenticate();
  void cmdReadBlock();
  void cmdSectorAnalyzer();
  void cmdAccessBits();
};
