#include <Arduino.h>
#include "Tip.h"

// Tip LED pin from platformio.ini (-D TIP_LED_PIN); GPIO44 / D7 is the last free XIAO pin.
#ifndef TIP_LED_PIN
#define TIP_LED_PIN 44
#endif

namespace {
const int LEDC_CH = 4;      // avoid the low channels the audio/other libs may grab
const int LEDC_FREQ = 5000;
const int LEDC_RES = 8;     // 8-bit duty (0-255)
const uint32_t ARC_MS = 500;  // active-arcing duration before the decay tail

// Non-blocking strike state so the flicker can run *concurrently* with the
// (blocking) click sound - main pumps update() from inside Sound::play().
enum Phase { IDLE, ARC, DECAY };
Phase    phase     = IDLE;
uint32_t arcEnd    = 0;     // millis() when arcing stops -> decay
uint32_t nextStep  = 0;     // millis() of the next flicker/decay step
int      decayLevel = 0;
}  // namespace

void Tip::begin() {
  ledcSetup(LEDC_CH, LEDC_FREQ, LEDC_RES);
  ledcAttachPin(TIP_LED_PIN, LEDC_CH);
  ledcWrite(LEDC_CH, 0);
  phase = IDLE;
}

void Tip::off() {
  ledcWrite(LEDC_CH, 0);
  phase = IDLE;
}

// Direct brightness control for externally-driven effects (clip disco mode).
void Tip::set(uint8_t duty) {
  ledcWrite(LEDC_CH, duty);
  phase = IDLE;
}

// Kick off a taser arc: fast random-brightness flicker (mostly bright with
// random dropouts) for ~0.5s, then a quick decay to dark. Non-blocking - the
// actual flicker advances in update(), so call that frequently until idle.
void Tip::strike() {
  phase    = ARC;
  arcEnd   = millis() + ARC_MS;
  nextStep = 0;               // fire the first flicker on the next update()
}

void Tip::update() {
  if (phase == IDLE) return;
  const uint32_t now = millis();

  if (phase == ARC) {
    if ((int32_t)(now - arcEnd) >= 0) {         // arcing done -> start decay
      phase = DECAY;
      decayLevel = 220;
      nextStep = now;                            // decay immediately below
    } else {
      if ((int32_t)(now - nextStep) >= 0) {
        uint8_t duty = (random(100) < 75) ? random(160, 256)  // bright stab
                                          : random(0, 40);     // brief dropout
        ledcWrite(LEDC_CH, duty);
        nextStep = now + random(8, 45);
      }
      return;
    }
  }

  if (phase == DECAY && (int32_t)(now - nextStep) >= 0) {
    if (decayLevel < 0) { ledcWrite(LEDC_CH, 0); phase = IDLE; return; }
    ledcWrite(LEDC_CH, decayLevel);
    decayLevel -= 22;
    nextStep = now + 15;
  }
}
