#pragma once
#include <Arduino.h>

// Second screen: a small I2C OLED showing battery level, styled like the Zeus x27
// charge meter. Reads state-of-charge from a MAX17048 LiPo fuel gauge on the same
// I2C bus (SDA=I2C_SDA, SCL=I2C_SCL). Both are optional at runtime -- a missing
// device just degrades (OLED shows a placeholder, no crash).
namespace Gauge {
  void begin();     // Wire + OLED + fuel gauge init
  void update();    // read charge and redraw; call periodically (cheap)

  // Daily 2137 clip: show "21:37" in the 7-seg style instead of the battery.
  // The next update() repaints the battery, so no explicit restore needed.
  void showClock2137();
  // Hardware invert of the whole panel (single I2C command; cheap enough to
  // toggle on every music beat). Remember to turn it off after the clip.
  void setInvert(bool on);
}
