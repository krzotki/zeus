#pragma once
#include <Arduino.h>

// Daily "2137" event: at CLIP_HOUR:CLIP_MIN local time (build flags), loop
// /2137.gif on the TFT while /2137.wav plays. The WAV length (~60s) sets the
// duration; pressing the button aborts. Needs NTP time (synced after WiFi).
namespace Clip {
  // True once when the scheduled minute arrives (guarded per day). Cheap to
  // call every loop(); it self-throttles the clock check.
  bool due();
  // Blocking playback. abortBtnPin is polled (active LOW) to cut it short.
  void play(uint8_t abortBtnPin);
}
