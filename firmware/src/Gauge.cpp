#include "Gauge.h"
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_MAX1704X.h>

// I2C pins from platformio.ini (reclaimed from TFT backlight/reset).
#ifndef I2C_SDA
#define I2C_SDA 1
#endif
#ifndef I2C_SCL
#define I2C_SCL 4
#endif

namespace {
// 0.42" 72x40 SSD1306. This U8g2 profile bakes in the panel's column/row
// offset (the 72x40 window sits inside the controller's 128x64 RAM), so we
// draw in plain 0..71 / 0..39 coords. Full-buffer = 360 bytes.
U8G2_SSD1306_72X40_ER_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
Adafruit_MAX17048 gauge;

bool oledOk  = false;
bool gaugeOk = false;

// 7-segment digit patterns (bit order: a,b,c,d,e,f,g). Mirrors the StatTrak
// counter look from Display.cpp, redrawn by hand so it fits the 40px OLED.
//   aaa
//  f   b
//  f   b
//   ggg
//  e   c
//  e   c
//   ddd
const uint8_t kSeg7[10] = {
  0b0111111,  // 0: a b c d e f
  0b0000110,  // 1: b c
  0b1011011,  // 2: a b d e g
  0b1001111,  // 3: a b c d g
  0b1100110,  // 4: b c f g
  0b1101101,  // 5: a c d f g
  0b1111101,  // 6: a c d e f g
  0b0000111,  // 7: a b c
  0b1111111,  // 8: all
  0b1101111,  // 9: a b c d f g
};

// Lightning bolt, 8x18 XBM (LSB = leftmost pixel). Drawn XOR over the battery
// slices while charging.
const uint8_t kBolt[] = {
  0x30, 0x18, 0x18, 0x0C, 0x0C, 0x06, 0xFF, 0x7F, 0x60,
  0x30, 0x30, 0x18, 0x18, 0x0C, 0x0C, 0x06, 0x06, 0x03,
};

// Draw one 7-seg digit in a w x h cell at (x,y) with stroke thickness t. Only
// lit segments are drawn (filled boxes) -- no ghost/unlit outlines.
void drawSeg7(int x, int y, int w, int h, int t, int digit) {
  if (digit < 0 || digit > 9) return;
  const uint8_t m     = kSeg7[digit];
  const int     halfH = h / 2;
  const int     vh    = halfH - (3 * t) / 2;   // vertical segment length
  const int     hw    = w - 2 * t;             // horizontal segment length
  const int     seg[7][4] = {
    { x + t,     y,                 hw, t  },   // a  top
    { x + w - t, y + t,             t,  vh },   // b  top-right
    { x + w - t, y + halfH + t / 2, t,  vh },   // c  bottom-right
    { x + t,     y + h - t,         hw, t  },   // d  bottom
    { x,         y + halfH + t / 2, t,  vh },   // e  bottom-left
    { x,         y + t,             t,  vh },   // f  top-left
    { x + t,     y + halfH - t / 2, hw, t  },   // g  middle
  };
  for (int s = 0; s < 7; s++) {
    if (m & (1 << s)) oled.drawBox(seg[s][0], seg[s][1], seg[s][2], seg[s][3]);
  }
}
}  // namespace

void Gauge::begin() {
  // Set the bus pins first; U8g2/Adafruit call Wire.begin() with no args on
  // ESP32, which preserves already-set pins (keeps us on GPIO1/GPIO4).
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  oled.setI2CAddress(0x3C << 1);
  oled.begin();                        // U8g2 has no presence check; assume present
  oledOk = true;

  gaugeOk = gauge.begin(&Wire);
  if (!gaugeOk) Serial.println(F("[gauge] MAX17048 not found @0x36 (USB-only?)"));
}

// "21:37" full-screen in the same 7-seg style as the percent digits. Shown for
// the daily clip; the next update() naturally repaints the battery over it.
void Gauge::showClock2137() {
  if (!oledOk) return;
  oled.clearBuffer();

  const int dh = 26, dt = 2, dy = 7, gap = 3;
  const int wWide = 10, wOne = 5, wColon = 3;
  // 2 1 : 3 7  ->  total width, centered on the 72px panel
  const int total = wWide + gap + wOne + gap + wColon + gap + wWide + gap + wWide;
  int x = (72 - total) / 2;

  drawSeg7(x, dy, wWide, dh, dt, 2);  x += wWide + gap;
  drawSeg7(x, dy, wOne,  dh, dt, 1);  x += wOne + gap;
  oled.drawBox(x, dy + dh / 2 - 7, wColon, 3);   // colon: two dots
  oled.drawBox(x, dy + dh / 2 + 4, wColon, 3);
  x += wColon + gap;
  drawSeg7(x, dy, wWide, dh, dt, 3);  x += wWide + gap;
  drawSeg7(x, dy, wWide, dh, dt, 7);

  oled.sendBuffer();
}

