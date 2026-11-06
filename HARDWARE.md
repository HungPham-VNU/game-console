# Hardware – ES3N28P build

Source: https://www.lcdwiki.com/2.8inch_ESP32-S3_Display
Local copies: `docs/es3n28p/` (outline drawing, schematic, spec, STEP 3D model).

## 1. What's on the board

| Function | Part | ESP32-S3 pins |
|---|---|---|
| Display 2.8" 240×320 | ILI9341V, SPI | CS 10 · DC 46 · SCK 12 · MOSI 11 · MISO 13 · BL 45 · RST = EN (shared) |
| microSD | SDIO 4-bit | CLK 38 · CMD 40 · D0 39 · D1 41 · D2 48 · D3 47 |
| Audio | ES8311 codec (I2C 0x18) + FM8002E amp | MCLK 4 · BCLK 5 · LRCK 7 · DOUT 8 · DIN 6 · **AMP_EN 1 (low = on)** |
| I2C bus (codec + header) | – | SDA 16 · SCL 15 |
| Touch (ES3C28P only) | FT6336G (I2C 0x38) | RST 18 · INT 17 |
| RGB LED | WS2812-type | 42 |
| Battery level | divider → ADC | 9 |
| Charger / regulator | TP4054 · ME6217 (low dropout) | – |
| Keys | BOOT = IO0, RESET = EN | – |
| UART header | – | TX 43 · RX 44 |
| **Free (expansion header)** | – | **2 · 3 · 14 · 21** |

## 2. Wiring

```
   ES3N28P (back side)
 ┌───────────────────────────┐
 │ I2C hdr  [3V3 GND SDA SCL]├──── 4-wire MX1.25 ────┐
 │ SPEAKER  [+ -]            ├──── speaker 8Ω 1W     │
 │ BAT      [+ -]            ├──┐                    │
 │ EXPAND   [IO2 IO3 IO14 IO21 ...]                  │
 └───────────────────────────┘  │                    │
                                │               ┌────▼───────────┐
   LiPo 603450 (+) ── SS12D00 ──┘ (+)           │ PCF8574  @0x20 │
   LiPo (-) ─────────────────────── (-)         │ P0 UP          │
                                                │ P1 DOWN        │
                                                │ P2 LEFT        │
                                                │ P3 RIGHT       │
                                                │ P4 A / ROTATE  │
                                                │ P5 B / SOUND   │
                                                │ P6 START       │
                                                │ P7 SELECT      │
                                                │ INT ── IO14 (optional)
                                                └────────────────┘
               every switch:  Px ──[switch]── GND
```

Rules:
- **Find the pin order of each 1.25 mm header on the schematic/silkscreen.** Pigtail
  wire colours mean nothing. Measure with the multimeter before connecting.
- **Battery polarity:** check it twice. Reversing the battery can kill the charger chip.
- 3.3 V only on the PCF8574. Its modules have their own I2C pull-ups, which is fine.
- With the power switch in the battery line, the battery **only charges while the
  switch is ON**.
- The first test after wiring is an I2C scan, which must show **0x18** (codec) and **0x20** (PCF8574).

## 3. Button mapping (same wiring for both firmwares)

| PCF8574 | Retro-Go | Brick Game |
|---|---|---|
| P0–P3 | UP DOWN LEFT RIGHT | UP DOWN LEFT RIGHT |
| P4 | A | ROTATE |
| P5 | B | SOUND on/off |
| P6 | START | START / PAUSE |
| P7 | SELECT | hold = RESET to menu |
| slide switch | power | power |

## 4. Board mechanical data (from ES3N28P_Size.pdf)

| Item | Value |
|---|---|
| PCB | 50.00 × 86.00 × 1.6 mm, corner R3.5 |
| Mounting holes | 4 × Ø3.2 (pad Ø5.6), centres 42.0 × 78.0 mm, 4.0 mm from the edges |
| LCD outline | 50.0 × 69.2 mm, starts 8.22 mm from the top edge |
| LCD active area | 43.2 × 57.6 mm, starts 11.12 mm from the top and 3.5 mm from the left |
| Stack (front → back) | LCD 2.3 + glue 0.5 + PCB 1.6 + parts 4.7 = **9.1 mm** |
| USB-C, RESET, BOOT | bottom edge, on the back |
| SD slot, UART, BAT | right side of the back (seen from the back) |
| I2C, SPEAKER | left side of the back (seen from the back) |

In the case, the board is mounted **upside down (rotated 180°)** so the USB-C port faces the top
edge of the console. The firmware rotates the image to match.
