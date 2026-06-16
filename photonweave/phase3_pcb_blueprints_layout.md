# PhotonWeave — Phase 3: PCB Blueprints & Layout

## 1. Board Overview

### Mechanical Specifications
| Parameter | Value | Notes |
|-----------|-------|-------|
| Form Factor | PCIe FHHL (Full-Height, Half-Length) | PCIe CEM 3.0 |
| Board Dimensions | 111.15 mm × 167.65 mm (4.375" × 6.600") | Standard FHHL |
| Board Thickness | 1.60 mm (62 mil) | Standard FR-4 |
| Layers | 16 | High-density FPGA design |
| Copper Weight | Outer: 1 oz (35 µm), Inner: 0.5 oz (17 µm) | Standard |
| Surface Finish | ENIG (Electroless Nickel Immersion Gold) | Fine-pitch BGA compatible |
| Solder Mask | Green LPI, both sides | |
| Silkscreen | White epoxy ink, top side only | |
| Min Trace/Space | 3.5 mil / 3.5 mil | Class 3 |
| Min Drill | 0.20 mm (8 mil) | Mechanical drill |
| Via Type | Through-hole, blind (L1-L3, L14-L16), buried (L3-L14) | HDI for BGA escape |
| PCIe Edge Connector | 98-position, gold flash (0.76 µm Au over 2.54 µm Ni) | Per PCIe CEM spec |
| Board Cutouts | None | |
| Mounting Holes | 4× M2.5 (2.5 mm dia), non-plated | PCIe bracket + stabilizer |

### PCIe Bracket
- Full-height bracket (120.0 mm × 18.4 mm)
- Cutout for HDMI connector (15.0 mm × 8.0 mm)
- Cutout for QSFP+ cage (18.5 mm × 13.5 mm)
- Cutout for USB 3.0 Micro-B (8.0 mm × 3.5 mm)
- Ventilation slots: 3× (40 mm × 3 mm) for airflow
- Material: 0.8 mm steel, nickel-plated

## 2. Layer Stackup (16-Layer)

```
Layer 1  (TOP)          — Signal (microstrip), Components, PCIe edge
   ↓ Prepreg: 1080 (2.8 mil, εr=3.8)
Layer 2  (GND1)         — Ground Plane (solid, no splits)
   ↓ Core: 0.10 mm (4 mil, εr=4.2)
Layer 3  (SIG1)         — Signal (stripline), PCIe TX pairs, HDMI TMDS
   ↓ Prepreg: 2116 (4.5 mil, εr=4.0)
Layer 4  (GND2)         — Ground Plane (solid)
   ↓ Core: 0.10 mm (4 mil, εr=4.2)
Layer 5  (SIG2)         — Signal (stripline), DDR4 DQ byte lanes
   ↓ Prepreg: 2116 (4.5 mil, εr=4.0)
Layer 6  (PWR1)         — Power Plane: VCCINT (1.0V), MGTAVCC (1.0V)
   ↓ Core: 0.20 mm (8 mil, εr=4.2)
Layer 7  (PWR2)         — Power Plane: VDD_DDR (1.2V), VCCO_3V3 (3.3V)
   ↓ Prepreg: 2116 (4.5 mil, εr=4.0)
Layer 8  (GND3)         — Ground Plane (solid)
   ↓ Core: 0.20 mm (8 mil, εr=4.2)
Layer 9  (GND4)         — Ground Plane (solid)
   ↓ Prepreg: 2116 (4.5 mil, εr=4.0)
Layer 10 (PWR3)         — Power Plane: MGTAVTT (1.2V), VCCAUX (1.8V)
   ↓ Core: 0.20 mm (8 mil, εr=4.2)
Layer 11 (PWR4)         — Power Plane: 3.3V_STM32, VTT_DDR (0.6V)
   ↓ Prepreg: 2116 (4.5 mil, εr=4.0)
Layer 12 (SIG3)         — Signal (stripline), DDR4 ADDR/CMD fly-by
   ↓ Core: 0.10 mm (4 mil, εr=4.2)
Layer 13 (GND5)         — Ground Plane (solid)
   ↓ Prepreg: 2116 (4.5 mil, εr=4.0)
Layer 14 (SIG4)         — Signal (stripline), QSFP+ RX/TX, USB 3.0 SS
   ↓ Core: 0.10 mm (4 mil, εr=4.2)
Layer 15 (GND6)         — Ground Plane (solid)
   ↓ Prepreg: 1080 (2.8 mil, εr=3.8)
Layer 16 (BOTTOM)       — Signal (microstrip), Decoupling caps, JTAG
```

