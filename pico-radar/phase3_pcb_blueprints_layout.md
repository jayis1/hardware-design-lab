# PicoRadar — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

8-layer, 1.6 mm total thickness, FR-4 (Isola 370HR, Dk = 4.2, Df = 0.015 @ 1 GHz).

| Layer | Name | Material | Thickness | Copper Weight | Purpose |
|---|---|---|---|---|---|
| L1 | Top (Signal) | FR-4 370HR | 0.10 mm prepreg | 1 oz (35 µm) | Component placement, high-speed signal routing, QSPI, SPI, RMII |
| L2 | GND Plane 1 | FR-4 core | 0.20 mm core | 1 oz (35 µm) | Solid ground reference for L1 signals; radar thermal pad heatsink |
| L3 | Inner Signal 1 | FR-4 prepreg | 0.10 mm prepreg | 0.5 oz (17.5 µm) | USB diff pairs, Ethernet diff pairs (shielded by L2 and L4) |
| L4 | Power Plane (3.3 V) | FR-4 core | 0.20 mm core | 1 oz (35 µm) | 3.3 V power island for VDDIO, PHY, ESP32 |
| L5 | Power Plane (1.2 V/1.8 V) | FR-4 prepreg | 0.10 mm prepreg | 0.5 oz (17.5 µm) | Split plane: 1.2 V core, 1.8 V I/O; island fill |
| L6 | Inner Signal 2 | FR-4 core | 0.20 mm core | 0.5 oz (17.5 µm) | UART, I2C, GPIO routing, low-speed signals |
| L7 | GND Plane 2 | FR-4 prepreg | 0.10 mm prepreg | 1 oz (35 µm) | Bottom ground reference, additional shielding |
| L8 | Bottom (Signal) | FR-4 core | 0.10 mm | 1 oz (35 µm) | Bottom-side passives, test points, expansion header |

**Controlled Impedance Stackup (50 Ω single, 100 Ω diff on L1):**
- L1 to L2 dielectric: 0.10 mm FR-4 prepreg (Dk ≈ 4.2)
- 50 Ω microstrip: trace width = 0.20 mm, gap to coplanar GND = 0.15 mm
- 100 Ω differential: trace width = 0.18 mm, pair gap = 0.15 mm

**Controlled Impedance Stackup (90 Ω diff on L3 — USB):**
- L3 is 0.10 mm from L2 (GND) and 0.10 mm from L4 (PWR)
- 90 Ω stripline: trace width = 0.20 mm, pair gap = 0.15 mm
- Add ground stitching vias every 2 mm along diff pair route

## 2. Board Outline & Dimensions

```
     85 mm
  ┌──────────────────────────────────────┐
  │  ○                            ○      │ 55 mm
  │  M2   ┌──────────────────┐     M2    │
  │       │  IWR6843AOP      │           │
  │       │  (AiP Radar)     │           │
  │       └──────────────────┘           │
  │                                      │
  │    ┌──────────────────────┐          │
  │    │   STM32H743VIT6      │          │
  │    │   (LQFP-100)         │          │
  │    └──────────────────────┘          │
  │                                      │
  │  ┌────┐  ┌────┐  ┌──────┐  ┌──────┐ │
  │  │OLED│  │IMU │  │ESP32 │  │PMIC  │ │
  │  └────┘  └────┘  └──────┘  └──────┘ │
  │                                      │
  │  ┌──────────┐        ┌──────────┐   │
  │  │  RJ45    │        │  USB-C   │   │
  │  │  +PoE    │        │         │   │
  │  └──────────┘        └──────────┘   │
  │  ○                            ○      │
  │  M2                           M2    │
  └──────────────────────────────────────┘
```

