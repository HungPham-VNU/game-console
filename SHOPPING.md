# Shopping list

Prices are rough Shopee/Lazada (VN) prices. Total ≈ **0.9 – 1.2 M ₫** (+ printing).

## 1. Electronics – order these

| # | Item | Qty | What to check before buying | ~Price |
|---|---|---|---|---|
| 1 | **ES3N28P** – 2.8" ESP32-S3 display board, *no touch* (LCDwiki / QDtech) | 1 | Ask the seller to confirm **N16R8** (16 MB flash + **8 MB PSRAM**). Retro-Go needs the PSRAM. | 440k |
| 2 | **PCF8574 I/O expander module** (I2C, address 0x20) | 1–2 | Must be **PCF8574**, not PCF8574**A** (0x38) | 25k |
| 3 | Tactile switch **12×12×7.3 mm** + **round caps** (Ø ~12 mm) | 6 (+4 spare) | D-pad ×4, A, B | 30k |
| 4 | Tactile switch **6×6 mm, tall stem (9–10 mm)** | 2 (+3 spare) | START, SELECT | 10k |
| 5 | Double-sided perfboard **7×9 cm** (cut to 66×46 mm) | 1–2 | | 15k |
| 6 | **LiPo 3.7 V ~1000 mAh, 603450** (6×34×50 mm) **with protection board** | 1 | Plug must be **MX1.25 / 1.25 mm 2-pin**, or buy a pigtail (#8). **Check polarity** against the board's BAT socket | 70–100k |
| 7 | Speaker **8 Ω 1 W, 20×30 mm (2030)** | 1 | Max 1.5 W @ 8 Ω for this board | 15k |
| 8 | **MX1.25 (1.25 mm) pigtail set**: 4-pin ×2, 2-pin ×3 | 1 set | For I2C header, battery, speaker | 25k |
| 9 | Slide switch **SS12D00** (or SK12D07) | 2 | Power switch in the battery line | 5k |
| 10 | microSD card 16–32 GB | 1 | FAT32 | 80k |
| 11 | Silicone wire 26–28 AWG, 2–3 colours | 1 set | | 30k |

## 2. Case hardware

| # | Item | Qty | ~Price |
|---|---|---|---|
| 12 | **M3 heat-set inserts** (M3 × 5 mm long, OD ~4.5) | 4 (+spares) | 20k / 50 pcs |
| 13 | M3 × 8 countersunk screws (lid) | 4 | 15k |
| 14 | M3 × 6 pan-head self-tapping or machine screws (board + button board) | 8 | 15k |
| 15 | Thin double-sided foam tape (battery, speaker, LCD cushion) | 1 | 15k |
| 16 | Filament PLA or PETG (~80 g) **or** a print service | – | 100–250k |

## 3. Tools you need

| Tool | Notes |
|---|---|
| Soldering iron (temp-controlled) + solder + flux | Also used to press in heat-set inserts |
| **Digital calipers** | To measure the real board and update the CAD before printing |
| Multimeter | Polarity, shorts, battery voltage |
| Flush cutter, wire stripper, tweezers | |
| Small screwdriver set, hand drill 3 mm (perfboard mounting holes) | |
| USB-C **data** cable | Some cables are charge-only |
| **3D printer** – optional | No printer? Use a local print service or JLC3DP and send them the STL |

## 4. Software (free)

| Tool | Use |
|---|---|
| VS Code + **PlatformIO** | Brick Game firmware (C++ / Arduino) |
| Docker | Retro-Go build (ESP-IDF 4.4.8) |
| **OpenSCAD** | Edit `enclosure/case.scad` → export STL |
| FreeCAD (optional) | Open `docs/es3n28p/ES3N28P_3D.step` to check the board in 3D |
| Slicer: OrcaSlicer / PrusaSlicer / Bambu Studio | Print the case |

## ❌ No longer needed (from the brass plan)
Separate ESP32-S3 CAM board, separate 2.4" display, MAX98357A amp, TP4056 charger,
LDO regulator, 10440 cells, brass tube/wire, acrylic sheet.
