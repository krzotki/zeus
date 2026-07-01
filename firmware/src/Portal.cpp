#include "Portal.h"
#include "Config.h"
#include "Display.h"
#include <WiFi.h>
#include <WiFiManager.h>

namespace {
const char* AP_NAME = "ZeusX27-Setup";
const int   PORTAL_TIMEOUT_S = 180;   // close portal after 3 min idle

// Custom fields shown on the captive-portal page, alongside WiFi selection.
WiFiManagerParameter pSteam("steam", "Steam ID64 / vanity / inspect link", "", 200);
WiFiManagerParameter pServer("server", "Inspect server URL (http://ip:3000)", "", 100);
WiFiManagerParameter pPoll("poll", "Refresh minutes", "5", 5);
WiFiManagerParameter pKey("apikey", "Steam API key (optional, for vanity)", "", 40);

// Pull the submitted custom fields into Config and persist.
void saveParams() {
  String steam = pSteam.getValue();
  steam.trim();
  if (steam.length()) cfg.steamSource = steam;

  String server = pServer.getValue();
  server.trim();
  if (server.length()) cfg.serverBase = server;

  int poll = String(pPoll.getValue()).toInt();
  if (poll >= 1) cfg.pollMinutes = poll;

  String key = pKey.getValue();
  key.trim();
  cfg.apiKey = key;

  cfg.save();
  cfg.refreshRequested = true;
}

void onApMode(WiFiManager* wm) {
  Display::showSetup(AP_NAME, WiFi.softAPIP().toString());
}

void prime(WiFiManager& wm) {
  // Seed current values so the form shows what's already configured.
  pSteam.setValue(cfg.steamSource.c_str(), 200);
  pServer.setValue(cfg.serverBase.c_str(), 100);
  char pollBuf[6];
  snprintf(pollBuf, sizeof(pollBuf), "%u", cfg.pollMinutes);
  pPoll.setValue(pollBuf, 5);

  wm.addParameter(&pSteam);
  wm.addParameter(&pServer);
  wm.addParameter(&pPoll);
  wm.addParameter(&pKey);
  wm.setSaveParamsCallback(saveParams);
  wm.setAPCallback(onApMode);
  wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
  wm.setTitle("Zeus x27");
}
}  // namespace

bool Portal::connect(bool forcePortal) {
  WiFiManager wm;
  prime(wm);

  bool connected;
  if (forcePortal) {
    Display::showSetup(AP_NAME, "192.168.4.1");
    connected = wm.startConfigPortal(AP_NAME);
  } else {
    // Uses saved creds; opens portal only if they fail.
    connected = wm.autoConnect(AP_NAME);
  }
  return connected && WiFi.status() == WL_CONNECTED;
}

void Portal::openConfigPortal() {
  WiFiManager wm;
  prime(wm);
  Display::showSetup(AP_NAME, "192.168.4.1");
  wm.startConfigPortal(AP_NAME);
}