- **Board size:** 85 mm × 55 mm × 1.6 mm
- **Corner mounting holes:** 4× M2 (3.2 mm diameter), centered 3 mm from each edge
- **Keepout zone above IWR6843AOP:** 15 mm × 15 mm centered on radar IC — no copper, no components, no silkscreen (mmWave transparent window area)
- **Connector placement:** USB-C on right edge, RJ45 on left edge, SWD header on top edge
- **Component side:** L1 (top) — all ICs except some decoupling
- **Solder side:** L8 (bottom) — some decoupling caps, test points, expansion header J4

## 3. Component Placement Strategy

### 3.1 Zone Assignments

| Zone | Components | Notes |
|---|---|---|
| **Radar zone** (top-center) | IWR6843AOP, W25Q128, 40 MHz TCXO, radar decoupling | Keep away from high-freq digital; thermal pad to L2 GND |
| **Host zone** (center) | STM32H743VIT6, MX25L25645G, 8 MHz crystal | Central placement for equal-length routing |
| **Connectivity zone** (bottom-left) | ESP32-C3, chip antenna keepout | Antenna keepout per Espressif guidelines |
| **Ethernet zone** (left edge) | LAN8720A, RJ45 w/magnetics, 25 MHz crystal | Short diff-pair path to connector |
| **Power zone** (bottom-right) | TPS65263, inductors L1–L3, TPS2378 | Isolate from sensitive analog |
| **Sensor zone** (bottom-center) | ICM-42688, SH1106 OLED | Near STM32 for short SPI/I2C |
| **USB zone** (right edge) | USB-C J1, USBLC6 ESD | Short D+/D– path |

### 3.2 Placement Rules

1. **IWR6843AOP:** Centered at (42.5 mm, 20 mm) with thermal pad; no copper on L1 in the 15×15 mm zone above the IC (antenna window)
2. **STM32H743:** Centered at (42.5 mm, 35 mm); QSPI flash within 10 mm on the right
3. **Decoupling caps:** Within 2 mm of each power pin; via to power/GND plane directly at cap pad
4. **TCXO (40 MHz):** Within 5 mm of IWR6843 REFCLK input pin; guard ring of GND vias
5. **Crystal 8 MHz:** Within 5 mm of STM32H7 OSC_IN/OSC_OUT; guard ring of GND vias
6. **Ethernet diff pairs:** LAN8720 → RJ45 via shortest path on L3; ≤ 25 mm total length

## 4. High-Speed Routing Rules

### 4.1 USB 2.0 Full-Speed (12 Mbps)

| Rule | Value |
|---|---|
| Pair | D+ / D– |
| Impedance | 90 Ω ±10% differential |
| Layer | L3 (stripline between L2 GND and L4 PWR) |
| Trace width | 0.20 mm |
| Pair gap | 0.15 mm |
| Max length | 50 mm (STM32 to USB-C) |
| Length skew | D+ and D– matched within 0.15 mm |
| ESD placement | USBLC6 within 5 mm of connector |
| Via count | Max 2 vias per trace (layer change at connector entry) |
| GND stitching | Via every 2 mm along pair path on both sides |

### 4.2 Ethernet RMII (50 MHz REFCLK + data)

| Rule | Value |
|---|---|
| REFCLK impedance | 50 Ω ±10% single-ended |
| REFCLK max length | 50 mm |
| REFCLK series termination | 49.9 Ω at source |
| Data signals | 50 Ω single-ended, length-matched to REFCLK within 10 mm |
| Differential pairs (TXP/TXN, RXP/RXN) | 100 Ω ±10% diff |
| Diff pair layer | L3 |
| Diff pair max length | 25 mm (PHY to RJ45 magnetics) |
| Diff pair via count | 0 (route entirely on L3, no layer changes) |

### 4.3 QSPI (80 MHz clock, STM32H7 → MX25L25645G)

| Rule | Value |
|---|---|
| Signal impedance | 50 Ω ±15% single-ended |
| Clock max length | 30 mm |
| Data max length | 35 mm |
| Length matching | CLK skew: all DQ matched to CLK within 5 mm |
| Series termination | 33 Ω at source (CLK only) |
| Layer | L1 (top layer, shortest path) |
| Via count | 0 preferred, max 1 per signal |

