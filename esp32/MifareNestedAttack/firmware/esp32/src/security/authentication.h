#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "../rfid/rfid_reader.h"

enum class KeyType : uint8_t { KeyA = 0, KeyB = 1 };

class AuthenticationDemo {
 public:
  explicit AuthenticationDemo(RfidReader& reader);

  /** Authenticate with provided 6-byte key. Prints SUCCESS/FAILURE clearly. */
  bool authenticate(uint8_t sector, KeyType keyType, const uint8_t key[6]);

  /** Convenience: factory FF..FF KeyA on sector (test cards). */
  bool authenticateDefaultKeyA(uint8_t sector);

 private:
  RfidReader& reader_;
};
