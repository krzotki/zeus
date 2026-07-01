#pragma once
#include <Arduino.h>

// Resolves the configured Steam source to a live StatTrak kill count.
//
// The count comes from the owner's PUBLIC community inventory JSON, where Steam puts
// it as a tooltip line {"name":"stattrak_score","value":"StatTrak™ Confirmed Kills: N"}.
// The bot (see bot/) does that fetch+parse; the ESP32 just asks it:
//   GET <cfg.serverBase>/kills?steam=<SteamID64|vanity>  ->  { "killeater_value": N }
// Requires the owner's inventory to be public. Updates after matches, like Steam.
namespace Steam {
  struct Result {
    bool    ok    = false;
    int32_t value = -1;          // kill count, -1 on failure
    String  name  = "Zeus x27";
    String  err;
  };

  Result fetch();                // uses the global cfg
}
