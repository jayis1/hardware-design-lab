# WaveForge — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

| Layer | Material | Thickness | Copper Weight | Purpose |
|---|---|---|---|---|
| Top (L1) | — | 0.035 mm | 1 oz (35 µm) | Signal + component pads |
| PP1 | FR-4 2116 | 0.10 mm | — | Prepreg |
| Inner1 (L2) | — | 0.035 mm | 1 oz (35 µm) | Ground plane (continuous) |
| Core | FR-4 | 0.80 mm | — | Rigid core |
| Inner2 (L3) | — | 0.035 mm | 1 oz (35 µm) | Power plane (3.3 V, 1.2 V split) |
| PP2 | FR-4 2116 | 0.10 mm | — | Prepreg |
| Bottom (L4) | — | 0.035 mm | 1 oz (35 µm) | Signal routing (bottom-side components) |

**Total board thickness:** ~1.13 mm (44 mil) — compatible with standard 1.6 mm PCB re-specification if needed by adding core thickness.

**Adjusted for 1.6 mm standard:**

| Layer | Material | Thickness | Copper Weight | Purpose |
|---|---|---|---|---|
| Top (L1) | — | 0.035 mm | 1 oz (35 µm) | Signal + component pads |
| PP1 | FR-4 1080 | 0.15 mm | — | Prepreg |
| Inner1 (L2) | — | 0.035 mm | 0.5 oz (17.5 µm) | Ground plane (continuous) |
| Core | FR-4 | 1.20 mm | — | Rigid core |
| Inner2 (L3) | — | 0.035 mm | 0.5 oz (17.5 µm) | Power plane (3.3 V / 1.2 V split) |
| PP2 | FR-4 1080 | 0.15 mm | — | Prepreg |
| Bottom (L4) | — | 0.035 mm | 1 oz (35 µm) | Signal routing (bottom-side passives) |

**Total:** 1.64 mm (≈ 1.6 mm standard)

**Dielectric constants:** FR-4 core Er = 4.5, prepreg Er = 4.2

## 2. Board Outline

```
┌──────────────────────────────────────────────────────────────────┐
│                                                                  │
│  ┌──────┐                           ┌─────┐  ┌─────┐            │
│  │MIDI  │                           │LINE │  │LINE │  ┌──────┐  │
│  │ IN   │                           │OUT L│  │OUT R│  │ HP   │  │
│  └──────┘                           └─────┘  └─────┘  │ OUT  │  │
│                                                          └──────┘  │
│  ┌──────┐  ┌──────────────────────────────────┐                     │
│  │MIDI  │  │                                  │  ┌─────┐ ┌─────┐  │
│  │THRU  │  │       STM32H743VIT6              │  │LINE │ │LINE │  │
│  └──────┘  │          (U1)                    │  │IN L │ │IN R │  │
│            │                                  │  └─────┘ └─────┘  │
│  ┌──────┐  └──────────────────────────────────┘                     │
│  │BARREL│                    ┌──────┐                                │
│  │ 7-12V│                    │WM8778│     ┌──────────┐              │
│  └──────┘                    └──────┘     │W25Q256  │              │
│                                           └──────────┘              │
│  ┌────────────────┐   ┌────┐                                      │
│  │   USB-C        │   │SWD │   ┌────────┐    CV IN ┌───────┐     │
│  └────────────────┘   └────┘   │TPA6130 │   ┌──┐   │CV HDR │     │
│                                └────────┘   │  │   └───────┘     │
│                                             └──┘                   │
│  0  10  20  30  40  50  60  70  80  90 100mm                       │
│  (Mounting holes at 4 corners, M3, inside outline)                │
└──────────────────────────────────────────────────────────────────┘
```

- **Board dimensions:** 100 mm × 80 mm
- **Corner radii:** 2 mm
- **Mounting holes:** 4 × M3 (3.2 mm diameter), 5 mm from each edge
- **Board thickness:** 1.6 mm
- **Surface finish:** ENIG (Ni/Au) for fine-pitch ICs
- **Solder mask:** Green (both sides)
- **Silkscreen:** White (both sides)

## 3. High-Speed Routing Rules

### 3.1 USB 2.0 Full-Speed (12 Mbps)

