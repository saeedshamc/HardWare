#pragma once

#include <Arduino.h>
#include "../display/tft_display.h"

struct ScanUiData {
  const char* nameLatin;
  const char* department;
  const char* direction;   // "in" / "out"
  const char* timeStr;
  const char* dateJalali;
  const uint16_t* nameBitmapRgb565;
  uint16_t nameBmpW;
  uint16_t nameBmpH;
  const uint8_t* nameBitmapMono;
  uint16_t nameMonoW;
  uint16_t nameMonoH;
};

class UiScreens {
 public:
  explicit UiScreens(TftDisplay& display);

  void showIdle(const char* clockStr, const char* dateStr, bool online, bool wifiOk);
  void showSuccess(const ScanUiData& data);
  void showDenied(const char* reason);
  void showDuplicate(const ScanUiData& data);
  void showOfflineBanner(bool offline);

  /** Non-blocking: call each loop; returns true while a result screen is held. */
  bool update(uint32_t nowMs);

  bool isShowingResult() const { return resultUntilMs_ != 0; }

 private:
  TftDisplay& display_;
  uint32_t resultUntilMs_ = 0;
  bool lastOnline_ = true;
  char lastClock_[16] = {0};
};
