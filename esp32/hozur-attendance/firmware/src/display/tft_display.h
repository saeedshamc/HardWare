#pragma once

#include <Arduino.h>
#include <stdint.h>

enum class UiTheme : uint8_t { Idle, Success, Denied, Duplicate };

class TftDisplay {
 public:
  void begin();

  void fillScreen(uint16_t color);
  void drawHeader(const char* title, uint16_t bg, uint16_t fg);
  void drawCentered(const char* text, int16_t y, uint16_t color, uint8_t size = 2);
  void drawStatusLine(const char* text, uint16_t color);

  /** Draw pre-rendered RGB565 bitmap (row-major, big-endian bytes in stream as LE pixels). */
  void drawRgb565Bitmap(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint16_t* pixels);

  /** Draw 1-bit packed MSB-first monochrome bitmap with fg/bg. */
  void drawMonoBitmap(int16_t x, int16_t y, uint16_t w, uint16_t h,
                      const uint8_t* data, uint16_t fg, uint16_t bg);

  static constexpr uint16_t COL_BG     = 0x10A2;
  static constexpr uint16_t COL_GREEN  = 0x07E0;
  static constexpr uint16_t COL_RED    = 0xF800;
  static constexpr uint16_t COL_YELLOW = 0xFFE0;
  static constexpr uint16_t COL_WHITE  = 0xFFFF;
  static constexpr uint16_t COL_GRAY   = 0x8410;
  static constexpr uint16_t COL_BLUE   = 0x001F;

 private:
  bool ready_ = false;
};
