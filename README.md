# Zeus x27 — Functional StatTrak Replica

A real-size 3D-printed CS2 **Zeus x27** with a small color screen showing a **live StatTrak kill count** pulled from a real Steam account. WiFi onboarding and the Steam source are set from a phone (captive portal) or over USB.

## Project status (2026-07)

**Firmware — complete & compiling** ([firmware/](firmware/)): in-game-style StatTrak readout, WiFi captive-portal + USB onboarding, live count via the self-hosted bot, event **sounds** (your WAVs on LittleFS), the **tip LED** lightning-arc on button press, and a **battery "charge" meter** on a 2nd I2C OLED (MAX17048 fuel gauge).
**Bot — deployed** ([bot/](bot/)): Dockerized; resolves the vanity name and scrapes the live StatTrak count; running at the configured inspect server.
**Bench — working**: XIAO + 0.96" screen verified on breadboard, count displays, colours/offset dialled in.

**Remaining to finish the build:**
- [ ] Buy + wire the **sound** parts (MAX98357A amp), **tip LED** (LED + resistor, transistor for brightness), and **battery screen** (I2C OLED + MAX17048) — see [Bill of materials](#bill-of-materials).
- [ ] Drop your `.wav` files in [firmware/data/](firmware/data/) and flash them (`pio run -t uploadfs`).
- [ ] Solder everything onto the perfboard and connect the screen via ribbon.
- [ ] Go **cordless**: LiPo + slide switch on the XIAO BAT pads (also feeds the fuel gauge).
- [ ] Print the shell ([cad/make_shell.py](cad/make_shell.py)) and do final assembly.

## How the StatTrak number is fetched

The Steam Web API does **not** expose the StatTrak count. It lives in the item's `kill_eater` attribute and is only readable by *inspecting* the item through Steam's Game Coordinator. So the firmware does two hops:

1. **Inventory JSON** — `steamcommunity.com/inventory/<steamid64>/730/2` → find the StatTrak Zeus x27 → build its inspect link (from the item's `actions` template + assetid). *(Skip this by configuring a full inspect link directly — most reliable.)*
2. **Inspect via the bot** — send that inspect link to your **self-hosted inspect bot** ([bot/](bot/)) → it queries the Game Coordinator → returns `killeater_value`.

Polling is every few minutes (StatTrak only changes while playing).

> **Why a self-hosted bot?** The free public inspect APIs (CSFloat/CSGOFloat etc.) are currently rate-limited/blocked by Valve (`"Bots are temporarily not allowed"`). The bot in [bot/](bot/) logs a Steam account into CS2 and inspects items itself — reliable and under your control. Set its URL on the Zeus as the **inspect server** (USB config tool or portal). See [bot/README.md](bot/README.md).

## Bill of materials

> 🛒 **Where to buy** (Kamami / Botland / Allegro links, prices, one-store option): see **[SHOPPING.md](SHOPPING.md)**.

| Part | Notes |
|------|-------|
| Seeed XIAO ESP32-S3 | Native USB (power + config + flashing), WiFi, 8MB PSRAM (for inventory parse) |
| 0.96" ST7735S TFT, 80×160 SPI | IPS color; mounts on a body panel. Swappable — any SPI color TFT works, just match the build flags |
| Momentary push button | Setup/refresh (to a GPIO + GND) |
| MAX98357A I2S amp | Drives the speaker; needed because ESP32-S3 has no DAC and can't drive 8Ω directly |
| Speaker, 8Ω 0.5W (e.g. MG24-15) | Event sounds |
| Blue LED (tip) + ~150–220Ω resistor | Lightning-arc flash on button press. For full brightness add a small NPN transistor (2N2222/BC337) so it runs off 5V — a blue LED is dim straight off a 3.3V pin |
| 0.42" OLED 72×40 **I2C** (SSD1306) | 2nd screen: battery "charge" meter. Tiny module (~12×17mm) so it fits; I2C 4-pin, addr 0x3C. Driven by U8g2 (72×40 profile) |
| MAX17048 LiPo fuel gauge | Accurate battery %; shares the OLED's I2C bus (addr 0x36). Only meaningful with the LiPo |
| USB-C **data** cable | Power, config, flashing |
| Double-sided perfboard, ~30×70mm | The "motherboard": solder the XIAO + amp here, everything wires to it |
| Ribbon cable, 10-wire (AWG28) | Screen link (8 wires) + spares to button/speaker |
| Hookup wire, M2/M3 heat-set inserts + screws, PLA/PETG | Assembly |

## Wiring (XIAO ESP32-S3 → ST7735)

| TFT pin | XIAO pad | GPIO |
|---------|----------|------|
| VCC | 3V3 | — |
| GND | GND | — |
| SCL/SCK | D8 | 7 |
| SDA/MOSI | D10 | 9 |
| RES/RST | **3V3** | — (software reset; frees GPIO4 for I2C — see below) |
| DC/A0 | D2 | 3 |
| CS | D1 | 2 |
| BLK/LEDA | **3V3** | — (always on; frees GPIO1 for I2C — see below) |
| Button | D9 | 8 → GND |

> **Note:** RST and BL go to **3V3** (not GPIOs). This frees GPIO4 + GPIO1 for the I2C battery
> screen. The main display uses software reset and a fixed-on backlight.

Pins are set in [platformio.ini](firmware/platformio.ini) build flags (display) and `BTN_PIN` in [main.cpp](firmware/src/main.cpp). Keep wiring and flags in sync if you change them.

### Battery screen (XIAO ESP32-S3 → OLED + MAX17048, shared I2C)

Both devices sit on one 2-wire I2C bus (SDA=**D0/GPIO1**, SCL=**D3/GPIO4**, reclaimed from the TFT BL/RST pins):

| Device | VCC | GND | SDA | SCL | Extra |
|--------|-----|-----|-----|-----|-------|
| OLED SSD1306 0.42" 72×40 | 3V3 | GND | D0 | D3 | I2C addr 0x3C; **pin order GND/VCC/SCL/SDA** |
| MAX17048 fuel gauge | 3V3 | GND | D0 | D3 | addr 0x36; **CELL/BAT → LiPo +** (same node as XIAO BAT+) |

The OLED shows a battery outline + `NN%` (with a `CHG` marker while charging). A missing gauge just shows `USB`.

### Sound (XIAO ESP32-S3 → MAX98357A → speaker)

| MAX98357A | XIAO pad | GPIO | Note |
|-----------|----------|------|------|
| VIN | 5V | — | Louder than 3V3; the 0.5W speaker is safe |
| GND | GND | — | |
| LRC | D5 | 6 | I2S word-select |
| BCLK | D4 | 5 | I2S bit clock |
| DIN | D6 | 43 | I2S data |
| GAIN | — | — | Leave floating (9 dB); GND = +12 dB louder |
| SD | — | — | Leave floating (enabled, mono) |
| Speaker +/− | to speaker | | Polarity not critical |

I2S pins are the `-D I2S_*` flags in [platformio.ini](firmware/platformio.ini).

### Tip LED (blue lightning arc)

Flickers on button press to mimic a taser arc. Pin is `-D TIP_LED_PIN` (GPIO44 / **D7**, the last free pin).

- **Dim / simplest:** `D7 → resistor (150–220Ω) → LED(+) → LED(−) → GND`. Blue is faint at 3.3V.
- **Bright (recommended):** drive from 5V through an NPN transistor —
  `D7 → 1kΩ → transistor base`, `LED(+) → 5V`, `LED(−) → resistor (150–220Ω) → transistor collector`, `emitter → GND`.
- A **WS2812** at the tip is a drop-in upgrade (one data wire on D7, off 5V) if you want color/animation later — it needs a small firmware change in [Tip.cpp](firmware/src/Tip.cpp).

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
REFRESH
PORTAL
CLEAR                     # factory reset (wipes WiFi + settings)
```

**Steam source** accepts any of:
- a **SteamID64** (`76561198...`) — inventory is searched for the StatTrak Zeus (inventory must be public),
- a **vanity name** — needs a [Steam Web API key](https://steamcommunity.com/dev/apikey),
- a **full inspect link** (`steam://...preview...`) — used directly, most reliable.

## Sounds

The Zeus plays your own **WAV files** through the MAX98357A on events:

| File (in [firmware/data/](firmware/data/)) | Plays when… |
|--------|-------------|
| `boot.wav` | Powered on + WiFi connected |
| `click.wav` | Button pressed |
| `loaded.wav` | First StatTrak value fetched this session |
| `levelup.wav` | StatTrak count goes **up** (a kill landed) |
| `portal.wav` | WiFi setup portal opened |
| `error.wav` | Fetch or WiFi failed |

Drop the files in [firmware/data/](firmware/data/) and flash them **separately** from the firmware:

```
pio run -d firmware -t uploadfs -e seeed_xiao_esp32s3
```

Format: **16-bit PCM WAV**, mono, ~22050 Hz, short. Any missing file just stays silent. Convert with:
`ffmpeg -i in.mp3 -ac 1 -ar 22050 -sample_fmt s16 boot.wav`. Playback is always at full volume — set each clip's loudness in the WAV itself (e.g. ffmpeg `-af loudnorm`).

## 3D shell

Base mesh: `taser-zeus-x27-gun-model-cs2/zeus.stl`. [cad/make_shell.py](cad/make_shell.py) turns it into a printable two-part shell (verified to run in Blender 5.1):

```
blender --background --python cad/make_shell.py
```

It imports the STL, **auto-scales to real mm** (longest dim = `TARGET_LONGEST_MM`, default 247), reports manifold status, makes it a closed manifold (the base mesh is non-manifold, so `USE_REMESH=True` SHARP remesh at `OCTREE_DEPTH=9` ≈ 0.5 mm), solidifies a ~2 mm wall, optionally cuts the screen window + USB-C slot, splits into left/right halves at the symmetry plane, and writes `cad/output/zeus_left.stl` / `zeus_right.stl` (~33 MB each at depth 9).

Workflow:
1. First run with `DO_CUTS=False` → clean hollow halves. The script prints the model bounds.
2. Open the model in Blender, read the screen-panel / USB coordinates, fill in `SCREEN_*` / `USB_*`, set `DO_CUTS=True`, re-run.
3. Tune `OCTREE_DEPTH` (higher = finer/heavier) and `WALL_MM` to taste.

Print PLA or PETG; the seam is open by design (that's the access for electronics); add M2/M3 heat-set inserts for the screws.

## Verify (end-to-end)

- Count on screen matches your in-game StatTrak value; get a kill → updates within one poll cycle.
- Captive portal: change the Steam source to another public account with a StatTrak Zeus → display switches; settings survive a power-cycle.
- Failure states show clearly (private inventory / no Zeus / no WiFi / rate-limit) without reboot loops.
- All electronics close inside the shell; screen readable through the window; USB-C reachable without opening.

## Project layout

```
firmware/         PlatformIO project (ESP32-S3)
  platformio.ini  board + display + sound + USB build flags
  src/            Config, Portal (WiFi), Steam, Display, Sound, Tip (LED), Gauge (battery OLED), main
  data/           event WAV files (flashed with `pio run -t uploadfs`)
bot/              Self-hosted Steam Game Coordinator inspect service (Node)
config-tool/      Web Serial USB config page
cad/make_shell.py Blender script: zeus.stl -> printable split shell
taser-...-cs2/    base zeus.stl (+ source GLB + textures)
```
