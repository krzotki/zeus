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

// Percent chance a click plays the levelup-laced variant. 0 disables.
#ifndef LEVELUP_CHANCE
#define LEVELUP_CHANCE 10
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
bool stopRequested = false;

// Optional WAVs, probed once in begin(). LittleFS.exists() is implemented as an
// open(), so testing a file that isn't there logs an [E] vfs_api line *every*
// call - cache the answer instead of re-asking on every click.
bool haveBirthday   = false;
bool haveClickLevel = false;

// ---- Music level / beat detection (drives the clip's LED disco) ----------
// Samples are tapped on their way to the I2S amp, so no microphone is needed.
const int      TAP_WIN       = 512;     // samples per energy window (~23ms @ 22050)
const int      HIST          = 32;      // energy history (~0.75s) the beat compares against
const float    BEAT_RATIO    = 1.4f;    // window energy > ratio*average -> beat
const uint32_t BEAT_GAP_MS   = 250;     // refractory: max ~4 beats/s
const float    SILENCE_FLOOR = 40.0f;   // avg RMS below this = silence, never a beat

uint64_t winAcc  = 0;                   // sum of sample^2 within the current window
int      winN    = 0;
float    hist[HIST];
int      histPos = 0, histFill = 0;
uint8_t  curLevel   = 0;                // smoothed loudness, 0-255
bool     beatLatch  = false;            // set here, consumed by Sound::beat()
uint32_t lastBeatAt = 0;

void tapReset() {
  winAcc = 0; winN = 0;
  histPos = 0; histFill = 0;
  curLevel = 0; beatLatch = false;
}

void tapSample(int16_t s) {
  winAcc += (int32_t)s * (int32_t)s;
  if (++winN < TAP_WIN) return;

  const float rms = sqrtf((float)(winAcc / TAP_WIN));   // 0..32767
  winAcc = 0; winN = 0;

  float avg = 0;
  for (int i = 0; i < histFill; i++) avg += hist[i];
  if (histFill) avg /= histFill;

  hist[histPos] = rms;
  histPos = (histPos + 1) % HIST;
  if (histFill < HIST) histFill++;

  // Perceptual-ish loudness: sqrt compresses the top, lifts quiet passages.
  float lv = sqrtf(rms / 32767.0f) * 255.0f;
  curLevel = (lv > 255.0f) ? 255 : (uint8_t)lv;

  if (histFill >= HIST / 2 && avg > SILENCE_FLOOR && rms > BEAT_RATIO * avg &&
      millis() - lastBeatAt >= BEAT_GAP_MS) {
    lastBeatAt = millis();
    beatLatch = true;
  }
}

// Forwards every sample to the real I2S output while feeding the detector.
// Tap only samples the output accepted - a full DMA buffer makes the
// generator retry the same sample, which must not be counted twice.
class TapOutput : public AudioOutput {
 public:
  AudioOutput* dst = nullptr;
  bool SetRate(int hz) override { return dst->SetRate(hz); }
  bool SetBitsPerSample(int b) override { return dst->SetBitsPerSample(b); }
  bool SetChannels(int c) override { return dst->SetChannels(c); }
  bool SetGain(float f) override { return dst->SetGain(f); }
  bool begin() override { return dst->begin(); }
  bool stop() override { return dst->stop(); }
  bool ConsumeSample(int16_t sample[2]) override {
    if (!dst->ConsumeSample(sample)) return false;
    tapSample(sample[0]);
    return true;
  }
};
TapOutput tap;

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
      if (haveBirthday && isBirthday()) return "/birthday.wav";
      // Rare payoff: /clicklevel.wav is click.wav with levelup.wav mixed in
      // starting halfway through it (pre-mixed offline, see data/README.md).
      if (haveClickLevel && random(100) < LEVELUP_CHANCE) return "/clicklevel.wav";
      return "/click.wav";
    case Sound::Loaded:  return "/loaded.wav";
    case Sound::LevelUp: return "/levelup.wav";   // unused: only a mix source now
    case Sound::Portal:  return "/portal.wav";
    case Sound::Error:   return "/error.wav";     // unused: failures are shown, not heard
  }
  return "/click.wav";
}
}  // namespace

void Sound::begin() {
  fsReady = LittleFS.begin();
  if (!fsReady) Serial.println(F("[sound] LittleFS mount failed - run: pio run -t uploadfs"));

  if (fsReady) {   // one [E] line each at boot if absent, instead of one per play
    haveBirthday   = LittleFS.exists("/birthday.wav");
    haveClickLevel = LittleFS.exists("/clicklevel.wav");
  }

  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  out->SetOutputModeMono(true);   // one speaker; duplicate mono content
  tap.dst = out;
  applyVolume();
}

// Volume/mute from cfg as software I2S gain (1.0 = full scale; >1.0 would
// clip). Mute is gain 0, NOT a playback skip: samples keep flowing so the
// clip's GIF timing and beat detection (pre-gain tap) still run silently.
void Sound::applyVolume() {
  if (!out) return;
  out->SetGain(cfg.soundEnabled ? (cfg.soundVolume / 100.0f) : 0.0f);
}

void Sound::play(Effect e, void (*pump)()) {
  playFile(path(e), pump);
}

void Sound::playFile(const char* p, void (*pump)()) {
  if (!out || !fsReady) return;
  if (!LittleFS.exists(p)) { Serial.printf("[sound] missing %s\n", p); return; }

  stopRequested = false;
  tapReset();
  AudioFileSourceLittleFS src(p);
  AudioGeneratorWAV gen;
  if (!gen.begin(&src, &tap)) { Serial.printf("[sound] bad WAV %s (need 16-bit PCM)\n", p); return; }
  while (gen.isRunning()) {
    if (!gen.loop() || stopRequested) gen.stop();
    if (pump) pump();   // run the concurrent animation (e.g. Tip::update)
  }
}

void Sound::requestStop() { stopRequested = true; }

uint8_t Sound::level() { return curLevel; }

bool Sound::beat() {
  const bool b = beatLatch;
  beatLatch = false;
  return b;
}
