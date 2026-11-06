# Hardware

## 1. Bill of materials

Parts match the reference build and the pin map in
`firmware/retro-go-gmt024-s3/components/retro-go/targets/gmt024-s3/config.h`.

| # | Part | Qty | Notes | ~Cost |
|---|---|---|---|---|
| 1 | **ESP32-S3 N16R8 "CAM" board** (WROOM-1 N16R8, built-in microSD slot) | 1 | SD wired to GPIO38/39/40. Camera not used. Buy 2 – one for the breadboard, one for the build. | $8–12 |
| 2 | **2.4" IPS TFT, GMT024-08-SPI8P** (ST7789V, 240×320, 8-pin SPI) | 1 | Must be the 8-pin SPI version (GND VCC SCL SDA RES DC CS BL). | $5–7 |
| 3 | **MAX98357A** I2S class-D amp module | 1 | 3 W mono, runs on 2.5–5.5 V | $2 |
| 4 | Speaker, 8 Ω 1–3 W, thin (e.g. 20×30 mm or Ø28 mm) | 1 | | $1–2 |
| 5 | **Li-ion 10440 (AAA-size) 3.7 V** cells | 3 | Wired in parallel. Same brand/batch. | $6 |
| 6 | **USB-C Li-ion charger module, 4.2 V, with protection** (TP4056 + DW01 + 8205A) | 1 | Use the 6-pad version with B+/B- and OUT+/OUT- | $1 |
| 7 | Slide switch (SS12D00 or mini) | 1 | | $0.30 |
| 8 | Tactile switches 6×6 mm or 4.5×4.5 mm, short stem | 8 (+spares) | UP DOWN LEFT RIGHT A B START SELECT | $1 |
| 9 | Brass tube, **OD 2 mm**, 300 mm lengths | 4 | Frame (= GND bus) | $4 |
| 10 | Brass / copper wire **1 mm**, solid | 2 m | Signal "traces" | $3 |
| 11 | Clear acrylic sheet **2 mm** | 1 small sheet | Button plate | $2 |
| 12 | microSD card 8–32 GB (FAT32) | 1 | | $4 |
| 13 | Kapton tape, heat-shrink, CA glue / 5-min epoxy, M2 screws (optional) | – | | $5 |
| | **Total** | | | **≈ $45–55** |

Prototyping extras: breadboard ×2, Dupont jumpers (M-M, M-F), 2.54 mm pin headers.

## 2. Tools

**Required**
- Temperature-controlled soldering iron (350–400 °C for brass), chisel/bevel tip
- Solder 0.6–0.8 mm + **liquid or paste flux** (brass needs lots of flux)
- Flat-nose + round-nose pliers, flush cutter
- Tube cutter or rotary tool with cut-off disc, small files, 400–800 grit sandpaper
- Acrylic scoring knife + steel ruler; hand drill/pin vise with 1.0/1.2/1.5 mm bits
- **Multimeter** (continuity beeper is used constantly)
- Helping hands / clamp, heat-resistant mat, masking tape
- Isopropyl alcohol + old toothbrush (flux cleaning)

**Nice to have**: desoldering pump/wick, magnifier, small vise, USB power meter.

**Not required**: 3D printer, custom PCB, oscilloscope.

## 3. Pin map (from the firmware – do not change unless you rebuild)

### Display – GMT024 (ST7789V, SPI2 @ 40 MHz)
| Display pin | ESP32-S3 |
|---|---|
| SCL / SCK | GPIO10 |
| SDA / MOSI | GPIO11 |
| RES / RST | GPIO12 |
| DC | GPIO13 |
| CS | GPIO14 |
| BL | 3V3 (always on, no dimming) |
| VCC | 3V3 |
| GND | GND |

### Audio – MAX98357A (I2S)
| Amp pin | ESP32-S3 |
|---|---|
| DIN | GPIO3 |
| BCLK | GPIO46 |
| LRC / WS | GPIO9 |
| VIN | Battery + (after the switch) or 5 V |
| GND | GND |
| SD / GAIN | leave unconnected (default: on, 9 dB) |

