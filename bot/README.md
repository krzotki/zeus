# Zeus x27 kill-count bot

A small Node service that looks up the **StatTrak kill count** of your CS2 StatTrak
Zeus x27 and serves it to the Zeus display over HTTP. The ESP32 can't comfortably
parse Steam's ~350 KB inventory JSON, so the bot does that work and hands back one number.

```
Zeus firmware  ──HTTP──▶  this bot  ──HTTPS──▶  steamcommunity.com (public inventory)
     GET /kills?steam=<steamid64 | vanity>   ->   { "killeater_value": 1337, "name": "..." }
```

## Endpoints

| Route | What it does | Needs Steam login? |
|-------|--------------|--------------------|
| `GET /kills?steam=<id64\|vanity>` | Reads the count from the owner's **public** inventory. **This is what the Zeus uses.** | No |
| `GET /inspect?url=<inspect link>` | Legacy: inspects an item through the CS2 Game Coordinator | Yes |
| `GET /` | Health: `{ ready, gc }` | — |

For `/kills`, the owner's Steam inventory must be **public**. A vanity name is resolved
to a SteamID64 automatically, with no API key needed.

## Requirements

- **Node.js 18+** (or Docker) on a machine that stays on, e.g. a PC, a Raspberry Pi or a small VPS.
- Only for `/inspect`: a **Steam account** for the bot (a secondary account is fine) that owns CS2 (free).

## Setup

```bash
cd bot
npm install
cp .env.example .env      # set PORT; Steam credentials only if you want /inspect
npm start
```

Leave `STEAM_USERNAME` / `STEAM_PASSWORD` blank to run login-free (only `/kills`).
If you do set them, the first login asks for a **Steam Guard code** (unless you set
`STEAM_SHARED_SECRET`). A refresh token is then saved to `steam-data/`, so restarts
skip the prompt.

## Run with Docker

```bash
cd bot
cp .env.example .env      # set PORT=2137 (matches docker-compose.yml)

# Only if using Steam login without STEAM_SHARED_SECRET: run once interactively
# to type the Guard code, then Ctrl+C.
docker compose run --service-ports --rm zeus-bot

docker compose up -d
docker compose logs -f
```

The port mapping in `docker-compose.yml` is `2137:2137`. Keep it in sync with `PORT`
in `.env`. `steam-data/` is a volume, so the login survives container rebuilds.

## Test it

```bash
curl "http://localhost:2137/kills?steam=<your vanity or SteamID64>"
# -> {"killeater_value":1337,"name":"StatTrak™ Zeus x27 | ..."}
```

## Point the Zeus at it

Find this machine's LAN IP (`ipconfig` / `ip addr`, e.g. `192.168.1.50`) and enter it on
the Zeus (captive portal or USB config tool, "Inspect server URL"):

```
http://192.168.1.50:2137
```

The firmware appends `/kills?steam=...` itself. If the bot can't be reached, the
display falls back to its cached value.

## Testing without Steam: `fakebot.js`

`node fakebot.js` serves a fake `/kills` on port 2137. The count goes up by 1 every 10 s
(`/set?v=N` forces a value, `/peek` reads it). Use it to test the display and the level-up
sound without playing. Set the device to `SET interval 1`.
