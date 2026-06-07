/*
 * display_tft.cpp — پیاده‌سازی ماژول TFT ST7735
 */

#include "display_tft.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Arduino.h>

static Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void display_init(void) {
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(1);
}

void display_fill_screen(uint16_t color) {
  tft.fillScreen(color);
}

void display_clear_ship(int x, int y) {
  tft.fillTriangle(
    x + SHIP_W / 2, y,
    x, y + SHIP_H,
    x + SHIP_W, y + SHIP_H,
    ST77XX_BLACK
  );
}

void display_draw_ship(int x, int y) {
  tft.fillTriangle(
    x + SHIP_W / 2, y,
    x, y + SHIP_H,
    x + SHIP_W, y + SHIP_H,
    ST77XX_CYAN
  );
}

void display_clear_bullets(const GameState *s) {
  for (uint8_t i = 0; i < BULLET_MAX; i++) {
    if (s->bullet_active[i])
      tft.fillRect(s->bullet_x[i], s->bullet_y[i], BULLET_W, BULLET_H, ST77XX_BLACK);
  }
}

void display_draw_bullets(const GameState *s) {
  for (uint8_t i = 0; i < BULLET_MAX; i++) {
    if (s->bullet_active[i])
      tft.fillRect(s->bullet_x[i], s->bullet_y[i], BULLET_W, BULLET_H, ST77XX_YELLOW);
  }
}

void display_clear_enemies(const GameState *s) {
  for (uint8_t i = 0; i < ENEMY_MAX; i++) {
    if (s->enemy_active[i])
      tft.fillRect(s->enemy_x[i], s->enemy_y[i], ENEMY_SIZE, ENEMY_SIZE, ST77XX_BLACK);
  }
}

void display_draw_enemies(const GameState *s) {
  for (uint8_t i = 0; i < ENEMY_MAX; i++) {
    if (s->enemy_active[i])
      tft.fillRect(s->enemy_x[i], s->enemy_y[i], ENEMY_SIZE, ENEMY_SIZE, ST77XX_RED);
  }
}

void display_draw_score(uint16_t score) {
  tft.fillRect(0, 0, 40, 10, ST77XX_BLACK);
  tft.setCursor(2, 2);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.print(score);
}

void display_draw_game_over(uint16_t score) {
  tft.setCursor(SCREEN_W / 2 - 30, SCREEN_H / 2 - 8);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("GAME OVER");
  tft.setCursor(SCREEN_W / 2 - 20, SCREEN_H / 2 + 4);
  tft.print("Score:");
  tft.print(score);
}
