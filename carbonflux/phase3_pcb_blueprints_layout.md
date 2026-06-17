# Phase 3: PCB Blueprints & Layout — CarbonFlux

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**

---

## 1. PCB Stackup

**4-layer FR-4, 1.6 mm total thickness, ENIG finish, 35 µm copper outer, 17.5 µm inner.**

| Layer | Name | Type | Weight | Notes |
|-------|------|------|--------|-------|
| 1 | Top Signal | Signal + Power | 35 µm | Components, RF traces, USB diff pair |
| 2 | GND | Ground plane | 17.5 µm | Solid copper pour, no splits |
| 3 | SIG/PWR | Signal + Power | 17.5 µm | 3.3V, 5V, 12V pours, SPI/QSPI traces |
| 4 | Bottom | Signal + GND | 35 µm | Components (optional), GND fill |

### 1.1 Why 4-layer?

- **RF integrity** — SX1262 output and matching network need a solid GND plane directly below Layer 1. A 2-layer board would require routed ground returns, degrading +22 dBm TX performance.
- **QSPI at 40 MHz** — Clean return path for 40 MHz SPI clock prevents EMI.
- **Power distribution** — Dedicated inner layer for 3.3V pour reduces IR drop to <10 mV across the 120 × 80 mm board.
- **Cost** — 4-layer FR-4 at JLCPCB/PCBWay is ~$12 for 10 boards, well within budget.

## 2. Board Outline

**120.0 mm × 80.0 mm, rectangular, 4× M3 mounting holes at corners (5 mm from edge).**

### 2.1 Keepout Zones

| Zone | Location | Size | Reason |
|------|----------|------|--------|
| Antenna keepout | Top-right, 25 × 15 mm | No copper on Layer 1 or Layer 2 | SX1262 RF output + matching + SMA |
| USB keepout | Bottom edge, center | 10 × 10 mm | USB-C connector clearance |
| Servo header | Left edge | 7.5 × 2.54 mm | 3-pin header for external servo cable |
| Stake connector | Right edge | 12 × 8 mm | 8-pin waterproof connector |

## 3. Component Placement

### 3.1 Top Layer (Primary)

```
┌────────────────────────────────────────────────────────────────┐
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────┐  ┌─────┐  │
│  │ SMA ANT  │  │ Matching │  │ SX1262   │  │DPS310│  │SCD41│  │
│  │ (Top-R)  │  │ Network  │  │ (Top-R)   │  │      │  │     │  │
│  └──────────┘  └──────────┘  └──────────┘  └──────┘  └─────┘  │
│                                          ┌──────────┐         │
│                                          │ TMP117   │         │
│                                          └──────────┘         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                     │
│  │ USB-C    │  │ STM32U5  │  │ W25Q128  │  ┌──────────┐      │
│  │ (Bot-Ctr)│  │ (Center) │  │          │  │ RV-8803  │      │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘      │
│                    │                                           │
│                    │  ┌──────────┐  ┌──────────┐              │
│                    │  │ CN3791   │  │ TPS63070 │              │
│                    │  │ (Left)   │  │ (Left)   │              │
│                    │  └──────────┘  └──────────┘              │
│                    └─────── Screw Terminals ────────────────────┘
│                       BAT+ ─ ─ ─ Solar+ ─ ─ ─ GND            │
│  ┌──────┐  ┌──────┐                                            │
│  │Servo │  │Stake │  ┌─── DIP ──┐  ┌─── LED ───┐            │
│  │Hdr   │  │Conn  │  │Switch    │  │ R▬ G▬ B▬   │            │
│  └──────┘  └──────┘  └──────────┘  └───────────┘            │
└────────────────────────────────────────────────────────────────┘
```

### 3.2 Placement Rules

1. **SX1262 + matching** — Placed as close as possible to SMA connector. Matching network components (capacitors, inductors) within 5 mm of SX1262 RF pins. No traces under matching network on Layer 2.
2. **STM32U5** — Centered to equalize trace lengths. 100 nF decoupling caps within 3 mm of each VDD pin pair.
3. **USB-C** — On bottom edge for easy access. D+ / D− traces routed as 90Ω differential pair with <2 mm length mismatch.
4. **Power components** — TPS63070 and CN3791 on left side, near screw terminals. Inductors placed with proper clearance from copper pour.
5. **Sensors** — SCD41, DPS310, TMP117 grouped together on right side with I²C bus routed as short as possible. SCD41 placed away from heat sources (TPS63070).

## 4. Routing Rules

