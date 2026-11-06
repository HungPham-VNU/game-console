# Firmware

The firmware is a Retro-Go fork with a custom target `gmt024-s3`
(base commit `4ced120669750ca7228fd0414211430c1d923166`, upstream
https://github.com/ducalex/retro-go). License: GPLv2 (see `COPYING`).

Systems in this build: NES, GB, GBC, Game & Watch, SMS/SG-1000/GG,
ColecoVision, PC Engine, Atari Lynx. SNES is disabled.

## 1. Flash the prebuilt image (fastest)

Image: `firmware/prebuilt/retro-go-v7-gmt024-s3.img`
sha256 `b84ab2bc1ad405241e898782ab771f1f27b96d45dafdefbcd0e0b1fbf298968e`

### Option A – browser (esptool-js)
1. Open https://espressif.github.io/esptool-js/ in **Chrome/Chromium** (WebSerial).
2. Plug the board into the port marked **USB/COM**. If it doesn't connect, hold
   **BOOT**, tap **RST**, release BOOT.
3. Baud 921600 → **Connect**.
4. **Erase Flash** (first time only).
5. Flash address `0x0`, file = the `.img`, then **Program**.
6. Tap RST. The Retro-Go launcher should appear.

On Linux, if the port is not listed: `sudo usermod -aG dialout $USER`, then log out and back in.

### Option B – command line
```sh
pip install esptool
esptool.py --chip esp32s3 --port /dev/ttyACM0 erase_flash
esptool.py --chip esp32s3 --port /dev/ttyACM0 write_flash \
  --flash_mode dio --flash_size 16MB 0x0 firmware/prebuilt/retro-go-v7-gmt024-s3.img
```

Serial log (useful for debugging): `python -m serial.tools.miniterm /dev/ttyACM0 115200`

## 2. Build from source

Use **ESP-IDF 4.4.8**, the version the author tested with. The Docker method is
the least effort and doesn't touch your system:

```sh
cd firmware/retro-go-gmt024-s3
docker run --rm -it -v "$PWD":/app -w /app espressif/idf:v4.4.8 bash -c '
  cd /opt/esp/idf &&
  patch -p1 --ignore-whitespace -i "/app/tools/patches/panic-hook (esp-idf 4).diff" &&
  patch -p1 --ignore-whitespace -i "/app/tools/patches/sdcard-fix (esp-idf 4).diff" &&
  cd /app &&
  python rg_tool.py --target=gmt024-s3 clean launcher retro-core &&
  python rg_tool.py --target=gmt024-s3 build-img launcher retro-core'
# → retro-go_<version>_gmt024-s3.img in the project root
```

Native install: follow the ESP-IDF v4.4 get-started guide, apply the same two
patches to `$IDF_PATH`, run `. $IDF_PATH/export.sh`, then `./build-gmt024-s3.sh`.

### What to edit
All hardware settings live in
`components/retro-go/targets/gmt024-s3/config.h`:

| Macro | Purpose |
|---|---|
| `RG_GAMEPAD_GPIO_MAP` | Button pins |
| `RG_GPIO_LCD_*` | Display SPI pins |
| `RG_SCREEN_SPEED` | SPI clock. Drop to `SPI_MASTER_FREQ_20M` if long wires cause glitches |
| `RG_SCREEN_ROTATION`, `RG_SCREEN_RGB_BGR`, `RG_SCREEN_INIT()` | Orientation, colour order, ST7789 init |
| `RG_GPIO_SND_I2S_*` | MAX98357A pins |
| `RG_GPIO_SDSPI_*` | SD card (SDMMC 1-bit) pins |
| `RG_BATTERY_DRIVER` | `0` = no battery gauge (see milestone M10 to add one) |

Flash and PSRAM settings: `targets/gmt024-s3/sdkconfig` and `env.py`.
Always `clean` after editing a target.

## 3. SD card layout

Format the card as **FAT32**, then create:

```
/roms/nes   /roms/gb   /roms/gbc   /roms/gw
/roms/sms   /roms/gg   /roms/col   /roms/pce   /roms/lnx
```
Retro-Go creates its own `/retro-go/` folder (saves, config) on first boot.

## 4. Test ROMs (legal)

The `roms.zip` from the reference project was not downloaded, because packs like
that usually contain commercial ROMs. Free homebrew works just as well for testing:

| Source | System |
|---|---|
| Homebrew Hub – https://hh.gbdev.io | Hundreds of free GB/GBC homebrew games |
| Tobu Tobu Girl (free, open source) – https://tangramgames.dk/tobutobugirl/ | GB |
| itch.io – search "game boy homebrew free" / "nes homebrew free" | GB / NES |
| NESdev wiki homebrew list – https://www.nesdev.org/wiki/Homebrew_games | NES |
| Blargg / Mooneye test ROMs | Emulator accuracy tests (GB/NES) |

Commercial games: only ROMs you have dumped from cartridges you own.
