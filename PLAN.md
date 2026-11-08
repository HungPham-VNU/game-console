# Plan to completion

**Goal:** a Brick-Game-style handheld built on the ES3N28P, with a 3D-printed case and 8 buttons.
It runs **my own Brick Game firmware (C++)** first, then **Retro-Go** (NES/GB emulators).

## The big picture

```
 WEEK 0            WEEKS 1–2 (parts shipping)                 WEEKS 2–4 (hardware here)              WEEK 5+
 ──────            ───────────────────────────                ─────────────────────────              ───────
                ┌─► F1 Brick Game firmware skeleton ─┐
 M0 ORDER ──────┤   (compiles, no board needed)      │
 PARTS          ├─► F2 Retro-Go "es3n28p" target ────┤
                │   (builds in Docker)               │
                └─► E1 Case CAD v1 (done ✔)          │
                                                     ▼
                                     M1 BOARD BRING-UP ──► M2 BUTTONS ──► M3 BRICK GAME ──► M4 SOUND
                                     flash, screen,        PCF8574,        playable on      + battery
                                     PSRAM, I2C scan       input test      the board        + charging
                                                     │                                          │
                                                     ▼                                          ▼
                                     E2 MEASURE BOARD ──► E3 TEST PRINT ──► E4 FULL PRINT ──► M5 ASSEMBLY
                                     update case.scad     front plate      shell + lid      everything
                                                                                            in the case
                                                                                                │
                                                                                                ▼
                                                                      M6 RETRO-GO ON THE CONSOLE ──► M7 DONE 🎮
```

Three tracks run side by side: **F = firmware**, **E = enclosure**, **M = main build**.

---

## M0 – Order parts *(today)*
- [x] **First order placed (2026-11-06): the display board** (staged ordering; see HANDOFF.md)
- [ ] Second order: dev kit (MX1.25 pigtails, PCF8574, buttons, breadboard, speaker)
- [ ] Third order (after measuring the board): battery, slide switch, final buttons, case hardware
- [ ] Order everything in [SHOPPING.md](SHOPPING.md) §1–2
- [x] N16R8 (8 MB PSRAM) confirmed on the real board (2026-11-08). Version: **ES3C28P (touch)**
- [ ] Battery plug = MX1.25 2-pin, or order pigtails
- [ ] Decide: own printer or print service
- [ ] Install VS Code + PlatformIO, Docker, OpenSCAD
- [ ] `sudo usermod -aG dialout $USER`, then log out and back in (serial port access)

## F1 – Brick Game firmware skeleton *(while shipping)*
- [x] PlatformIO project `brick-game/` for ESP32-S3 N16R8 (builds in Docker via `brick-game/pio.sh`)
- [x] Display driver config for this board's pins (HARDWARE.md §1)
- [x] Input module for the PCF8574 (debounce, auto-repeat), BOOT-button fallback
- [x] Grid renderer with the LCD look, plus the side panel (score / next / speed / level)
- [x] App state machine: MENU → PLAYING ⇄ PAUSED → GAME OVER
- [x] Tetris: pieces, collision, rotation with wall kick, line clear, scoring, 7-bag randomizer
- [x] Compiles without errors or warnings (2026-11-08: RAM 6.4 %, flash 400 KB)
- [x] Host unit tests for core/ + games/: `make -C brick-game/test_host` (115 checks pass)
- **Exit:** `pio run` succeeds ✔ (untested on hardware: panel invert/rgb_order still to tune in M1/M3)

## F2 – Retro-Go target *(while shipping)*
- [ ] Copy the `gmt024-s3` target to `es3n28p`
- [ ] Set the LCD pins (CS 10, DC 46, SCK 12, MOSI 11, BL 45) and rotation
- [ ] Gamepad through the PCF8574: `RG_I2C_GPIO_DRIVER 5`, SDA 16 / SCL 15
- [ ] SD card: SDMMC, CLK 38 / CMD 40 / D0 39
- [ ] Battery ADC on IO9, amp enable IO1 (active low)
- [ ] Builds in Docker (ESP-IDF 4.4.8)
- **Exit:** a `.img` builds (audio comes later, in M6)

