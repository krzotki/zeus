# Zeus x27 Build — Step by Step

Follow top to bottom. Don't print the shell until the electronics work on the bench (steps 1–5).

---

## 1. Buy parts

> 🛒 Full parts list with rough prices in **[SHOPPING.md](SHOPPING.md)**.

- [ ] Seeed XIAO ESP32-S3
- [ ] 0.96" ST7735S color TFT, 80×160, SPI (8-pin: VCC GND SCL SDA RES DC CS BLK)
      — or any SPI color TFT; adjust the display build flags in `firmware/platformio.ini` to match
- [ ] Momentary push button
- [ ] MAX98357A I2S amplifier board
- [ ] Speaker, 8Ω 0.5W (e.g. MG24-15)
- [ ] Blue LED for the tip + ~150–220Ω resistor (+ optional NPN transistor for full brightness)
- [ ] 0.42" OLED 72×40 **I2C** (SSD1306) — the battery "charge" screen (tiny ~12×17mm module so it fits)
- [ ] MAX17048 LiPo fuel gauge module (battery %)
- [ ] LiPo 3.7V 500–1000 mAh + a **rotary switch** (PCB, 3-position 2-circuit, M10 mount) for power (for cordless)
- [ ] USB-C **data** cable (not charge-only)
- [ ] Hookup wire / jumpers; soldering iron + solder
- [ ] Filament (PLA or PETG)
- [ ] M2 or M3 heat-set inserts + screws (for closing the shell)

## 2. Install software (on your PC)

- [ ] Install **VS Code** → https://code.visualstudio.com
- [ ] In VS Code, install the **PlatformIO IDE** extension
- [ ] Install **Blender** → https://www.blender.org/download
- [ ] (Chrome or Edge browser — needed for the USB config page)

## 3. Wire the electronics (breadboard first)

Connect XIAO ESP32-S3 → ST7735:

| TFT pin | XIAO pad | GPIO |
|---------|----------|------|
| VCC | 3V3 | — |
| GND | GND | — |
| SCL/SCK | D8 | 7 |
| SDA/MOSI | D10 | 9 |
| RES/RST | **3V3** | — (software reset; frees D3 for I2C) |
| DC/A0 | D2 | 3 |
| CS | D1 | 2 |
| BLK/LEDA | **3V3** | — (always on; frees D0 for I2C) |
| Button (one leg) | D9 | 8 |
| Button (other leg) | GND | — |

Then the sound amp — XIAO → MAX98357A (speaker on its output):

| MAX98357A | XIAO pad | GPIO |
|-----------|----------|------|
| VIN | 5V | — |
| GND | GND | — |
| LRC | D5 | 6 |
| BCLK | D4 | 5 |
| DIN | D6 | 43 |
| GAIN / SD | leave unconnected | — |
| Speaker +/− | to speaker | — |

Then the battery screen — OLED + MAX17048 on one **I2C** bus (SDA=**D0**, SCL=**D3**):

| Device | VCC | GND | SDA | SCL |
|--------|-----|-----|-----|-----|
| OLED SSD1306 0.42" 72×40 I2C | 3V3 | GND | D0 | D3 |  (pin order on module: GND/VCC/SCL/SDA)
| MAX17048 fuel gauge | 3V3 | GND | D0 | D3 |

(MAX17048 CELL/BAT input → LiPo +, same node as the XIAO BAT+ pad.)

Tip LED (blue arc) on **D7 / GPIO44**:
- Simple/dim: `D7 → 150–220Ω → LED(+) → LED(−) → GND`.
- Bright: `D7 → 1kΩ → NPN base`; `LED(+) → 5V`, `LED(−) → 150–220Ω → collector`, `emitter → GND`.

- [ ] Double-check 3V3 and GND before powering.

## 4. Flash the firmware

- [ ] Open the `firmware/` folder in VS Code (PlatformIO).
- [ ] Wait for PlatformIO to auto-install libraries (first time, ~minutes).
- [ ] Plug XIAO in via USB-C.
- [ ] Click **PlatformIO: Upload** (→ arrow in the bottom bar).
  - If not detected: hold **BOOT**, tap **RESET**, release BOOT, upload again.
- [ ] Open **PlatformIO: Serial Monitor** (115200 baud) to read logs.
- [ ] Screen should light up and show "Connecting..." / "WiFi Setup".

