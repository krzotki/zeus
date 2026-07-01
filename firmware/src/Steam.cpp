#include "Steam.h"
#include "Config.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace {

String urlEncode(const String& s) {
  String out;
  const char* hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += c;
    else { out += '%'; out += hex[(c >> 4) & 0xF]; out += hex[c & 0xF]; }
  }
  return out;
}

}  // namespace

// The heavy lifting (fetch + parse the ~350 KB inventory JSON) lives in the bot; the
// ESP32 just asks it for the number. bot: GET <serverBase>/kills?steam=<id64|vanity>
//   -> { "killeater_value": <n>, "name": "..." }   or   { "error": "..." }
Steam::Result Steam::fetch() {
  Result r;
  String src = cfg.steamSource; src.trim();
  if (src.isEmpty()) { r.err = "not configured"; return r; }

  String base = cfg.serverBase; base.trim();
  if (base.isEmpty()) { r.err = "no server set"; return r; }
  while (base.endsWith("/")) base.remove(base.length() - 1);
  String url = base + "/kills?steam=" + urlEncode(src);
  Serial.println("[kills] GET " + url);

  HTTPClient http; http.setTimeout(20000);
  WiFiClient       plain;
  WiFiClientSecure secure;
  bool https = url.startsWith("https");
  bool begun;
  if (https) { secure.setInsecure(); begun = http.begin(secure, url); }
  else       { begun = http.begin(plain, url); }
  if (!begun) { r.err = "server begin"; return r; }

  int code = http.GET();
  Serial.println("[kills] HTTP " + String(code));
  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, http.getStream());
  http.end();
  if (e) { r.err = String("json:") + e.c_str(); return r; }

  if (code != 200) {
    r.err = doc["error"].is<const char*>() ? String((const char*)doc["error"]) : ("HTTP " + String(code));
    return r;
  }
  if (!doc["killeater_value"].is<long>()) { r.err = "no killeater"; return r; }
  r.value = doc["killeater_value"].as<long>();
  r.name  = "Zeus x27";
  r.ok    = true;
  Serial.println("[kills] value=" + String(r.value));
  return r;
}
