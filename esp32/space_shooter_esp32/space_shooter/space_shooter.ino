/*
  Space Shooter
  ESP32-WROOM-32 + TFT 1.8" ST7735 (128x160) + Joystick HW-504

  - Joystick: move the ship (left/right/up/down)
  - Auto-fire: always shooting
  - Button (SW): start / restart
  - Drawing is done in an off-screen buffer (canvas), so there is no flicker.
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
#define FIRE_DELAY    170     // ms between shots
#define FRAME_MS      33      // ~30 FPS
#define START_LIVES   3

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

// ===================== Game objects =====================
#define MAX_PB     10   // player bullets
#define MAX_EN      7   // enemies
#define MAX_EB      8   // enemy bullets
#define MAX_BOOM    8   // explosions
#define MAX_STARS  32

const int EW = 12;      // enemy width
const int EH = 10;      // enemy height

struct Bullet { int16_t x, y; bool on; };
struct Enemy  { float x, y, baseX, vy, phase, amp; uint8_t type; int8_t hp; bool on; };
struct Star   { int16_t x, y; uint8_t spd; };
struct Boom   { int16_t x, y; uint8_t t; bool on; };

Bullet pb[MAX_PB];
Bullet eb[MAX_EB];
Enemy  en[MAX_EN];
Boom   booms[MAX_BOOM];
Star   stars[MAX_STARS];

enum GameState { ST_TITLE, ST_PLAY, ST_OVER };
GameState state = ST_TITLE;

float px, py;                 // ship position (px = center x, py = top y)
int   score = 0, best = 0, lives = START_LIVES, level = 1;
int   invincible = 0;
unsigned long lastShot = 0, lastSpawn = 0, lastFrame = 0, overTime = 0;
uint32_t frameCount = 0;
bool  armed = false;          // input must be released before start/restart

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

// ===================== Drawing ships =====================
void drawPlayer(int x, int y) {
  canvas.fillTriangle(x, y, x - 6, y + 12, x + 6, y + 12, C_CYAN);   // body
  canvas.fillRect(x - 1, y + 4, 3, 6, C_WHITE);                      // cockpit
  canvas.fillRect(x - 8, y + 8, 3, 5, C_BLUE);                       // left wing
  canvas.fillRect(x + 6, y + 8, 3, 5, C_BLUE);                       // right wing
  if (frameCount & 1) canvas.fillTriangle(x - 2, y + 13, x + 2, y + 13, x, y + 17, C_ORANGE);
  else                canvas.fillTriangle(x - 1, y + 13, x + 1, y + 13, x, y + 15, C_YELLOW);
}

void drawEnemy(int x, int y, uint8_t type) {
  uint16_t c = (type == 0) ? C_RED : C_MAGENTA;
  canvas.fillRect(x + 2, y, 8, 6, c);
  canvas.fillRect(x, y + 2, 12, 4, c);
  canvas.fillRect(x + 2, y + 6, 2, 4, c);
  canvas.fillRect(x + 8, y + 6, 2, 4, c);
  canvas.fillRect(x + 3, y + 2, 2, 2, C_WHITE);
  canvas.fillRect(x + 7, y + 2, 2, 2, C_WHITE);
}

// ===================== Bullets =====================
void addPlayerBullet(int x, int y) {
  for (int i = 0; i < MAX_PB; i++) {
    if (!pb[i].on) {
      pb[i].x = x;
      pb[i].y = y;
      pb[i].on = true;
      return;
    }
  }
}

void firePlayer() {
  if (level >= 3) {                       // double shot from level 3
    addPlayerBullet((int)px - 6, (int)py + 4);
    addPlayerBullet((int)px + 5, (int)py + 4);
  } else {
    addPlayerBullet((int)px - 1, (int)py);
  }
}

void enemyShoot(Enemy &e) {
  for (int i = 0; i < MAX_EB; i++) {
    if (!eb[i].on) {
      eb[i].x = (int)e.x + EW / 2 - 1;
      eb[i].y = (int)e.y + EH;
      eb[i].on = true;
      return;
    }
  }
}

// ===================== Enemies =====================
void spawnEnemy() {
  for (int i = 0; i < MAX_EN; i++) {
    if (!en[i].on) {
      en[i].on = true;
      en[i].baseX = random(12, SCR_W - EW - 12);
      en[i].x = en[i].baseX;
      en[i].y = -EH;
      en[i].phase = random(0, 628) / 100.0f;
      en[i].amp = random(0, 13);
      en[i].type = (level >= 2 && random(100) < 30) ? 1 : 0;
      en[i].hp = (en[i].type == 1) ? 2 : 1;
      float v = 0.8f + level * 0.15f + random(0, 40) / 100.0f;
      en[i].vy = min(v, 2.4f);
      return;
    }
  }
}

// ===================== Game flow =====================
void resetGame() {
  px = SCR_W / 2;
  py = SCR_H - 30;
  score = 0;
  lives = START_LIVES;
  level = 1;
  invincible = 0;
  lastShot = lastSpawn = millis();
  for (int i = 0; i < MAX_PB; i++)   pb[i].on = false;
  for (int i = 0; i < MAX_EB; i++)   eb[i].on = false;
  for (int i = 0; i < MAX_EN; i++)   en[i].on = false;
  for (int i = 0; i < MAX_BOOM; i++) booms[i].on = false;
}

void hitPlayer() {
  lives--;
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
  canvas.setTextColor(C_WHITE);
  canvas.setCursor(2, 2);
  canvas.print(score);
  canvas.setTextColor(C_YELLOW);
  canvas.setCursor(54, 2);
  canvas.print("L");
  canvas.print(level);
  for (int i = 0; i < lives; i++) {
    int x = SCR_W - 7 - i * 9;
    canvas.fillTriangle(x, 2, x - 3, 9, x + 3, 9, C_CYAN);
  }
}

void updatePlay() {
  // ---- input & movement ----
  float jx, jy;
  readJoy(jx, jy);
  px += jx * PLAYER_SPEED;
  py += jy * PLAYER_SPEED;
  px = constrain(px, 9.0f, (float)(SCR_W - 9));
  py = constrain(py, 70.0f, (float)(SCR_H - 18));

  // ---- auto fire ----
  if (millis() - lastShot >= FIRE_DELAY) {
    lastShot = millis();
    firePlayer();
  }

  // ---- level & spawning ----
  level = 1 + score / 150;
  unsigned long interval = max(400, 1200 - (level - 1) * 110);
  if (millis() - lastSpawn >= interval) {
    lastSpawn = millis();
    spawnEnemy();
  }

  updateStars();

  // ---- player bullets ----
  for (int i = 0; i < MAX_PB; i++) {
    if (!pb[i].on) continue;
    pb[i].y -= 6;
    if (pb[i].y < -6) pb[i].on = false;
  }

  // ---- enemies ----
  for (int i = 0; i < MAX_EN; i++) {
    if (!en[i].on) continue;
    en[i].y += en[i].vy;
    en[i].phase += 0.07f;
    en[i].x = en[i].baseX + sinf(en[i].phase) * en[i].amp;
    if (en[i].y > SCR_H) { en[i].on = false; continue; }
    if (en[i].type == 1 && en[i].y > 0 && en[i].y < SCR_H - 70 && random(1000) < 12) {
      enemyShoot(en[i]);
    }
  }

  // ---- bullet vs enemy ----
  for (int b = 0; b < MAX_PB; b++) {
    if (!pb[b].on) continue;
    for (int e = 0; e < MAX_EN; e++) {
      if (!en[e].on) continue;
      if (hit(pb[b].x, pb[b].y, 2, 5, (int)en[e].x, (int)en[e].y, EW, EH)) {
        pb[b].on = false;
        en[e].hp--;
        if (en[e].hp <= 0) {
          en[e].on = false;
          spawnBoom((int)en[e].x + EW / 2, (int)en[e].y + EH / 2);
          score += (en[e].type == 1) ? 20 : 10;
        }
        break;
      }
    }
  }

  // ---- enemy bullets ----
  for (int i = 0; i < MAX_EB; i++) {
    if (!eb[i].on) continue;
    eb[i].y += 3;
    if (eb[i].y > SCR_H) { eb[i].on = false; continue; }
    if (invincible == 0 && hit(eb[i].x, eb[i].y, 2, 5, (int)px - 5, (int)py + 2, 11, 11)) {
      eb[i].on = false;
      hitPlayer();
      if (state != ST_PLAY) return;
    }
  }

  // ---- enemy vs player ----
  if (invincible == 0) {
    for (int e = 0; e < MAX_EN; e++) {
      if (!en[e].on) continue;
      if (hit((int)px - 5, (int)py + 2, 11, 11, (int)en[e].x, (int)en[e].y, EW, EH)) {
        en[e].on = false;
        spawnBoom((int)en[e].x + EW / 2, (int)en[e].y + EH / 2);
        hitPlayer();
        break;
      }
    }
  }

  if (invincible > 0) invincible--;
  updateBooms();
}

void drawPlay() {
  canvas.fillScreen(C_BLACK);
  drawStars();

  for (int i = 0; i < MAX_EN; i++)
    if (en[i].on) drawEnemy((int)en[i].x, (int)en[i].y, en[i].type);

  for (int i = 0; i < MAX_PB; i++)
    if (pb[i].on) canvas.fillRect(pb[i].x, pb[i].y, 2, 5, C_YELLOW);

  for (int i = 0; i < MAX_EB; i++)
    if (eb[i].on) canvas.fillRect(eb[i].x, eb[i].y, 2, 5, C_GREEN);

  if (state == ST_PLAY && (invincible == 0 || ((invincible / 4) % 2 == 0)))
    drawPlayer((int)px, (int)py);

  drawBooms();
  drawHUD();
}

// ===================== Screens =====================
void doTitle() {
  updateStars();
  canvas.fillScreen(C_BLACK);
  drawStars();
  printCentered("SPACE", 22, 2, C_CYAN);
  printCentered("SHOOTER", 42, 2, C_YELLOW);
  drawPlayer(SCR_W / 2, 85);
  if ((frameCount / 15) % 2 == 0) printCentered("PRESS BUTTON", 130, 1, C_WHITE);
  if (best > 0) {
    char buf[24];
    snprintf(buf, sizeof(buf), "BEST: %d", best);
    printCentered(buf, 145, 1, C_GRAY);
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
  printCentered("GAME OVER", 40, 2, C_RED);
  snprintf(buf, sizeof(buf), "SCORE: %d", score);
  printCentered(buf, 80, 1, C_WHITE);
  snprintf(buf, sizeof(buf), "BEST: %d", best);
  printCentered(buf, 94, 1, C_YELLOW);
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