### Buttons (each: GPIO → switch → GND, internal pull-up)
| Button | GPIO | | Button | GPIO |
|---|---|---|---|---|
| UP | 42 | | A | 21 |
| DOWN | 2 | | B | 47 |
| LEFT | 1 | | START | 48 |
| RIGHT | 41 | | SELECT | 45 |

Combos: **START+SELECT** = in-game menu · **SELECT+A** = quick options ·
**hold DOWN at power-on** = recovery to launcher.

### microSD (on-board, SDMMC 1-bit) – already wired on the CAM board
CLK GPIO39 · CMD GPIO38 · D0 GPIO40

### Pins to watch out for
- **GPIO45 (SELECT)** and **GPIO46 (BCLK)** are strapping pins. Do **not** add
  external pull-up/pull-down resistors. Don't hold SELECT while powering on.
- **GPIO3** is a JTAG strap – fine for I2S DIN, no external pulls.
- **GPIO48 (START)**: some S3 boards have an on-board RGB LED on GPIO48. If START
  acts strangely, check your board and remove/ignore the LED.
- GPIO0 = BOOT button on the board – keep it reachable for re-flashing.
- GPIO26–37 are used by flash/PSRAM on N16R8 – never use them.

## 4. Power

```
             USB-C
               │
     ┌─────────▼──────────┐
     │ TP4056 + DW01 4.2V │
     │  B+  B-  OUT+ OUT- │
     └──┬───┬────┬────┬───┘
        │   │    │    └──────────────┐ GND (brass frame)
  ┌─────┴───┴─┐  │                   │
  │ 3× 10440  │  └─► slide switch ─┬─► ESP32 board 5V/VIN pin
  │ parallel  │                    └─► MAX98357A VIN
  └───────────┘
```

Checks before trusting it:
1. Charge every cell to 4.2 V **separately** before connecting them in parallel
   (cells at different voltages dump current into each other).
2. With the battery at ~3.5 V, measure the board's **3V3 pin**. It must stay
   ≥ 3.2 V while a game runs. If it drops (high-dropout AMS1117 regulator), add an
   ME6211/HT7333 LDO from battery to 3V3 and stop feeding the 5V pin.
3. Insulate cell ends and the charger's B+ pad with Kapton – the frame is GND
   and bare brass is everywhere.
4. Expected draw: ~120–200 mA while playing → ~2–3 h per 300 mAh cell ×3.

## 5. Mechanical layout (front view, approx. 65 × 90 mm)

```
 ┌──────────────────────────────┐  ← front brass rectangle (OD 2 mm)
 │ ┌──────────────────────────┐ │
 │ │                          │ │
 │ │   2.4" ST7789 320×240    │ │
 │ │                          │ │
 │ └──────────────────────────┘ │
 │   [speaker]                  │
 │ ┌──────── acrylic ─────────┐ │
 │ │    ▲            SEL  ST  │ │
 │ │  ◄   ►                   │ │
 │ │    ▼          B    A     │ │
 │ │   (ESP32-S3 visible under│ │
 │ │    the acrylic)          │ │
 │ └──────────────────────────┘ │
 └──────────────────────────────┘
 Back layer: 3× 10440, TP4056 (USB-C on the bottom edge), slide switch on the side
 Depth: 4 corner posts, ~8–15 mm depending on your stack
```

Wiring rules for the sculpture:
- Frame = GND. Every button's second leg, display GND, amp GND, charger OUT- go to it.
- Signal wires: 1 mm brass, 90° bends, ≥ 1.5 mm gap between wires and from the frame.
- Where a wire must pass near another, sleeve it with clear heat-shrink.
- Keep the SPI wires (GPIO10–14) as **short and parallel** as possible; if the
  image glitches, rebuild with `RG_SCREEN_SPEED SPI_MASTER_FREQ_20M`.