### 4.4 SPI1 (10 MHz, STM32H7 → ICM-42688)

| Rule | Value |
|---|---|
| Signal impedance | 50 Ω ±15% single-ended |
| Max length | 20 mm |
| Length matching | SCK, MOSI, MISO within 10 mm |
| Layer | L1 |
| CSz | Routed with 10 kΩ pull-up at destination |

## 5. DDR Length Matching (N/A)

This design has no DDR memory. The STM32H743 uses internal 1 MB SRAM and external QSPI flash for code XIP (execute-in-place). No length matching required beyond QSPI rules above.

## 6. Via Strategy

| Via Type | Drill | Pad | Layers | Usage |
|---|---|---|---|---|
| Standard via | 0.20 mm | 0.45 mm | All 8 | Signal routing, power/GND transitions |
| Micro via (laser) | 0.10 mm | 0.30 mm | L1–L2 only | Decoupling cap GND connection (via-in-pad) |
| Thermal via | 0.30 mm | 0.60 mm | All 8 | IWR6843 thermal pad (3×3 array) |
| Stitching via | 0.20 mm | 0.45 mm | All 8 | GND plane stitching, diff pair guard |

### 6.1 Via Placement Rules

1. **No vias under IWR6843AOP antenna window** (15 × 15 mm keepout)
2. **Via-in-pad** allowed for 0402 decoupling caps (micro via, filled/plugged)
3. **Thermal vias** under IWR6843: 3×3 grid, 1.0 mm pitch, connected to L2 GND
4. **GND stitching vias** every 2 mm along USB and Ethernet diff pairs
5. **Power via arrays** at each regulator output (2×2 minimum per rail)
6. **Via-to-via spacing** ≥ 0.50 mm (center-to-center) except thermal arrays

## 7. Thermal Management

### 7.1 IWR6843AOP Thermal

- **Power dissipation:** ~1.5 W (active radar operation)
- **Thermal pad:** Exposed pad on bottom of BGA package
- **Thermal vias:** 3×3 array (0.3 mm drill) connecting pad to L2 GND plane
- **GND plane heatsink:** L2 (1 oz) acts as primary heat spreader — 50 mm × 30 mm copper pour
- **Thermal resistance:** θ_JA ≈ 25 °C/W (with 9 thermal vias + L2 plane)
- **Junction temp at 60 °C ambient:** T_J = 60 + 1.5 × 25 = 97.5 °C (within 125 °C max)

### 7.2 STM32H743 Thermal

- **Power dissipation:** ~0.5 W at 480 MHz
- **Thermal pad:** Exposed pad on QFP bottom
- **Thermal vias:** 2×2 array to L2 GND
- **θ_JA ≈** 35 °C/W
- **Junction temp at 60 °C ambient:** T_J = 60 + 0.5 × 35 = 77.5 °C (safe)

### 7.3 ESP32-C3 Thermal

- **Power dissipation:** ~0.3 W (WiFi active)
- **No thermal pad** — relies on module PCB copper
- **Place near board edge** for ventilation

### 7.4 PMIC (TPS65263) Thermal

- **Power dissipation:** ~0.4 W (at 3.5 W load, ~90% efficiency per buck)
- **Thermal pad:** VQFN exposed pad
- **4 thermal vias** to L2 GND

## 8. Clearance & Creepage

| Parameter | Value | Standard | Notes |
|---|---|---|---|
| Minimum trace spacing (LV) | 0.10 mm | IPC-2221B | For signals ≤ 15 V |
| Minimum trace spacing (mains) | 0.50 mm | IEC 61010-1 | PoE input (48 V) isolated zone |
| Minimum trace width (power) | 0.25 mm | IPC-2221B | 0.5 A capacity (1 oz) |
| Minimum trace width (3.3 V plane) | Fill | — | Power plane provides current |
| Creepage (PoE to LV) | 1.5 mm | IEC 60950-1 | TPS2378 output to 5 V rail |
| Clearance (PoE to LV) | 1.5 mm | IEC 60950-1 | Air gap between PoE and SELV |
| Solder mask sliver | ≥ 0.10 mm | — | Between pads |
| Copper-to-edge | ≥ 0.50 mm | — | All layers |

