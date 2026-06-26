#pragma once
#include <Arduino.h>

// On-device WiFi onboarding via captive portal (no PC needed).
// The owner joins the "ZeusX27-Setup" hotspot from a phone; a page opens where
// they pick their WiFi and enter their Steam source. Built on tzapu/WiFiManager.
namespace Portal {
  // Try saved WiFi. If none/can't connect (or forcePortal), open the captive
  // portal and block until configured or timeout. Returns true if connected.
  bool connect(bool forcePortal);

  // Force-open the config portal at runtime (button hold / serial PORTAL).
  void openConfigPortal();
}