## M1 – Board bring-up *(board arrives)*
- [x] Plug in USB and check the port appears: `/dev/ttyACM0` (2026-11-08)
- [x] `esptool flash-id`: Embedded PSRAM 8 MB, 16 MB flash
- [x] Factory flash backed up to `~/es3c28p-backup/factory-16MB.bin`
- [ ] Flash a test sketch: screen red → green → blue, backlight on
- [x] Serial log shows **PSRAM 8 MB** found (8189 KB, 2026-11-08)
- [x] I2C scan shows **0x18** (codec), plus **0x38** (touch, ES3C28P)
- [ ] Owner confirms the colour test on screen (red/green/blue order, "TOP" at the top edge) and the Tetris menu
- [ ] Test the SD card: list files
- **Exit:** screen, PSRAM, SD and I2C all confirmed

## M2 – Buttons
- [ ] Solder 6 big + 2 small switches onto the perfboard (positions from `case.scad`)
- [ ] Wire the PCF8574 (P0–P7) and the GND bus
- [ ] Make the 4-wire MX1.25 cable to the I2C header, **check its pin order**
- [ ] I2C scan shows **0x18 + 0x20**
- [ ] Input test screen: every button lights up its own box
- **Exit:** 8 buttons, no ghost presses, auto-repeat feels right

## M3 – Brick Game playable
- [ ] Flash the F1 firmware
- [ ] Tune colours/rotation (`invert`, `rgb_order`, rotation 180° to match the case)
- [ ] Play Tetris for 15 minutes with no glitches
- [ ] High score saved (Preferences), pause/reset work
- [ ] Add Snake and Racing, plus a game-select menu
- **Exit:** 3 games, high scores survive a reboot

## M4 – Sound & power
- [ ] ES8311 init over I2C, amp enable IO1 = LOW
- [ ] Beeps for move / rotate / line / game over; SOUND button mutes
- [ ] Battery → slide switch → BAT header (**check polarity**)
- [ ] Battery % from IO9 shown on screen
- [ ] Charge test: battery reaches 4.2 V, nothing gets hot
- [ ] Runtime test: play until it shuts down and note the time
- **Exit:** sound works, runs ≥ 3 h on battery, charges over USB-C

## E2 – Measure & update CAD *(board + parts here)*
- [ ] Measure USB-C position, SD slot, switch + cap height, battery, slide switch
- [ ] Update the parameters in `enclosure/case.scad` (see ENCLOSURE.md §3)
- [ ] Open the STEP model in FreeCAD and check the USB and SD positions

## E3 – Test print
- [ ] Print `test_front.stl` (~30 min)
- [ ] Check: screen window lines up, buttons move freely, screw holes match the board
- [ ] Fix `case.scad` and reprint if needed

## E4 – Full print
- [ ] Print the shell (front face down) and the lid
- [ ] Clean up, then press in the heat-set inserts

## M5 – Assembly
- [ ] Follow ENCLOSURE.md §5
- [ ] Full test before closing: screen, buttons, sound, charging, SD
- [ ] Close the lid
- **Exit:** a finished handheld running Brick Game on battery

## M6 – Retro-Go on the console
- [ ] Add an ES8311 driver to Retro-Go (I2C init + MCLK on IO4, or SCLK as the clock source)
- [ ] Flash, copy homebrew ROMs to `/roms/gb` and `/roms/nes`
- [ ] All buttons, sound, save states and the battery icon work
- [ ] Choose: switch firmware by re-flashing, **or** port Brick Game as a Retro-Go app
- **Exit:** emulators run on the console

## M7 – Done 🎮
- [ ] Final QA: 30-minute heat check, drop the lid screws in a bag, label nothing loose
- [ ] Photos + write-up

---

## Risks
| Risk | Plan |
|---|---|
| The board turns out to have no PSRAM | Brick Game still works; Retro-Go doesn't → return it or get an N16R8 board |
| The ES8311 codec is hard to set up | Reuse the ES8311 code from LCDwiki's Arduino examples; do sound last |
| The case doesn't fit the first time | Test-print the front plate first; everything in `case.scad` is a parameter |
| Battery wired backwards | Check with the multimeter before plugging in; use a battery with a protection board |
| I2C cable pin order wrong | Check on the schematic, measure 3V3/GND before connecting the PCF8574 |
