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
  float rate = gauge.chargeRate();     // %/hr; >0 = charging
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  bool charging = rate > 1.0f;

  // Battery outline (58 wide) with a nub + proportional fill.
  oled.drawFrame(0, 0, 58, 18);
  oled.drawBox(58, 6, 4, 6);                       // + terminal nub
  int fillw = (int)((58 - 4) * (pct / 100.0f));
  if (fillw > 0) oled.drawBox(2, 2, fillw, 14);

  // Charging marker, top-right.
  if (charging) {
    oled.setFont(u8g2_font_6x10_tf);
    oled.drawStr(64, 11, "+");
  }

  // Big percent below the bar.
  char buf[8];
  snprintf(buf, sizeof(buf), "%d%%", (int)(pct + 0.5f));
  oled.setFont(u8g2_font_ncenB14_tr);
  oled.drawStr(2, 38, buf);

  oled.sendBuffer();
}