| Parameter | Value |
|---|---|
| Differential impedance | 90 Ω ±10% |
| Single-ended impedance | 45 Ω ±10% |
| Trace width (L1, coplanar) | 0.20 mm |
| Trace spacing (differential) | 0.15 mm |
| Length matching | Within 0.5 mm |
| Maximum via count | 2 per signal |
| ESD protection | TPD4E05U06, placed within 5 mm of USB-C connector |
| Route on | L1 (top) preferred, minimize layer transitions |

### 3.2 I2S Audio Bus (up to 3.072 MHz)

| Parameter | Value |
|---|---|
| Trace width | 0.15 mm (signal) |
| Spacing | ≥ 0.20 mm from other nets |
| Length matching (MCLK, BCLK, LRCK, SDIN, SDOUT) | Within 5 mm of each other |
| Series termination | 33 Ω on MCLK, BCLK |
| Route on | L1 preferred (short runs) |

### 3.3 SPI/QSPI Flash (up to 80 MHz)

| Parameter | Value |
|---|---|
| Trace width | 0.15 mm |
| Spacing | ≥ 0.20 mm |
| Length matching (SCK, MISO, MOSI, CS) | Within 10 mm |
| Keepout | No high-frequency traces within 1 mm of SPI group |

### 3.4 I2C Bus (400 kHz)

| Parameter | Value |
|---|---|
| Trace width | 0.15 mm |
| SCL/SDA length matching | Not required |
| Pull-up placement | Within 10 mm of last device |

### 3.5 General Signal Routing

| Parameter | Value |
|---|---|
| Minimum trace width | 0.10 mm (0.15 mm preferred) |
| Minimum trace spacing | 0.10 mm (0.15 mm preferred) |
| Minimum via diameter | 0.40 mm (pad) / 0.20 mm (drill) |
| Default net class width | 0.15 mm |

## 4. DDR Length Matching

Not applicable — WaveForge uses internal SRAM only (no external DDR). However, QSPI flash traces should follow length-matching rules in Section 3.3.

## 5. Via Strategy

| Via Type | Drill | Pad | Usage |
|---|---|---|---|
| Standard via | 0.20 mm | 0.40 mm | Signal routing, layer transitions |
| Power via | 0.30 mm | 0.60 mm | Power/GND stitching, decoupling |
| Thermal via | 0.30 mm | 0.60 mm | Under thermal pads (ICs, regulators) |
| Micro via | N/A | N/A | Not used (4-layer standard fab) |

### Via Placement Rules

- **Stitching vias:** Place power vias every 3 mm along power rail traces
- **Decoupling vias:** One via per decoupling cap pad to ground plane (L2)
- **Via-in-pad:** Allowed on QFN center pads (U2, U4) with filled + capped vias
- **Fanout:** Each BGA/QFN pad gets its own via to inner layers within 0.5 mm
- **Via spacing from pads:** Minimum 0.25 mm clearance from pad edge to via

## 6. Thermal Management

### 6.1 Thermal Budget

| Component | Power Dissipation | Junction Temp (Ta=70°C) | Margin |
|---|---|---|---|
| STM32H743 | 300 mW (worst case) | ~85°C (θJA=50°C/W) | ✅ 15°C to 100°C limit |
| WM8778 | 250 mW | ~80°C | ✅ |
| TLV62569 | 150 mW (buck losses) | ~78°C | ✅ |
| TPA6130 | 200 mW (HP drive) | ~82°C | ✅ |
| W25Q256 | 50 mW | ~73°C | ✅ |

### 6.2 Thermal Vias

- **STM32H743 (LQFP-100):** No center thermal pad, but use large copper pour on L1 under IC with 4× thermal vias to L2 (GND)
- **WM8778 (QFN-28):** Exposed center pad, 6× thermal vias (0.3 mm drill) connecting to L2 GND
- **TPA6130A2 (QFN-16):** Exposed center pad, 4× thermal vias to L2 GND
- **TLV62569:** Thermal pad under IC, 3× thermal vias to GND pour on L1 and L2

### 6.3 Copper Pour Areas

- **L1 top pour:** GND fill in unused areas (analog and digital ground separated by 0.5 mm gap near codec, bridged at single point)
- **L2:** Continuous GND plane (no splits)
- **L3:** Split power plane — 3.3 V pour covering ~70%, 1.2 V pour covering ~30% (under STM32 core region)
- **L4 bottom pour:** GND fill, with isolated analog ground island around codec analog section

### 6.4 Heat Dissipation Path