### Stackup Summary
| Layer | Type | Thickness | εr | Function |
|-------|------|-----------|-----|----------|
| L1 | Signal (microstrip) | 1.9 mil (after plating) | 3.8 | Top routing, components |
| L2 | Plane (GND) | 1.2 mil | — | Solid ground reference |
| L3 | Signal (stripline) | 0.7 mil | 4.2 | High-speed diff pairs |
| L4 | Plane (GND) | 1.2 mil | — | Solid ground reference |
| L5 | Signal (stripline) | 0.7 mil | 4.2 | DDR4 DQ routing |
| L6 | Plane (PWR) | 1.2 mil | — | VCCINT, MGTAVCC |
| L7 | Plane (PWR) | 1.2 mil | — | VDD_DDR, VCCO_3V3 |
| L8 | Plane (GND) | 1.2 mil | — | Solid ground reference |
| L9 | Plane (GND) | 1.2 mil | — | Solid ground reference |
| L10 | Plane (PWR) | 1.2 mil | — | MGTAVTT, VCCAUX |
| L11 | Plane (PWR) | 1.2 mil | — | 3.3V_STM32, VTT_DDR |
| L12 | Signal (stripline) | 0.7 mil | 4.2 | DDR4 ADDR/CMD |
| L13 | Plane (GND) | 1.2 mil | — | Solid ground reference |
| L14 | Signal (stripline) | 0.7 mil | 4.2 | QSFP+, USB 3.0 |
| L15 | Plane (GND) | 1.2 mil | — | Solid ground reference |
| L16 | Signal (microstrip) | 1.9 mil (after plating) | 3.8 | Bottom routing, decoupling |

Total thickness: ~62 mil (1.57 mm) — within PCIe CEM tolerance.

## 3. Impedance Calculations

### Microstrip (L1, L16)
Using IPC-2141 formula for surface microstrip:
```
Z0 = (87 / √(εr + 1.41)) × ln(5.98 × h / (0.8 × w + t))

For 50 Ω single-ended (εr=3.8, h=2.8 mil prepreg, t=1.9 mil):
  w = 5.0 mil trace width

For 85 Ω differential (PCIe):
  w = 4.0 mil, s = 6.0 mil spacing

For 100 Ω differential (HDMI):
  w = 3.5 mil, s = 5.5 mil spacing

For 90 Ω differential (USB 3.0):
  w = 4.0 mil, s = 7.0 mil spacing
```

### Stripline (L3, L5, L12, L14)
Using IPC-2141 formula for symmetric stripline:
```
Z0 = (60 / √εr) × ln(4 × h / (0.67 × π × (0.8 × w + t)))

For 50 Ω single-ended (εr=4.2, h=4 mil to each plane, t=0.7 mil):
  w = 3.5 mil trace width

For 85 Ω differential (PCIe, L3):
  w = 3.0 mil, s = 5.0 mil spacing

For 100 Ω differential (HDMI, L3):
  w = 2.8 mil, s = 4.5 mil spacing

For 40 Ω single-ended (DDR4 DQ, L5):
  w = 5.5 mil trace width
```

## 4. High-Speed Routing Rules