// SSD1306 hardware invert (0xA7 on / 0xA6 off): one command byte over I2C,
// flips the panel instantly without touching the frame buffer.
void Gauge::setInvert(bool on) {
  if (!oledOk) return;
  oled.sendF("c", on ? 0x0a7 : 0x0a6);
}

void Gauge::update() {
  if (!oledOk) return;

  oled.clearBuffer();

  if (!gaugeOk) {
    // No fuel gauge -> running on USB (or gauge unwired).
    oled.setFont(u8g2_font_ncenB14_tr);
    oled.drawStr(16, 30, "USB");
    oled.sendBuffer();
    return;
  }

  float pct  = gauge.cellPercent();
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;

  // Smooth the percent (EMA) so the voltage "jump" when USB is plugged in eases
  // in over a few seconds instead of snapping. ~1s refresh -> ~5s settle.
  static float pctDisp = -1.0f;
  if (pctDisp < 0) pctDisp = pct;                  // first reading: seed exactly
  else             pctDisp += (pct - pctDisp) * 0.2f;
  pct = pctDisp;

  // ---- Vertical battery (left) -------------------------------------------
  const int bx = 2, by = 4, bw = 30, bh = 34;    // body (wide: '1' digit is thin)
  oled.drawBox((bx + (bw - 10) / 2), by - 3, 10, 3);   // top cap/terminal
  oled.drawFrame(bx, by, bw, bh);                      // body outline

  // 10 horizontal slices, lit from the bottom up in proportion to %.
  const int segs   = 10;
  const int innerX = bx + 2, innerW = bw - 4;    // 26 wide
  const int innerBottom = by + bh - 2;           // just inside the frame
  const int sh   = 2, gap = 1;                   // slice height + gap
  int lit = (int)(pct / 100.0f * segs + 0.5f);   // round to nearest 10%
  for (int i = 0; i < lit; i++) {
    int sy = innerBottom - sh - i * (sh + gap);
    oled.drawBox(innerX, sy, innerW, sh);
  }

  // Bolt: solid white, centered. The 1px black halo is drawn ONLY over the
  // filled (lit) region, so the bolt gets a separating border where bars are
  // behind it, but stays a plain white glyph where the battery is empty.
  {
    const int boltX = bx + (bw - 8) / 2;
    const int boltY = by + (bh - 18) / 2;
    if (lit > 0) {
      int fillTop = innerBottom - sh - (lit - 1) * (sh + gap);
      oled.setClipWindow(innerX, fillTop, innerX + innerW, innerBottom + 1);
      oled.setDrawColor(0);                        // black padding, filled area only
      for (int ox = -1; ox <= 1; ox++)
        for (int oy = -1; oy <= 1; oy++)
          if (ox || oy) oled.drawXBM(boltX + ox, boltY + oy, 8, 18, kBolt);
      oled.setDrawColor(1);
      oled.setMaxClipWindow();                     // restore full drawing area
    }
    oled.drawXBM(boltX, boltY, 8, 18, kBolt);      // white bolt on top
  }

  // ---- Percentage in StatTrak 7-seg digits (right) -----------------------
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", (int)(pct + 0.5f));
  const int dh = 22, dt = 2, dgap = 2, dy = 9;
  const int rightEdge = 64;                        // digits end here; % follows
  const int n = (int)strlen(buf);
  auto dwOf = [](char c) { return c == '1' ? 5 : 10; };   // '1' needs less room
  int total = 0;
  for (int i = 0; i < n; i++) total += dwOf(buf[i]) + (i ? dgap : 0);
  int dx = rightEdge - total;
  for (int i = 0; i < n; i++) {
    int w = dwOf(buf[i]);
    drawSeg7(dx, dy, w, dh, dt, buf[i] - '0');
    dx += w + dgap;
  }
  oled.setFont(u8g2_font_5x7_tr);                  // small "%" sign
  oled.drawStr(66, 18, "%");

  oled.sendBuffer();
}
