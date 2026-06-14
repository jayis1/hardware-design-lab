# SkyPilot — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

**6-Layer, 1.0 mm total thickness** (weight-optimized for drone use)

| Layer | Material | Thickness | Copper | Function |
|---|---|---|---|---|
| L1 (Top) | — | 0.035 mm | 1 oz (35µm) | Signal + component pads |
| PP1 | Isola 370HR | 0.10 mm | — | Prepreg |
| L2 | — | 0.035 mm | 0.5 oz (17.5µm) | Ground plane (unbroken) |
| Core1 | Isola 370HR | 0.21 mm | — | FR-4 core |
| L3 | — | 0.035 mm | 0.5 oz (17.5µm) | Power plane (3V3 + 5V splits) |
| PP2 | Isola 370HR | 0.10 mm | — | Prepreg |
| L4 | — | 0.035 mm | 0.5 oz (17.5µm) | Signal (high-speed: SPI, USB, UART) |
| Core2 | Isola 370HR | 0.21 mm | — | FR-4 core |
| L5 | — | 0.035 mm | 0.5 oz (17.5µm) | Ground plane (unbroken) |
| PP3 | Isola 370HR | 0.10 mm | — | Prepreg |
| L6 (Bottom) | — | 0.035 mm | 1 oz (35µm) | Signal + motor pads + battery pads |

**Total: 1.0 mm ± 0.08 mm**

**Dielectric constant (Dk):** 4.2 @ 1 GHz (Isola 370HR)  
**Loss tangent (Df):** 0.015 @ 1 GHz  
**Glass transition (Tg):** 170°C

### Impedance Calculations (Si8000):

| Trace Type | Layer | Width | Spacing | Target Z | Calculated Z |
|---|---|---|---|---|---|
| 50Ω SE (co-planar) | L1 | 0.15 mm | 0.20 mm (coplanar gap) | 50Ω | 49.8Ω |
| 90Ω diff (USB) | L1 | 0.10 mm | 0.15 mm (pair gap) | 90Ω diff | 89.3Ω diff |
| 50Ω SE (inner) | L4 | 0.12 mm | — | 50Ω | 50.2Ω |

## 2. Board Outline

```
    ┌─────────────────────────────────────────────┐
    │                                             │
    │  ●M1  ●M2  ●M3  ●M4     ●M5  ●M6  ●M7 ●M8│
    │                                             │
    │    ┌─────────────────────────────────┐      │
    │    │                                 │      │
    │    │    STM32H743 (center)          │      │
    │    │         10mm×10mm               │      │
    │    │                                 │      │
    │    │  ┌────┐  ┌────┐  ┌────┐       │      │
    │    │  │ICM │  │BMI │  │BMP │       │      │
    │    │  │42688│  │270 │  │390 │       │      │
    │    │  └────┘  └────┘  └────┘        │      │
    │    │                                 │      │
    │    │  ┌────────────┐ ┌──────────┐  │      │
    │    │  │  LARA-R6   │ │ SAM-M10Q │  │      │
    │    │  │  (16×26mm) │ │(9.7×10mm)│  │      │
    │    │  └────────────┘ └──────────┘  │      │
    │    │                                 │      │
    │    │  ┌──────┐  ┌──────┐          │      │
    │    │  │ESP32 │  │W25Q  │          │      │
    │    │  │-H2   │  │128   │          │      │
    │    │  └──────┘  └──────┘          │      │
    │    │                                 │      │
    │    └─────────────────────────────────┘      │
    │                                             │
    │  ●M9  ●M10 ●M11 ●M12                    │
    │                                             │
    │  [USB-C]  [40-pin JST]  [u.FL] [u.FL]    │
    │                                             │
    │  ◉ 30.5mm  ◉ 30.5mm  ◉ 30.5mm  ◉ 30.5mm  │
    └─────────────────────────────────────────────┘
```

**Dimensions:** 45 mm × 45 mm (1.0 mm thick)  
**Mounting holes:** 4× M3 (Ø3.2mm) at 30.5 mm square pattern  
**Corner radius:** 1.5 mm  
**Board weight target:** ≤ 12 g (bare PCB)

## 3. Component Placement Strategy

### 3.1 Placement Zones

