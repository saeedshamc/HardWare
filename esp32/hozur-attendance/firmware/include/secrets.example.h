#pragma once

// Copy this file to secrets.h and fill in real values.
// secrets.h is gitignored — never commit it.

#define WIFI_SSID        "YOUR_WIFI_SSID"
#define WIFI_PASSWORD    "YOUR_WIFI_PASSWORD"

// e.g. "http://192.168.1.50:8000"  (no trailing slash)
#define SERVER_BASE_URL  "http://192.168.1.50:8000"

#define READER_ID        1
#define READER_API_KEY   "replace-with-reader-api-key"

// 16+ random bytes recommended. Must match server HOZUR_MASTER_KEY if
// server-side key derivation is enabled.
#define MASTER_KEY       "CHANGE_ME_TO_A_LONG_RANDOM_SECRET"
