/*
 * بازی کشتی فضایی — Arduino Pro Mini
 * جویستیک: HW-504  |  نمایشگر: TFT SPI (ST7735 1.8")
 *
 * کتابخانه‌ها: Adafruit_GFX, Adafruit_ST7735
 *
 * ساختار ماژولاری:
 *   config.h       — پین‌ها و ثابت‌ها
 *   joystick_hw504 — خواندن جویستیک
 *   game           — وضعیت و منطق بازی
 *   display_tft    — رسم روی TFT
 */

#include "config.h"
#include "joystick_hw504.h"
#include "game.h"
#include "display_tft.h"
#include <Adafruit_ST7735.h>

static GameState state;
static uint8_t game_over_drawn;

void setup() {
  joystick_init();
  display_init();
  game_init(&state);
  game_over_drawn = 0;
}

void loop() {
  if (state.game_over) {
    if (!game_over_drawn) {
      display_fill_screen(ST77XX_BLACK);
      game_over_drawn = 1;
    }
    display_draw_game_over(state.score);

    joy_dir_t dx, dy;
    uint8_t fire;
    joystick_read(&dx, &dy, &fire);
    if (fire) {
      game_over_drawn = 0;
      delay(300);
      display_fill_screen(ST77XX_BLACK);
      game_init(&state);
    }
    delay(80);
    return;
  }

  joy_dir_t dx, dy;
  uint8_t fire;
  joystick_read(&dx, &dy, &fire);

  /* پاک کردن در موقعیت فعلی، بعد به‌روزرسانی، بعد رسم در موقعیت جدید */
  display_clear_ship(state.ship_x, state.ship_y);
  display_clear_bullets(&state);
  display_clear_enemies(&state);

  game_update(&state, dx, dy, fire);

  display_draw_ship(state.ship_x, state.ship_y);
  display_draw_bullets(&state);
  display_draw_enemies(&state);
  display_draw_score(state.score);

  delay(40);
}
