# Zeus x27 inspect bot

Small local service that reads the **StatTrak kill count** for your CS2 item by
inspecting it through Valve's Game Coordinator, and serves it to the Zeus x27
display over HTTP. Use this because the free public inspect APIs (CSFloat etc.)
are currently rate-limited/blocked by Valve.

```
Zeus firmware  ──HTTP──▶  this bot  ──Game Coordinator──▶  Steam
             GET /inspect?url=<inspect link>   ->   { "killeater_value": 1337 }
```

## Requirements

- **Node.js 18+** on a machine that stays on (your PC, a Raspberry Pi, a small VPS).
- A **Steam account** for the bot. A throwaway/secondary account is fine; it must
  own CS2 (free) so it can open a Game Coordinator session. It does **not** need to
  own the Zeus — it can inspect any valid inspect link.

## Setup

```bash
cd bot
npm install
cp .env.example .env      # then edit .env with the bot account credentials
npm start
```

On first login you'll be asked for a **Steam Guard code** (unless you set
`STEAM_SHARED_SECRET`). After that a refresh token is saved to `steam-data/` and
restarts skip the prompt. Wait for:

```
Connected to CS2 Game Coordinator — ready to inspect.
Inspect bot HTTP listening on :3000
```

## Test it

```bash
curl "http://localhost:3000/"                       # {"ready":true,"gc":true}
curl "http://localhost:3000/inspect?url=steam://run/730//+csgo_econ_action_preview%20S76561198...A...D..."
# -> {"killeater_value":1337,"score_type":0,...}
```

**Inspect link format matters.** The most reliable link is an *owned-item* link with
`S…A…D…` (right-click the item in CS2 → **Copy Inspect Link**). Bare hex/"masked"
links may not resolve through the GC.

## Point the Zeus at it

Find this machine's LAN IP (`ipconfig` / `ip addr`, e.g. `192.168.1.50`), then set
it on the Zeus (USB config tool or portal, "Inspect server URL"):

```
http://192.168.1.50:3000
```

The firmware appends `/inspect?url=<your inspect link>` automatically. Keep the bot
running whenever you want live updates; the display falls back to its cached value
when the bot is unreachable.