### PCIe Gen3 (8 GT/s) — L3 Stripline
| Parameter | Requirement | Implementation |
|-----------|-------------|----------------|
| Differential Impedance | 85 Ω ±10% | 3.0 mil trace, 5.0 mil gap |
| Intra-pair Skew | <1 ps/mm (<5 mil total) | Serpentine matching within pair |
| Inter-pair Skew | <25 mil across all 8 lanes | Length matching at connector end |
| Max Trace Length | <10 inches | ~4.5 inches on FHHL board |
| AC Coupling Caps | 220 nF, 0402 | Placed at TX end (FPGA side) |
| Via Count | ≤2 per net | 1 via max (BGA escape + layer transition) |
| Reference Plane | Solid GND on L2 and L4 | No splits under diff pairs |
| Return Path Vias | 1 GND via per signal via | Stitching vias every 200 mil along route |

### HDMI 2.0 TMDS (6 Gbps per lane) — L3 Stripline
| Parameter | Requirement | Implementation |
|-----------|-------------|----------------|
| Differential Impedance | 100 Ω ±5% | 2.8 mil trace, 4.5 mil gap |
| Intra-pair Skew | <0.5 ps/mm (<3 mil total) | Tight serpentine matching |
| Inter-pair Skew | <10 mil across 4 lanes | Clock-to-data matching critical |
| Max Trace Length | <6 inches | ~3.0 inches to connector |
| AC Coupling Caps | 100 nF, 0201 | Placed at FPGA TX end |
| ESD Protection | TPD4E05U06 at connector | Within 2 mm of HDMI receptacle |
| Reference Plane | Solid GND on L2 and L4 | No splits |

### DDR4-2400 (1.2 Gbps per pin) — L5 Stripline (DQ), L12 Stripline (ADDR/CMD)
| Parameter | Requirement | Implementation |
|-----------|-------------|----------------|
| Single-Ended Impedance | 40 Ω ±10% | 5.5 mil trace (L5), 5.5 mil (L12) |
| DQS-to-DQ Skew | <5 mil within byte lane | Serpentine on DQ lines |
| Byte Lane-to-Byte Lane Skew | <50 mil | Length matching at FPGA BGA |
| Clock-to-DQS Skew | <10 mil | CK pair matched to DQS |
| ADDR/CMD-to-Clock Skew | <25 mil | Fly-by topology matching |
| Series Termination | 22 Ω at FPGA end | Placed within 200 mil of BGA ball |
| VTT Termination | 40 Ω to VTT at last DRAM | TPS51200 sink/source regulator |
| Max Trace Length | <3 inches | ~2.0 inches (close placement) |
| Reference Plane | Solid GND on L4 and L6 (DQ), L11 and L13 (ADDR/CMD) | |
| Fly-by Topology | ADDR/CMD/CLK daisy-chained U6→U7→U8→U9 | 4-drop fly-by |

### DDR4 Fly-by Routing Detail
```
FPGA Bank 12 (ADDR/CMD) → 22 Ω series → U6 (first DRAM)
  ├── CK_P/N: 100 Ω diff pair, 22 Ω series at FPGA
  ├── ADDR[0:17]: 40 Ω SE, 22 Ω series at FPGA
  └── Control (CKE,CS,ODT): 40 Ω SE, 22 Ω series at FPGA

U6 → U7 segment: ~600 mil (DRAM-to-DRAM pitch)
U7 → U8 segment: ~600 mil
U8 → U9 segment: ~600 mil

VTT termination at U9 (last DRAM):
  ADDR/CMD → 40 Ω → VTT (0.6V)
  CK_P/N → 50 Ω each → VTT
  Control → 40 Ω → VTT
```

