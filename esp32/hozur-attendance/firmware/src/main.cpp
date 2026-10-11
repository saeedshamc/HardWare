/**
 * Hozur Attendance — ESP32 smart reader
 * Phase 1: RFID read, diversified keys, enrollment, watchdog, cooldown.
 */
#include <Arduino.h>

#include "config.h"
#include "rfid/rfid_reader.h"
#include "storage/secrets_store.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "secrets.h missing — copy include/secrets.example.h to include/secrets.h"
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define SERVER_BASE_URL "http://127.0.0.1:8000"
#define READER_ID 1
#define READER_API_KEY ""
#define MASTER_KEY "DEV_ONLY_CHANGE_ME"
#endif

static RfidReader gRfid;
static bool gEnrollMode = false;
static uint32_t gEnrollUserId = 0;
static uint32_t gLastUidHash = 0;
static uint32_t gLastScanMs = 0;

static uint32_t hashUid(const uint8_t* uid, uint8_t len) {
  uint32_t h = 2166136261u;
  for (uint8_t i = 0; i < len; ++i) {
    h ^= uid[i];
    h *= 16777619u;
  }
  return h;
}

static void handleSerial() {
  if (!Serial.available()) {
    return;
  }
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) {
    return;
  }

  if (line.equalsIgnoreCase("HELP")) {
    Serial.println(F("Commands: HELP | ENROLL <user_id> | CANCEL"));
    return;
  }
  if (line.equalsIgnoreCase("CANCEL")) {
    gEnrollMode = false;
    Serial.println(F("Enroll cancelled"));
    return;
  }
  if (line.startsWith("ENROLL ") || line.startsWith("enroll ")) {
    const long id = line.substring(7).toInt();
    if (id <= 0) {
      Serial.println(F("Usage: ENROLL <user_id>"));
      return;
    }
    gEnrollUserId = (uint32_t)id;
    gEnrollMode = true;
    Serial.printf("Enroll mode: present card for user_id=%lu (dry_run=%d)\n",
                  (unsigned long)gEnrollUserId, ENROLL_DRY_RUN);
    return;
  }
  Serial.println(F("Unknown command. Type HELP"));
}

static bool modeButtonHeld() {
  // Active low with pull-up; hold > 2s enters enroll of user 0 placeholder → serial preferred
  static uint32_t pressedSince = 0;
  if (digitalRead(PIN_MODE_BTN) == LOW) {
    if (pressedSince == 0) {
      pressedSince = millis();
    } else if (millis() - pressedSince > 3000 && !gEnrollMode) {
      Serial.println(F("Mode button held — send: ENROLL <user_id>"));
      pressedSince = millis();  // debounce message
    }
    return true;
  }
  pressedSince = 0;
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== Hozur Attendance (ESP32) ==="));
  Serial.printf("Reader ID: %d\n", READER_ID);
  Serial.printf("Server: %s\n", SERVER_BASE_URL);
  Serial.printf("ENROLL_DRY_RUN=%d\n", ENROLL_DRY_RUN);

  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_MODE_BTN, INPUT_PULLUP);
  digitalWrite(PIN_STATUS_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  uint8_t master[64];
  size_t masterLen = 0;
  if (!secrets_load_master_key(master, sizeof(master), &masterLen)) {
    Serial.println(F("FATAL: cannot load MASTER_KEY"));
  } else {
    Serial.printf("MASTER_KEY loaded (%u bytes) from NVS or secrets.h\n",
                  (unsigned)masterLen);
  }

  gRfid.begin();
  gRfid.setMasterKey(master, masterLen);

  // Loop task watchdog — device resets if loop stalls too long
  enableLoopWDT();

  Serial.println(F("Ready. Commands: HELP | ENROLL <user_id>"));
}

void loop() {
  handleSerial();
  modeButtonHeld();

  if (gEnrollMode) {
    RfidScanResult res;
    if (gRfid.enroll(gEnrollUserId, &res)) {
      Serial.printf("ENROLL_OK uid=%s user_id=%lu\n", res.uidHex,
                    (unsigned long)res.user_id);
      // Server notify is wired in the net phase.
      gEnrollMode = false;
      digitalWrite(PIN_STATUS_LED, HIGH);
      delay(100);
      digitalWrite(PIN_STATUS_LED, LOW);
    }
    delay(50);
    return;
  }

  RfidScanResult scan;
  if (!gRfid.pollScan(&scan)) {
    delay(20);
    return;
  }

  const uint32_t h = hashUid(scan.uid, scan.uidLen);
  if (h == gLastUidHash && (millis() - gLastScanMs) < CARD_COOLDOWN_MS) {
    Serial.println(F("Cooldown — ignoring same card"));
    return;
  }
  gLastUidHash = h;
  gLastScanMs = millis();

  Serial.printf("SCAN uid=%s user_id=%lu counter=%lu\n", scan.uidHex,
                (unsigned long)scan.user_id, (unsigned long)scan.counter);
  // POST /api/scan/ added in net phase.
  digitalWrite(PIN_BUZZER, HIGH);
  delay(40);
  digitalWrite(PIN_BUZZER, LOW);
}
