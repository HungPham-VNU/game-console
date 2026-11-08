# Brick Game firmware (F1)

C++ / Arduino / LovyanGFX firmware for the ES3C28P / ES3N28P (ESP32-S3 N16R8).
Tetris for now; Snake and Racing come in M3.

## Build, flash, monitor

No host install is needed: PlatformIO runs in Docker (`pio.sh`), and passing the device
into the container means you don't need the `dialout` group either.

```sh
./pio.sh run                 # build  (first run downloads the toolchain, ~1 GB, cached in ~/.cache/pio-docker)
./pio.sh run -t upload       # flash /dev/ttyACM0   (PORT=/dev/ttyACM1 ./pio.sh ... to change)
./pio.sh device monitor      # serial log, Ctrl+C to quit
make -C test_host            # host unit tests for core/ + games/ (g++, ASan/UBSan)
```

If you install PlatformIO natively, plain `pio run` works the same.

The factory firmware is backed up in `~/es3c28p-backup/factory-16MB.bin`.

## Layout

```
src/
  core/     platform-free: Grid, Frame, Game interface, App state machine, Rng, 3×5 font
  games/    Tetris (more games implement core/Game.h)
  render/   LcdRenderer: Frame → screen, segment-LCD look, redraws only what changed
  hal/      Display (LovyanGFX config), Input (PCF8574), Sound (stub), Storage (Preferences)
  main.cpp  wiring + boot diagnostics
test_host/  host build + unit tests of core/ and games/
```

Rule: **games only write a 10×20 `Field` (plus the NEXT box and HUD numbers in a `Frame`); only
the renderer touches the display.**

## Controls

| Button | Menu | Tetris | Anywhere |
|---|---|---|---|
| UP / DOWN | choose game | UP hard drop · DOWN soft drop | |
| LEFT / RIGHT | start speed 0–9 | move | |
| A | start | rotate | |
| START | start | pause / resume | |
| B | | | sound on/off |
| SELECT (hold 1 s) | | | end the game, back to the menu |

Without a PCF8574 on the bus, the **BOOT** button acts as **A**, so the bare board can be tested.

## Boot diagnostics (serial)

At startup the serial log prints chip, flash, PSRAM, heap and an I2C scan. Expected addresses on the
ES3C28P: `0x18` ES8311 codec, `0x38` FT6336G touch, and `0x20` once the PCF8574 is wired.
Then the screen shows RED → GREEN → BLUE → WHITE → BLACK with "TOP" at the top edge.

If the colours are wrong, set the build flags in `platformio.ini`:
- colours look like a photo negative (white shows as black) → `-DLCD_INVERT=true`
- red and blue are swapped → `-DLCD_RGB_ORDER=true`
- upside down → `-DLCD_ROTATION=0` (2 = 180°, which is what the case needs)
- skip the colour test → `-DBOOT_COLOR_TEST=0`

## Tetris rules

7-bag randomizer, wall kicks {0, −1, +1, −2, +2}, score 100/300/500/800 × (L+1) where
L = start speed + level, level up every 10 lines, gravity 800 ms × 0.85^L (min 50 ms),
300 ms line-clear flash, curtain animation at game over. High score is saved to NVS only
when a game ends.
