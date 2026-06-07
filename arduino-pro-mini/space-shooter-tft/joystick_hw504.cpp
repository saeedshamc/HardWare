/*
 * joystick_hw504.cpp — پیاده‌سازی ماژول جویستیک HW-504
 */

#include "joystick_hw504.h"
#include "config.h"
#include <Arduino.h>

void joystick_init(void) {
  pinMode(JOY_PIN_SW, INPUT_PULLUP);
}

void joystick_read(joy_dir_t *dx, joy_dir_t *dy, uint8_t *fire) {
  int x = analogRead(JOY_PIN_X);
  int y = analogRead(JOY_PIN_Y);
  *fire = (digitalRead(JOY_PIN_SW) == LOW) ? 1u : 0u;

  *dx = 0;
  *dy = 0;
  if (x < (int)(JOY_CENTER - JOY_DEAD_ZONE)) *dx = -1;
  if (x > (int)(JOY_CENTER + JOY_DEAD_ZONE)) *dx =  1;
  if (y < (int)(JOY_CENTER - JOY_DEAD_ZONE)) *dy = -1;
  if (y > (int)(JOY_CENTER + JOY_DEAD_ZONE)) *dy =  1;
}
