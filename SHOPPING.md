# Zeus x27 — Parts List

Everything used in the build. Prices are rough (PLN, 2026). Any electronics store should
have these; search by the part name.

## Core
| Part | Notes | ~Price |
|------|-------|--------|
| **Seeed XIAO ESP32-S3** | The brain: WiFi, native USB, 8 MB PSRAM | ~40 zł |
| **0.96" ST7735S TFT, 80×160, SPI** | The StatTrak screen (8-pin: VCC GND SCL SDA RES DC CS BLK) | ~15 zł |
| **Momentary push button** | Refresh / setup; hold at boot for the WiFi portal | ~1 zł |
| **MAX98357A I2S amp** | Drives the speaker (the ESP32-S3 has no DAC) | ~10 zł |
| **Speaker 8Ω 0.5W** (e.g. MG24-15) | Event sounds | ~5 zł |
| **USB-C data cable** | Power, config, flashing (not charge-only) | — |
| **Double-sided perfboard, 30×70 mm** | The "motherboard" for the XIAO + amp | ~5 zł |
| **Ribbon cable, 10-wire AWG28** / 28AWG silicone wire | Screen link + hookup | ~3 zł |

## Tip LED (lightning arc)
| Part | Notes | ~Price |
|------|-------|--------|
| **Blue LED, 5 mm** | The tip flash | ~1 zł |
| **Resistors 150–220Ω + 1kΩ** | LED current limit + transistor base | ~1 zł |
| **NPN transistor** (BC337 / 2N2222) | Runs the LED off 5V for full brightness (it's dim straight off 3.3V) | ~1 zł |

## Battery screen + cordless
| Part | Notes | ~Price |
|------|-------|--------|
| **OLED 0.42" 72×40 I2C** (SSD1306) | Battery "charge" screen. Must be this tiny module: a 0.96" 128×64 won't fit | ~15 zł |
| **MAX17048 fuel gauge** (e.g. Adafruit) | Battery %. The firmware uses `Adafruit_MAX1704X`; the similar LC709203F is a different chip and won't work | ~35 zł |
| **LiPo 3.7V 500–1000 mAh** (flat, JST) | Soldered to the XIAO BAT pads; charged over USB-C | ~20 zł |
| **Rotary switch, PCB, 3-position 2-circuit** (2P3T) | Power on/off on the LiPo + line. 1A/30VDC rating, M10×0.75 panel mount, Ø6 mm × 30 mm shaft, Ø26 mm body, 30° per step | ~5 zł |

## Assembly
PLA or PETG filament · M2/M3 heat-set inserts + screws · heat-shrink · solder.

Handy tools: flux pen, helping hands / PCB vise, hot-glue gun, wire strippers, a cheap
multimeter (check continuity before powering up).

## Minimum vs. full
- **Minimum working build:** XIAO + TFT + button (+ amp and speaker for sound).
- **+ Tip strike:** LED + transistor + resistors.
- **+ Battery screen:** OLED + MAX17048 (needs the LiPo).
- **+ Cordless:** LiPo + rotary switch.

See [README.md](README.md#bill-of-materials) for how each part is wired.
