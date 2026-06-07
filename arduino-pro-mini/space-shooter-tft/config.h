/*
 * config.h — تنظیمات مرکزی پروژه
 * بازی کشتی فضایی — Arduino Pro Mini + HW-504 + TFT ST7735
 */

#ifndef SPACE_SHOOTER_CONFIG_H
#define SPACE_SHOOTER_CONFIG_H

/* ─── پین‌های TFT (SPI) ───────────────────────────────────────────── */
#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9

/* ─── پین‌های جویستیک HW-504 ───────────────────────────────────────── */
#define JOY_PIN_X    A0   /* VRx — محور افقی */
#define JOY_PIN_Y    A1   /* VRy — محور عمودی */
#define JOY_PIN_SW   2    /* SW  — دکمه (شلیک) */

/* ─── ابعاد صفحه (حالت افقی، ۱٫۸ اینچ) ────────────────────────────── */
#define SCREEN_W    160
#define SCREEN_H    128

/* ─── پارامترهای بازی ─────────────────────────────────────────────── */
#define SHIP_W         8
#define SHIP_H         8
#define BULLET_MAX     4
#define ENEMY_MAX      5
#define JOY_DEAD_ZONE  80   /* ناحیه مرده حول ۵۱۲ */
#define BULLET_SPEED   5
#define ENEMY_SPEED    2
#define ENEMY_SPAWN_MS 800
#define SHIP_MOVE_X    3
#define SHIP_MOVE_Y    2
#define SCORE_PER_HIT  10

/* ─── ابعاد گلوله و دشمن (برای رسم و برخورد) ───────────────────────── */
#define BULLET_W       2
#define BULLET_H       4
#define ENEMY_SIZE     6

/* ─── محدوده مرکز آنالوگ جویستیک ───────────────────────────────────── */
#define JOY_CENTER     512

#endif /* SPACE_SHOOTER_CONFIG_H */
