/**
 * Hozur Attendance — ESP32 smart reader entry point.
 * Modules are filled in subsequent phases (RFID, UI, net, offline).
 */
#include <Arduino.h>
#include "config.h"

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

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== Hozur Attendance (ESP32) ==="));
  Serial.printf("Reader ID: %d\n", READER_ID);
  Serial.printf("Server: %s\n", SERVER_BASE_URL);
  Serial.printf("Card sector=%u data_block=%u\n", CARD_SECTOR, CARD_DATA_BLOCK);
  Serial.printf("ENROLL_DRY_RUN=%d\n", ENROLL_DRY_RUN);
  Serial.println(F("Skeleton ready. Flash phase-1 firmware modules next."));

  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_MODE_BTN, INPUT_PULLUP);
  digitalWrite(PIN_STATUS_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
}

void loop() {
  // Heartbeat LED so a blank flash is visibly alive.
  static uint32_t lastToggle = 0;
  if (millis() - lastToggle >= 1000) {
    lastToggle = millis();
    digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED));
  }
}
