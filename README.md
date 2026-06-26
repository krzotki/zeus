# Zeus x27 — Functional StatTrak Replica

A real-size 3D-printed CS2 **Zeus x27** with a small color screen showing a **live StatTrak kill count** pulled from a real Steam account. WiFi onboarding and the Steam source are set from a phone (captive portal) or over USB.

## How the StatTrak number is fetched

The Steam Web API does **not** expose the StatTrak count. It lives in the item's `kill_eater` attribute and is only readable by *inspecting* the item through Steam's Game Coordinator. So the firmware does two hops:

1. **Inventory JSON** — `steamcommunity.com/inventory/<steamid64>/730/2` → find the StatTrak Zeus x27 → build its inspect link (from the item's `actions` template + assetid).
2. **Inspect API** — call [CSFloat](https://csfloat.com)'s inspect endpoint with that link → read `killeater_value`.

You can also skip step 1 by configuring a **full inspect link** directly (most reliable). Polling is every few minutes (StatTrak only changes while playing).

> The inspect-API field name / endpoint can change. Verify `iteminfo.killeater_value` on first run (see [Steam.cpp](firmware/src/Steam.cpp)). If the public API gets unreliable, swap in a self-hosted Steam bot (`steam-user` + `node-globaloffensive`).

## Bill of materials

| Part | Notes |
|------|-------|
| Seeed XIAO ESP32-S3 | Native USB (power + config + flashing), WiFi, 8MB PSRAM (for inventory parse) |
| 1.69" ST7789v2 TFT, 240×280 SPI | IPS color; mounts on a body panel at real scale. Swappable — any SPI color TFT works, just match the build flags |
| Momentary push button | Setup/refresh (to a GPIO + GND) |
| USB-C **data** cable | Power, config, flashing |
| Hookup wire, M2/M3 heat-set inserts + screws, PLA/PETG | Assembly |

## Wiring (XIAO ESP32-S3 → ST7735)

| TFT pin | XIAO pad | GPIO |
|---------|----------|------|
| VCC | 3V3 | — |
| GND | GND | — |
| SCL/SCK | D8 | 7 |
| SDA/MOSI | D10 | 9 |
| RES/RST | D3 | 4 |
| DC/A0 | D2 | 3 |
| CS | D1 | 2 |
| BLK/LEDA | 3V3 (or D0/GPIO1) | — |
| Button | D9 | 8 → GND |

Pins are set in [platformio.ini](firmware/platformio.ini) build flags (display) and `BTN_PIN` in [main.cpp](firmware/src/main.cpp). Keep wiring and flags in sync if you change them.

## Build & flash the firmware

1. Install **VS Code** + the **PlatformIO** extension.
2. Open the [firmware/](firmware/) folder. PlatformIO auto-installs the libraries on first build.
3. Plug the XIAO in via USB-C → **Build** then **Upload**. (If the first flash isn't detected: hold **BOOT**, tap **RESET**, release BOOT, retry.)
4. Open the **Serial Monitor** (115200) to watch logs.

## Onboarding (set WiFi + Steam source)

### Option A — phone captive portal (no PC)
1. Power the Zeus. With no saved WiFi (or hold the button at boot), it broadcasts WiFi **`ZeusX27-Setup`**.
2. Join that network from a phone; the setup page opens automatically (or visit `192.168.4.1`).
3. Pick your home WiFi, enter the password, and fill the **Steam source** field. Save.
4. It reboots, connects, and shows the count. To reconfigure later: **hold the button at boot**.

### Option B — USB (advanced / fallback)
Open [config-tool/index.html](config-tool/index.html) in **Chrome/Edge**, click **Connect via USB**, pick the port, then use the fields. Under the hood it sends serial commands:

```
GET
SET wifi <ssid> <password>
SET steam <steamid64 | vanity | inspect-link>
SET interval <minutes>
SET apikey <key>          # only needed to resolve vanity names
REFRESH
PORTAL
CLEAR                     # factory reset (wipes WiFi + settings)
```

**Steam source** accepts any of:
- a **SteamID64** (`76561198...`) — inventory is searched for the StatTrak Zeus (inventory must be public),
- a **vanity name** — needs a [Steam Web API key](https://steamcommunity.com/dev/apikey),
- a **full inspect link** (`steam://...preview...`) — used directly, most reliable.

## 3D shell

The accurate mesh is already here: `taser-zeus-x27-gun-model-cs2/source/TASER.glb` (~48×176×247 mm, real scale). [cad/make_shell.py](cad/make_shell.py) turns it into a printable two-part shell:

```
blender --background --python cad/make_shell.py
```

It imports the GLB, scales m→mm, decimates, solidifies into a ~2 mm shell, cuts the **screen window** at the numeral plane and a **USB-C slot**, splits into left/right halves, and exports to `cad/output/`. The cut coordinates are starting points — open the GLB in Blender, read the real positions of the numeral panel / grip, adjust `SCREEN_*` / `USB_*` at the top of the script, and re-run. Print PLA or PETG; add M2/M3 heat-set inserts for the screws.

## Verify (end-to-end)

- Count on screen matches your in-game StatTrak value; get a kill → updates within one poll cycle.
- Captive portal: change the Steam source to another public account with a StatTrak Zeus → display switches; settings survive a power-cycle.
- Failure states show clearly (private inventory / no Zeus / no WiFi / rate-limit) without reboot loops.
- All electronics close inside the shell; screen readable through the window; USB-C reachable without opening.

## Project layout

```
firmware/         PlatformIO project (ESP32-S3)
  platformio.ini  board + display + USB build flags
  src/            Config, Portal (WiFi), Steam, Display, main
config-tool/      Web Serial USB config page
cad/make_shell.py Blender script: GLB -> printable split shell
taser-...-cs2/    source GLB + textures
```