### QSFP+ (10.3125 Gbps per lane) — L14 Stripline
| Parameter | Requirement | Implementation |
|-----------|-------------|----------------|
| Differential Impedance | 100 Ω ±10% | 3.0 mil trace, 5.0 mil gap |
| Intra-pair Skew | <0.5 ps/mm | Serpentine matching |
| Max Trace Length | <4 inches | ~2.5 inches to cage |
| AC Coupling Caps | 100 nF, 0201 | At FPGA RX end |
| Reference Plane | Solid GND on L13 and L15 | |

### USB 3.0 SuperSpeed (5 Gbps) — L14 Stripline
| Parameter | Requirement | Implementation |
|-----------|-------------|----------------|
| Differential Impedance | 90 Ω ±7% | 3.5 mil trace, 6.0 mil gap |
| Intra-pair Skew | <5 mil | Serpentine matching |
| Max Trace Length | <3 inches | ~2.0 inches to FT601 |
| AC Coupling Caps | 100 nF, 0201 | At FT601 end |
| ESD Protection | TPD4E02B04 at connector | Within 2 mm of USB receptacle |

## 5. BGA Escape Routing (XC7K325T FFG900)

### Ball Grid Pattern
- 31×31 array, 1.0 mm pitch
- 900 balls total (some depopulated in center)
- Outer rows: full signal escape
- Inner rows: power/ground (direct via to plane)

### Escape Strategy
```
Row 1 (outermost): Dog-bone via, trace on L1 (top microstrip)
Row 2: Dog-bone via, trace on L1 (between row 1 pads)
Row 3: Via-in-pad (VIP), trace on L3 (stripline)
Row 4: Via-in-pad (VIP), trace on L3
Row 5: Via-in-pad (VIP), trace on L5 (stripline)
Row 6: Via-in-pad (VIP), trace on L5
Row 7+: Power/ground balls, direct via to plane layers

Via-in-pad: 0.20 mm drill, 0.45 mm pad, filled and capped (VIPPO)
Dog-bone via: 0.20 mm drill, 0.50 mm pad, 0.25 mm trace to pad
```

### DDR4 BGA Escape (per interface)
- 4 DDR4 chips placed within 25 mm of FPGA edge
- Byte lanes grouped: DQ[0:7] on L5, DQ[8:15] on L5 (adjacent routing channels)
- DQS differential pairs on L5, tightly coupled
- ADDR/CMD on L12, fly-by topology

## 6. Via Strategy

### Via Types
| Type | Drill | Pad | Antipad | Use |
|------|-------|-----|---------|-----|
| Signal Via (through) | 0.20 mm (8 mil) | 0.45 mm (18 mil) | 0.65 mm (26 mil) | General signal layer transitions |
| Signal Via (blind L1-L3) | 0.15 mm (6 mil) | 0.35 mm (14 mil) | 0.55 mm (22 mil) | BGA escape row 3-4 |
| Signal Via (blind L14-L16) | 0.15 mm (6 mil) | 0.35 mm (14 mil) | 0.55 mm (22 mil) | Bottom BGA escape |
| Power Via | 0.30 mm (12 mil) | 0.60 mm (24 mil) | 0.80 mm (32 mil) | Power plane connections |
| Ground Via | 0.30 mm (12 mil) | 0.60 mm (24 mil) | 0.80 mm (32 mil) | Ground stitching |
| Thermal Via | 0.30 mm (12 mil) | 0.60 mm (24 mil) | 0.80 mm (32 mil) | Under FPGA, PMIC, DDR4 |
| VIPPO (via-in-pad plated over) | 0.20 mm (8 mil) | 0.45 mm (18 mil) | — | BGA pad direct via |

### Via Count Budget
| Net Class | Max Vias | Typical |
|-----------|----------|---------|
| PCIe TX/RX | 2 | 1 |
| HDMI TMDS | 2 | 1 |
| DDR4 DQ | 2 | 1 |
| DDR4 ADDR/CMD | 3 | 2 (fly-by) |
| QSFP+ | 2 | 1 |
| USB 3.0 SS | 2 | 1 |
| General I/O | 4 | 2 |

