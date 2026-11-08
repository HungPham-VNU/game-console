# Retro-Go target `es3c28p` (Pocket Brick Console)

Retro-Go for the LCDwiki **ES3C28P / ES3N28P** 2.8" ESP32-S3 board (N16R8). Target files:
`components/retro-go/targets/es3c28p/` (`config.h`, `env.py`, `sdkconfig`).

## Build & flash (Docker, no host ESP-IDF)

```sh
./build-es3c28p-docker.sh          # → retro-go_<ver>_es3c28p.img (launcher + retro-core)
./build-es3c28p-docker.sh flash    # build, then write the .img to /dev/ttyACM0
APPS="launcher retro-core gwenesis" ./build-es3c28p-docker.sh   # add Mega Drive, etc.
```

It uses `espressif/idf:v4.4.8` and applies `tools/patches/*(esp-idf 4).diff` inside the
throwaway container on each run. Serial log: `../../brick-game/pio.sh bootlog` (the console
is on the USB-Serial/JTAG port).

Back to Brick Game: `cd ../../brick-game && ./pio.sh run -t upload`.

## What is configured

| Part | Setting |
|---|---|
| Screen | ILI9341 240×320 **portrait**, MADCTL rotation 4 (180°, board upside down in the case), BGR, 40 MHz, PWM backlight IO45 |
| Buttons | PCF8574 @0x20 (`RG_I2C_GPIO_DRIVER 5`), P0..P7 = UP DOWN LEFT RIGHT A B START SELECT, active low |
| Combos | MENU = START+SELECT, OPTION = SELECT+A, hold DOWN at boot = recovery |
| SD card | SDMMC 1-bit: CLK 38, CMD 40, D0 39 |
| Battery | ADC1 ch 8 (IO9), 200K/200K divider (×2), 3.5–4.2 V = 0–100 % |
| Audio | I2S BCK 5 / WS 7 / DATA 8, amp enable IO1 (inverted: low = on). **Silent**: the ES8311 codec is not initialised by Retro-Go yet (M6) |
| Console | USB-Serial/JTAG, log level INFO |
| SNES | disabled (too slow), as in the gmt024-s3 target |

Fork changes outside the target folder:
- `components/retro-go/config.h`: `RG_TARGET_ES3C28P` added to the target list.
- `components/retro-go/rg_i2c.c`: `rg_i2c_gpio_read_port()` returns -1 without touching the bus
  when the extender failed to initialise (otherwise the input task logs ~90 I2C errors/s while
  the PCF8574 isn't wired).

## Verified on the board (2026-11-08)

Boots to the launcher at 50 FPS, PSRAM 8 MB found (8181 KB free), battery ADC reads
(~4.1 V from the charger with no cell), audio sink initialises. Without a PCF8574 and an SD
card, input and storage fail gracefully. Not verified yet: screen orientation/colours
(owner to confirm), buttons (needs the PCF8574), SD/ROMs (needs a card), sound (M6).

Touch is not used by Retro-Go: on this target you need the physical buttons to play.
