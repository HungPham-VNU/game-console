# Build plan – milestones & checklist

Rule of the project: **nothing gets soldered into brass until it works on the breadboard.**

| # | Milestone | Exit criteria | Est. time |
|---|---|---|---|
| M0 | Parts & bench ready | All BOM parts on the desk, tools ready | 1–3 weeks (shipping) |
| M1 | Board alive | Prebuilt firmware flashed, serial log shows boot | 1 h |
| M2 | Display working | Retro-Go launcher visible, correct colours/orientation | 1 evening |
| M3 | Full breadboard system | Game plays from SD with all 8 buttons + sound | 1 evening |
| M4 | Own firmware build | Self-built `.img` runs the same as the prebuilt | 1 evening |
| M5 | Layout & template | 1:1 paper template with every part and wire route | 1 evening |
| M6 | Brass frame | Rigid front/back frame soldered | 1–2 evenings |
| M7 | Button plate | Acrylic plate with 8 switches, GND legs to the frame | 1 evening |
| M8 | Sculpture wiring | All modules mounted & wired, works on USB power | 2–3 evenings |
| M9 | Battery power | Runs unplugged, charges via USB-C, safe | 1 evening |
| M10 | Polish & extras | QA pass; optional battery gauge, Brick Game app | open |

---

## M0 – Parts & bench ready
- [ ] Order everything in HARDWARE.md §1 (2× ESP32-S3 board, spare switches, extra brass)
- [ ] Check the board really is **N16R8** (16 MB flash / 8 MB PSRAM) and has the microSD slot
- [ ] Check the display is the **8-pin SPI** GMT024 (ST7789V), not the parallel/touch version
- [ ] Soldering station, flux, multimeter, pliers ready
- [ ] microSD card formatted FAT32
- [ ] Charge each 10440 cell to 4.2 V individually

## M1 – Board alive
- [ ] Add your user to `dialout` (Linux) and check `/dev/ttyACM0` appears
- [ ] Erase flash and write `firmware/prebuilt/retro-go-v7-gmt024-s3.img` at `0x0` (FIRMWARE.md §1)
- [ ] Open the serial monitor at 115200 and confirm the Retro-Go boot log (no boot loop, PSRAM detected)
- **Exit:** clean boot log

## M2 – Display working
- [ ] Solder headers to the display, wire it per HARDWARE.md §3 (GPIO10–14, BL→3V3)
- [ ] Power on: launcher appears in landscape
- [ ] Colours correct (if red↔blue swapped → `RG_SCREEN_RGB_BGR`, needs M4 rebuild)
- [ ] No tearing or noise (if noisy → shorter wires or 20 MHz SPI)
- **Exit:** clean launcher screen

## M3 – Full breadboard system
- [ ] Wire all 8 buttons (GPIO → switch → GND), no external resistors
- [ ] Wire the MAX98357A (DIN 3, BCLK 46, LRC 9, VIN 5V, GND) + speaker
- [ ] Copy free homebrew ROMs to `/roms/gb`, `/roms/nes` (FIRMWARE.md §4)
- [ ] Every button works in the launcher and in game
- [ ] START+SELECT opens the menu; SELECT+A opens options
- [ ] Sound is clear at mid volume, no buzzing
- [ ] Save state, power-cycle, load state
- [ ] Recovery works: hold DOWN at power-on
- [ ] Take photos of the working breadboard (your wiring reference)
- **Exit:** 15 min of play with no crash, glitch or missed input

## M4 – Own firmware build
- [ ] Docker (or native ESP-IDF 4.4.8) set up; the two IDF patches applied
- [ ] `rg_tool.py --target=gmt024-s3 build-img launcher retro-core` succeeds
- [ ] Flash your own image; it behaves the same as the prebuilt
- [ ] Make one tiny change (e.g. `RG_TARGET_NAME`) to prove the edit → build → flash loop works
- [ ] `git init` the project and commit the source + your changes
- **Exit:** you can change a pin and rebuild in under 10 minutes

