#include "wifi_manager.h"

#include <WiFi.h>

void WifiManager::begin(const char* ssid, const char* password) {
  ssid_ = ssid;
  password_ = password;
  WiFi.mode(WIFI_STA);
  if (ssid_ && ssid_[0]) {
    WiFi.begin(ssid_, password_ ? password_ : "");
    Serial.printf("[WiFi] Connecting to %s\n", ssid_);
  } else {
    Serial.println(F("[WiFi] SSID empty — skip"));
  }
  lastAttemptMs_ = millis();
}

void WifiManager::update() {
  if (!ssid_ || !ssid_[0]) return;
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastAttemptMs_ < 10000) return;
  lastAttemptMs_ = millis();
  Serial.println(F("[WiFi] Reconnecting..."));
  WiFi.disconnect();
  WiFi.begin(ssid_, password_ ? password_ : "");
}

bool WifiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

const char* WifiManager::ipAddress() const {
  if (!isConnected()) {
    return "0.0.0.0";
  }
  const IPAddress ip = WiFi.localIP();
  snprintf(ipBuf_, sizeof(ipBuf_), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return ipBuf_;
}
