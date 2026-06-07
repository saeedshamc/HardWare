/*
 * display_tft.h — ماژول نمایشگر TFT (ST7735)
 * رسم عناصر بازی روی صفحه
 */

#ifndef SPACE_SHOOTER_DISPLAY_TFT_H
#define SPACE_SHOOTER_DISPLAY_TFT_H

#include "config.h"
#include "game.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * مقداردهی TFT و چرخش صفحه.
 * یک بار در setup() فراخوانی شود.
 */
void display_init(void);

/**
 * پر کردن کل صفحه با یک رنگ.
 */
void display_fill_screen(uint16_t color);

/**
 * پاک کردن کشتی در موقعیت داده‌شده (رسم با سیاه).
 */
void display_clear_ship(int x, int y);

/**
 * رسم کشتی در موقعیت داده‌شده.
 */
void display_draw_ship(int x, int y);

/**
 * پاک کردن همه گلوله‌های فعال طبق state.
 */
void display_clear_bullets(const GameState *state);

/**
 * رسم همه گلوله‌های فعال طبق state.
 */
void display_draw_bullets(const GameState *state);

/**
 * پاک کردن همه دشمنان فعال طبق state.
 */
void display_clear_enemies(const GameState *state);

/**
 * رسم همه دشمنان فعال طبق state.
 */
void display_draw_enemies(const GameState *state);

/**
 * رسم امتیاز در گوشه (ناحیه امتیاز پاک و دوباره رسم می‌شود).
 */
void display_draw_score(uint16_t score);

/**
 * رسم صفحه Game Over با امتیاز نهایی.
 */
void display_draw_game_over(uint16_t score);

#ifdef __cplusplus
}
#endif

#endif /* SPACE_SHOOTER_DISPLAY_TFT_H */
