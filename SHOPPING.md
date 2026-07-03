# Zeus x27 — Shopping List

Everything needed to build the Zeus x27, with links across **Kamami**, **Botland**, and **Allegro**.
Prices are approximate (PLN) and vary by seller.

> **One-store tip:** [botland_pl on Allegro](https://allegro.pl/uzytkownik/botland_pl) (or Kamami's
> Allegro store) carries nearly everything below in a single order with buyer protection — only a
> plain slide switch may need a generic seller.

---

## ✅ Already have
XIAO ESP32-S3 · 0.96" ST7735 SPI screen · push button · MG24-15 speaker (8Ω 0.5W) · blue LEDs · USB-C data cable

---

## 🛒 Core — required
| Part | Kamami | Botland | Allegro | ~Price |
|------|--------|---------|---------|--------|
| **MAX98357 I2S amp** (sound) | [583663](https://kamami.pl/en/amplifier-modules/583663-class-d-3w-audio-amplifier-module-with-max98357.html) | [SparkFun MAX98357A](https://botland.com.pl/en/mp3-wav-oog-midi/13062-max98357a-decoder-stereo-dac-i2s-sparkfun-dev-14809.html) | [DAC I2S 3W](https://allegro.pl/oferta/wzmacniacz-max98357a-dekoder-dac-i2s-3w-18141751879) | ~10 zł |
| **Perfboard 30×70** double-sided | [KAmod Proto 30×70](https://kamami.pl/plytki-uniwersalne/1201646-kamod-proto-30x70-dwustronna-plytka--5902186330986.html) | [30×70mm](https://botland.com.pl/pl/plytki-uniwersalne/2715-plytka-uniwersalna-dwustronna-30x70mm.html) | — | ~5 zł |
| **Hookup wire / ribbon** | [28AWG silicone set](https://kamami.pl/en/single-core-cables/1178402-wire28awgsilicone6-colorsbox-5906623468461.html) | [TLWY 8-wire ribbon](https://botland.com.pl/przewody-wielozylowe/16269-przewod-wstazkowy-tlwy-8x035mmawg-22-wielokolorowy-50m-5904422325626.html) | [AWG28 10-wire ribbon](https://allegro.pl/oferta/tasma-kolorowa-awg28-do-gniazda-idc-10-zyl-1mb-7308381579) | ~3 zł |

---

## 💡 Tip LED — bright lightning arc
| Part | Kamami | Botland | Allegro | ~Price |
|------|--------|---------|---------|--------|
| **NPN transistor** (BC337/2N2222) | [Transistor set 200pc](https://kamami.pl/bipolarne/1184128-zestaw-200-sztuk-tranzystorow-bipolarnych-pnp-i-npn-w-organizerze-5906623487844.html) | [BC337-40, 5pc](https://botland.com.pl/pl/tranzystory-bipolarne/1239-tranzystor-bipolarny-npn-bc337-40-45v08a-5szt.html) | — | ~3 zł |
| **Resistors** (150–220Ω + 1kΩ) | [Resistor kits](https://kamami.pl/en/13554-sets-of-resistors) | [LED+resistor set 160pc](https://botland.com.pl/pl/diody-led/11552-zestaw-diod-led-5mm-160szt-z-rezystorami-organizer-5903351240253.html) | — | ~5 zł |
| **Blue LED** *(if needed)* | [5mm blue, 10pc](https://kamami.pl/jednokolorowe/1187175-led-5mm-niebieska-10-szt-5906623489046.html) | [5mm blue, 10pc](https://botland.com.pl/led-5mm-dyfuzyjne/459-dioda-led-5mm-niebieska-10-szt.html) | — | ~2 zł |

*Plain LED off a 3.3V pin is dim — the transistor lets it run off 5V for a real strike.*

---

## 🔋 Battery screen + cordless
| Part | Kamami | Botland | Allegro | ~Price |
|------|--------|---------|---------|--------|
| **OLED 0.42" 72×40 I2C** (SSD1306) | — | — | [oled 0.42 72x40 i2c](https://allegro.pl/listing?string=oled%200.42%2072x40%20i2c) | ~12–18 zł |
| **MAX17048 fuel gauge** | — | [LC709203F *(alt chip, diff lib)*](https://botland.com.pl/wskazniki-rozladowania/18235-lc709203f-wskaznik-poziomu-naladowania-akumulatora-li-pol-li-ion-stemma-qt-qwiic-adafruit-4712.html) | [Adafruit MAX17048](https://allegro.pl/oferta/stemma-qt-max17048-lipoly-liion-fuel-gauge-monitor-pracy-akumulatora-16052758177) | ~34.50 zł |
| **LiPo 3.7V 500–1000mAh** (flat) | [Akyga 500mAh](https://kamami.pl/akumulatory/1202736-akumulator-litowo-polimerowy-akyga-aky0823-lp542439-li-po-3-7v-500mah-pcm-z-cze-jst-2-54-2pin-150mm-5906574243384.html) | [Akyga 750mAh 1S](https://botland.com.pl/pl/akumulatory-li-pol/6035-akumulator-li-pol-akyga-750mah-1s-37v.html) | [search: lipo 3.7v 500mah](https://allegro.pl/listing?string=lipo+3.7v+500mah) | ~15–25 zł |
| **Slide switch** (through-hole) | [MSK-01 SPDT](https://kamami.pl/en/slide-switches/557802-msk-01-switch-5906623454815.html) | — | generic seller | ~2 zł |

> OLED must be the **0.42" 72×40 I2C** (4-pin) SSD1306 — the 0.96" 128×64 module is too big to fit
> next to the StatTrak screen. Firmware drives it via **U8g2** (72×40 profile). The MAX17048 matches
> the firmware's `Adafruit_MAX1704X` lib — the Botland LC709203F is a *different chip* needing
> `Adafruit_LC709203F` instead.

---

## 🔧 QoL tools — optional but worth it
Flux pen · helping-hands / PCB vise · hot-glue gun · heat-shrink (`koszulka termokurczliwa`) · wire strippers · cheap multimeter (continuity check before powering). Get wherever's cheapest.

---

## Minimum vs. full
- **Minimum working build:** the 3 **Core** parts (amp, perfboard, wire).
- **+ Tip strike:** transistor + resistors.
- **+ Battery screen:** OLED + MAX17048 (needs the LiPo).
- **+ Cordless:** LiPo + slide switch.

See [README.md](README.md#bill-of-materials) for how each part wires in.
