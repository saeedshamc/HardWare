#include "time_service.h"

#include "config.h"

#include <Wire.h>

#if ENABLE_DS3231
#include <RTClib.h>
static RTC_DS3231 rtc;
#endif

void TimeService::begin() {
#if ENABLE_DS3231
  Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL);
  rtcOk_ = rtc.begin();
  if (rtcOk_) {
    trySyncFromRtc();
    Serial.println(F("[TIME] DS3231 OK"));
  } else {
    Serial.println(F("[TIME] DS3231 not found"));
  }
#else
  (void)rtcOk_;
#endif
}

void TimeService::startNtp() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2);
  Serial.println(F("[TIME] NTP started (Asia/Tehran UTC+3:30)"));
}

void TimeService::trySyncFromRtc() {
#if ENABLE_DS3231
  if (!rtcOk_) return;
  DateTime now = rtc.now();
  if (now.year() < 2020) return;
  timeval tv;
  tv.tv_sec = now.unixtime();
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  synced_ = true;
#endif
}

void TimeService::pushToRtc(time_t epoch) {
#if ENABLE_DS3231
  if (!rtcOk_ || epoch < 1609459200) return;
  rtc.adjust(DateTime((uint32_t)epoch));
#else
  (void)epoch;
#endif
}

void TimeService::update() {
  time_t now = time(nullptr);
  if (now > 1609459200) {  // 2021-01-01
    if (!synced_) {
      synced_ = true;
      pushToRtc(now);
      Serial.println(F("[TIME] clock synced"));
    }
  }
}

bool TimeService::formatClock(char* out, size_t outLen) const {
  if (!out || outLen < 9) return false;
  time_t now = time(nullptr);
  if (now < 1609459200) {
    snprintf(out, outLen, "--:--:--");
    return false;
  }
  struct tm t;
  localtime_r(&now, &t);
  strftime(out, outLen, "%H:%M:%S", &t);
  return true;
}

bool TimeService::formatDate(char* out, size_t outLen) const {
  if (!out || outLen < 11) return false;
  time_t now = time(nullptr);
  if (now < 1609459200) {
    snprintf(out, outLen, "----/--/--");
    return false;
  }
  struct tm t;
  localtime_r(&now, &t);
  strftime(out, outLen, "%Y-%m-%d", &t);
  return true;
}

bool TimeService::formatDeviceTime(char* out, size_t outLen) const {
  if (!out || outLen < 26) return false;
  time_t now = time(nullptr);
  if (now < 1609459200) {
    snprintf(out, outLen, "1970-01-01T00:00:00+03:30");
    return false;
  }
  struct tm t;
  localtime_r(&now, &t);
  snprintf(out, outLen, "%04d-%02d-%02dT%02d:%02d:%02d+03:30",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
           t.tm_hour, t.tm_min, t.tm_sec);
  return true;
}
