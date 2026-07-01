# Event sounds (WAV)

Drop your own WAV files here, then flash them to the device's on-board LittleFS
**separately from the firmware**:

```
pio run -d firmware -t uploadfs -e seeed_xiao_esp32s3
```

## Files the firmware looks for

| File          | Plays when…                                   |
|---------------|-----------------------------------------------|
| `boot.wav`    | Powered on + WiFi connected                   |
| `click.wav`   | Button pressed                                |
| `loaded.wav`  | First StatTrak value fetched this session     |
| `levelup.wav` | StatTrak count goes **up** (a kill landed)    |
| `portal.wav`  | WiFi setup portal opened (hold button / setup)|
| `error.wav`   | Fetch or WiFi failed                           |

Any missing file just stays silent — you don't need all six.

## Format

- **PCM WAV, 16-bit** (not MP3, not float, not ADPCM)
- Mono, **~22050 Hz** recommended (44100 also works)
- Keep them short (< ~1 s) and small — the LittleFS partition is only ~1.5 MB

Convert anything to the right format with ffmpeg:

```
ffmpeg -i in.mp3 -ac 1 -ar 22050 -sample_fmt s16 boot.wav
```
