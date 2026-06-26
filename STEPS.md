# Zeus x27 Build — Step by Step

Follow top to bottom. Don't print the shell until the electronics work on the bench (steps 1–5).

---

## 1. Buy parts

- [ ] Seeed XIAO ESP32-S3
- [ ] 1.69" ST7789v2 color TFT, 240×280, SPI (8-pin: VCC GND SCL SDA RES DC CS BLK)
      — or any SPI color TFT that's in stock; tell me the exact model and I'll set the driver flags
- [ ] Momentary push button
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
| RES/RST | D3 | 4 |
| DC/A0 | D2 | 3 |
| CS | D1 | 2 |
| BLK/LEDA | 3V3 | — |
| Button (one leg) | D9 | 8 |
| Button (other leg) | GND | — |

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

**Easiest (phone):**
- [ ] Power the device. It broadcasts WiFi **`ZeusX27-Setup`** (hold the button at boot if it doesn't).
- [ ] Join that WiFi from your phone → setup page opens (or go to `192.168.4.1`).
- [ ] Pick your home WiFi + password.
- [ ] In **Steam source**, enter your **SteamID64** (or a full inspect link).
- [ ] Save. Device reboots, connects, shows the count.

**Or via USB (advanced):**
- [ ] Open `config-tool/index.html` in Chrome/Edge → **Connect via USB** → pick the port.
- [ ] Fill WiFi, Steam source, Save.

**Verify it works:**
- [ ] Number on screen matches your in-game StatTrak Zeus x27 count.
- [ ] (Optional) get a kill in-game → it updates within ~one poll cycle (default 5 min).
- [ ] Power-cycle → settings stick, count comes back.

> Your Steam inventory must be **public** for SteamID lookup. If it fails, use a full inspect link instead. Vanity names need a Steam API key.

## 6. Make the 3D shell

- [ ] Open a terminal in the project folder and run:
      `blender --background --python cad/make_shell.py`
      (or open `cad/make_shell.py` in Blender's Scripting tab and Run)
- [ ] Output appears in `cad/output/zeus_left.stl` and `zeus_right.stl`.
- [ ] Open the GLB in Blender to read real positions; adjust `SCREEN_*` and `USB_*`
      at the top of `cad/make_shell.py` so the screen window + USB slot line up. Re-run.

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
- Count shows `----` or an error → inventory private, no StatTrak Zeus, or inspect API down. Try a direct inspect link. Confirm `killeater_value` field in `firmware/src/Steam.cpp`.