| Signal Group | Min Width | Min Spacing | Length Matching | Impedance |
|-------------|-----------|-------------|-----------------|-----------|
| QSPI (40 MHz) | 0.25 mm | 0.20 mm | All traces ±5 mm | ~50Ω |
| SPI1 (8 MHz) | 0.20 mm | 0.15 mm | No requirement | — |
| I²C (400 kHz) | 0.20 mm | 0.15 mm | No requirement | — |
| USB D+/D− | 0.30 mm | 0.15 mm | ±2 mm within pair | 90Ω diff |
| SX1262 RF | 0.40 mm | 0.30 mm | N/A | 50Ω |
| Power (3.3V) | 0.50 mm | 0.20 mm | N/A | — |
| Power (12V) | 0.80 mm | 0.30 mm | N/A | — |
| 1-Wire | 0.25 mm | 0.20 mm | No requirement | — |

### 4.1 Critical Routing Details

#### USB Differential Pair
- Route on Layer 1, with solid GND reference on Layer 2 directly below.
- 90Ω differential impedance: trace width 0.30 mm, gap 0.15 mm, 1.6 mm board height.
- AC coupling capacitors (2× 100 nF, 0402) placed within 5 mm of USB-C connector.
- ESD protection (USBLC6-2SC6) at connector side of AC caps.

#### SX1262 RF Output
- 50Ω microstrip on Layer 1: trace width 0.40 mm, 0.30 mm clearance to GND on same layer.
- Keepout on Layer 2 directly below RF trace — no GND pour under RF line.
- LC matching network: L (3.3 nH 0402) + C (1.5 pF 0402) + C (1.0 pF 0402).
- Harmonic filter: L (4.7 nH) + C (0.8 pF) + L (4.7 nH) — Pi network.

#### QSPI Flash (40 MHz)
- Route SI, SO, SCK, CS on Layer 3 with GND reference on Layer 2.
- All traces length-matched to within ±5 mm.
- Series termination resistors (22Ω, 0402) placed at STM32U5 end of each trace.

## 5. Via Strategy

| Via Type | Drill | Pad | Use |
|----------|-------|-----|-----|
| Signal | 0.30 mm | 0.60 mm | General routing |
| Power | 0.50 mm | 0.90 mm | Power traces > 0.5 A |
| Thermal | 0.40 mm | 0.80 mm | TPS63070 exposed pad |
| Micro | 0.20 mm | 0.40 mm | BGA escape (STM32U5) |

- No via-in-pad required (STM32U5 BGA escape uses 0.20 mm micro vias from Layer 1 to Layer 3).
- TPS63070 exposed pad: 9-via array (3×3) for thermal transfer to Layer 2 GND plane.

## 6. Thermal Management

### 6.1 Copper Pour
- **Layer 2**: Full GND pour. No splits.
- **Layer 1**: GND pour in all open areas, 0.20 mm clearance from signal traces.
- **Layer 3**: 3.3V power pour (hatch pattern, 0.30 mm width, 0.50 mm gap).

### 6.2 Hot Components
| Component | Power Dissipation | Cooling Method |
|-----------|------------------|----------------|
| TPS63070 | 400 mW | 1 cm² copper on Layer 1, thermal vias to Layer 2 |
| SX1262 TX | 500 mW | GND plane via stitching, 30°C temp rise OK |
| STM32U5 | 60 mW | Passive, no special cooling needed |

## 7. DFM (Design for Manufacturing)

| Parameter | Value | Standard |
|-----------|-------|----------|
| Minimum trace width | 0.15 mm | JLCPCB (0.1 mm) |
| Minimum spacing | 0.15 mm | JLCPCB (0.1 mm) |
| Minimum drill | 0.20 mm | JLCPCB (0.15 mm) |
| PCB thickness | 1.6 mm ±10% | Standard |
| Copper weight | 1 oz outer, 0.5 oz inner | Standard |
| Surface finish | ENIG | Lead-free, RoHS |
| Silkscreen color | White | Standard |
| Solder mask | Green | Standard |

## 8. Test Points

| Test Point | Signal | Location | Purpose |
|------------|--------|----------|---------|
| TP1 | VDD_3V3 | Near TPS63070 output | Power rail verification |
| TP2 | VBAT | Near battery terminal | Battery voltage check |
| TP3 | 3V3_SENS | Near SCD41 | Sensor power rail |
| TP4 | GND | Multiple locations | Ground reference |
| TP5 | SWO | STM32U5 PA5 | Trace output for debugging |
| TP6 | BOOT0 | STM32U5 BOOT0 | Pull to 3.3V for DFU mode |

## 9. PCB Fabrication Notes

- Board name: `CARBONFLUX_REV_A`
- Datestamp on silkscreen: `2026-06-17`
- Author: `jayis1`
- Panelization: 2-up panel with mouse bites, V-score not required
- E-test: Flying probe, 100% netlist coverage