# Event sounds (WAV)

Drop your own WAV files here, then flash them to the device's on-board LittleFS
**separately from the firmware**:

```
pio run -d firmware -t uploadfs -e seeed_xiao_esp32s3
```

## Files the firmware looks for

| File             | Plays when…                                       |
|------------------|---------------------------------------------------|
| `boot.wav`       | Powered on + WiFi connected                       |
| `click.wav`      | Button pressed                                    |
| `clicklevel.wav` | Button pressed, `LEVELUP_CHANCE`% of the time     |
| `loaded.wav`     | First StatTrak value fetched this session         |
| `levelup.wav`    | Never played directly — mix source for the above   |
| `portal.wav`     | WiFi setup portal opened (hold button / setup)    |
| `error.wav`      | Never played — fetch/WiFi failures are shown only |

Any missing file just stays silent — you don't need all of them. `error.wav`
isn't shipped and isn't referenced at runtime; failures go to the display.

## The rare click (`clicklevel.wav`)

`clicklevel.wav` is `click.wav` with `levelup.wav` mixed in **starting halfway
through the click**. The firmware has no runtime mixer, so the overlap is baked
in offline; the button just rolls a die and picks this file instead
(`LEVELUP_CHANCE` in `platformio.ini`, default 10%). The birthday sound is
checked first, so it never gets the roll.

**Regenerate it whenever `click.wav` or `levelup.wav` changes** — set `adelay`
to half the click's duration (`ffprobe -show_entries format=duration click.wav`):

```
ffmpeg -y -i click.wav -i levelup.wav -filter_complex \
  "[1:a]adelay=537|537[b];[0:a][b]amix=inputs=2:duration=longest:dropout_transition=0:normalize=0,alimiter=limit=0.95[out]" \
  -map "[out]" -ac 2 -ar 22050 -c:a pcm_s16le clicklevel.wav
```

`normalize=0` matters — `amix` otherwise halves both inputs. `alimiter` catches
the sum clipping where the two overlap. Audition the result on a PC before
flashing; the overlap timing is decided entirely here.

## The daily clip (`2137.gif` + `2137.wav`)

`2137.gif` (80×160, looped on the TFT) is included. **`2137.wav` is not**, because it's
copyrighted music. Add your own clip audio (~60 s, same WAV format as below). The
WAV's length sets how long the clip runs. If the file is missing, the clip ends
right away. The time of day is set by `CLIP_HOUR` / `CLIP_MIN` in `platformio.ini`.

## Format

- **PCM WAV, 16-bit** (not MP3, not float, not ADPCM)
- Mono, **~22050 Hz** recommended (44100 also works)
- Keep them short (< ~1 s) and small — the LittleFS partition is ~4.8 MB, and
  the daily 2137 clip already claims most of it

Convert anything to the right format with ffmpeg:

```
ffmpeg -i in.mp3 -ac 1 -ar 22050 -sample_fmt s16 boot.wav
```
