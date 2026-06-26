#pragma once
#include <Arduino.h>

// Resolves the configured Steam source to a live StatTrak kill count.
//
// The Steam Web API does NOT expose the StatTrak number directly. We get it by
// "inspecting" the item via a public inspect API (CSFloat). Flow:
//   inspect link  -> used as-is
//   SteamID64     -> inventory JSON -> find StatTrak Zeus x27 -> build inspect link
//   vanity name   -> ResolveVanityURL (needs Steam API key) -> SteamID64 -> ...
namespace Steam {
  struct Result {
    bool    ok    = false;
    int32_t value = -1;          // kill count, -1 on failure
    String  name  = "Zeus x27";
    String  err;
  };

  Result fetch();                // uses the global cfg
}
