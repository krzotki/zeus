#include "Gauge.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MAX1704X.h>

// I2C pins from platformio.ini (reclaimed from TFT backlight/reset).
#ifndef I2C_SDA
#define I2C_SDA 1
#endif
#ifndef I2C_SCL
#define I2C_SCL 4
#endif

namespace {
const uint8_t OLED_ADDR = 0x3C;
const int OLED_W = 128, OLED_H = 64;

Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
Adafruit_MAX17048 gauge;

bool oledOk  = false;
bool gaugeOk = false;

void drawNoGauge() {
  oled.setTextSize(2);
  oled.setCursor(30, 24);
  oled.print("USB");
}
}  // namespace

void Gauge::begin() {
  Wire.begin(I2C_SDA, I2C_SCL);

  oledOk = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!oledOk) { Serial.println(F("[gauge] OLED not found @0x3C")); }

  gaugeOk = gauge.begin(&Wire);
  if (!gaugeOk) { Serial.println(F("[gauge] MAX17048 not found @0x36 (USB-only?)")); }

  if (oledOk) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.display();
  }
}

void Gauge::update() {
  if (!oledOk) return;

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // Title row.
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(F("CHARGE"));

  if (!gaugeOk) {
    drawNoGauge();
    oled.display();
    return;
  }

  float pct  = gauge.cellPercent();
  float volts = gauge.cellVoltage();
  float rate = gauge.chargeRate();     // %/hr; >0 = charging
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  bool charging = rate > 1.0f;

  if (charging) { oled.setCursor(104, 0); oled.print(F("CHG")); }

  // Battery outline with a nub, proportional fill.
  const int bx = 0, by = 14, bw = 118, bh = 24;
  oled.drawRect(bx, by, bw, bh, SSD1306_WHITE);
  oled.fillRect(bx + bw, by + 7, 6, bh - 14, SSD1306_WHITE);       // + terminal nub
  int fillw = (int)((bw - 4) * (pct / 100.0f));
  oled.fillRect(bx + 2, by + 2, fillw, bh - 4, SSD1306_WHITE);

  // Big percent + voltage below the bar.
  oled.setTextSize(2);
  oled.setCursor(0, 46);
  oled.print((int)(pct + 0.5f));
  oled.print('%');

  oled.setTextSize(1);
  oled.setCursor(78, 52);
  oled.print(volts, 2);
  oled.print('V');

  oled.display();
}
