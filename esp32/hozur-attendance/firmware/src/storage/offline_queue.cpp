#include "offline_queue.h"

#include <LittleFS.h>
#include <ArduinoJson.h>

bool OfflineQueue::begin() {
  if (!LittleFS.begin(true)) {
    Serial.println(F("[QUEUE] LittleFS mount failed"));
    return false;
  }
  if (!LittleFS.exists(kPath)) {
    File f = LittleFS.open(kPath, "w");
    if (f) f.close();
  }
  Serial.printf("[QUEUE] ready size=%u\n", (unsigned)size());
  return true;
}

bool OfflineQueue::enqueue(const QueuedScan& scan) {
  File f = LittleFS.open(kPath, "a");
  if (!f) {
    return false;
  }
  JsonDocument doc;
  doc["uid"] = scan.uid;
  doc["user_id"] = scan.user_id;
  doc["counter"] = scan.counter;
  doc["device_time"] = scan.device_time;
  serializeJson(doc, f);
  f.print('\n');
  f.close();
  return true;
}

bool OfflineQueue::peek(QueuedScan* out) const {
  if (!out) return false;
  File f = LittleFS.open(kPath, "r");
  if (!f) return false;
  String line = f.readStringUntil('\n');
  f.close();
  line.trim();
  if (line.length() == 0) return false;

  JsonDocument doc;
  if (deserializeJson(doc, line)) return false;
  memset(out, 0, sizeof(*out));
  strlcpy(out->uid, doc["uid"] | "", sizeof(out->uid));
  out->user_id = doc["user_id"] | 0;
  out->counter = doc["counter"] | 0;
  strlcpy(out->device_time, doc["device_time"] | "", sizeof(out->device_time));
  return out->uid[0] != '\0';
}

bool OfflineQueue::rewriteWithoutFirstLine() {
  File in = LittleFS.open(kPath, "r");
  if (!in) return false;
  String rest;
  // skip first line
  in.readStringUntil('\n');
  while (in.available()) {
    rest += (char)in.read();
  }
  in.close();

  File out = LittleFS.open(kPath, "w");
  if (!out) return false;
  out.print(rest);
  out.close();
  return true;
}

bool OfflineQueue::pop() {
  return rewriteWithoutFirstLine();
}

size_t OfflineQueue::size() const {
  File f = LittleFS.open(kPath, "r");
  if (!f) return 0;
  size_t n = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) ++n;
  }
  f.close();
  return n;
}
