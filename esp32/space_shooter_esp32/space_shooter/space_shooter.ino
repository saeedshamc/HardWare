/*
  Space Shooter v3 (harder: bigger enemies, tanks, waves, bosses)
  ESP32-WROOM-32 + TFT 1.8" ST7735 (128x160) + Joystick HW-504

  - Joystick: move the ship (left/right/up/down)
  - Auto-fire: always shooting
  - Button (SW): start / restart

  Power-ups (dropped randomly by destroyed enemies):
    [W] orange box  -> weapon upgrade (level 1..4)
    [heart] red     -> +1 life (max 5)
    [R] cyan box    -> rapid fire for 8 seconds
  When you get hit, your weapon drops one level.

  Enemies:
    red   = small ship (fast)
    pink  = shooter (bigger, 3 HP, fires straight)
    orange= tank (very big, slow, many HP, 3-way spread)  [from level 3]
    purple= BOSS every 5 levels (huge, fan + aimed shots)
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ===================== Pins =====================
#define TFT_CS   5
#define TFT_RST  4
#define TFT_DC   2
// TFT MOSI(SDA) = GPIO 23 , TFT SCK = GPIO 18  (hardware VSPI)

#define JOY_X    34    // VRx  (ADC1)
#define JOY_Y    35    // VRy  (ADC1)
#define JOY_SW   32    // SW (button)

// ===================== Settings =====================
#define INVERT_X      false   // if left/right is reversed, set true
#define INVERT_Y      false   // if up/down is reversed, set true
#define DEADZONE      300     // joystick dead zone (0..2000)
#define PLAYER_SPEED  3.5f    // ship speed (pixels per frame)
#define FIRE_DELAY    170     // ms between shots (rapid fire = half)
#define FRAME_MS      33      // ~30 FPS
#define START_LIVES   3
#define MAX_LIVES     5
#define MAX_WEAPON    4
#define RAPID_MS      8000UL  // rapid fire duration

// ----- difficulty -----
#define SCORE_PER_LEVEL 120   // score needed per level (lower = harder)
#define BOSS_EVERY      5     // a boss every N levels

// Drop chances (percent) for each destroyed enemy
#define DROP_WEAPON   10
#define DROP_LIFE     5
#define DROP_RAPID    6

// ===================== Screen =====================
const int16_t SCR_W = 128;
const int16_t SCR_H = 160;

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(SCR_W, SCR_H);

// ===================== Colors (RGB565) =====================
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_RED     0xF800
#define C_GREEN   0x07E0
#define C_BLUE    0x001F
#define C_CYAN    0x07FF
#define C_MAGENTA 0xF81F
#define C_YELLOW  0xFFE0
#define C_ORANGE  0xFD20
#define C_GRAY    0x8410
#define C_DGRAY   0x4208
#define C_TANK    0xE300   // dark orange
#define C_TANKD   0x8180   // brown
#define C_BOSS    0x7977   // purple
#define C_BOSSL   0xC31F   // light purple

// ===================== Game objects =====================
#define MAX_PB     40   // player bullets
#define MAX_EN     14   // enemies (+ boss)
#define MAX_EB     40   // enemy bullets
#define MAX_BOOM   16   // explosions
#define MAX_STARS  32
#define MAX_ITEMS   4   // power-up items

const int IS = 10;      // item size

// enemy types: 0 = small, 1 = shooter, 2 = tank, 3 = boss
const uint8_t SZ_W[3] = {14, 17, 24};
const uint8_t SZ_H[3] = {11, 14, 18};
const int SCORE_BY[3] = {10, 25, 60};
const int DROP_BONUS[3] = {0, 10, 25};

enum ItemType { IT_WEAPON = 0, IT_LIFE = 1, IT_RAPID = 2 };

struct Bullet  { int16_t x, y; int8_t dx; bool on; };
struct EBullet { float x, y, dx, vy; uint16_t col; bool on; };
struct Enemy   {
  float x, y, baseX, vy, phase, amp, t;
  uint8_t type, w, h, mode;
  int16_t hp, maxhp;
  int8_t flash;
  unsigned long nextFire;
  bool on;
};
struct Star    { int16_t x, y; uint8_t spd; };
struct Boom    { int16_t x, y; uint8_t t; bool on; };
struct Item    { float x, y; uint8_t type; bool on; };

Bullet  pb[MAX_PB];
EBullet eb[MAX_EB];
Enemy   en[MAX_EN];
Boom    booms[MAX_BOOM];
Star    stars[MAX_STARS];
Item    items[MAX_ITEMS];

enum GameState { ST_TITLE, ST_PLAY, ST_OVER };
GameState state = ST_TITLE;

float px, py;                 // ship position (px = center x, py = top y)
int   score = 0, best = 0, lives = START_LIVES, level = 1;
int   weaponLevel = 1;
int   invincible = 0;
int   nextBoss = BOSS_EVERY, spawnCount = 0;
unsigned long rapidUntil = 0;
unsigned long lastShot = 0, lastSpawn = 0, lastFrame = 0, overTime = 0;
uint32_t frameCount = 0;
bool  armed = false;          // input must be released before start/restart

const char *popupMsg = "";
int popupTimer = 0;

int centerX = 2048, centerY = 2048;

// ===================== Joystick =====================
void calibrateJoy() {
  long sx = 0, sy = 0;
  for (int i = 0; i < 32; i++) {
    sx += analogRead(JOY_X);
    sy += analogRead(JOY_Y);
    delay(5);
  }
  centerX = sx / 32;
  centerY = sy / 32;
}

void readJoy(float &jx, float &jy) {
  int rx = analogRead(JOY_X) - centerX;
  int ry = analogRead(JOY_Y) - centerY;
  if (abs(rx) < DEADZONE) rx = 0;
  if (abs(ry) < DEADZONE) ry = 0;
  jx = constrain(rx / 1800.0f, -1.0f, 1.0f);
  jy = constrain(ry / 1800.0f, -1.0f, 1.0f);
  if (INVERT_X) jx = -jx;
  if (INVERT_Y) jy = -jy;
}

bool actionPressed() {
  if (digitalRead(JOY_SW) == LOW) return true;
  float jx, jy;
  readJoy(jx, jy);
  return (fabs(jx) > 0.7f || fabs(jy) > 0.7f);
}

// ===================== Helpers =====================
bool hit(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

void printCentered(const char *s, int y, uint8_t size, uint16_t col) {
  int16_t x1, y1;
  uint16_t w, h;
  canvas.setTextSize(size);
  canvas.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  canvas.setCursor((SCR_W - (int)w) / 2 - x1, y);
  canvas.setTextColor(col);
  canvas.print(s);
}

void pushFrame() {
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), SCR_W, SCR_H);
}

void showPopup(const char *msg) {
  popupMsg = msg;
  popupTimer = 45;
}

// ===================== Stars =====================
void initStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(0, SCR_W);
    stars[i].y = random(0, SCR_H);
    stars[i].spd = random(1, 4);
  }
}

void updateStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].y += stars[i].spd;
    if (stars[i].y >= SCR_H) {
      stars[i].y = 0;
      stars[i].x = random(0, SCR_W);
    }
  }
}

void drawStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    uint16_t c = (stars[i].spd == 1) ? C_DGRAY : (stars[i].spd == 2 ? C_GRAY : C_WHITE);
    canvas.drawPixel(stars[i].x, stars[i].y, c);
  }
}

// ===================== Explosions =====================
void spawnBoom(int x, int y) {
  for (int i = 0; i < MAX_BOOM; i++) {
    if (!booms[i].on) {
      booms[i].x = x;
      booms[i].y = y;
      booms[i].t = 0;
      booms[i].on = true;
      return;
    }
  }
}

void updateBooms() {
  for (int i = 0; i < MAX_BOOM; i++) {
    if (booms[i].on) {
      booms[i].t++;
      if (booms[i].t > 9) booms[i].on = false;
    }
  }
}

void drawBooms() {
  for (int i = 0; i < MAX_BOOM; i++) {
    if (!booms[i].on) continue;
    int t = booms[i].t;
    canvas.drawCircle(booms[i].x, booms[i].y, t, t < 5 ? C_YELLOW : C_ORANGE);
    if (t > 1) canvas.drawCircle(booms[i].x, booms[i].y, t - 2, C_RED);
    if (t < 6)  canvas.fillCircle(booms[i].x, booms[i].y, 4 - t / 2, C_WHITE);
  }
}

// ===================== Drawing ships, enemies & items =====================
void drawPlayer(int x, int y) {
  canvas.fillTriangle(x, y, x - 6, y + 12, x + 6, y + 12, C_CYAN);   // body
  canvas.fillRect(x - 1, y + 4, 3, 6, C_WHITE);                      // cockpit
  canvas.fillRect(x - 8, y + 8, 3, 5, C_BLUE);                       // left wing
  canvas.fillRect(x + 6, y + 8, 3, 5, C_BLUE);                       // right wing
  if (frameCount & 1) canvas.fillTriangle(x - 2, y + 13, x + 2, y + 13, x, y + 17, C_ORANGE);
  else                canvas.fillTriangle(x - 1, y + 13, x + 1, y + 13, x, y + 15, C_YELLOW);
}

// small ship (type 0) and shooter (type 1), scaled to w x h
void drawAlien(int x, int y, int w, int h, uint8_t type, bool flash) {
  uint16_t c   = flash ? C_WHITE : (type ? C_MAGENTA : C_RED);
  uint16_t eye = flash ? C_BLACK : C_WHITE;
  int ex = max(2, w / 6);
  int ey = max(2, h / 5);
  canvas.fillRect(x + w / 6, y, (w * 8) / 12, (h * 6) / 10, c);          // head
  canvas.fillRect(x, y + h / 5, w, (h * 4) / 10, c);                     // wide body
  canvas.fillRect(x + w / 6, y + (h * 6) / 10, ex, h - (h * 6) / 10, c); // left leg
  canvas.fillRect(x + (w * 8) / 12, y + (h * 6) / 10, ex, h - (h * 6) / 10, c); // right leg
  canvas.fillRect(x + w / 4, y + h / 5, ex, ey, eye);                    // eyes
  canvas.fillRect(x + (w * 7) / 12, y + h / 5, ex, ey, eye);
  if (type == 1) canvas.fillRect(x + (w * 5) / 12, y + (h * 6) / 10, ex, (h * 3) / 10, C_YELLOW); // cannon
}

void drawTank(int x, int y, bool f) {      // 24 x 18
  uint16_t a = f ? C_WHITE : C_TANK;
  uint16_t d = f ? C_WHITE : C_TANKD;
  canvas.fillRect(x + 3, y, 18, 12, a);
  canvas.fillRect(x, y + 4, 24, 8, a);
  canvas.fillRect(x, y + 8, 24, 2, d);
  canvas.fillRect(x + 2, y + 12, 5, 6, d);
  canvas.fillRect(x + 17, y + 12, 5, 6, d);
  canvas.fillRect(x + 10, y + 12, 4, 6, a);
  canvas.fillRect(x + 5, y + 4, 5, 4, C_WHITE);
  canvas.fillRect(x + 14, y + 4, 5, 4, C_WHITE);
  canvas.fillRect(x + 6, y + 5, 2, 2, C_RED);
  canvas.fillRect(x + 15, y + 5, 2, 2, C_RED);
}

void drawBoss(int x, int y, bool f) {      // 44 x 28
  uint16_t a = f ? C_WHITE : C_BOSS;
  uint16_t b = f ? C_WHITE : C_BOSSL;
  canvas.fillRect(x + 7, y, 30, 20, a);
  canvas.fillRect(x, y + 7, 44, 10, a);
  canvas.fillRect(x + 4, y + 17, 6, 11, b);
  canvas.fillRect(x + 34, y + 17, 6, 11, b);
  canvas.fillRect(x + 18, y + 20, 8, 8, b);
  canvas.fillRect(x + 12, y + 4, 7, 7, C_RED);
  canvas.fillRect(x + 25, y + 4, 7, 7, C_RED);
  canvas.fillRect(x + 14, y + 6, 3, 3, C_WHITE);
  canvas.fillRect(x + 27, y + 6, 3, 3, C_WHITE);
  canvas.fillCircle(x + 22, y + 14, 3, C_YELLOW);
}

void drawEnemy(const Enemy &e) {
  int x = (int)e.x, y = (int)e.y;
  bool f = e.flash > 0;
  if (e.type == 3)      drawBoss(x, y, f);
  else if (e.type == 2) drawTank(x, y, f);
  else                  drawAlien(x, y, e.w, e.h, e.type, f);
  if (e.type >= 2 || e.hp < e.maxhp) {                       // health bar
    int hp = e.hp < 0 ? 0 : e.hp;
    canvas.fillRect(x, y + e.h + 1, e.w, 2, C_DGRAY);
    canvas.fillRect(x, y + e.h + 1, (e.w * hp) / e.maxhp, 2, e.type == 3 ? C_RED : C_GREEN);
  }
}

void drawItem(int x, int y, uint8_t type) {
  if (type == IT_LIFE) {
    canvas.fillCircle(x + 3, y + 3, 3, C_RED);
    canvas.fillCircle(x + 6, y + 3, 3, C_RED);
    canvas.fillTriangle(x, y + 4, x + 9, y + 4, x + 4, y + 9, C_RED);
  } else {
    uint16_t c = (type == IT_WEAPON) ? C_ORANGE : C_CYAN;
    canvas.fillRect(x, y, IS, IS, c);
    canvas.drawRect(x, y, IS, IS, C_WHITE);
    canvas.setTextSize(1);
    canvas.setTextColor(C_BLACK);
    canvas.setCursor(x + 2, y + 1);
    canvas.print(type == IT_WEAPON ? 'W' : 'R');
  }
}

// ===================== Bullets =====================
void addPlayerBullet(int x, int y, int dx) {
  for (int i = 0; i < MAX_PB; i++) {
    if (!pb[i].on) {
      pb[i].x = x;
      pb[i].y = y;
      pb[i].dx = dx;
      pb[i].on = true;
      return;
    }
  }
}

void firePlayer() {
  int x = (int)px;
  int y = (int)py;
  switch (weaponLevel) {
    case 1:                                   // single
      addPlayerBullet(x - 1, y, 0);
      break;
    case 2:                                   // double
      addPlayerBullet(x - 6, y + 4, 0);
      addPlayerBullet(x + 5, y + 4, 0);
      break;
    case 3:                                   // triple spread
      addPlayerBullet(x - 1, y, 0);
      addPlayerBullet(x - 5, y + 3, -1);
      addPlayerBullet(x + 3, y + 3, 1);
      break;
    default:                                  // double + spread
      addPlayerBullet(x - 6, y + 4, 0);
      addPlayerBullet(x + 5, y + 4, 0);
      addPlayerBullet(x - 7, y + 5, -1);
      addPlayerBullet(x + 6, y + 5, 1);
      break;
  }
}

void ebullet(float x, float y, float dx, float vy, uint16_t col) {
  for (int i = 0; i < MAX_EB; i++) {
    if (!eb[i].on) {
      eb[i].x = x;
      eb[i].y = y;
      eb[i].dx = dx;
      eb[i].vy = vy;
      eb[i].col = col;
      eb[i].on = true;
      return;
    }
  }
}

float enemySpeed() {
  return 2.6f + min(2.0f, level * 0.12f);
}

void enemyFire(Enemy &e) {
  float cx = e.x + e.w / 2.0f;
  float cy = e.y + e.h;
  float sp = enemySpeed();
  if (e.type == 1) {
    ebullet(cx, cy, 0, sp, C_GREEN);
  } else if (e.type == 2) {                   // tank: 3-way spread
    ebullet(cx, cy, -0.9f, sp * 0.95f, C_GREEN);
    ebullet(cx, cy, 0.0f,  sp * 0.95f, C_GREEN);
    ebullet(cx, cy, 0.9f,  sp * 0.95f, C_GREEN);
  }
}

void bossFire(Enemy &e) {
  float cx = e.x + e.w / 2.0f;
  float cy = e.y + e.h - 4;
  float sp = 2.8f + min(1.2f, level * 0.05f);
  if (e.mode == 0) {                          // fan of 5
    for (int i = -2; i <= 2; i++) {
      float a = i * 0.28f;
      ebullet(cx, cy, sinf(a) * sp, cosf(a) * sp, C_ORANGE);
    }
  } else {                                    // aimed triple
    float base = atan2f(px - cx, py - cy);
    for (int i = -1; i <= 1; i++) {
      float a = base + i * 0.18f;
      ebullet(cx, cy, sinf(a) * sp * 1.1f, cosf(a) * sp * 1.1f, C_ORANGE);
    }
  }
  if (e.hp < e.maxhp / 2) {                   // enraged: extra side shots
    ebullet(e.x + 4, e.y + e.h, 0, sp, C_ORANGE);
    ebullet(e.x + e.w - 4, e.y + e.h, 0, sp, C_ORANGE);
  }
}

// ===================== Items =====================
void spawnItem(int x, int y, uint8_t type) {
  for (int i = 0; i < MAX_ITEMS; i++) {
    if (!items[i].on) {
      items[i].x = constrain(x, 0, SCR_W - IS);
      items[i].y = y;
      items[i].type = type;
      items[i].on = true;
      return;
    }
  }
}

void maybeDrop(Enemy &e) {
  int bonus = DROP_BONUS[e.type < 3 ? e.type : 0];
  int r = random(100);
  int cx = (int)e.x + e.w / 2 - IS / 2;
  int cy = (int)e.y;
  int t1 = DROP_WEAPON + bonus;
  int t2 = t1 + DROP_LIFE;
  int t3 = t2 + DROP_RAPID;
  if (r < t1) {
    spawnItem(cx, cy, IT_WEAPON);
  } else if (r < t2) {
    if (lives < MAX_LIVES) spawnItem(cx, cy, IT_LIFE);
  } else if (r < t3) {
    spawnItem(cx, cy, IT_RAPID);
  }
}

void collectItem(uint8_t type) {
  if (type == IT_WEAPON) {
    if (weaponLevel < MAX_WEAPON) { weaponLevel++; showPopup("WEAPON UP!"); }
    else { score += 50; showPopup("MAX +50"); }
  } else if (type == IT_LIFE) {
    if (lives < MAX_LIVES) { lives++; showPopup("+1 LIFE"); }
    else { score += 50; showPopup("MAX +50"); }
  } else {
    rapidUntil = millis() + RAPID_MS;
    showPopup("RAPID FIRE!");
  }
}

// ===================== Enemies =====================
bool bossAlive() {
  for (int i = 0; i < MAX_EN; i++)
    if (en[i].on && en[i].type == 3) return true;
  return false;
}

int countEnemies() {
  int n = 0;
  for (int i = 0; i < MAX_EN; i++)
    if (en[i].on) n++;
  return n;
}

int maxEnemies() {
  return min(6 + level, 12);
}

void addEnemy(uint8_t type, float baseX, float y0, float amp, float vy, float phase) {
  for (int i = 0; i < MAX_EN; i++) {
    if (en[i].on) continue;
    Enemy &e = en[i];
    e.on = true;
    e.type = type;
    e.x = baseX;
    e.baseX = baseX;
    e.y = y0;
    e.amp = amp;
    e.vy = vy;
    e.phase = phase;
    e.w = SZ_W[type];
    e.h = SZ_H[type];
    int hp = (type == 0) ? 1 + level / 6 : (type == 1 ? 3 + level / 8 : 6 + level / 3);
    e.hp = hp;
    e.maxhp = hp;
    e.flash = 0;
    e.t = 0;
    e.mode = 0;
    e.nextFire = 0;
    return;
  }
}

void spawnEnemy(bool gruntOnly) {
  if (countEnemies() >= maxEnemies()) return;
  uint8_t type = 0;
  if (!gruntOnly) {
    float r  = random(1000) / 1000.0f;
    float ps = (level >= 2) ? min(0.5f, 0.15f + level * 0.05f) : 0.0f;
    float pt = (level >= 3) ? min(0.25f, 0.05f + level * 0.02f) : 0.0f;
    if (r < pt) type = 2;
    else if (r < pt + ps) type = 1;
  }
  int w = SZ_W[type];
  int h = SZ_H[type];
  float vy;
  if (type == 0)      vy = min(0.9f + level * 0.15f + random(0, 40) / 100.0f, 3.0f);
  else if (type == 1) vy = min(0.7f + level * 0.10f + random(0, 30) / 100.0f, 2.0f);
  else                vy = min(0.5f + level * 0.05f, 1.1f);
  float amp = (type == 2) ? random(0, 6) : random(0, 13);
  addEnemy(type, random(14, SCR_W - w - 14), -h, amp, vy, random(0, 628) / 100.0f);
}

void spawnWave() {                            // V-formation of 3 small ships
  if (countEnemies() + 3 > maxEnemies() + 2) return;
  float cx = random(32, 81);
  float vy = min(1.3f + level * 0.1f, 2.6f);
  addEnemy(0, cx - 30, -26, 0, vy, 0);
  addEnemy(0, cx,      -12, 0, vy, 0);
  addEnemy(0, cx + 30, -26, 0, vy, 0);
}

void spawnBoss(int lv) {
  for (int i = 0; i < MAX_EN; i++) {
    if (en[i].on) continue;
    Enemy &e = en[i];
    e.on = true;
    e.type = 3;
    e.w = 44;
    e.h = 28;
    e.x = (SCR_W - 44) / 2.0f;
    e.baseX = e.x;
    e.y = -30;
    e.vy = 0;
    e.phase = 0;
    e.amp = 0;
    e.t = 0;
    e.mode = 0;
    int hp = 60 + 25 * (lv / BOSS_EVERY - 1);
    e.hp = hp;
    e.maxhp = hp;
    e.flash = 0;
    e.nextFire = millis() + 1500;
    showPopup("WARNING! BOSS");
    return;
  }
}

void killEnemy(Enemy &e) {
  e.on = false;
  spawnBoom((int)e.x + e.w / 2, (int)e.y + e.h / 2);
  if (e.type == 3) {
    for (int i = 0; i < 8; i++)
      spawnBoom((int)e.x + random(0, e.w), (int)e.y + random(0, e.h));
    score += 200 + level * 5;
    showPopup("STAGE CLEAR!");
    for (int i = 0; i < MAX_EB; i++) eb[i].on = false;   // clear enemy bullets
    spawnItem((int)e.x + e.w / 2 - 15, (int)e.y + e.h, IT_WEAPON);
    spawnItem((int)e.x + e.w / 2 + 5,  (int)e.y + e.h, lives < MAX_LIVES ? IT_LIFE : IT_RAPID);
  } else {
    score += SCORE_BY[e.type];
    maybeDrop(e);
  }
}

// ===================== Game flow =====================
void resetGame() {
  px = SCR_W / 2;
  py = SCR_H - 30;
  score = 0;
  lives = START_LIVES;
  level = 1;
  weaponLevel = 1;
  rapidUntil = 0;
  invincible = 0;
  popupTimer = 0;
  nextBoss = BOSS_EVERY;
  spawnCount = 0;
  lastShot = lastSpawn = millis();
  for (int i = 0; i < MAX_PB; i++)    pb[i].on = false;
  for (int i = 0; i < MAX_EB; i++)    eb[i].on = false;
  for (int i = 0; i < MAX_EN; i++)    en[i].on = false;
  for (int i = 0; i < MAX_BOOM; i++)  booms[i].on = false;
  for (int i = 0; i < MAX_ITEMS; i++) items[i].on = false;
}

void hitPlayer() {
  lives--;
  if (weaponLevel > 1) weaponLevel--;       // weapon is damaged
  spawnBoom((int)px, (int)py + 6);
  invincible = 60;
  if (lives <= 0) {
    spawnBoom((int)px - 6, (int)py + 2);
    spawnBoom((int)px + 6, (int)py + 10);
    if (score > best) best = score;
    state = ST_OVER;
    overTime = millis();
    armed = false;
  }
}

void drawHUD() {
  canvas.setTextSize(1);

  // row 1: score, level, lives
  canvas.setTextColor(C_WHITE);
  canvas.setCursor(2, 2);
  canvas.print(score);
  canvas.setTextColor(bossAlive() ? C_RED : C_YELLOW);
  canvas.setCursor(54, 2);
  canvas.print("L");
  canvas.print(level);
  for (int i = 0; i < lives; i++) {
    int x = SCR_W - 7 - i * 9;
    canvas.fillTriangle(x, 2, x - 3, 9, x + 3, 9, C_CYAN);
  }

  // row 2: weapon level + rapid fire timer
  canvas.setTextColor(C_ORANGE);
  canvas.setCursor(2, 12);
  canvas.print("W");
  for (int i = 0; i < weaponLevel; i++) {
    canvas.fillRect(10 + i * 5, 13, 4, 6, C_ORANGE);
  }
  if (millis() < rapidUntil) {
    canvas.setTextColor(C_CYAN);
    canvas.setCursor(36, 12);
    canvas.print("R");
    int w = (int)((rapidUntil - millis()) * 20UL / RAPID_MS);
    canvas.fillRect(44, 13, w, 6, C_CYAN);
  }
}

void updatePlay() {
  unsigned long now = millis();

  // ---- input & movement ----
  float jx, jy;
  readJoy(jx, jy);
  px += jx * PLAYER_SPEED;
  py += jy * PLAYER_SPEED;
  px = constrain(px, 9.0f, (float)(SCR_W - 9));
  py = constrain(py, 70.0f, (float)(SCR_H - 18));

  // ---- auto fire ----
  unsigned long fd = (now < rapidUntil) ? (FIRE_DELAY / 2) : FIRE_DELAY;
  if (now - lastShot >= fd) {
    lastShot = now;
    firePlayer();
  }

  // ---- level, boss & spawning ----
  level = 1 + score / SCORE_PER_LEVEL;
  bool boss = bossAlive();
  if (level >= nextBoss && !boss) {
    spawnBoss(nextBoss);
    nextBoss += BOSS_EVERY;
    boss = true;
  }
  unsigned long interval = max(320, 1000 - (level - 1) * 90);
  if (boss) interval = interval * 5 / 2;
  if (now - lastSpawn >= interval) {
    lastSpawn = now;
    spawnCount++;
    if (!boss && level >= 3 && spawnCount % 6 == 0) spawnWave();
    else spawnEnemy(boss);
  }

  updateStars();

  // ---- player bullets ----
  for (int i = 0; i < MAX_PB; i++) {
    if (!pb[i].on) continue;
    pb[i].y -= 6;
    pb[i].x += pb[i].dx;
    if (pb[i].y < -6 || pb[i].x < -3 || pb[i].x > SCR_W + 1) pb[i].on = false;
  }

  // ---- enemies ----
  for (int i = 0; i < MAX_EN; i++) {
    Enemy &e = en[i];
    if (!e.on) continue;
    if (e.flash > 0) e.flash--;

    if (e.type == 3) {                        // boss movement & fire
      if (e.y < 22) {
        e.y += 1;
      } else {
        e.t += 1;
        e.x = (SCR_W - e.w) / 2.0f + sinf(e.t * 0.035f) * ((SCR_W - e.w) / 2.0f - 3);
        if (now >= e.nextFire) {
          bossFire(e);
          e.nextFire = now + ((e.hp < e.maxhp / 2) ? 700 : 1000);
          e.mode ^= 1;
        }
      }
      continue;
    }

    e.y += e.vy;
    e.phase += 0.07f;
    e.x = e.baseX + sinf(e.phase) * e.amp;
    if (e.y > SCR_H) { e.on = false; continue; }
    if ((e.type == 1 || e.type == 2) && e.y > 0 && e.y < SCR_H - 70) {
      float p = (e.type == 1 ? 12.0f : 10.0f) * (1.0f + level * 0.1f);
      if (random(1000) < p) enemyFire(e);
    }
  }

  // ---- bullet vs enemy ----
  for (int b = 0; b < MAX_PB; b++) {
    if (!pb[b].on) continue;
    for (int j = 0; j < MAX_EN; j++) {
      if (!en[j].on) continue;
      if (hit(pb[b].x, pb[b].y, 2, 5, (int)en[j].x, (int)en[j].y, en[j].w, en[j].h)) {
        pb[b].on = false;
        en[j].hp--;
        en[j].flash = 3;
        if (en[j].hp <= 0) killEnemy(en[j]);
        break;
      }
    }
  }

  // ---- enemy bullets ----
  for (int i = 0; i < MAX_EB; i++) {
    if (!eb[i].on) continue;
    eb[i].x += eb[i].dx;
    eb[i].y += eb[i].vy;
    if (eb[i].y > SCR_H || eb[i].x < -4 || eb[i].x > SCR_W + 4) { eb[i].on = false; continue; }
    if (invincible == 0 && hit((int)eb[i].x - 1, (int)eb[i].y, 3, 6, (int)px - 5, (int)py + 2, 11, 11)) {
      eb[i].on = false;
      hitPlayer();
      if (state != ST_PLAY) return;
    }
  }

  // ---- enemy vs player (ramming): enemy takes 3 damage, player loses a life ----
  if (invincible == 0) {
    for (int j = 0; j < MAX_EN; j++) {
      if (!en[j].on) continue;
      if (hit((int)px - 5, (int)py + 2, 11, 11, (int)en[j].x, (int)en[j].y, en[j].w, en[j].h)) {
        if (en[j].type != 3) {
          en[j].hp -= 3;
          en[j].flash = 3;
          if (en[j].hp <= 0) killEnemy(en[j]);
        }
        hitPlayer();
        if (state != ST_PLAY) return;
        break;
      }
    }
  }

  // ---- power-up items ----
  for (int i = 0; i < MAX_ITEMS; i++) {
    if (!items[i].on) continue;
    items[i].y += 1.3f;
    if (items[i].y > SCR_H) { items[i].on = false; continue; }
    if (hit((int)px - 6, (int)py, 13, 14, (int)items[i].x, (int)items[i].y, IS, IS)) {
      items[i].on = false;
      collectItem(items[i].type);
    }
  }

  if (invincible > 0) invincible--;
  if (popupTimer > 0) popupTimer--;
  updateBooms();
}

void drawPlay() {
  canvas.fillScreen(C_BLACK);
  drawStars();

  for (int i = 0; i < MAX_ITEMS; i++)
    if (items[i].on) drawItem((int)items[i].x, (int)items[i].y, items[i].type);

  for (int i = 0; i < MAX_EN; i++)
    if (en[i].on) drawEnemy(en[i]);

  for (int i = 0; i < MAX_PB; i++)
    if (pb[i].on) canvas.fillRect(pb[i].x, pb[i].y, 2, 5, C_YELLOW);

  for (int i = 0; i < MAX_EB; i++)
    if (eb[i].on) canvas.fillRect((int)eb[i].x - 1, (int)eb[i].y, 3, 6, eb[i].col);

  if (state == ST_PLAY && (invincible == 0 || ((invincible / 4) % 2 == 0)))
    drawPlayer((int)px, (int)py);

  drawBooms();
  drawHUD();

  if (popupTimer > 0 && (popupTimer > 12 || (popupTimer / 3) % 2 == 0))
    printCentered(popupMsg, 62, 1, C_YELLOW);
}

// ===================== Screens =====================
void doTitle() {
  updateStars();
  canvas.fillScreen(C_BLACK);
  drawStars();
  printCentered("SPACE", 22, 2, C_CYAN);
  printCentered("SHOOTER", 42, 2, C_YELLOW);
  drawPlayer(SCR_W / 2, 80);
  drawItem(34, 108, IT_WEAPON);
  drawItem(59, 108, IT_LIFE);
  drawItem(84, 108, IT_RAPID);
  if ((frameCount / 15) % 2 == 0) printCentered("PRESS BUTTON", 132, 1, C_WHITE);
  if (best > 0) {
    char buf[24];
    snprintf(buf, sizeof(buf), "BEST: %d", best);
    printCentered(buf, 146, 1, C_GRAY);
  }

  bool a = actionPressed();
  if (!a) armed = true;
  if (armed && a) {
    resetGame();
    state = ST_PLAY;
  }
}

void doOver() {
  updateStars();
  updateBooms();
  canvas.fillScreen(C_BLACK);
  drawStars();
  drawBooms();

  char buf[24];
  printCentered("GAME OVER", 38, 2, C_RED);
  snprintf(buf, sizeof(buf), "SCORE: %d", score);
  printCentered(buf, 76, 1, C_WHITE);
  snprintf(buf, sizeof(buf), "LEVEL: %d", level);
  printCentered(buf, 88, 1, C_YELLOW);
  snprintf(buf, sizeof(buf), "BEST: %d", best);
  printCentered(buf, 100, 1, C_GRAY);
  if ((frameCount / 15) % 2 == 0) printCentered("PRESS BUTTON", 130, 1, C_WHITE);

  bool a = actionPressed();
  if (!a) armed = true;
  if (armed && a && millis() - overTime > 1000) {
    resetGame();
    state = ST_PLAY;
  }
}

// ===================== Arduino =====================
void setup() {
  pinMode(JOY_SW, INPUT_PULLUP);
  analogReadResolution(12);

  tft.initR(INITR_BLACKTAB);   // if colors/position look wrong try INITR_GREENTAB or INITR_REDTAB
  tft.setRotation(0);          // portrait 128x160
  tft.fillScreen(C_BLACK);

  if (canvas.getBuffer() == NULL) {
    tft.setTextColor(C_RED);
    tft.setCursor(0, 0);
    tft.print("Not enough memory!");
    while (true) delay(1000);
  }

  calibrateJoy();              // keep the joystick centered at power-up
  randomSeed(micros());
  initStars();
  resetGame();
  state = ST_TITLE;
}

void loop() {
  unsigned long now = millis();
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;
  frameCount++;

  switch (state) {
    case ST_TITLE:
      doTitle();
      break;
    case ST_PLAY:
      updatePlay();
      drawPlay();
      break;
    case ST_OVER:
      doOver();
      break;
  }

  pushFrame();
}