## 5. Configure WiFi + Steam, then test

- [ ] Run the kill-count bot somewhere the Zeus can reach (see [bot/README.md](bot/README.md)) and note its URL, e.g. `http://192.168.1.50:2137`.

**Easiest (phone):**
- [ ] Power the device. It broadcasts WiFi **`ZeusX27-Setup`** (hold the button at boot if it doesn't).
- [ ] Join that WiFi from your phone → setup page opens (or go to `192.168.4.1`).
- [ ] Pick your home WiFi + password.
- [ ] Enter your **Steam vanity name or SteamID64** and the **Inspect server URL** (your bot).
- [ ] Save. Device reboots, connects, shows the count.

**Or via USB (advanced):**
- [ ] Open `config-tool/index.html` in Chrome/Edge → **Connect via USB** → pick the port.
- [ ] Fill in WiFi, Steam name and server URL, then save.

**Verify it works:**
- [ ] Number on screen matches your in-game StatTrak Zeus x27 count.
- [ ] (Optional) get a kill in-game → it updates within ~one poll cycle (default 5 min).
- [ ] Power-cycle → settings stick, count comes back.

> Your Steam inventory must be **public**: the bot reads the count from the public inventory data.

## 5b. Add sounds (optional)

- [ ] Put your WAV files in `firmware/data/` named `boot.wav`, `click.wav`, `loaded.wav`,
      `levelup.wav`, `portal.wav`, `error.wav` (any you skip just stay silent).
      Format: **16-bit PCM WAV**, mono, ~22050 Hz, short. See `firmware/data/README.md`.
- [ ] Flash them to the device (separate from the firmware upload):
      **PlatformIO → Project Tasks → Platform → Upload Filesystem Image**
      (or `pio run -d firmware -t uploadfs -e seeed_xiao_esp32s3`).
- [ ] Set **volume** / **sound on** in the phone portal or USB config page.
- [ ] Power-cycle → boot sound plays; press the button → click; a kill in-game → level-up sound.

## 5c. Battery screen + cordless (optional)

- [ ] Wire the **OLED + MAX17048** on the I2C bus (table in step 3). Power up → the OLED shows
      a battery meter (`USB` if no gauge/LiPo is connected yet).
- [ ] Go cordless: solder the **LiPo** to the XIAO **BAT+ / BAT−** pads (meter polarity first!),
      put the **rotary switch** on the + line, and tap the LiPo + to the gauge's CELL input.
- [ ] The XIAO charges the LiPo over USB-C. Run on battery → the % drops; plug USB → shows `CHG`.

## 6. Make the 3D shell

- [ ] Run (first pass, no holes — gives clean halves):
      `blender --background --python cad/make_shell.py`
      (or open `cad/make_shell.py` in Blender's Scripting tab and Run)
- [ ] Output appears in `cad/output/zeus_left.stl` and `zeus_right.stl` (auto-scaled to real mm).
- [ ] Open the model in Blender, read the screen-panel + USB positions, fill in `SCREEN_*`
      and `USB_*` at the top of `cad/make_shell.py`, set `DO_CUTS=True`, and re-run.
- [ ] Adjust `OCTREE_DEPTH` (detail) / `WALL_MM` (wall thickness) to taste.

## 7. Print + test fit

- [ ] Slice and print the two halves (PLA or PETG).
- [ ] **Dry-fit** the XIAO + screen + button + USB cable inside before gluing/screwing anything.
- [ ] If parts don't fit, tweak wall thickness / cut sizes in the script and reprint.

## 8. Final assembly

- [ ] Install heat-set inserts into the bosses.
- [ ] Mount screen behind the window; mount the board; route the USB-C cable to its slot.
- [ ] Wire/solder permanently (shorten jumpers as needed).
- [ ] Close the shell with screws.
- [ ] Power up → done. Reconfigure later by holding the button at boot.

---

### If something breaks
- Screen blank/garbled → check wiring; if colors/offset wrong, tweak the display flags in `firmware/platformio.ini` (driver, `CGRAM_OFFSET`, `TFT_INVERSION_ON`, `TFT_RGB_ORDER`).
- "No WiFi" → re-run setup (hold button at boot).
- Count shows `----` or an error → inventory private, no StatTrak Zeus, or the bot unreachable. Check the bot logs and `curl http://<bot>/kills?steam=<you>`.
