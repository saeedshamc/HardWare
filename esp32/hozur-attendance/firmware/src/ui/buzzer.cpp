#include "buzzer.h"

void Buzzer::begin(int pin) {
  pin_ = pin;
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, LOW);
  pattern_ = BuzzPattern::None;
}

void Buzzer::play(BuzzPattern pattern) {
  pattern_ = pattern;
  step_ = 0;
  nextMs_ = millis();
}

void Buzzer::update(uint32_t nowMs) {
  if (pin_ < 0 || pattern_ == BuzzPattern::None) {
    return;
  }
  if ((int32_t)(nowMs - nextMs_) < 0) {
    return;
  }

  // step even = ON, odd = OFF / advance
  // Ok: ON40 OFF0 done
  // Denied: ON60 OFF80 ON60 done
  // Enroll: ON30 OFF40 ON30 OFF40 ON80 done

  struct Pulse { uint16_t onMs; uint16_t gapMs; };
  const Pulse* pulses = nullptr;
  uint8_t count = 0;

  static const Pulse okP[] = {{40, 0}};
  static const Pulse deniedP[] = {{60, 80}, {60, 0}};
  static const Pulse enrollP[] = {{30, 40}, {30, 40}, {80, 0}};

  switch (pattern_) {
    case BuzzPattern::Ok: pulses = okP; count = 1; break;
    case BuzzPattern::Denied: pulses = deniedP; count = 2; break;
    case BuzzPattern::Enroll: pulses = enrollP; count = 3; break;
    default: pattern_ = BuzzPattern::None; return;
  }

  if (step_ >= count) {
    digitalWrite(pin_, LOW);
    pattern_ = BuzzPattern::None;
    return;
  }

  // Fire a short blocking pulse (tens of ms) then schedule gap.
  digitalWrite(pin_, HIGH);
  delay(pulses[step_].onMs);
  digitalWrite(pin_, LOW);
  nextMs_ = millis() + pulses[step_].gapMs;
  ++step_;
  if (step_ >= count) {
    pattern_ = BuzzPattern::None;
  }
}