### Ground Stitching Vias
- Along all differential pair routes: 1 GND via every 200 mil (5 mm)
- At layer transitions: 2 GND vias adjacent to each signal via
- Around board perimeter: GND vias every 100 mil (2.5 mm) for edge shielding
- Under FPGA: GND vias on 2 mm grid connecting all GND planes

## 7. Power Distribution Network (PDN) Design

### Plane Assignments
```
L6 (PWR1):
  ┌──────────────────────────────────────────┐
  │  VCCINT (1.0V) — 70% of plane area       │
  │  MGTAVCC (1.0V) — 30% of plane area      │
  │  Separated by 50 mil gap                 │
  └──────────────────────────────────────────┘

L7 (PWR2):
  ┌──────────────────────────────────────────┐
  │  VDD_DDR (1.2V) — 60% of plane area      │
  │  VCCO_3V3 (3.3V) — 40% of plane area     │
  │  Separated by 50 mil gap                 │
  └──────────────────────────────────────────┘

L10 (PWR3):
  ┌──────────────────────────────────────────┐
  │  MGTAVTT (1.2V) — 50% of plane area      │
  │  VCCAUX (1.8V) — 50% of plane area       │
  │  Separated by 50 mil gap                 │
  └──────────────────────────────────────────┘

L11 (PWR4):
  ┌──────────────────────────────────────────┐
  │  3.3V_STM32 — 40% of plane area          │
  │  VTT_DDR (0.6V) — 60% of plane area      │
  │  Separated by 50 mil gap                 │
  └──────────────────────────────────────────┘
```

### PDN Target Impedance
| Rail | DC Current | Target Z (DC-100 MHz) | Decoupling Strategy |
|------|------------|----------------------|---------------------|
| VCCINT | 25A | <5 mΩ | 4×330µF bulk + 8×100µF mid + 40×0.1µF HF |
| MGTAVCC | 8A | <10 mΩ | 2×330µF bulk + 4×100µF mid + 16×0.1µF HF |
| VDD_DDR | 12A | <8 mΩ | 4×330µF bulk + 8×100µF mid + 16×0.1µF HF |
| VCCO_3V3 | 5A | <15 mΩ | 2×100µF bulk + 20×0.1µF HF |
| MGTAVTT | 4A | <15 mΩ | 1×330µF bulk + 2×100µF mid + 8×0.1µF HF |
| VCCAUX | 3A | <20 mΩ | 1×100µF bulk + 12×0.1µF HF |

### Plane Resonance Management
- Plane pairs (PWR/GND) form parallel-plate capacitors
- L6-L4: VCCINT to GND2, ~500 pF/in², resonance ~200 MHz
- Mitigation: distributed 0.1 µF caps shift resonance below 10 MHz
- Edge termination: 10 Ω + 10 µF series RC at plane edges (4 per rail)

## 8. Thermal Management

### Heat Sources
| Component | Power | Junction Temp Limit | Cooling |
|-----------|-------|---------------------|---------|
| XC7K325T | 45W | 100°C (Tj max) | Heatsink + airflow |
| DDR4 ×4 | 8W (2W each) | 95°C (Tcase) | PCB copper spread + airflow |
| TPS65218 | 5W (losses) | 125°C (Tj) | PCB copper polygon |
| FT601 | 1.5W | 85°C (Tj) | PCB copper |
| Si5345 | 0.8W | 85°C (Tj) | PCB copper |
| STM32H743 | 0.5W | 85°C (Tj) | PCB copper |

### Heatsink Design
- **Type**: Aluminum extruded fin heatsink
- **Dimensions**: 80 mm × 120 mm × 35 mm (W×L×H)
- **Base thickness**: 5 mm
- **Fin count**: 25 fins, 1.5 mm thick, 2.5 mm pitch
- **Thermal resistance**: ~0.35°C/W (natural convection), ~0.15°C/W (200 LFM airflow)
- **Attachment**: 4× M2.5 spring-loaded screws to PCB standoffs
- **Interface material**: Honeywell PCM45F phase-change pad, 0.25 mm thick, 3.5 W/m·K
- **Coverage**: FPGA + DDR4 region

