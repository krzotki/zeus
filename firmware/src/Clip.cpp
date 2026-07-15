#include "Clip.h"
#include <time.h>
#include <LittleFS.h>
#include <AnimatedGIF.h>
#include "Display.h"
#include "Sound.h"
#include "Tip.h"
#include "Gauge.h"

// Schedule (local time). These are only fallbacks - the real schedule comes
// from platformio.ini build_flags (-D CLIP_HOUR / -D CLIP_MIN); edit it THERE.
#ifndef CLIP_HOUR
#define CLIP_HOUR 15
#endif
#ifndef CLIP_MIN
#define CLIP_MIN 5
#endif

namespace {
const char* GIF_PATH = "/2137.gif";
const char* WAV_PATH = "/2137.wav";

// Panel size (Display keeps its tft private; the ST7735 is fixed 160x80).
const int SCREEN_W = 160;
const int SCREEN_H = 80;

AnimatedGIF gif;
File gifFile;
bool gifOk = false;
int xOff = 0, yOff = 0;      // centering offset for GIFs smaller than the panel
uint8_t btnPin = 0;
uint32_t nextFrameAt = 0;
int lastPlayedDay = -1;      // tm_yday of the last auto-play (once per day)

// Disco state: beat -> full-bright strobe + OLED invert toggle; between beats
// the LED brightness follows the music loudness (Sound::level).
const uint32_t BEAT_FLASH_MS = 90;
const uint8_t  LED_FLOOR = 8;        // faint glow even in quiet passages
uint32_t beatFlashUntil = 0;
bool oledInverted = false;

// ---- AnimatedGIF <-> LittleFS glue ----
void* fOpen(const char* fname, int32_t* pSize) {
  gifFile = LittleFS.open(fname, "r");
  if (!gifFile) return nullptr;
  *pSize = gifFile.size();
  return (void*)&gifFile;
}

void fClose(void* pHandle) { ((File*)pHandle)->close(); }

int32_t fRead(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen) {
  File* f = (File*)pFile->fHandle;
  int32_t iBytesRead = iLen;
  if ((pFile->iSize - pFile->iPos) < iLen) iBytesRead = pFile->iSize - pFile->iPos;
  if (iBytesRead <= 0) return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = (int32_t)f->position();
  return iBytesRead;
}

int32_t fSeek(GIFFILE* pFile, int32_t iPosition) {
  File* f = (File*)pFile->fHandle;
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  return pFile->iPos;
}

// One decoded scanline -> TFT. Standard AnimatedGIF draw callback: handle
// restore-to-background disposal, then push opaque pixel runs.
void gifDraw(GIFDRAW* pDraw) {
  static uint16_t line[SCREEN_W];
  uint16_t* pal = pDraw->pPalette;
  uint8_t* s = pDraw->pPixels;
  int y = yOff + pDraw->iY + pDraw->y;
  int iWidth = pDraw->iWidth;
  if (iWidth > SCREEN_W) iWidth = SCREEN_W;
  if (y < 0 || y >= SCREEN_H) return;

  if (pDraw->ucDisposalMethod == 2) {  // restore to background colour
    for (int x = 0; x < iWidth; x++)
      if (s[x] == pDraw->ucTransparent) s[x] = pDraw->ucBackground;
    pDraw->ucHasTransparency = 0;
  }

  if (pDraw->ucHasTransparency) {
    const uint8_t t = pDraw->ucTransparent;
    int x = 0;
    while (x < iWidth) {
      while (x < iWidth && s[x] == t) x++;             // skip transparent run
      const int start = x;
      while (x < iWidth && s[x] != t) { line[x - start] = pal[s[x]]; x++; }
      if (x > start) Display::blit(xOff + pDraw->iX + start, y, x - start, 1, line);
    }
  } else {
    for (int x = 0; x < iWidth; x++) line[x] = pal[s[x]];
    Display::blit(xOff + pDraw->iX, y, iWidth, 1, line);
  }
}

// Runs every audio decode step (see Sound::playFile): advance the GIF when its
// frame delay elapses, loop it at the end, run the LED/OLED disco, and abort
// everything on the button.
void pump() {
  if (digitalRead(btnPin) == LOW) Sound::requestStop();

  if (Sound::beat()) {
    beatFlashUntil = millis() + BEAT_FLASH_MS;
    oledInverted = !oledInverted;
    Gauge::setInvert(oledInverted);       // 21:37 blinks negative on the beat
  }
  if ((int32_t)(millis() - beatFlashUntil) < 0) {
    Tip::set(255);                        // beat strobe
  } else {
    uint8_t lv = Sound::level();
    Tip::set(lv > LED_FLOOR ? lv : LED_FLOOR);   // loudness glow between beats
  }

  if (!gifOk) return;
  if ((int32_t)(millis() - nextFrameAt) < 0) return;
  int delayMs = 0;
  if (gif.playFrame(false, &delayMs) == 0) gif.reset();  // last frame -> loop
  if (delayMs < 20) delayMs = 20;   // some GIFs claim 0ms; cap at ~50fps
  nextFrameAt = millis() + delayMs;
}
}  // namespace

bool Clip::due() {
  static uint32_t nextCheckAt = 0;
  if ((int32_t)(millis() - nextCheckAt) < 0) return false;
  nextCheckAt = millis() + 1000;

  struct tm t;
  if (!getLocalTime(&t, 0)) return false;   // no NTP sync yet
  if (t.tm_hour != CLIP_HOUR || t.tm_min != CLIP_MIN) return false;
  if (t.tm_yday == lastPlayedDay) return false;
  lastPlayedDay = t.tm_yday;
  return true;
}

void Clip::play(uint8_t abortBtnPin) {
  btnPin = abortBtnPin;

  gif.begin(GIF_PALETTE_RGB565_LE);   // host-order palette; Display::blit swaps
  gifOk = gif.open(GIF_PATH, fOpen, fClose, fRead, fSeek, gifDraw);
  if (gifOk) {
    xOff = (SCREEN_W - gif.getCanvasWidth()) / 2;  if (xOff < 0) xOff = 0;
    yOff = (SCREEN_H - gif.getCanvasHeight()) / 2; if (yOff < 0) yOff = 0;
  } else {
    Serial.printf("[clip] missing/bad %s\n", GIF_PATH);  // sound still plays
  }

  Display::clear();
  Gauge::showClock2137();             // battery meter -> the sacred hour
  beatFlashUntil = 0;
  oledInverted = false;

  nextFrameAt = millis();             // first frame right away
  Sound::playFile(WAV_PATH, pump);    // blocks; WAV length = clip duration

  if (gifOk) gif.close();
  gifOk = false;
  Tip::off();
  Gauge::setInvert(false);            // battery screen returns via main's Gauge::update()
  while (digitalRead(btnPin) == LOW) delay(10);  // abort press must not become a click
}
