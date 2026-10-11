#pragma once

#include <Arduino.h>
#include <time.h>

class TimeService {
 public:
  void begin();
  /** Call after WiFi connects to sync NTP. */
  void startNtp();
  void update();

  bool isSynced() const { return synced_; }
  /** Format local Asia/Tehran clock HH:MM:SS */
  bool formatClock(char* out, size_t outLen) const;
  /** Format local date YYYY-MM-DD (Gregorian; Jalali comes from server). */
  bool formatDate(char* out, size_t outLen) const;
  /** ISO-8601-ish with +03:30 offset for API device_time. */
  bool formatDeviceTime(char* out, size_t outLen) const;

 private:
  bool synced_ = false;
  bool rtcOk_ = false;
  void trySyncFromRtc();
  void pushToRtc(time_t epoch);
};
