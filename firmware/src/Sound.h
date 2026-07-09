#pragma once
#include <Arduino.h>

// Event sound effects, played through a MAX98357A I2S amp -> 8ohm speaker.
// Each effect maps to a WAV file you upload to the on-board LittleFS
// (firmware/data/*.wav, flashed with `pio run -t uploadfs`):
//   Boot=/boot.wav  Click=/click.wav  Loaded=/loaded.wav
//   LevelUp=/levelup.wav  Portal=/portal.wav  Error=/error.wav
// A missing file just plays nothing. Volume is fixed at full scale (gain 1.0);
// set clip loudness in the WAV itself, or the amp's GAIN pin for more output.
// WAV must be PCM (16-bit); mono ~22050 Hz recommended.
namespace Sound {
  enum Effect { Boot, Click, Loaded, LevelUp, Portal, Error };

  void begin();          // mount LittleFS + set up the I2S output once
  // Blocking until the clip ends. No-op if the file is missing.
  // pump(), if given, is called every decode step so an animation (e.g.
  // Tip::update) can run concurrently with the sound.
  void play(Effect e, void (*pump)() = nullptr);
}
