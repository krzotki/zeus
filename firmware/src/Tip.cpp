#include "Tip.h"
#include <Arduino.h>

// Tip LED pin from platformio.ini (-D TIP_LED_PIN); GPIO44 / D7 is the last free XIAO pin.
#ifndef TIP_LED_PIN
#define TIP_LED_PIN 44
#endif

namespace {
const int LEDC_CH = 4;      // avoid the low channels the audio/other libs may grab
const int LEDC_FREQ = 5000;
const int LEDC_RES = 8;     // 8-bit duty (0-255)
}  // namespace

void Tip::begin() {
  ledcSetup(LEDC_CH, LEDC_FREQ, LEDC_RES);
  ledcAttachPin(TIP_LED_PIN, LEDC_CH);
  ledcWrite(LEDC_CH, 0);
}

void Tip::off() {
  ledcWrite(LEDC_CH, 0);
}

// Fake a taser arc: fast random-brightness flicker (mostly bright with random
// dropouts) for ~0.5s, then a quick decay to dark.
void Tip::strike() {
  const uint32_t dur = 500;                 // ms of active arcing
  const uint32_t t0 = millis();
  while (millis() - t0 < dur) {
    uint8_t duty = (random(100) < 75) ? random(160, 256)  // bright stab
                                      : random(0, 40);     // brief dropout
    ledcWrite(LEDC_CH, duty);
    delay(random(8, 45));
  }
  for (int b = 220; b >= 0; b -= 22) {      // decay tail
    ledcWrite(LEDC_CH, b);
    delay(15);
  }
  ledcWrite(LEDC_CH, 0);
}
