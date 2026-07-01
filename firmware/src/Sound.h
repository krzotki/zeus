#pragma once
#include <Arduino.h>

// Event sound effects, played through a MAX98357A I2S amp -> 8ohm speaker.
// Each effect maps to a WAV file you upload to the on-board LittleFS
// (firmware/data/*.wav, flashed with `pio run -t uploadfs`):
//   Boot=/boot.wav  Click=/click.wav  Loaded=/loaded.wav
//   LevelUp=/levelup.wav  Portal=/portal.wav  Error=/error.wav
// A missing file just plays nothing. Volume/mute come from the global cfg.
// WAV must be PCM (16-bit); mono ~22050 Hz recommended.
namespace Sound {
  enum Effect { Boot, Click, Loaded, LevelUp, Portal, Error };

  void begin();          // mount LittleFS + set up the I2S output once
  void play(Effect e);   // blocking until the clip ends. No-op if muted/volume 0 or file missing
  void applyVolume();    // push cfg.soundVolume / soundEnabled to the I2S gain
}