### 8.1 PoE Isolation Zone

- TPS2378 PoE PD controller and associated high-voltage components placed in a **2 mm isolation moat** around the RJ45 connector
- No LV traces cross the PoE zone except through the TPS2378's integrated isolation diode output
- Slot cut in L2–L7 between PoE zone and main PCB ground (connected only at single point)

## 9. Design Rules Summary

| Rule | Value |
|---|---|
| Minimum trace width | 0.10 mm (4 mil) |
| Minimum trace spacing | 0.10 mm (4 mil) |
| Minimum via drill | 0.20 mm (8 mil) |
| Minimum via pad | 0.45 mm (18 mil) |
| Minimum annular ring | 0.125 mm (5 mil) |
| Minimum solder mask opening | 0.05 mm (2 mil) over pad |
| Minimum silkscreen width | 0.15 mm (6 mil) |
| Board outline tolerance | ±0.10 mm |
| Impedance tolerance | ±10% (high-speed), ±15% (general) |
| Max via count on diff pair | 2 |
| Net class: High-speed diff | 90/100 Ω diff, 0.15 mm gap, matched length |
| Net class: High-speed SE | 50 Ω, length-matched |
| Net class: Power | 0.25 mm min width, no neck below 0.15 mm |
| Net class: General | 0.10 mm width, no special rules |

## 10. Keepout Zones

| Zone | Location | Size | Restrictions |
|---|---|---|---|
| **Radar antenna window** | Centered on IWR6843AOP | 15 × 15 mm | No copper on L1, no vias on any layer, no components, no silkscreen |
| **ESP32 antenna keepout** | Below ESP32-C3 module | 15 × 20 mm | No copper on any layer below chip antenna area |
| **USB ESD** | Around USBLC6 | 2 × 3 mm | No via cuts between D+/D– paths |
| **PoE isolation** | Left edge, RJ45 area | 10 × 15 mm | Creepage/clearance rules per IEC 60950-1 |
| **Crystal guard** | Around each crystal | 3 × 5 mm | No high-speed signals crossing; GND via fence |

## 11. Grounding Strategy

1. **L2 and L7 are solid GND planes** — no splits, no traces (except thermal vias)
2. **Single-point star ground** at TPS65263 GND pad — all ground currents return here
3. **Analog ground:** STM32H7 VDDA uses ferrite bead (FB3) from 3.3 V plane; GND side tied to L2 directly
4. **Chassis ground:** RJ45 shield connected to L7 GND via 1 nF cap + 1 MΩ resistor (ESD drain)
5. **USB shield:** Connected to GND via ferrite bead FB1
6. **Ground stitching:** Via arrays every 5 mm around board perimeter connecting L2 and L7

## 12. Test Points

| TP | Net | Location | Type |
|---|---|---|---|
| TP1 | 3.3 V | Near BUCK1 output | Probe pad (1 mm) |
| TP2 | 1.8 V | Near BUCK2 output | Probe pad |
| TP3 | 1.2 V | Near BUCK3 output | Probe pad |
| TP4 | GND | Board edge | Probe pad |
| TP5 | RADAR_TX | Near UART route | Loop for scope probe |
| TP6 | ETH_REFCLK | Near PHY | Loop for scope probe |
| TP7 | USB_DP | Near USBLC6 | Loop for scope probe |
| TP8 | PGOOD | Near TPS65263 | Probe pad |
| TP9 | IMU_INT1 | Near ICM-42688 | Probe pad |
| TP10 | SWD_CLK | Debug header | Probe pad |