```
IC junction → die → QFN thermal pad → solder → copper pour (L1) → thermal vias → GND plane (L2) → board edges → enclosure → air
```

## 7. Clearance & Creepage

### 7.1 Standard Clearances (IPC-2221B)

| Voltage | Clearance | Creepage | Notes |
|---|---|---|---|
| < 15 V (all signal) | 0.10 mm | 0.10 mm | Default for all digital nets |
| 15–30 V (barrel input) | 0.20 mm | 0.40 mm | DC jack input 7–12 V nominal |
| 30–50 V | 0.40 mm | 0.80 mm | Not applicable (no high voltage) |
| Mains | N/A | N/A | No mains connection |

### 7.2 Special Clearances

- **MIDI input opto-isolator:** 2 mm clearance between primary (MIDI side) and secondary (MCU side) circuits
- **USB ESD zone:** 0.5 mm clearance between ESD diode and USB connector pads
- **Analog island (codec):** 0.5 mm gap between analog GND pour and digital GND pour on L1, bridged at one point near the codec
- **Crystal keepout:** No signal traces within 1 mm of HSE/LSE crystals, ground guard ring on L1

## 8. Design Rules Summary

| Rule | Value |
|---|---|
| Minimum trace width | 0.10 mm |
| Minimum trace spacing | 0.10 mm |
| Minimum via drill | 0.20 mm |
| Minimum via pad | 0.40 mm |
| Minimum via clearance | 0.25 mm |
| Pad-to-via spacing | 0.25 mm |
| Solder mask clearance | 0.05 mm |
| Silkscreen line width | 0.12 mm |
| Silkscreen text height | 0.80 mm minimum |
| Board edge copper clearance | 0.50 mm |
| Drill-to-drill minimum | 0.50 mm |
| Annular ring minimum | 0.10 mm |
| DRC error limit | 0 |

## 9. Net Classes

| Net Class | Width | Spacing | Via Size | Notes |
|---|---|---|---|---|
| Default | 0.15 mm | 0.15 mm | 0.40/0.20 mm | All signals |
| Power | 0.30 mm | 0.20 mm | 0.60/0.30 mm | VDD, VBUS, 3.3 V, 1.2 V |
| Ground | 0.30 mm | 0.20 mm | 0.60/0.30 mm | GND nets |
| USB Diff | 0.20 mm | 0.15 mm (diff) | 0.40/0.20 mm | USB_DP, USB_DM |
| Audio | 0.15 mm | 0.20 mm | 0.40/0.20 mm | I2S signals, analog audio |
| High Speed | 0.15 mm | 0.20 mm | 0.40/0.20 mm | SPI, QSPI |

## 10. Component Placement Strategy

### Top Side (L1)

```
┌───────────────────────────────────────────────────┐
│  [MIDI IN]  [MIDI THRU]                [LINE OUT] │
│                          [HP OUT]                  │
│                                                     │
│    [6N138]    ┌─────────────────┐                  │
│               │  STM32H743VIT6  │    [LINE IN]     │
│               │     (U1)        │                  │
│  [Crystal]    └─────────────────┘                  │
│                                                     │
│  [BARREL]            [WM8778]   [W25Q256]          │
│                                                     │
│  [USB-C]    [SWD]    [TPA6130]  [CV HDR]          │
│                                                     │
│  [LD39050×2]  [TLV62569]  [24C256]  [LEDs]         │
└───────────────────────────────────────────────────┘
```

### Bottom Side (L4)

- Decoupling capacitors for STM32 (7 pairs, positioned directly under respective VDD pins)
- Pull-up/pull-down resistors for I2C, NRST, BOOT0
- USB ESD protection (TPD4E05U06)
- Audio coupling capacitors for line in/out
- MIDI current-loop resistors

### Placement Priorities

1. **Connectors first:** USB-C, MIDI, audio jacks, barrel jack — placed at board edges per outline
2. **Main ICs:** STM32H743 center, WM8778 near audio jacks, W25Q256 near STM32 QSPI pins
3. **Power regulators:** TLV62569 + LD39050s near power input, close to their load ICs
4. **Decoupling caps:** As close as possible to IC power pins, short loop to GND via
5. **Crystals:** HSE near STM32 OSC pins, LSE near RTC pins, with ground guard rings
6. **Audio path:** Keep analog traces short, away from digital switching nets