### PCB Thermal Features
- **Thermal vias under FPGA**: 100× 0.30 mm vias on 1.5 mm grid, connecting L1 to L16
- **Copper pours**: 2 oz copper on L1 and L16 under FPGA footprint
- **DDR4 thermal relief**: 0.5 oz copper spokes to plane (not full thermal relief — keep low inductance)
- **PMIC thermal pad**: 20× thermal vias under TPS65218 exposed pad to L16 copper pour

### Airflow Requirements
- **Minimum**: 200 LFM (linear feet per minute) across heatsink
- **Provided by**: PCIe slot adjacency — typical chassis provides 150-300 LFM
- **Auxiliary**: Optional 40 mm × 10 mm fan (J7 connector), 5V, 0.1A

## 9. Clearance and Creepage

### Voltage Classes
| Voltage | Clearance (uncoated) | Creepage (uncoated) | Notes |
|---------|---------------------|--------------------|-------|
| 12V (PCIe) | 0.5 mm | 1.0 mm | Primary power |
| 3.3V | 0.2 mm | 0.5 mm | I/O and peripherals |
| 1.8V | 0.2 mm | 0.5 mm | Auxiliary |
| 1.2V | 0.2 mm | 0.5 mm | DDR4, MGTAVTT |
| 1.0V | 0.2 mm | 0.5 mm | Core, MGTAVCC |
| 0.6V | 0.2 mm | 0.5 mm | DDR4 VTT |
| 5V (HDMI) | 0.5 mm | 1.0 mm | HDMI power output |

### Special Clearance Zones
- **PCIe edge connector**: 1.0 mm clearance from gold fingers to any copper
- **HDMI connector**: 2.0 mm clearance around TMDS pairs to other signals
- **QSFP+ cage**: 1.5 mm clearance around high-speed pairs
- **Mounting holes**: 3.0 mm keepout radius (no copper, no components)

## 10. Component Placement

### Top Side (L1)
```
┌────────────────────────────────────────────────────────────┐
│ PCIe BRACKET                                              │
│ ┌──────────┐  ┌──────────┐  ┌──────┐                      │
│ │ HDMI (J2)│  │QSFP+(J3) │  │USB(J4)│                     │
│ └──────────┘  └──────────┘  └──────┘                      │
│                                                            │
│  ┌──────────────────────────────────────┐                  │
│  │         XC7K325T (U1)                 │                  │
│  │         FFG900 BGA                    │                  │
│  │         Centered on board             │                  │
│  └──────────────────────────────────────┘                  │
│                                                            │
│  ┌────┐ ┌────┐  ┌────┐ ┌────┐                             │
│  │U6  │ │U7  │  │U8  │ │U9  │  DDR4 (top of FPGA)        │
│  │DDR4│ │DDR4│  │DDR4│ │DDR4│                             │
│  └────┘ └────┘  └────┘ └────┘                             │
│                                                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                 │
│  │TPS65218  │  │ Si5345   │  │ FT601    │                 │
│  │PMIC (U4) │  │Clock(U3) │  │USB3 (U5) │                 │
│  └──────────┘  └──────────┘  └──────────┘                 │
│                                                            │
│  ┌──────────┐  ┌──────────┐                               │
│  │STM32H743 │  │MT25QU512 │                               │
│  │(U2)      │  │QSPI (U10)│                               │
│  └──────────┘  └──────────┘                               │
│                                                            │
│  ┌─────┐┌─────┐┌─────┐┌─────┐  TMP117 sensors             │
│  │U12  ││U13  ││U14  ││U15  │                              │
│  └─────┘└─────┘└─────┘└─────┘                              │
│                                                            │
│  [J5 JTAG]  [J6 I2C]  [J7 FAN]                            │
│                                                            │
│  D1 D2 D3 D4  (LEDs)                                       │
└────────────────────────────────────────────────────────────┘
```

