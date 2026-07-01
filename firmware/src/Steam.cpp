#include "Steam.h"
#include "Config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>

namespace {

// ArduinoJson allocator that puts the (large) inventory document in PSRAM.
struct PsramAllocator : ArduinoJson::Allocator {
  void* allocate(size_t n) override { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM); }
  void  deallocate(void* p) override { heap_caps_free(p); }
  void* reallocate(void* p, size_t n) override { return heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM); }
};

bool allDigits(const String& s) {
  if (s.isEmpty()) return false;
  for (size_t i = 0; i < s.length(); i++) if (!isDigit(s[i])) return false;
  return true;
}

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

// ---- Step: SteamID64 -> inventory -> StatTrak Zeus inspect link ----
bool findInspectLink(const String& steamid, String& link, String& err) {
  WiFiClientSecure client;
  client.setInsecure();  // hobby build: skip cert validation (see README to pin CA)
  HTTPClient http;
  http.setTimeout(15000);
  String url = "https://steamcommunity.com/inventory/" + steamid + "/730/2?l=english&count=2000";
  if (!http.begin(client, url)) { err = "inv begin"; return false; }
  http.addHeader("Accept-Encoding", "identity");  // avoid gzip (ESP32 can't unzip)
  http.addHeader("User-Agent", "Mozilla/5.0 ZeusX27");
  int code = http.GET();
  if (code != 200) { err = "inventory HTTP " + String(code); http.end(); return false; }

  PsramAllocator alloc;
  JsonDocument doc(&alloc);
  JsonDocument filter(&alloc);
  filter["assets"][0]["assetid"] = true;
  filter["assets"][0]["classid"] = true;
  filter["assets"][0]["instanceid"] = true;
  filter["descriptions"][0]["classid"] = true;
  filter["descriptions"][0]["instanceid"] = true;
  filter["descriptions"][0]["market_hash_name"] = true;
  filter["descriptions"][0]["actions"] = true;

  DeserializationError e = deserializeJson(doc, http.getStream(),
                                           DeserializationOption::Filter(filter));
  http.end();
  if (e) { err = String("inv json:") + e.c_str(); return false; }

  // Find the StatTrak Zeus x27 description and its inspect-link template.
  String cid, iid, tmpl;
  for (JsonObject d : doc["descriptions"].as<JsonArray>()) {
    const char* mhn = d["market_hash_name"];
    if (!mhn) continue;
    String n(mhn);
    if (n.indexOf("Zeus x27") >= 0 && n.indexOf("StatTrak") >= 0) {
      cid = (const char*)d["classid"];
      iid = d["instanceid"].is<const char*>() ? (const char*)d["instanceid"] : "";
      for (JsonObject a : d["actions"].as<JsonArray>()) {
        const char* l = a["link"];
        if (l && String(l).indexOf("preview") >= 0) { tmpl = l; break; }
      }
      break;
    }
  }
  if (cid.isEmpty() || tmpl.isEmpty()) { err = "no StatTrak Zeus in inventory"; return false; }

  // Find the owned asset of that class to get its assetid.
  String assetid;
  for (JsonObject a : doc["assets"].as<JsonArray>()) {
    const char* ac = a["classid"];
    const char* ai = a["instanceid"];
    if (ac && cid == ac && (iid.isEmpty() || (ai && iid == ai))) {
      assetid = (const char*)a["assetid"];
      break;
    }
  }
  if (assetid.isEmpty()) { err = "asset not found"; return false; }

  tmpl.replace("%owner_steamid%", steamid);
  tmpl.replace("%assetid%", assetid);
  link = tmpl;
  return true;
}

// ---- Step: vanity name -> SteamID64 ----
bool resolveVanity(const String& vanity, String& steamid, String& err) {
  if (cfg.apiKey.isEmpty()) { err = "vanity needs Steam API key"; return false; }
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http; http.setTimeout(15000);
  String url = "https://api.steampowered.com/ISteamUser/ResolveVanityURL/v1/?key=" +
               cfg.apiKey + "&vanityurl=" + urlEncode(vanity);
  if (!http.begin(client, url)) { err = "vanity begin"; return false; }
  int code = http.GET();
  if (code != 200) { err = "vanity HTTP " + String(code); http.end(); return false; }
  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, http.getStream());
  http.end();
  if (e) { err = String("vanity json:") + e.c_str(); return false; }
  if (doc["response"]["success"].as<int>() != 1) { err = "vanity not found"; return false; }
  steamid = (const char*)doc["response"]["steamid"];
  return true;
}

// ---- Step: inspect link -> kill count (self-hosted bot) ----
// The free public inspect APIs are rate-limited/blocked by Valve, so we query a
// local Game Coordinator bot instead (see bot/). It answers:
//   GET <serverBase>/inspect?url=<inspect link>  ->  { "killeater_value": <n>, ... }
bool inspectKillEater(const String& link, int32_t& value, String& err) {
  String base = cfg.serverBase; base.trim();
  if (base.isEmpty()) { err = "no inspect server"; return false; }
  while (base.endsWith("/")) base.remove(base.length() - 1);
  String url = base + "/inspect?url=" + urlEncode(link);

  HTTPClient http; http.setTimeout(20000);
  WiFiClient     plain;
  WiFiClientSecure secure;
  bool https = url.startsWith("https");
  bool begun;
  if (https) { secure.setInsecure(); begun = http.begin(secure, url); }
  else       { begun = http.begin(plain, url); }
  if (!begun) { err = "server begin"; return false; }
  http.addHeader("User-Agent", "ZeusX27/1.0");
  int code = http.GET();
  if (code != 200) {
    // Surface the bot's JSON error message when present (e.g. "GC not ready").
    String body = http.getString();
    http.end();
    JsonDocument ed;
    if (!deserializeJson(ed, body) && ed["error"].is<const char*>())
      err = String("bot:") + (const char*)ed["error"];
    else
      err = "inspect HTTP " + String(code);
    return false;
  }

  JsonDocument doc;
  DeserializationError e = deserializeJson(doc, http.getStream());
  http.end();
  if (e) { err = String("inspect json:") + e.c_str(); return false; }
  if (!doc["killeater_value"].is<long>()) { err = "not StatTrak / no killeater"; return false; }
  value = doc["killeater_value"].as<long>();
  return true;
}

}  // namespace

Steam::Result Steam::fetch() {
  Result r;
  String src = cfg.steamSource; src.trim();
  if (src.isEmpty()) { r.err = "not configured"; return r; }

  String link;
  if (src.indexOf("preview") >= 0 || src.startsWith("steam://")) {
    link = src;  // already an inspect link
  } else {
    String steamid = src;
    if (!allDigits(src)) {  // treat as vanity name
      if (!resolveVanity(src, steamid, r.err)) return r;
    }
    if (!findInspectLink(steamid, link, r.err)) return r;
  }

  int32_t v;
  if (!inspectKillEater(link, v, r.err)) return r;
  r.value = v;
  r.name  = "Zeus x27";
  r.ok    = true;
  return r;
}
