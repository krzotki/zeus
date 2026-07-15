#pragma once
#include <Arduino.h>

// Small color TFT (ST7735 160x80). Mimics the in-game StatTrak look:
// orange label + big orange counter on black.
namespace Display {
  void begin();
  void showCount(const String& name, int32_t value);  // main StatTrak readout
  void showStatus(const String& msg);                 // transient status line
  void showSetup(const String& apName, const String& ip);
  void showError(const String& msg);
  void clear();                                       // fill screen black
  // Raw RGB565 block push (used by the GIF player, one scanline at a time).
  void blit(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* px);
}
