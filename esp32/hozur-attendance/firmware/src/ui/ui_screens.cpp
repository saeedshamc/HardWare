#include "ui_screens.h"

#include "config.h"

#include <string.h>

UiScreens::UiScreens(TftDisplay& display) : display_(display) {}

void UiScreens::showIdle(const char* clockStr, const char* dateStr, bool online,
                         bool wifiOk) {
  if (resultUntilMs_ != 0) {
    return;  // don't overwrite result screen
  }

  const bool clockChanged =
      !clockStr || strncmp(lastClock_, clockStr, sizeof(lastClock_)) != 0;
  if (!clockChanged && online == lastOnline_) {
    return;
  }

  if (clockStr) {
    strncpy(lastClock_, clockStr, sizeof(lastClock_) - 1);
  }
  lastOnline_ = online;

  display_.fillScreen(TftDisplay::COL_BG);
  display_.drawHeader("HOZUR", TftDisplay::COL_BLUE, TftDisplay::COL_WHITE);
  display_.drawCentered(clockStr ? clockStr : "--:--:--", 120, TftDisplay::COL_WHITE, 2);
  display_.drawCentered(dateStr ? dateStr : "", 170, TftDisplay::COL_GRAY, 1);

  char status[48];
  snprintf(status, sizeof(status), "%s | %s",
           wifiOk ? "WiFi OK" : "WiFi --",
           online ? "Online" : "OFFLINE");
  display_.drawStatusLine(status, online ? TftDisplay::COL_GREEN : TftDisplay::COL_YELLOW);
}

void UiScreens::showSuccess(const ScanUiData& data) {
  display_.fillScreen(0x0320);
  display_.drawHeader("OK", TftDisplay::COL_GREEN, TftDisplay::COL_WHITE);

  if (data.nameBitmapRgb565 && data.nameBmpW && data.nameBmpH) {
    const int16_t x = (int16_t)((TFT_W - data.nameBmpW) / 2);
    display_.drawRgb565Bitmap(x, 70, data.nameBmpW, data.nameBmpH, data.nameBitmapRgb565);
  } else if (data.nameBitmapMono && data.nameMonoW && data.nameMonoH) {
    const int16_t x = (int16_t)((TFT_W - data.nameMonoW) / 2);
    display_.drawMonoBitmap(x, 70, data.nameMonoW, data.nameMonoH, data.nameBitmapMono,
                            TftDisplay::COL_WHITE, 0x0320);
  } else {
    display_.drawCentered(data.nameLatin ? data.nameLatin : "User", 80,
                          TftDisplay::COL_WHITE, 2);
  }

  display_.drawCentered(data.department ? data.department : "", 130, TftDisplay::COL_WHITE, 1);
  char line[48];
  snprintf(line, sizeof(line), "%s  %s",
           data.direction && strcmp(data.direction, "out") == 0 ? "OUT" : "IN",
           data.timeStr ? data.timeStr : "");
  display_.drawCentered(line, 180, TftDisplay::COL_WHITE, 2);
  display_.drawCentered(data.dateJalali ? data.dateJalali : "", 230, TftDisplay::COL_GRAY, 1);

  resultUntilMs_ = millis() + UI_RESULT_HOLD_MS;
}

void UiScreens::showDenied(const char* reason) {
  display_.fillScreen(0x4000);
  display_.drawHeader("DENIED", TftDisplay::COL_RED, TftDisplay::COL_WHITE);
  display_.drawCentered(reason ? reason : "Access denied", 140, TftDisplay::COL_WHITE, 2);
  resultUntilMs_ = millis() + UI_RESULT_HOLD_MS;
}

void UiScreens::showDuplicate(const ScanUiData& data) {
  display_.fillScreen(0x4220);
  display_.drawHeader("DUPLICATE", TftDisplay::COL_YELLOW, 0x0000);
  display_.drawCentered(data.nameLatin ? data.nameLatin : "Already scanned", 120,
                        0x0000, 2);
  display_.drawCentered(data.timeStr ? data.timeStr : "", 180, 0x0000, 1);
  resultUntilMs_ = millis() + UI_RESULT_HOLD_MS;
}

void UiScreens::showOfflineBanner(bool offline) {
  lastOnline_ = !offline;
}

bool UiScreens::update(uint32_t nowMs) {
  if (resultUntilMs_ == 0) {
    return false;
  }
  if ((int32_t)(nowMs - resultUntilMs_) >= 0) {
    resultUntilMs_ = 0;
    lastClock_[0] = '\0';  // force idle redraw
    return false;
  }
  return true;
}
