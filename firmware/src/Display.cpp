#include "Display.h"
#include <TFT_eSPI.h>

namespace {
TFT_eSPI tft;

// StatTrak palette
uint16_t ORANGE;  // StatTrak amber/orange
uint16_t DIM;     // muted label
const uint16_t BG = TFT_BLACK;

int W, H;

void header() {
  tft.fillScreen(BG);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(ORANGE, BG);
  tft.drawString("StatTrak", 3, 2, 2);
}
}  // namespace

void Display::begin() {
  tft.init();
  tft.setRotation(3);  // 160x80 landscape; try 1/3 to flip to match mounting
  W = tft.width();
  H = tft.height();
  ORANGE = tft.color565(0xE8, 0x9B, 0x3B);
  DIM    = tft.color565(0x80, 0x70, 0x55);
  tft.fillScreen(BG);
}

void Display::showCount(const String& name, int32_t value) {
  header();

  // Big counter, 7-segment font (font 7) for that hardware-counter feel.
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(ORANGE, BG);
  String num = (value < 0) ? String("----") : String(value);
  // Font 7 is wide; drop to font 4 if the number won't fit.
  int fw = tft.textWidth(num, 7);
  int font = (fw <= W - 6) ? 7 : 4;
  tft.drawString(num, W / 2, H / 2 + 4, font);

  // Item name footer.
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(DIM, BG);
  tft.drawString(name, W / 2, H - 1, 1);
}

void Display::showStatus(const String& msg) {
  header();
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(DIM, BG);
  tft.drawString(msg, W / 2, H / 2, 2);
}

void Display::showError(const String& msg) {
  header();
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(tft.color565(0xC0, 0x40, 0x30), BG);
  tft.drawString(msg, W / 2, H / 2, 2);
}

void Display::showSetup(const String& apName, const String& ip) {
  tft.fillScreen(BG);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(ORANGE, BG);
  tft.drawString("WiFi Setup", W / 2, 4, 2);
  tft.setTextColor(TFT_WHITE, BG);
  tft.drawString("Join WiFi:", W / 2, 26, 1);
  tft.setTextColor(ORANGE, BG);
  tft.drawString(apName, W / 2, 38, 2);
  tft.setTextColor(DIM, BG);
  tft.drawString("then open " + ip, W / 2, 60, 1);
}