### Bottom Side (L16)
- Decoupling capacitors: 0402 array under FPGA footprint
- TPS51200 VTT regulator (U20) — DDR4 termination
- TXS0102 level shifter (U21) — HDMI DDC
- SiT5356 TCXO (U16) — clock reference
- PCIe edge connector gold fingers

### Placement Constraints
| Component | Min Spacing to FPGA | Reason |
|-----------|---------------------|--------|
| DDR4 U6-U9 | 15 mm | Signal integrity, length matching |
| TPS65218 | 25 mm | Power plane IR drop |
| Si5345 | 20 mm | Clock routing |
| FT601 | 20 mm | USB 3.0 signal integrity |
| STM32H743 | 30 mm | Not critical, general I/O |
| QSPI Flash | 15 mm | Short SPI traces |
| HDMI Connector | 40 mm | TMDS routing to edge |
| QSFP+ Cage | 35 mm | High-speed routing to edge |

## 11. Keepout Zones

| Zone | Location | Dimensions | Reason |
|------|----------|------------|--------|
| PCIe Edge Finger | Bottom edge, L1-L16 | 56 mm × 8 mm | Gold finger contact area |
| FPGA BGA | Center | 31 mm × 31 mm | Component footprint |
| Heatsink Mounting | 4 corners of FPGA | 5 mm dia each | Standoff clearance |
| HDMI Connector | Top edge, left | 15 mm × 12 mm | Through-hole connector |
| QSFP+ Cage | Top edge, center | 22 mm × 18 mm | Cage footprint |
| USB Connector | Top edge, right | 10 mm × 8 mm | Through-hole connector |
| Mounting Holes | 4 corners | 5 mm dia each | Screw clearance |
| PCIe Bracket | Top edge | 120 mm × 5 mm | Mechanical interference |
| Board Edge | Perimeter | 1.0 mm inward | Manufacturing clearance |

## 12. Design Rule Checks (DRC)

### Net Classes
| Class | Trace Width | Clearance | Via Type | Max Length |
|-------|-------------|-----------|----------|------------|
| PCIE_DIFF | 3.0 mil | 5.0 mil (pair), 15 mil (other) | 0.20/0.45 mm | 5000 mil |
| HDMI_DIFF | 2.8 mil | 4.5 mil (pair), 15 mil (other) | 0.20/0.45 mm | 4000 mil |
| DDR4_DQ | 5.5 mil | 8.0 mil | 0.20/0.45 mm | 3000 mil |
| DDR4_ADDR | 5.5 mil | 8.0 mil | 0.20/0.45 mm | 4000 mil |
| DDR4_DIFF | 4.0 mil | 6.0 mil (pair), 12 mil (other) | 0.20/0.45 mm | 3000 mil |
| QSFP_DIFF | 3.0 mil | 5.0 mil (pair), 15 mil (other) | 0.20/0.45 mm | 3000 mil |
| USB3_DIFF | 3.5 mil | 6.0 mil (pair), 12 mil (other) | 0.20/0.45 mm | 2500 mil |
| GENERAL | 5.0 mil | 5.0 mil | 0.20/0.45 mm | 10000 mil |
| POWER | 20.0 mil | 10.0 mil | 0.30/0.60 mm | — |

### Manufacturing Constraints
| Parameter | Value | Notes |
|-----------|-------|-------|
| Min Annular Ring | 5 mil (0.125 mm) | IPC Class 3 |
| Min Solder Mask Web | 3 mil (0.075 mm) | Between pads |
| Solder Mask Expansion | 2 mil (0.05 mm) | Per pad |
| Min Silkscreen Width | 6 mil (0.15 mm) | |
| Min Silkscreen Text Height | 40 mil (1.0 mm) | |
| Board Edge Clearance | 20 mil (0.5 mm) | Copper to edge |
| Route Keepout from Edge | 40 mil (1.0 mm) | Components to edge |

