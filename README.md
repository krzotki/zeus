# Zeus x27 — Functional StatTrak Replica

A life-size 3D-printed CS2 **Zeus x27** with a small color screen that shows the **live StatTrak kill count** from a real Steam account. You set WiFi and the Steam account from a phone (captive portal) or over USB.



https://github.com/user-attachments/assets/bd952927-ce93-405a-acd1-b2a3ea051d04




## Features

- **Live StatTrak count** on a 0.96" color TFT, styled like the in-game counter
- **Phone onboarding** through a WiFi captive portal, or a **USB config page** (Web Serial)
- **Event sounds** (boot, click, level-up…) through an I2S amp, played from WAVs on LittleFS
- **Tip LED** that flickers like a taser arc when you press the button
- **Battery "charge" meter** on a second tiny OLED (MAX17048 fuel gauge)
- **Daily 21:37 clip**: a GIF on the screen plus audio at a set local time (bring your own audio)
- A small **self-hosted bot** (Node, Docker-ready) that does the Steam lookups
- A **printable two-part shell**, on [Thingiverse](https://www.thingiverse.com/thing:7413723)

## How the StatTrak number is fetched

The Steam Web API has no StatTrak count. It does show up in the item's **public inventory** data as a "StatTrak™ Confirmed Kills" description line. Parsing that JSON (~350 KB) is too heavy for the ESP32, so a small bot does it:

```
Zeus firmware ──HTTP──▶ bot (bot/) ──HTTPS──▶ steamcommunity.com/inventory/<id>/730/2
      GET /kills?steam=<steamid64 | vanity>  ->  { "killeater_value": 1337 }
```

- The Steam **inventory must be public**.
- The bot also resolves a vanity name to a SteamID64 (no API key needed).
- The bot no longer needs a Steam login for this. An older `/inspect` route (Game Coordinator, needs a bot Steam account) is still there. See [bot/README.md](bot/README.md).

The count is polled every few minutes (default 5), since StatTrak only changes while you play.

## Bill of materials

> 🛒 A fuller parts list with rough prices and a minimum-vs-full breakdown is in **[SHOPPING.md](SHOPPING.md)**.

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
| LiPo 3.7V 500–1000 mAh (flat) | Cordless power, on the XIAO BAT pads (charged over USB-C); also feeds the fuel gauge |
| Rotary switch, PCB, 3-position 2-circuit (2P3T) | Power on/off on the LiPo + line. 1A/30VDC, M10×0.75 panel mount, Ø6 mm shaft, Ø26 mm body |
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

## Onboarding (set WiFi + Steam account)

First, run the bot somewhere the Zeus can reach it ([bot/README.md](bot/README.md)). There's **no default server**, so you have to enter your bot's URL.

### Option A — phone captive portal (no PC)
1. Power the Zeus. With no saved WiFi (or if you hold the button at boot), it broadcasts the WiFi network **`ZeusX27-Setup`**.
2. Join it from a phone. The setup page opens on its own (or visit `192.168.4.1`).
3. Pick your home WiFi, enter the password, then fill in the **Steam vanity name / SteamID64** and the **Inspect server URL** (e.g. `http://192.168.1.50:2137`). Save.
4. The Zeus reboots, connects and shows the count. To reconfigure later, **hold the button at boot**.

### Option B — USB
Open [config-tool/index.html](config-tool/index.html) in **Chrome/Edge**, click **Connect via USB**, pick the port and fill in the fields. Under the hood the page sends these serial commands (you can also type them in any serial monitor at 115200):

```
GET
SET wifi <ssid> <password>
SET steam <steamid64 | vanity>
SET server <bot url>
SET interval <minutes>
SET volume <0-100>        # sound volume
SET sound <0|1>           # mute / unmute
REFRESH
PORTAL
CLIP                      # play the daily 21:37 clip now
CLEAR                     # factory reset (wipes WiFi + settings)
```

## Sounds

The event sounds and the clip GIF ship in [firmware/data/](firmware/data/). Swap in your own if you like; the filenames, the format and the ffmpeg commands are in [firmware/data/README.md](firmware/data/README.md). The **21:37 clip audio (`2137.wav`) isn't included** (copyrighted music), so add your own ~60 s WAV (without it the clip is skipped). Flash the files **separately** from the firmware:

```
pio run -d firmware -t uploadfs -e seeed_xiao_esp32s3
```

A missing file just stays silent. The clip time, the birthday date and the rare-click chance are build flags in [platformio.ini](firmware/platformio.ini).

## 3D-printed shell

The printable shell (left/right halves with the screen window and USB-C slot) is on **[Thingiverse](https://www.thingiverse.com/thing:7413723)**.

Print in PLA or PETG. The seam is open on purpose so you can get to the electronics. Use M2/M3 heat-set inserts for the screws. For a full build walkthrough, see [STEPS.md](STEPS.md).

## Project layout

```
firmware/         PlatformIO project (ESP32-S3)
  platformio.ini  board + display + sound + feature build flags
  src/            Config, Portal (WiFi), Steam, Display, Sound, Tip (LED), Gauge (battery OLED), Clip, main
  data/           event WAVs + clip GIF (add your own 2137.wav), flashed with `pio run -t uploadfs`
bot/              Self-hosted kill-count service (Node, Docker)
config-tool/      Web Serial USB config page
STEPS.md          Step-by-step build guide
SHOPPING.md       Parts list with rough prices
XIAO.md           XIAO ESP32-S3 pinout cheat sheet
```

## License

The code and docs are [MIT](LICENSE). This doesn't cover any third-party assets you add yourself (e.g. the clip audio).

## Disclaimer

This is a fan project. It's not affiliated with or endorsed by Valve. Counter-Strike, CS2, StatTrak and Zeus x27 are trademarks of Valve Corporation. The optional `/inspect` route logs a Steam account into the Game Coordinator, so use a secondary account at your own risk.
