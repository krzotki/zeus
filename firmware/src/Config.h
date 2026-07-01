#pragma once
#include <Arduino.h>

// Persistent settings + the USB-serial config protocol.
// WiFi credentials are owned by the WiFi/WiFiManager NVS, not here. This stores
// the Steam "source", poll interval, optional inspect-API key, and a cached last
// reading so the screen can show something immediately on boot.
class Config {
public:
  // The Steam source the owner configures. Accepts any of:
  //   - SteamID64 (e.g. 76561198000000000)  -> inventory is searched for the Zeus
  //   - vanity name (e.g. "s1mple")          -> resolved to SteamID64 (needs API key)
  //   - a full CS2 inspect link              -> used directly (most reliable)
  String steamSource;
  uint16_t pollMinutes = 5;       // how often to refresh
  String apiKey;                  // optional: Steam Web API key (vanity resolve) / inspect-API key
  String serverBase;              // base URL of the self-hosted inspect bot, e.g. http://192.168.1.50:3000

  // Sound (MAX98357A I2S amp). Volume is applied as software I2S gain.
  uint8_t soundVolume = 60;       // 0-100
  bool    soundEnabled = true;    // master mute

  // Cached last successful reading (shown on boot before first fetch).
  String  cachedName  = "Zeus x27";
  int32_t cachedValue = -1;       // -1 = unknown

  bool refreshRequested = false;  // set by serial/button to force an immediate poll
  bool portalRequested  = false;  // set by serial to (re)open the WiFi setup portal

  void begin();                   // load from NVS
  void save();                    // persist editable fields
  void saveCache(const String& name, int32_t value);

  String toJson() const;

  // Feed one line from Serial. Returns true if it was a recognised command.
  bool handleSerialLine(const String& line);
  // Call every loop(); reads Serial and dispatches complete lines.
  void pollSerial();

private:
  String _rxbuf;
};

extern Config cfg;
