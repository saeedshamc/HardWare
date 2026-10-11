#pragma once

#include <Arduino.h>

class WifiManager {
 public:
  void begin(const char* ssid, const char* password);
  void update();
  bool isConnected() const;
  const char* ipAddress() const;

 private:
  const char* ssid_ = nullptr;
  const char* password_ = nullptr;
  uint32_t lastAttemptMs_ = 0;
  mutable char ipBuf_[16] = "0.0.0.0";
};