| Zone | Location | Components | Rationale |
|---|---|---|---|
| A (Top-Center) | Center 14×14mm | U1 (STM32H743) | Shortest traces to all peripherals |
| B (Top-Left) | Left of U1, <8mm | U2 (ICM-42688), U3 (BMI270) | Minimize SPI trace length; IMUs as close to CG as possible |
| C (Top-Right) | Right of U1, <8mm | U4 (BMP390) | Barometer near center, away from motor pads |
| D (Bottom-Left) | Lower-left quadrant | U6 (LARA-R6) | LTE modem isolated from analog sensors, RF trace to u.FL |
| E (Bottom-Center) | Below U1 | U8 (TPS65294), U9/U10 (LDOs) | Power tree centralized, short 5V/3V3 pours |
| F (Bottom-Right) | Lower-right quadrant | U5 (SAM-M10Q) | GNSS module, antenna trace to u.FL |
| G (Right-Edge) | Right side | U7 (ESP32-H2) | Companion, UART to STM32 |
| H (Top-Left edge) | Near M1-M4 pads | Motor driver pads M1-M4 | Direct DShot traces from TIM1 |
| I (Bottom edge) | Bottom side, edge | Motor driver pads M5-M8, M9-M12 | Direct DShot traces from TIM1/TIM2/TIM8 |
| J (Left edge) | Bottom, left | USB-C (J1), 40-pin JST (J2) | Connectors on same edge for cable management |

### 3.2 Critical Placement Rules

1. **IMU placement**: ICM-42688-P and BMI270 must be within 8mm of board center of gravity. Orient both IMUs such that their die axes are aligned (X→X, Y→Y). Place on L1 (top) only — do not place on bottom.
2. **Barometer isolation**: BMP390 must be >10mm from any switching regulator and >15mm from motor pad traces. Provide a small vent hole (Ø0.5mm) through the PCB beneath the sensor for static pressure equalization.
3. **LTE modem**: LARA-R6 must be at board edge, with antenna u.FL connector as close as possible. Keep >5mm separation from IMUs.
4. **Crystal placement**: 24MHz HSE crystal must be within 5mm of STM32 OSC pins with guard ground ring.
5. **Decoupling placement**: All 100nF decoupling caps must be within 2mm of their respective IC VDD pin.

## 4. High-Speed Routing Rules

### 4.1 SPI Buses (30 MHz)

| Rule | SPI1 (ICM-42688) | SPI2 (BMI270) |
|---|---|---|
| Max trace length | ≤ 25mm | ≤ 25mm |
| Length matching (SCK/MOSI/MISO) | ±2mm | ±2mm |
| Via count per signal | ≤ 2 | ≤ 2 |
| Ground via spacing | 1 per signal via (stitching) | Same |
| Layer | L1 (preferred) or L4 | L1 (preferred) or L4 |
| Impedance | 50Ω ±15% SE | 50Ω ±15% SE |
| ESD protection | TVS on CS only | TVS on CS only |

### 4.2 USB 2.0 FS (12 Mbps)

| Parameter | Value |
|---|---|
| Differential impedance | 90Ω ±10% |
| Pair length matching | ±0.5mm |
| Max trace length | ≤ 80mm |
| Layer | L1 (top) with grounded coplanar |
| Via count | ≤ 4 per signal (with ground stitching) |
| ESD | TPD4E05U06 (TVS array) near connector |
| Series resistor | 27Ω ±1% on each line (near STM32) |

### 4.3 UART8 (LTE Modem, 921600 baud)

| Parameter | Value |
|---|---|
| Max trace length | ≤ 40mm |
| Impedance | Non-critical (low speed) |
| Series resistor | 22Ω (EMI suppression) |
| Ground stitching | 1 ground via per 3mm |

### 4.4 LTE RF Trace (LARA-R6 → u.FL)

| Parameter | Value |
|---|---|
| Impedance | 50Ω ±5Ω SE |
| Layer | L1 only (no vias) |
| Max length | ≤ 15mm |
| Width | 0.15mm (coplanar waveguide) |
| Coplanar gap | 0.20mm |
| Ground stitching vias | Every 1mm along RF ground |
| Keepout | No other traces within 3mm |

### 4.5 GNSS RF Trace (SAM-M10Q → u.FL)

| Parameter | Value |
|---|---|
| Impedance | 50Ω ±5Ω SE |
| Layer | L1 only (no vias) |
| Max length | ≤ 15mm |
| Width | 0.15mm |
| Keepout | No other traces within 3mm |

## 5. DDR Length Matching

N/A — This design uses on-chip SRAM (1MB) and does not have external DDR.

## 6. Via Strategy

### 6.1 Via Types

| Via Type | Drill | Pad | Plating | Usage |
|---|---|---|---|---|
| Standard via | 0.2mm | 0.4mm | 0.025mm | General signal routing |
| Power via | 0.3mm | 0.6mm | 0.025mm | Power/ground connections |
| Thermal via | 0.3mm | 0.6mm | 0.025mm | Under thermal pads of ICs |
| Stitching via | 0.2mm | 0.4mm | 0.025mm | Ground plane stitching |

