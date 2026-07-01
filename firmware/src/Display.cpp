#include "Display.h"
#include <TFT_eSPI.h>

namespace {
TFT_eSPI tft;

// StatTrak palette
uint16_t ORANGE;  // StatTrak amber/orange (label)
uint16_t LIT;     // lit 7-seg digit (bright amber)
uint16_t GHOST;   // unlit 7-seg segment (faint amber)
uint16_t DIM;     // muted label
const uint16_t BG = TFT_BLACK;

const int   MAX_CELLS = 6;   // most digit cells to fill with ghost segments

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
  LIT    = tft.color565(0xFF, 0xA8, 0x2A);  // bright lit segment
  GHOST  = tft.color565(0x3A, 0x22, 0x06);  // faint unlit segment
  DIM    = tft.color565(0x80, 0x70, 0x55);
  tft.fillScreen(BG);
}

// StatTrak module look: bright amber digits over faint "unlit" ghost segments,
// right-aligned like a hardware counter, with a "StatTrak(TM)" label plate below.
void Display::showCount(const String& name, int32_t value) {
  (void)name;   // label is the fixed "StatTrak" plate, not the item name
  tft.fillScreen(BG);

  const int F = 7;                            // 48px 7-segment font
  const int digitW = tft.textWidth("8", F);   // fixed-width font
  const int gap = 3;
  const int cell = digitW + gap;

  String num = (value < 0) ? String("") : String(value);

  int cells = (W - 4) / cell;
  if (cells > MAX_CELLS) cells = MAX_CELLS;
  if (cells < 1) cells = 1;

  // If the number somehow overflows the counter, fall back to a plain readout.
  if ((int)num.length() > cells) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(LIT, BG);
    tft.drawString(num, W / 2, H / 2 - 6, 4);
  } else {
    const int totalW = cells * digitW + (cells - 1) * gap;
    const int x0 = (W - totalW) / 2;
    const int y  = 2;

    // 1) faint full "8" behind every cell (the unlit segments).
    tft.setTextColor(GHOST, BG);
    for (int i = 0; i < cells; i++) tft.drawChar('8', x0 + i * cell, y, F);

    // 2) lit digits, right-aligned, drawn transparent so ghost shows through.
    if (num.length()) {
      tft.setTextColor(LIT);                  // single arg = transparent bg
      const int start = cells - num.length();
      for (int i = 0; i < (int)num.length(); i++)
        tft.drawChar(num[i], x0 + (start + i) * cell, y, F);
    }
  }

  // "StatTrak(TM)" label plate, bottom-left like the in-game module.
  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(ORANGE, BG);
  tft.drawString("StatTrak", 4, H - 1, 2);
  const int lw = tft.textWidth("StatTrak", 2);
  tft.drawString("TM", 5 + lw, H - 12, 1);    // superscript trademark
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
