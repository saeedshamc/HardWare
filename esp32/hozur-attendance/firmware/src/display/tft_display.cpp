#include "tft_display.h"

#include "config.h"

#include <SPI.h>
#include <TFT_eSPI.h>

static TFT_eSPI tft;

void TftDisplay::begin() {
  // Keep RC522 CS idle high while TFT uses HSPI
  pinMode(PIN_RC522_SS, OUTPUT);
  digitalWrite(PIN_RC522_SS, HIGH);

  tft.init();
  tft.setRotation(0);  // 240x320 portrait
  tft.fillScreen(COL_BG);
  tft.setTextDatum(MC_DATUM);
  ready_ = true;
}

void TftDisplay::fillScreen(uint16_t color) {
  if (!ready_) return;
  tft.fillScreen(color);
}

void TftDisplay::drawHeader(const char* title, uint16_t bg, uint16_t fg) {
  if (!ready_) return;
  tft.fillRect(0, 0, TFT_W, 36, bg);
  tft.setTextColor(fg, bg);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(title ? title : "", TFT_W / 2, 18, 2);
}

void TftDisplay::drawCentered(const char* text, int16_t y, uint16_t color, uint8_t size) {
  if (!ready_ || !text) return;
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(color, COL_BG);
  tft.setTextSize(size);
  tft.drawString(text, TFT_W / 2, y, size >= 2 ? 4 : 2);
  tft.setTextSize(1);
}

void TftDisplay::drawStatusLine(const char* text, uint16_t color) {
  if (!ready_) return;
  tft.fillRect(0, TFT_H - 28, TFT_W, 28, COL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(color, COL_BG);
  tft.drawString(text ? text : "", TFT_W / 2, TFT_H - 14, 2);
}

void TftDisplay::drawRgb565Bitmap(int16_t x, int16_t y, uint16_t w, uint16_t h,
                                  const uint16_t* pixels) {
  if (!ready_ || !pixels || w == 0 || h == 0) return;
  tft.pushImage(x, y, w, h, pixels);
}

void TftDisplay::drawMonoBitmap(int16_t x, int16_t y, uint16_t w, uint16_t h,
                                const uint8_t* data, uint16_t fg, uint16_t bg) {
  if (!ready_ || !data) return;
  const uint16_t rowBytes = (w + 7) / 8;
  for (uint16_t row = 0; row < h; ++row) {
    for (uint16_t col = 0; col < w; ++col) {
      const uint8_t byte = data[row * rowBytes + (col / 8)];
      const bool on = byte & (0x80 >> (col & 7));
      tft.drawPixel(x + col, y + row, on ? fg : bg);
    }
  }
}