## M5 – Layout & 1:1 template
- [ ] Measure every module (L×W×H, mounting holes, pin positions)
- [ ] Draw the front and back views at 1:1 (Inkscape or paper), using HARDWARE.md §5 as a start
- [ ] Decide the stack: display → acrylic + ESP32 → batteries/charger at the back; set the post length
- [ ] Draw every signal wire route; mark the crossing points that need sleeving
- [ ] Place the USB-C (charger) and the ESP32 USB port at reachable edges, and keep the **BOOT/RST buttons** reachable
- [ ] Print the template at 100 % and check it against the real parts
- **Exit:** every part and wire has a place on paper

## M6 – Brass frame
- [ ] Practise bending and soldering on scrap tube first
- [ ] Bend the front rectangle on the template (tape it down), then the identical back rectangle
- [ ] Cut 4 corner posts to length, file the ends square
- [ ] Solder the corners (flux and a hot iron) and check the frame is square and flat
- [ ] Clean the flux with IPA
- **Exit:** the frame stands on its own, doesn't flex, and the display fits

## M7 – Button plate
- [ ] Cut the acrylic to the inner frame size (score 5–6× and snap), sand the edges
- [ ] Drill the switch-leg holes using the paper template as a drill guide
- [ ] Fit the switches; glue them with a drop of CA or epoxy
- [ ] Bend each switch's GND leg to the frame; solder
- [ ] Continuity test: each switch shorts its signal leg to the frame only when pressed
- **Exit:** 8 switches mounted, all GND legs on the frame

## M8 – Sculpture wiring (USB powered)
- [ ] Mount the display on the front frame
- [ ] Mount the ESP32 board behind the acrylic and the amp + speaker under the display
- [ ] Wire in this order, testing after each group:
  - [ ] GND bonds (display, amp, ESP32 GND → frame)
  - [ ] 3V3 / 5V power wires (sleeved)
  - [ ] Display SPI (10, 11, 12, 13, 14) → **test: launcher shows**
  - [ ] Buttons (42, 2, 1, 41, 21, 47, 48, 45) → **test: all inputs**
  - [ ] I2S (3, 46, 9) → **test: sound**
- [ ] After every wire: continuity to the right pin, **no short to the frame or to its neighbours**
- [ ] Final short check: 3V3↔GND and 5V↔GND are not near 0 Ω
- **Exit:** the M3 test passes again on USB power

## M9 – Battery power
- [ ] Charge each cell to the same voltage, then join them in parallel (tabs or soldered wire, never solder directly onto a cell without proper tabs)
- [ ] Kapton on all cell ends and the B+ pad
- [ ] TP4056 B+/B- → pack; OUT+ → switch → board 5V/VIN and amp VIN; OUT- → frame
- [ ] Measure 3V3 at a pack voltage of about 3.5 V under load (≥ 3.2 V) – otherwise add an LDO (HARDWARE.md §4)
- [ ] Charge test: the LED turns red then green, the pack reaches 4.2 V, nothing gets hot
- [ ] Runtime test: play until shutdown and note the hours
- **Exit:** ≥ 2 h unplugged, safe charging, no hot spots

## M10 – Polish & extras
- [ ] 30-min heat check (ESP32, amp and charger are warm, not hot)
- [ ] Drop/flex check: tap the frame while playing, no resets
- [ ] Clean all flux; optionally clear-coat the brass to stop tarnish
- [ ] Optional: **battery gauge** – a 100k/100k divider from the pack to a free ADC pin (e.g. GPIO4), then set `RG_BATTERY_DRIVER` in config.h and rebuild
- [ ] Optional: **backlight control** – move BL from 3V3 to a GPIO through a transistor, then set `RG_SCREEN_BACKLIGHT`
- [ ] Optional: **Brick Game app** – write a Retro-Go app with the 10×20 Tetris engine from the earlier Brick Game plan, so it shows in the launcher next to the emulators
- [ ] Photos and video of the finished build 🎮

---

## Risk log
| Risk | Mitigation |
|---|---|
| Short to the brass frame (frame = GND) | Continuity test after every wire; Kapton tape; ≥ 1.5 mm gaps |
| Li-ion short or fire | Protected charger, insulated cells, equal-voltage paralleling, never leave it charging unattended at first |
| Display noise from long wires | Short parallel SPI runs; 20 MHz SPI fallback |
| Boot failure from strapping pins (45/46/3) | No external pulls; don't hold SELECT at power-on |
| Brown-out on battery | Measure 3V3 under load; add a low-dropout LDO |
| Bricked firmware | BOOT+RST download mode always works; keep BOOT reachable |
