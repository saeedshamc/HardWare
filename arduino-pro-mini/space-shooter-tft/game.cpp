/*
 * game.cpp — پیاده‌سازی منطق بازی
 */

#include "game.h"
#include <Arduino.h>

static void spawn_bullet(GameState *s);
static void spawn_enemy(GameState *s);
static void update_bullets(GameState *s);
static void update_enemies(GameState *s);
static void check_hits(GameState *s);

void game_init(GameState *s) {
  s->score = 0;
  s->game_over = 0;
  s->ship_x = SCREEN_W / 2 - SHIP_W / 2;
  s->ship_y = SCREEN_H - SHIP_H - 4;

  for (uint8_t i = 0; i < BULLET_MAX; i++)
    s->bullet_active[i] = 0;
  for (uint8_t i = 0; i < ENEMY_MAX; i++)
    s->enemy_active[i] = 0;

  s->last_enemy_spawn = millis();
  s->last_fire = 1;
}

void game_update(GameState *s, int8_t dx, int8_t dy, uint8_t fire) {
  if (s->game_over)
    return;

  /* حرکت کشتی با محدودیت مرزها */
  int nx = s->ship_x + (int)dx * SHIP_MOVE_X;
  int ny = s->ship_y + (int)dy * SHIP_MOVE_Y;
  if (nx < 0) nx = 0;
  if (nx > SCREEN_W - SHIP_W) nx = SCREEN_W - SHIP_W;
  if (ny < SCREEN_H / 2) ny = SCREEN_H / 2;
  if (ny > SCREEN_H - SHIP_H - 2) ny = SCREEN_H - SHIP_H - 2;
  s->ship_x = nx;
  s->ship_y = ny;

  /* شلیک فقط روی لبه پایین‌رونده دکمه */
  if (fire && !s->last_fire)
    spawn_bullet(s);
  s->last_fire = fire;

  /* اسپان دشمن با تاخیر */
  if (millis() - s->last_enemy_spawn >= (unsigned long)ENEMY_SPAWN_MS) {
    spawn_enemy(s);
    s->last_enemy_spawn = millis();
  }

  update_bullets(s);
  update_enemies(s);
  check_hits(s);
}

static void spawn_bullet(GameState *s) {
  for (uint8_t i = 0; i < BULLET_MAX; i++) {
    if (!s->bullet_active[i]) {
      s->bullet_active[i] = 1;
      s->bullet_x[i] = s->ship_x + SHIP_W / 2 - BULLET_W / 2;
      s->bullet_y[i] = s->ship_y;
      return;
    }
  }
}

static void spawn_enemy(GameState *s) {
  for (uint8_t i = 0; i < ENEMY_MAX; i++) {
    if (!s->enemy_active[i]) {
      s->enemy_active[i] = 1;
      s->enemy_x[i] = random(2, SCREEN_W - ENEMY_SIZE);
      s->enemy_y[i] = 0;
      return;
    }
  }
}

static void update_bullets(GameState *s) {
  for (uint8_t i = 0; i < BULLET_MAX; i++) {
    if (!s->bullet_active[i]) continue;
    s->bullet_y[i] -= BULLET_SPEED;
    if (s->bullet_y[i] < 0)
      s->bullet_active[i] = 0;
  }
}

static void update_enemies(GameState *s) {
  for (uint8_t i = 0; i < ENEMY_MAX; i++) {
    if (!s->enemy_active[i]) continue;
    s->enemy_y[i] += ENEMY_SPEED;
    if (s->enemy_y[i] >= SCREEN_H) {
      s->enemy_active[i] = 0;
      s->game_over = 1;
      return;
    }
    /* برخورد با کشتی */
    if (s->enemy_y[i] + ENEMY_SIZE >= s->ship_y &&
        s->enemy_y[i] <= s->ship_y + SHIP_H &&
        s->enemy_x[i] + ENEMY_SIZE >= s->ship_x &&
        s->enemy_x[i] <= s->ship_x + SHIP_W) {
      s->enemy_active[i] = 0;
      s->game_over = 1;
      return;
    }
  }
}

static void check_hits(GameState *s) {
  for (uint8_t b = 0; b < BULLET_MAX; b++) {
    if (!s->bullet_active[b]) continue;
    for (uint8_t e = 0; e < ENEMY_MAX; e++) {
      if (!s->enemy_active[e]) continue;
      if (s->bullet_y[b] <= s->enemy_y[e] + ENEMY_SIZE &&
          s->bullet_y[b] + BULLET_H >= s->enemy_y[e] &&
          s->bullet_x[b] + BULLET_W >= s->enemy_x[e] &&
          s->bullet_x[b] <= s->enemy_x[e] + ENEMY_SIZE) {
        s->bullet_active[b] = 0;
        s->enemy_active[e] = 0;
        s->score += SCORE_PER_HIT;
      }
    }
  }
}
