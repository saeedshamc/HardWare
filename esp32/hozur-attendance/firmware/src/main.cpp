/**
 * Hozur Attendance — ESP32 smart reader
 * Phase 4: signed HTTP API + LittleFS offline queue + sync.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

#include "config.h"
#include "rfid/rfid_reader.h"
#include "storage/secrets_store.h"
#include "storage/offline_queue.h"
#include "display/tft_display.h"
#include "ui/ui_screens.h"
#include "ui/buzzer.h"
#include "net/wifi_manager.h"
#include "net/time_service.h"
#include "net/api_client.h"

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
static TftDisplay gTft;
static UiScreens gUi(gTft);
static Buzzer gBuzzer;
static WifiManager gWifi;
static TimeService gTime;
static ApiClient gApi;
static OfflineQueue gQueue;

static bool gEnrollMode = false;
static uint32_t gEnrollUserId = 0;
static uint32_t gLastUidHash = 0;
static uint32_t gLastScanMs = 0;
static bool gNtpStarted = false;
static bool gServerOnline = false;
static uint32_t gLastSyncMs = 0;

static uint32_t hashUid(const uint8_t* uid, uint8_t len) {
  uint32_t h = 2166136261u;
  for (uint8_t i = 0; i < len; ++i) {
    h ^= uid[i];
    h *= 16777619u;
  }
  return h;
}

static void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  if (line.equalsIgnoreCase("HELP")) {
    Serial.println(F("Commands: HELP | ENROLL <user_id> | CANCEL | SYNC"));
    return;
  }
  if (line.equalsIgnoreCase("CANCEL")) {
    gEnrollMode = false;
    Serial.println(F("Enroll cancelled"));
    return;
  }
  if (line.equalsIgnoreCase("SYNC")) {
    gLastSyncMs = 0;
    Serial.println(F("Offline sync requested"));
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
    Serial.printf("Enroll mode user_id=%lu dry_run=%d\n",
                  (unsigned long)gEnrollUserId, ENROLL_DRY_RUN);
    return;
  }
  Serial.println(F("Unknown command. Type HELP"));
}

static void refreshIdleUi() {
  if (gUi.isShowingResult()) return;
  char clock[16];
  char date[16];
  gTime.formatClock(clock, sizeof(clock));
  gTime.formatDate(date, sizeof(date));
  gUi.showIdle(clock, date, gServerOnline, gWifi.isConnected());
}

static void queueCurrentScan(const RfidScanResult& scan) {
  QueuedScan q{};
  strlcpy(q.uid, scan.uidHex, sizeof(q.uid));
  q.user_id = scan.user_id;
  q.counter = scan.counter;
  gTime.formatDeviceTime(q.device_time, sizeof(q.device_time));
  if (gQueue.enqueue(q)) {
    Serial.printf("[QUEUE] enqueued uid=%s (size=%u)\n", q.uid, (unsigned)gQueue.size());
  }
  gServerOnline = false;
}

static void tryFlushOfflineQueue() {
  if (!gWifi.isConnected() || gQueue.empty()) return;

  JsonDocument arrDoc;
  JsonArray arr = arrDoc.to<JsonArray>();
  // Build batch from file by reading all lines (bounded)
  // Re-open via repeated peek/pop after success — first collect
  size_t n = gQueue.size();
  if (n > 20) n = 20;

  // Temporary: read file content by peeking without pop using size iteration
  // Simpler approach: sync one-by-one into a JSON array built from peeks then pop all on success
  std::vector<QueuedScan> batch;
  batch.reserve(n);
  // We only have peek of first; rebuild by reading raw file
  // Use post of currently queued items via iterative drain copy:
  for (size_t i = 0; i < n; ++i) {
    QueuedScan q;
    if (!gQueue.peek(&q)) break;
    // stash then pop into memory — if sync fails we'll re-enqueue
    batch.push_back(q);
    gQueue.pop();
    JsonObject o = arr.add<JsonObject>();
    o["uid"] = q.uid;
    o["user_id"] = q.user_id;
    o["counter"] = q.counter;
    o["device_time"] = q.device_time;
  }

  String scansJson;
  serializeJson(arr, scansJson);
  if (gApi.postOfflineSync(scansJson)) {
    Serial.printf("[QUEUE] synced %u items\n", (unsigned)batch.size());
    gServerOnline = true;
  } else {
    Serial.println(F("[QUEUE] sync failed — requeue"));
    for (const auto& q : batch) {
      gQueue.enqueue(q);
    }
    gServerOnline = false;
  }
}

static void showApiResult(ApiScanResponse& resp) {
  ScanUiData ui{};
  gApi.fillUiData(resp, &ui);
  if (strcmp(resp.status, "ok") == 0) {
    gBuzzer.play(BuzzPattern::Ok);
    gUi.showSuccess(ui);
  } else if (strcmp(resp.status, "duplicate") == 0) {
    gBuzzer.play(BuzzPattern::Denied);
    gUi.showDuplicate(ui);
  } else {
    gBuzzer.play(BuzzPattern::Denied);
    gUi.showDenied(resp.message[0] ? resp.message : "Denied");
  }
  ApiClient::freeNameBitmap(&resp);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== Hozur Attendance (ESP32) ==="));
  Serial.printf("Reader ID: %d\n", READER_ID);
  Serial.printf("Server: %s\n", SERVER_BASE_URL);

  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_MODE_BTN, INPUT_PULLUP);
  digitalWrite(PIN_STATUS_LED, LOW);

  gBuzzer.begin(PIN_BUZZER);
  gTime.begin();
  gQueue.begin();

  gTft.begin();
  gUi.showIdle("--:--:--", "----/--/--", false, false);

  uint8_t master[64];
  size_t masterLen = 0;
  if (!secrets_load_master_key(master, sizeof(master), &masterLen)) {
    Serial.println(F("FATAL: cannot load MASTER_KEY"));
    gUi.showDenied("No MASTER_KEY");
  }

  gRfid.begin();
  gRfid.setMasterKey(master, masterLen);
  gApi.begin(SERVER_BASE_URL, READER_ID, READER_API_KEY);
  gWifi.begin(WIFI_SSID, WIFI_PASSWORD);
  enableLoopWDT();
  Serial.println(F("Ready. HELP | ENROLL <id> | SYNC"));
}

void loop() {
  const uint32_t now = millis();
  handleSerial();
  gWifi.update();
  gBuzzer.update(now);
  gTime.update();

  if (gWifi.isConnected() && !gNtpStarted) {
    gTime.startNtp();
    gNtpStarted = true;
  }

  gUi.update(now);
  static uint32_t lastIdleMs = 0;
  if (!gUi.isShowingResult() && now - lastIdleMs > 500) {
    lastIdleMs = now;
    refreshIdleUi();
  }

  if (gWifi.isConnected() && (now - gLastSyncMs > OFFLINE_SYNC_EVERY_MS)) {
    gLastSyncMs = now;
    if (gQueue.empty()) {
      gServerOnline = gApi.getSyncOk();
    } else {
      tryFlushOfflineQueue();
    }
  }

  if (gEnrollMode) {
    RfidScanResult res;
    if (gRfid.enroll(gEnrollUserId, &res)) {
      Serial.printf("ENROLL_OK uid=%s user_id=%lu\n", res.uidHex,
                    (unsigned long)res.user_id);
      gEnrollMode = false;
      gBuzzer.play(BuzzPattern::Enroll);
      const bool ok = gApi.postEnroll(res.uidHex, res.user_id);
      ScanUiData ui{};
      ui.nameLatin = ok ? "ENROLLED" : "ENROLL LOCAL";
      ui.department = res.uidHex;
      ui.direction = "in";
      char tbuf[16];
      gTime.formatClock(tbuf, sizeof(tbuf));
      ui.timeStr = tbuf;
      gUi.showSuccess(ui);
      gServerOnline = ok;
    }
    return;
  }

  if (gUi.isShowingResult()) {
    return;
  }

  RfidScanResult scan;
  if (!gRfid.pollScan(&scan)) {
    return;
  }

  const uint32_t h = hashUid(scan.uid, scan.uidLen);
  if (h == gLastUidHash && (now - gLastScanMs) < CARD_COOLDOWN_MS) {
    Serial.println(F("Cooldown — ignoring same card"));
    gBuzzer.play(BuzzPattern::Denied);
    gUi.showDuplicate(ScanUiData{});
    return;
  }
  gLastUidHash = h;
  gLastScanMs = now;

  Serial.printf("SCAN uid=%s user_id=%lu counter=%lu\n", scan.uidHex,
                (unsigned long)scan.user_id, (unsigned long)scan.counter);

  char deviceTime[32];
  gTime.formatDeviceTime(deviceTime, sizeof(deviceTime));

  ApiScanRequest req{scan.uidHex, scan.user_id, scan.counter, deviceTime};
  ApiScanResponse resp{};
  if (gWifi.isConnected() && gApi.postScan(req, &resp)) {
    gServerOnline = true;
    showApiResult(resp);
  } else {
    queueCurrentScan(scan);
    gBuzzer.play(BuzzPattern::Ok);
    ScanUiData ui{};
    static char nameBuf[32];
    snprintf(nameBuf, sizeof(nameBuf), "User %lu", (unsigned long)scan.user_id);
    ui.nameLatin = nameBuf;
    ui.department = "queued offline";
    ui.direction = "in";
    char tbuf[16];
    gTime.formatClock(tbuf, sizeof(tbuf));
    ui.timeStr = tbuf;
    gUi.showSuccess(ui);
  }
}
