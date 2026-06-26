// Zeus x27 StatTrak display - firmware entry point.
// Boot -> WiFi (saved or captive portal) -> poll Steam inspect API -> show count.
#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "Display.h"
#include "Portal.h"
#include "Steam.h"

static const uint8_t BTN_PIN = 8;          // XIAO D9 / GPIO8, button to GND (INPUT_PULLUP)
static const uint32_t ERR_RETRY_MS = 60UL * 1000;     // retry sooner after a failure
static const uint32_t HOLD_PORTAL_MS = 3000;          // hold button this long -> setup portal

static uint32_t nextPollAt = 0;
static uint32_t btnDownAt = 0;
static bool     btnWasDown = false;

static void showCached() {
  if (cfg.cachedValue >= 0) Display::showCount(cfg.cachedName, cfg.cachedValue);
  else                      Display::showStatus("No data yet");
}

static void doPoll() {
  if (WiFi.status() != WL_CONNECTED) {
    Display::showStatus("WiFi lost");
    WiFi.reconnect();
    nextPollAt = millis() + ERR_RETRY_MS;
    return;
  }
  Steam::Result r = Steam::fetch();
  if (r.ok) {
    cfg.saveCache(r.name, r.value);
    Display::showCount(r.name, r.value);
    nextPollAt = millis() + (uint32_t)cfg.pollMinutes * 60UL * 1000UL;
  } else {
    Serial.println("fetch error: " + r.err);
    if (cfg.cachedValue >= 0) Display::showCount(cfg.cachedName, cfg.cachedValue);
    else                      Display::showError(r.err.substring(0, 18));
    nextPollAt = millis() + ERR_RETRY_MS;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);

  Display::begin();
  cfg.begin();
  showCached();

  bool forcePortal = (digitalRead(BTN_PIN) == LOW);  // hold button at boot = setup
  Display::showStatus(forcePortal ? "Setup..." : "Connecting...");

  if (Portal::connect(forcePortal)) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    nextPollAt = millis();   // poll immediately
  } else {
    Display::showError("No WiFi");
    nextPollAt = millis() + ERR_RETRY_MS;
  }
}

void loop() {
  cfg.pollSerial();

  // Serial-triggered actions.
  if (cfg.portalRequested) { cfg.portalRequested = false; Portal::openConfigPortal(); nextPollAt = millis(); }
  if (cfg.refreshRequested) { cfg.refreshRequested = false; nextPollAt = millis(); }

  // Button: short press = refresh now; hold = open setup portal.
  bool down = (digitalRead(BTN_PIN) == LOW);
  if (down && !btnWasDown) { btnDownAt = millis(); btnWasDown = true; }
  if (down && btnWasDown && millis() - btnDownAt > HOLD_PORTAL_MS) {
    Portal::openConfigPortal();
    nextPollAt = millis();
    btnWasDown = false;
    while (digitalRead(BTN_PIN) == LOW) delay(10);  // wait for release
  }
  if (!down && btnWasDown) {  // released before hold threshold -> refresh
    btnWasDown = false;
    if (millis() - btnDownAt < HOLD_PORTAL_MS) nextPollAt = millis();
  }

  if ((int32_t)(millis() - nextPollAt) >= 0) doPoll();

  delay(20);
}
