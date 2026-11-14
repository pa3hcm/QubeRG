// Host stand-in for Adafruit_SSD1306: draws into a 128x64 pixel buffer
// using the real Adafruit GFX 5x7 font (glcdfont.c).
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include "glcdfont.c"
#define SSD1306_WHITE 1
#define SSD1306_BLACK 0
#define SSD1306_SWITCHCAPVCC 2
struct TwoWire;
struct Frame { uint8_t px[64][128]; };
extern Frame g_last;
extern std::vector<Frame> g_frames;
struct Adafruit_SSD1306 {
  Frame f{};
  int cx = 0, cy = 0, ts = 1;
  Adafruit_SSD1306(int, int, TwoWire *, int) {}
  bool begin(int, int) { return true; }
  void clearDisplay() { memset(f.px, 0, sizeof(f.px)); }
  void setTextColor(int) {}
  void setTextSize(int s) { ts = s; }
  void setCursor(int x, int y) { cx = x; cy = y; }
  void fillRect(int x, int y, int w, int h, int c) {
    for (int j = y; j < y + h; j++)
      for (int i = x; i < x + w; i++)
        if (i >= 0 && i < 128 && j >= 0 && j < 64) f.px[j][i] = c;
  }
  void drawChar(int x, int y, unsigned char c, int s) {
    for (int i = 0; i < 5; i++) {
      uint8_t line = font[c * 5 + i];
      for (int j = 0; j < 8; j++, line >>= 1)
        if (line & 1) fillRect(x + i * s, y + j * s, s, s, 1);
    }
  }
  void print(const char *str) {
    for (; *str; str++) { drawChar(cx, cy, *str, ts); cx += 6 * ts; }
  }
  void display() { g_last = f; g_frames.push_back(f); }
};
