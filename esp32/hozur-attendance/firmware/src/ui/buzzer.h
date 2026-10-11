#pragma once

#include <Arduino.h>

enum class BuzzPattern : uint8_t { None, Ok, Denied, Enroll };

class Buzzer {
 public:
  void begin(int pin);
  void play(BuzzPattern pattern);
  /** Non-blocking update; call from loop. */
  void update(uint32_t nowMs);

 private:
  int pin_ = -1;
  BuzzPattern pattern_ = BuzzPattern::None;
  uint8_t step_ = 0;
  uint32_t nextMs_ = 0;
};
