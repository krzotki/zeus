#include "Config.h"
#include "Sound.h"
#include <Preferences.h>
#include <WiFi.h>

Config cfg;
static Preferences prefs;
static const char* NS = "zeus";

void Config::begin() {
  prefs.begin(NS, true);  // read-only
  steamSource = prefs.getString("steam", "");
  pollMinutes = prefs.getUShort("poll", 5);
  apiKey      = prefs.getString("apikey", "");
  serverBase  = prefs.getString("server", "");
  soundVolume = prefs.getUChar("vol", 60);
  soundEnabled = prefs.getBool("snden", true);
  cachedName  = prefs.getString("cname", "Zeus x27");
  cachedValue = prefs.getInt("cval", -1);
  prefs.end();
  if (pollMinutes < 1) pollMinutes = 1;
}

void Config::save() {
  prefs.begin(NS, false);
  prefs.putString("steam", steamSource);
  prefs.putUShort("poll", pollMinutes);
  prefs.putString("apikey", apiKey);
  prefs.putString("server", serverBase);
  prefs.putUChar("vol", soundVolume);
  prefs.putBool("snden", soundEnabled);
  prefs.end();
}

void Config::saveCache(const String& name, int32_t value) {
  cachedName = name;
  cachedValue = value;
  prefs.begin(NS, false);
  prefs.putString("cname", name);
  prefs.putInt("cval", value);
  prefs.end();
}

String Config::toJson() const {
  String s = "{";
  s += "\"steam\":\"" + steamSource + "\",";
  s += "\"poll\":" + String(pollMinutes) + ",";
  s += "\"apikey_set\":" + String(apiKey.length() ? "true" : "false") + ",";
  s += "\"server\":\"" + serverBase + "\",";
  s += "\"volume\":" + String(soundVolume) + ",";
  s += "\"sound\":" + String(soundEnabled ? "true" : "false") + ",";
  s += "\"wifi_ssid\":\"" + WiFi.SSID() + "\",";
  s += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  s += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  s += "\"cached_name\":\"" + cachedName + "\",";
  s += "\"cached_value\":" + String(cachedValue);
  s += "}";
  return s;
}

// ---- Serial command protocol (used by the Web Serial config page) ----
// Commands (newline-terminated):
//   GET
//   SET steam <steamid64 | vanity | inspect-link>
//   SET interval <minutes>
//   SET apikey <key>
//   SET wifi <ssid> <password>
//   REFRESH        force an immediate poll
//   PORTAL         (re)open the WiFi captive portal
//   CLEAR          wipe WiFi + all settings
//   HELP
bool Config::handleSerialLine(const String& raw) {
  String line = raw;
  line.trim();
  if (line.isEmpty()) return false;

  int sp = line.indexOf(' ');
  String cmd = (sp < 0) ? line : line.substring(0, sp);
  String arg = (sp < 0) ? ""   : line.substring(sp + 1);
  cmd.toUpperCase();

  if (cmd == "GET") {
    Serial.println(toJson());
    return true;
  }
  if (cmd == "HELP") {
    Serial.println(F("Commands: GET | SET steam <v> | SET server <url> | SET interval <min> | SET volume <0-100> | SET sound <0|1> | SET wifi <ssid> <pass> | REFRESH | PORTAL | CLEAR"));
    return true;
  }
  if (cmd == "REFRESH") {
    refreshRequested = true;
    Serial.println(F("OK refresh"));
    return true;
  }
  if (cmd == "PORTAL") {
    portalRequested = true;
    Serial.println(F("OK portal"));
    return true;
  }
  if (cmd == "CLEAR") {
    prefs.begin(NS, false);
    prefs.clear();
    prefs.end();
    WiFi.disconnect(true, true);  // erase stored WiFi creds too
    Serial.println(F("OK cleared, rebooting..."));
    Serial.flush();
    delay(200);
    ESP.restart();                // apply immediately so a reset really resets
    return true;
  }
  if (cmd == "SET") {
    int sp2 = arg.indexOf(' ');
    String key = (sp2 < 0) ? arg : arg.substring(0, sp2);
    String val = (sp2 < 0) ? ""  : arg.substring(sp2 + 1);
    key.toLowerCase();
    val.trim();

    if (key == "steam")    { steamSource = val; save(); refreshRequested = true; Serial.println(F("OK steam")); return true; }
    if (key == "interval") { pollMinutes = max(1, (int)val.toInt()); save(); Serial.println(F("OK interval")); return true; }
    if (key == "apikey")   { apiKey = val; save(); Serial.println(F("OK apikey")); return true; }
    if (key == "server")   { serverBase = val; save(); refreshRequested = true; Serial.println(F("OK server")); return true; }
    if (key == "volume")   { soundVolume = (uint8_t)constrain(val.toInt(), 0, 100); save(); Sound::applyVolume(); Serial.println(F("OK volume")); return true; }
    if (key == "sound")    { soundEnabled = (val.toInt() != 0); save(); Sound::applyVolume(); Serial.println(F("OK sound")); return true; }
    if (key == "wifi") {
      int s3 = val.indexOf(' ');
      String ssid = (s3 < 0) ? val : val.substring(0, s3);
      String pass = (s3 < 0) ? ""  : val.substring(s3 + 1);
      WiFi.persistent(true);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), pass.c_str());  // persisted by the WiFi stack
      Serial.println(F("OK wifi (connecting)"));
      return true;
    }
    Serial.println(F("ERR unknown SET key"));
    return true;
  }

  Serial.println(F("ERR unknown command"));
  return true;
}

void Config::pollSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (_rxbuf.length()) { handleSerialLine(_rxbuf); _rxbuf = ""; }
    } else {
      _rxbuf += c;
      if (_rxbuf.length() > 400) _rxbuf = "";  // guard
    }
  }
}