## 13. Fabrication Notes

### PCB Manufacturer Requirements
- **Capability**: 16-layer HDI, blind/buried vias, VIPPO
- **Minimum trace/space**: 3 mil / 3 mil
- **Impedance control**: ±10% (standard), ±5% (HDMI)
- **Surface finish**: ENIG per IPC-4552
- **Solder mask**: LPI green, both sides
- **Testing**: 100% electrical test (flying probe)
- **Certification**: UL 94V-0, IPC-6012 Class 3

### Assembly Notes
- **BGA placement**: X-ray inspection after reflow
- **Reflow profile**: Lead-free SAC305, peak 245°C ±5°C
- **Heatsink attachment**: Post-reflow, torque 0.4 N·m per screw
- **PCIe bracket**: Attach with 2× M2.5 screws after assembly
- **Conformal coating**: Optional acrylic (for harsh environments)

## 14. Signal Integrity Simulation Checklist

| Simulation | Tool | Pass Criteria |
|------------|------|---------------|
| PCIe Channel (TX→RX) | HyperLynx / SiSoft | Eye height >100 mV, eye width >0.5 UI at 8 GT/s |
| HDMI TMDS Channel | HyperLynx | Eye height >150 mV, eye width >0.6 UI at 6 Gbps |
| DDR4 DQ Write Eye | HyperLynx DDRx | Eye height >100 mV, eye width >0.3 UI at 2400 Mbps |
| DDR4 ADDR/CMD Setup/Hold | HyperLynx DDRx | Setup >50 ps, Hold >50 ps at DRAM ball |
| QSFP+ Channel | HyperLynx | Eye height >80 mV, eye width >0.5 UI at 10.3125 Gbps |
| USB 3.0 Channel | HyperLynx | Eye height >100 mV, eye width >0.6 UI at 5 Gbps |
| PDN Impedance | Sigrity PowerSI | Z < target across 0-100 MHz for all rails |
| PDN Resonance | Sigrity OptimizePI | No resonance peaks within 10-200 MHz |
| Thermal Simulation | FloTHERM / Icepak | Tj < 100°C at 45W FPGA, 200 LFM airflow |

## 15. PCB Manufacturing Data Package

### Gerber Files (RS-274X)
```
photonweave.GTL      — Top Copper (L1)
photonweave.GTS      — Top Solder Mask
photonweave.GTO      — Top Silkscreen
photonweave.GTP      — Top Paste
photonweave.G2L      — Inner Signal (L3)
photonweave.G3L      — Inner Signal (L5)
photonweave.G4L      — Power Plane (L6)
photonweave.G5L      — Power Plane (L7)
photonweave.G6L      — Inner Signal (L12)
photonweave.G7L      — Inner Signal (L14)
photonweave.GBL      — Bottom Copper (L16)
photonweave.GBS      — Bottom Solder Mask
photonweave.GBO      — Bottom Silkscreen
photonweave.GBP      — Bottom Paste
photonweave.GKO      — Board Outline / Keepout
photonweave.DRL      — NC Drill File
photonweave.DRR      — NC Drill Report
```

### Additional Files
```
photonweave.IPC356   — IPC-D-356 Netlist (for electrical test)
photonweave.BOM      — Bill of Materials (CSV)
photonweave.PP       — Pick & Place (CSV, centroid + rotation)
photonweave.STACKUP  — Layer stackup report (PDF)
photonweave.IMPEDANCE— Impedance coupon report
photonweave.FAB      — Fabrication drawing (PDF)
photonweave.ASSY     — Assembly drawing (PDF)
```
