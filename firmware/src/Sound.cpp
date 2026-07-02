#include "Sound.h"
#include "Config.h"
#include <time.h>
#include <LittleFS.h>
#include <AudioOutputI2S.h>
#include <AudioGeneratorWAV.h>
#include <AudioFileSourceLittleFS.h>

// Birthday date (local time). Overridable from platformio.ini build_flags.
#ifndef BIRTHDAY_MONTH
#define BIRTHDAY_MONTH 8
#endif
#ifndef BIRTHDAY_DAY
#define BIRTHDAY_DAY 22
#endif

// I2S pins come from platformio.ini (-D I2S_BCLK/I2S_LRC/I2S_DIN); fall back to defaults.
#ifndef I2S_BCLK
#define I2S_BCLK 5
#endif
#ifndef I2S_LRC
#define I2S_LRC 6
#endif
#ifndef I2S_DIN
#define I2S_DIN 43
#endif

namespace {
AudioOutputI2S* out = nullptr;
bool fsReady = false;

// True only on BIRTHDAY_MONTH/DAY in local time. False if NTP hasn't synced
// yet (getLocalTime fails) -> caller gets the normal click.
bool isBirthday() {
  struct tm t;
  if (!getLocalTime(&t, 0)) return false;
  return (t.tm_mon + 1) == BIRTHDAY_MONTH && t.tm_mday == BIRTHDAY_DAY;
}

// Effect -> WAV file on LittleFS. Drop your own; a missing file just stays silent.
const char* path(Sound::Effect e) {
  switch (e) {
    case Sound::Boot:    return "/boot.wav";
    case Sound::Click:
      // On the birthday, swap the click for /birthday.wav (if uploaded).
      if (isBirthday() && LittleFS.exists("/birthday.wav")) return "/birthday.wav";
      return "/click.wav";
    case Sound::Loaded:  return "/loaded.wav";
    case Sound::LevelUp: return "/levelup.wav";
    case Sound::Portal:  return "/portal.wav";
    case Sound::Error:   return "/error.wav";
  }
  return "/click.wav";
}
}  // namespace

void Sound::begin() {
  fsReady = LittleFS.begin();
  if (!fsReady) Serial.println(F("[sound] LittleFS mount failed - run: pio run -t uploadfs"));

  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  out->SetOutputModeMono(true);   // one speaker; duplicate mono content
  applyVolume();
}

void Sound::applyVolume() {
  if (!out) return;
  out->SetGain(cfg.soundEnabled ? (cfg.soundVolume / 100.0f) : 0.0f);
}

void Sound::play(Effect e) {
  if (!out || !fsReady) return;
  if (!cfg.soundEnabled || cfg.soundVolume == 0) return;

  const char* p = path(e);
  if (!LittleFS.exists(p)) { Serial.printf("[sound] missing %s\n", p); return; }

  AudioFileSourceLittleFS src(p);
  AudioGeneratorWAV gen;
  if (!gen.begin(&src, out)) { Serial.printf("[sound] bad WAV %s (need 16-bit PCM)\n", p); return; }
  while (gen.isRunning()) {
    if (!gen.loop()) gen.stop();
  }
}
