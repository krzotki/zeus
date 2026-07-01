#pragma once
#include <Arduino.h>

// Second screen: a small I2C OLED showing battery level, styled like the Zeus x27
// charge meter. Reads state-of-charge from a MAX17048 LiPo fuel gauge on the same
// I2C bus (SDA=I2C_SDA, SCL=I2C_SCL). Both are optional at runtime -- a missing
// device just degrades (OLED shows a placeholder, no crash).
namespace Gauge {
  void begin();     // Wire + OLED + fuel gauge init
  void update();    // read charge and redraw; call periodically (cheap)
}