### 6.2 Via Rules

1. **No vias on differential pairs** (USB_DP/DM) except at connector transitions.
2. **Ground stitching**: Minimum 1 ground via per 2mm² of ground pour on L1/L6.
3. **Via-in-pad**: Allowed only on LGA/BGA components (ICM-42688, BMI270) with filled and capped vias.
4. **Via fanout**: STM32H743 LQFP-100 uses 0.2mm standard vias for signal fanout, 0.3mm power vias for VDD/VSS.
5. **Escape routing**: Inner rows of LQFP use dog-bone fanout with vias to L4 for horizontal routing.
6. **Maximum via aspect ratio**: 5:1 (drill:board thickness = 0.2:1.0 = 5:1).

## 7. Thermal Management

### 7.1 Thermal Sources

| Component | Power Dissipation | Junction Temp Max | ΘJA (°C/W) | Expected Tj @ 70°C ambient |
|---|---|---|---|---|
| STM32H743 @ 480MHz | ~350 mW | 125°C | 38 (with pour) | ~83°C |
| LARA-R6 (average) | ~800 mW | 85°C | 25 (with pour) | ~90°C |
| TPS65294 (conversion) | ~200 mW | 150°C | 45 (with pour) | ~79°C |
| ESP32-H2 @ 96MHz | ~100 mW | 105°C | 60 | ~76°C |

### 7.2 Thermal Design

1. **STM32H743**: Exposed pad (EP) on LQFP-100 connected to GND pour with 4× thermal vias (0.3mm) to L2/L5 ground planes. Thermal resistance reduced to ~38°C/W.
2. **LARA-R6**: Large ground pad beneath modem with 8× thermal vias to L2/L5. Adjacent copper pour on L3 (3.3V) acts as heat spreader. Add 2oz copper fill on L3 beneath modem.
3. **TPS65294**: Ground pour on L1 beneath IC with 3× thermal vias. Polyfuse not required (internal OCP).
4. **General**: All ICs have continuous ground pours on L2/L5. No thermal relief on power pins — direct connect to copper pours.

### 7.3 Ventilation

- Ø0.5mm vent hole beneath BMP390 barometer for static pressure equalization
- Board designed for top-down airflow from prop wash

## 8. Clearance & Creepage

### 8.1 General Rules

| Parameter | Value | Standard |
|---|---|---|
| Minimum trace width | 0.1 mm | IPC-2221B |
| Minimum trace spacing | 0.1 mm | IPC-2221B |
| Minimum via land pattern | 0.15 mm annular ring | IPC-2221B |
| Component-to-edge | ≥ 1.0 mm | Manufacturing |
| Trace-to-mounting-hole | ≥ 1.5 mm | Mechanical |
| Solder mask clearance | 0.05 mm per side | Standard |

### 8.2 High-Voltage Separation (Battery Input)

| Parameter | Value |
|---|---|
| Battery input (VBAT_RAW) clearance | 0.5 mm (up to 26V) |
| Battery input (VBAT_RAW) creepage | 1.0 mm |
| 5V/3.3V signal clearance | 0.15 mm |
| Isolation groove around battery pads | 0.3mm slot, no copper |

### 8.3 RF Keepout Zones

| Zone | Area | Keepout |
|---|---|---|
| LARA-R6 antenna trace | 3mm either side of RF trace | No copper, no vias, no components |
| SAM-M10Q antenna trace | 3mm either side of RF trace | No copper, no vias, no components |
| LARA-R6 module | Directly beneath module | No signal traces on L1/L6 (power/GND only) |

## 9. Design Rules Summary

| Rule | Value |
|---|---|
| Minimum line width | 0.10 mm (4 mil) |
| Minimum line spacing | 0.10 mm (4 mil) |
| Minimum via drill | 0.20 mm (8 mil) |
| Minimum via annular ring | 0.15 mm (6 mil) |
| Minimum pad-to-pad spacing | 0.15 mm (6 mil) |
| Differential pair tolerance | ±0.025 mm (1 mil) |
| Length matching tolerance | ±2 mm (SPI), ±0.5 mm (USB) |
| Solder mask | LDI photo-imageable, 0.05mm clearance |
| Surface finish | ENIG (Au 0.05µm / Ni 3µm) |
| Silkscreen | White, both sides |
| Board thickness | 1.0 mm ± 0.08 mm |
| Copper weight | 1 oz (35µm) outer / 0.5 oz (17.5µm) inner |
| Impedance control | ±10% for controlled impedance |
| IPC class | Class 2 |
| UL rating | UL 94V-0 |