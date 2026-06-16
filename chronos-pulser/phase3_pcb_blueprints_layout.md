# Phase 3: PCB Blueprints & Layout — Chronos Pulser

## 1. PCB Stackup Definition

### 1.1 4-Layer Stackup (Standard FR-4, TG 170)

```
┌─────────────────────────────────────────────────────────────────┐
│ Layer 1: TOP (Signal + Components)                              │
│   Copper: 1 oz (35 µm), finished thickness after plating: ~50 µm│
│   Solder mask: Green LPI, 25 µm                                 │
│   Surface finish: ENIG (Ni 3–5 µm, Au 0.05–0.1 µm)             │
│   Silkscreen: White epoxy ink                                    │
├─────────────────────────────────────────────────────────────────┤
│ Prepreg: 2116 (2 plies), εr ≈ 4.2 @ 1 GHz, thickness 0.12 mm   │
├─────────────────────────────────────────────────────────────────┤
│ Layer 2: GND (Solid Ground Plane)                               │
│   Copper: 1 oz (35 µm)                                          │
│   Continuous ground plane with minimal splits                   │
│   Analog and digital GND regions connected at single point      │
│   (under ADC, via a 2 mm wide bridge)                           │
├─────────────────────────────────────────────────────────────────┤
│ Core: FR-4, εr ≈ 4.5 @ 1 GHz, thickness 0.71 mm                │
├─────────────────────────────────────────────────────────────────┤
│ Layer 3: PWR (Power Plane — Split)                              │
│   Copper: 1 oz (35 µm)                                          │
│   Split into regions: +3.3V, +1.2V, +1.8V, +1.35V, +5V, -5V    │
│   Each region isolated by 0.5 mm gaps                           │
│   +3.3V region: ~60% of board area                              │
│   +1.2V region: ~15% (under FPGA core)                         │
│   +1.8V region: ~10% (under ADC)                               │
│   +1.35V region: ~8% (under DDR3)                              │
│   +5V/-5V regions: ~7% (under VGA)                             │
├─────────────────────────────────────────────────────────────────┤
│ Prepreg: 2116 (2 plies), εr ≈ 4.2 @ 1 GHz, thickness 0.12 mm   │
├─────────────────────────────────────────────────────────────────┤
│ Layer 4: BOTTOM (Signal + Components)                            │
│   Copper: 1 oz (35 µm)                                          │
│   Solder mask: Green LPI, 25 µm                                 │
│   Surface finish: ENIG                                          │
│   Silkscreen: White epoxy ink                                    │
└─────────────────────────────────────────────────────────────────┘

Total board thickness: 1.6 mm ± 0.1 mm
```

### 1.2 Impedance Calculations

Using the stackup above, controlled impedance traces are calculated with the following parameters:

| Signal Type | Target Z₀ | Layer | Trace Width | Differential Spacing | Notes |
|------------|-----------|-------|-------------|---------------------|-------|
| 50 Ω single-ended (microstrip) | 50 Ω ±5% | L1 (Top) | 0.30 mm | N/A | Reference: L2 GND plane |
| 50 Ω single-ended (stripline) | 50 Ω ±5% | L4 (Bottom) | 0.22 mm | N/A | Reference: L3 PWR + L2 GND |
| 100 Ω differential (microstrip) | 100 Ω ±5% | L1 (Top) | 0.18 mm | 0.20 mm gap | Reference: L2 GND plane |
| 90 Ω differential (microstrip) | 90 Ω ±5% | L1 (Top) | 0.22 mm | 0.18 mm gap | Reference: L2 GND plane |
| 100 Ω differential (stripline) | 100 Ω ±5% | L4 (Bottom) | 0.14 mm | 0.22 mm gap | Reference: L2 GND + L3 PWR |

Calculations verified with Saturn PCB Toolkit V8.0, assuming εr = 4.2 for prepreg and 4.5 for core.

### 1.3 Layer Assignment Strategy

| Signal Group | Assigned Layer | Rationale |
|-------------|---------------|-----------|
| ADC LVDS data (12 pairs) | L1 (Top) | Shortest path, no vias, direct FPGA-to-ADC |
| ADC LVDS clock | L1 (Top) | Matched to data group |
| USB 3.0 SS pairs | L1 (Top) | Minimum via count, direct to connector |
| USB 2.0 DP/DM | L1 (Top) | Same as SS pairs |
| DDR3 DQ/DQS | L1 (Top) | Shortest path FPGA-to-DDR3 |
| DDR3 ADDR/CTRL/CLK | L1 (Top) | Fly-by topology on top layer |
| FPGA ref clock | L1 (Top) | Direct from oscillator to FPGA |
| VGA output to ADC | L1 (Top) | Analog signal, keep on top |
| SMA output trace | L1 (Top) | 50 Ω microstrip, no vias |
| FT601 32-bit data bus | L1 + L4 | Split across layers for routing density |
| SPI, I²C, GPIO | L4 (Bottom) | Low-speed, non-critical |
| Power distribution | L3 (PWR plane) | Low impedance power delivery |
| JTAG | L4 (Bottom) | Debug interface, non-critical |

## 2. Board Outline & Mechanical

### 2.1 Dimensions
```
Overall: 100.00 mm × 70.00 mm
Corner radius: 3.00 mm (all four corners)
Board thickness: 1.6 mm

Mounting holes: 4× M2.5 (2.70 mm finished hole, 4.50 mm annular ring)
  - H1: (4.00, 4.00) from bottom-left
  - H2: (96.00, 4.00)
  - H3: (4.00, 66.00)
  - H4: (96.00, 66.00)
  All holes: non-plated, no copper within 1.0 mm radius (keepout)
```

### 2.2 Connector Placements
```
J1 (USB Type-C): Right edge, centered vertically
  - Center at (100.00, 35.00)
  - Connector body extends beyond board edge by 3.5 mm
  - Edge fingers: 12 pads on top layer, 12 on bottom

J2 (SMA, DUT output): Left edge, upper portion
  - Center at (0.00, 55.00)
  - Edge-launch, center pin on L1, ground legs on L1+L2+L3+L4
  - Keepout zone: 3 mm radius around center pin on all layers

J3 (SMA, trigger sync): Left edge, lower portion
  - Center at (0.00, 20.00)
  - Edge-launch, same as J2

J4 (JTAG header): Bottom edge, right portion
  - Center at (85.00, 0.00)
  - 10-pin 1.27 mm pitch, vertical SMD
```

### 2.3 Major Component Placements
```
U1 (FPGA, caBGA-381, 17×17 mm): Board center
  - Center at (50.00, 35.00)
  - Orientation: Pin A1 at top-left

U2 (FT601Q, QFN-76, 9×9 mm): Right of FPGA, near USB connector
  - Center at (78.00, 35.00)
  - USB SS pairs route directly east to J1

U3 (AD9230, LFCSP-56, 8×8 mm): Left of FPGA
  - Center at (22.00, 50.00)
  - Analog input from left (VGA), LVDS output east to FPGA

U4 (LMH6517, WQFN-32, 5×5 mm): Left of ADC
  - Center at (10.00, 50.00)
  - Input from coupler (left), output to ADC (right)

U5 (DDR3, FBGA-96, 9×14 mm): Above FPGA
  - Center at (50.00, 58.00)
  - DQ lanes route south to FPGA

T1 (TCD-10-1X+, SMT-6): Far left, near SMA
  - Center at (5.00, 55.00)
  - RF_IN to J2, RF_OUT to VGA, COUPLED to VGA

U6 (TPS63070, VQFN-15): Bottom-right power area
  - Center at (80.00, 10.00)

U7 (TPS65131, VQFN-24): Bottom-right power area
  - Center at (65.00, 10.00)

U8 (LP5907, DSBGA-4): Under FPGA (bottom layer)
  - Center at (50.00, 35.00) on L4

U9 (ADP7118, SOIC-8): Bottom-right power area
  - Center at (72.00, 10.00)

U10 (TPS51200, VSON-10): Near DDR3
  - Center at (60.00, 58.00)

U11 (SiT9365, SOT23-5): Near FPGA clock input
  - Center at (35.00, 25.00)

U12 (TMP117, WSON-6): Near FPGA, away from hot components
  - Center at (55.00, 15.00)

U13 (W25Q128, SOIC-8): Near FPGA config pins
  - Center at (30.00, 15.00)
```

### 2.4 Keepout Zones
```
Zone 1: SMA edge-launch keepout (J2, J3)
  - 3.0 mm radius from center pin, all layers
  - No copper, no components, no vias

Zone 2: USB-C connector keepout (J1)
  - 2.0 mm from connector body on all sides, all layers
  - Except: USB SS/DP/DM traces on L1

Zone 3: FPGA BGA fanout keepout
  - No components on L1 within 3 mm of U1 perimeter
  - Decoupling caps on L4 directly under BGA allowed

Zone 4: Crystal oscillator keepout (Y1)
  - 5.0 mm radius from Y1 center, all layers
  - No copper pour, no high-speed traces

Zone 5: Edge clearance
  - 0.5 mm from board edge, all layers
  - No copper, no components
```

## 3. High-Speed Routing Rules

### 3.1 ADC LVDS Interface (12 data pairs + 1 clock pair)

```
Topology: Point-to-point, L1 microstrip
Length: ~28 mm (FPGA to ADC)
Impedance: 100 Ω differential
Trace width: 0.18 mm, spacing: 0.20 mm

Length matching:
  - All 12 data pairs: ±1 mm (within group)
  - Clock pair: matched to data group average ±2 mm
  - Intra-pair skew: ±0.1 mm (P and N must be identical length)

Spacing rules:
  - Pair-to-pair spacing: ≥ 1.0 mm (5× trace width)
  - Pair-to-other-signal spacing: ≥ 1.5 mm
  - No crossing splits in reference plane

Via policy:
  - ZERO vias on LVDS pairs (route entirely on L1)
  - If unavoidable, use GND stitching vias adjacent to signal vias
  - Via impedance discontinuity: < 5% deviation

Termination:
  - 100 Ω internal FPGA differential termination (DIFF_TERM=ON)
  - ADC outputs: internal 100 Ω source termination
```

### 3.2 USB 3.0 SuperSpeed Pairs

```
Topology: Point-to-point, L1 microstrip
Length: ~22 mm (FT601 to USB-C connector)
Impedance: 90 Ω differential
Trace width: 0.22 mm, spacing: 0.18 mm

Length matching:
  - TX pair to RX pair: ±2 mm
  - Intra-pair skew: ±0.05 mm (extremely tight for 5 Gbps)

Spacing rules:
  - Pair-to-pair spacing: ≥ 1.5 mm
  - Pair-to-other-signal spacing: ≥ 2.0 mm
  - No other signals parallel to SS pairs for > 5 mm

ESD placement:
  - TPD4E05U06 (U14) placed within 5 mm of J1
  - ESD device between connector and FT601
  - Stub from main trace to ESD device: < 2 mm

Via policy:
  - ZERO vias on SS pairs
  - USB-C connector pads on L1 only
```

### 3.3 DDR3 Interface

```
DQ/DQS byte lanes (L1 microstrip):
  - Byte lane 0 (DQ[7:0], DQS0, DM0): 9 traces
  - Byte lane 1 (DQ[15:8], DQS1, DM1): 9 traces
  - Impedance: 50 Ω single-ended
  - Trace width: 0.30 mm
  - Length: ~25 mm (FPGA to DDR3)

DQ length matching (per byte lane):
  - All DQ bits to their DQS strobe: ±1 mm
  - DQS to DQS: ±5 mm (between byte lanes)
  - Intra-byte-lane DQ skew: ±0.5 mm

ADDR/CTRL/CLK (fly-by topology, L1 microstrip):
  - Impedance: 50 Ω single-ended (addr/ctrl), 100 Ω diff (clk)
  - Route from FPGA → DDR3 chip 1 → DDR3 chip 2 → VTT termination
  - (Single chip design, so FPGA → DDR3 → VTT resistors)
  - Length: ~30 mm FPGA to DDR3, +5 mm to VTT
  - ADDR/CTRL group matched: ±5 mm
  - CLK matched to ADDR/CTRL: ±2 mm

Spacing rules:
  - DQ-to-DQ spacing: ≥ 1.0 mm (3× trace width)
  - DQ-to-ADDR spacing: ≥ 2.0 mm
  - DQS-to-DQ spacing: ≥ 1.5 mm (DQS is aggressor)

VTT termination:
  - ADDR/CTRL lines: 39.2 Ω pull-up to VTT (0.675V) at end of fly-by
  - CLK: 100 Ω differential termination at end
  - VTT resistors placed within 5 mm of last DDR3 chip
  - VTT island: small copper pour on L1, connected to U10 output

Via policy:
  - DQ/DQS: ZERO vias (route entirely on L1)
  - ADDR/CTRL: ZERO vias preferred; if needed, GND stitching vias adjacent
```

### 3.4 FPGA Reference Clock (250 MHz LVDS)

```
Topology: Point-to-point, L1 microstrip
Length: ~15 mm (SiT9365 to FPGA)
Impedance: 100 Ω differential
Trace width: 0.18 mm, spacing: 0.20 mm

Length matching:
  - Intra-pair skew: ±0.1 mm

Spacing:
  - ≥ 1.5 mm from any other signal
  - No parallel runs with switching signals

Termination:
  - 100 Ω internal FPGA differential termination
  - SiT9365: LVDS output, no external termination needed

Via policy:
  - ZERO vias
```

### 3.5 Analog Signal Path (VGA → ADC)

```
Topology: Point-to-point, L1 microstrip
Length: ~8 mm (VGA output to ADC input)
Impedance: 50 Ω single-ended (×2 for differential pair)
Trace width: 0.30 mm

Critical rules:
  - These are ANALOG signals — treat as RF
  - No digital traces crossing or running parallel within 3 mm
  - No power plane splits underneath
  - Continuous GND reference on L2 (no splits in analog region)
  - Guard traces: GND traces on both sides, stitched to L2 GND every 5 mm
  - No vias on signal path

Length matching:
  - VGA_OUT_P to VGA_OUT_N: ±0.2 mm
```

### 3.6 SMA Output Trace (Pulse to DUT)

```
Topology: Point-to-point, L1 microstrip
Length: ~12 mm (directional coupler to SMA edge-launch)
Impedance: 50 Ω single-ended
Trace width: 0.30 mm

Critical rules:
  - This trace carries the 65 ps rise-time pulse
  - Bandwidth requirement: > 5 GHz (0.35/tr = 0.35/65ps ≈ 5.4 GHz)
  - FR-4 loss at 5 GHz: ~0.5 dB/cm → ~0.6 dB total (acceptable)
  - No bends sharper than 45° (use mitered corners)
  - No stubs, no vias, no test points on this trace
  - Taper from coupler pad width to 0.30 mm over 1 mm distance
  - SMA center pin pad: 1.2 mm diameter, surrounded by GND clearance
```

## 4. Via Strategy

### 4.1 Via Types Used

| Via Type | Hole | Pad | Anti-pad | Use Case |
|----------|------|-----|----------|----------|
| Standard signal via | 0.20 mm | 0.45 mm | 0.65 mm | General signal routing |
| Power via | 0.30 mm | 0.60 mm | 0.80 mm | Power plane connections |
| Thermal via | 0.30 mm | 0.60 mm | 0.80 mm | Under thermal pads |
| GND stitching via | 0.20 mm | 0.45 mm | 0.65 mm | Near high-speed transitions |
| BGA fanout via | 0.15 mm | 0.35 mm (L1), 0.45 mm (L4) | 0.55 mm | FPGA BGA escape routing |

### 4.2 BGA Fanout Strategy (FPGA, caBGA-381, 1.0 mm pitch)

```
Row 1 (outermost): Route directly on L1, no vias needed
Row 2: Dog-bone fanout with via to L4
Row 3: Dog-bone fanout with via to L4 (staggered from row 2)
Row 4+: Via-in-pad (VIPPO) for inner rows, filled and capped

VIPPO specification:
  - Via hole: 0.15 mm
  - Via fill: conductive epoxy (DuPont CB100) or copper plating
  - Via cap: copper plated flat to pad surface
  - Pad diameter: 0.45 mm (standard BGA pad)
  - This allows routing on L4 from inner BGA balls

DDR3 BGA (FBGA-96, 0.8 mm pitch):
  - Outer 2 rows: dog-bone fanout on L1
  - Inner rows: via-in-pad to L4
  - Via hole: 0.15 mm, pad: 0.40 mm
```

### 4.3 GND Stitching Via Rules

```
Placement rules for stitching vias:
  - Near every high-speed signal via: 1 GND via within 1 mm
  - Along differential pair routes: every 10 mm
  - At layer transitions: 2 GND vias within 2 mm of signal vias
  - Along board edges: every 5 mm (edge shielding)
  - Around RF/analog regions: every 3 mm (faraday cage effect)
  - Under BGA packages: fill available space with GND vias

Total estimated stitching vias: ~400
```

### 4.4 Thermal Via Arrays

```
FPGA thermal pad (caBGA-381, center pad):
  - 5×5 array of 0.30 mm thermal vias
  - 1.0 mm pitch
  - Connect L1 pad → L2 GND → L3 GND copper → L4 GND copper
  - Via fill: not required (solder mask defined pad on L1)

ADC thermal pad (LFCSP-56, exposed pad):
  - 3×3 array of 0.30 mm thermal vias
  - 1.0 mm pitch
  - Connect to L2 GND plane (analog ground region)

VGA thermal pad (WQFN-32):
  - 2×2 array of 0.30 mm thermal vias
  - Connect to L2 GND plane

FT601Q thermal pad (QFN-76):
  - 3×3 array of 0.30 mm thermal vias
  - Connect to L2 GND plane

TPS63070, TPS65131 thermal pads:
  - 2×2 array each, 0.30 mm vias
```

## 5. Power Distribution Network (PDN)

### 5.1 Plane Splits (Layer 3 — PWR)

```
Plane region allocation on L3:

┌──────────────────────────────────────────────────┐
│  +1.35V (DDR3)    │   +1.2V (FPGA core)          │
│  15 × 20 mm       │   25 × 30 mm                 │
│  Under U5         │   Under U1 center             │
├───────────────────┼───────────────────────────────┤
│  +1.8V (ADC)      │                               │
│  15 × 15 mm       │   +3.3V (Main I/O)            │
│  Under U3         │   60 × 45 mm                  │
├───────────────────┤   Covers most of board         │
│  +5V (VGA)        │                               │
│  10 × 10 mm       │                               │
│  Under U4         │                               │
├───────────────────┤                               │
│  -5V (VGA)        │                               │
│  10 × 10 mm       │                               │
│  Under U4         │                               │
└──────────────────────────────────────────────────┘

Split gaps: 0.5 mm minimum
No traces crossing splits on L1 or L4 without stitching capacitors
Stitching capacitors: 100 nF placed across splits where signals cross
```

### 5.2 Decoupling Capacitor Placement

```
FPGA VCCINT (1.2V):
  - 12× 100 nF 0402: on L4, directly under BGA balls, within 2 mm
  - 2× 10 µF 0805: on L4, at FPGA perimeter
  - 1× 47 µF 1210: on L4, near U8 output

FPGA VCCIO banks:
  - 4× 100 nF per bank: on L4, under BGA perimeter
  - 1× 10 µF per bank: on L4, at bank edge

ADC AVDD (1.8V analog):
  - 100 nF + 10 nF + 1.0 µF: on L1, within 3 mm of pin 23
  - 10 µF bulk: on L1, within 10 mm

VGA V+/V-:
  - 100 nF per supply pin: on L1, within 1 mm
  - 1.0 µF + 10 µF: on L1, within 5 mm

DDR3 VDD:
  - 4× 100 nF: on L1, at BGA corners
  - 2× 1.0 µF: on L1, near VDD balls
  - 1× 10 µF: on L1, near U10 output

Bulk capacitance (shared):
  - 2× 47 µF tantalum on +3.3V rail (near U6 output)
  - 1× 47 µF tantalum on +1.2V rail (near U8 output)
  - 1× 22 µF ceramic on +1.8V rail (near U9 output)
```

### 5.3 PDN Target Impedance

| Rail | Target Z (DC–100 MHz) | Target Z (100 MHz–1 GHz) |
|------|----------------------|--------------------------|
| +3.3V (FPGA I/O) | < 50 mΩ | < 200 mΩ |
| +1.2V (FPGA core) | < 30 mΩ | < 150 mΩ |
| +1.8V (ADC) | < 20 mΩ | < 100 mΩ |
| +1.35V (DDR3) | < 40 mΩ | < 180 mΩ |
| +5V (VGA) | < 100 mΩ | < 300 mΩ |
| -5V (VGA) | < 100 mΩ | < 300 mΩ |

Achieved through:
- Low-inductance plane capacitance (L2–L3 cavity: ~2 nF for 3.3V region)
- Proper decoupling capacitor placement and values
- Multiple vias from capacitor pads to planes (2 vias per pad minimum)

## 6. Thermal Management

### 6.1 Power Dissipation by Component

| Component | Power (W) | Package | θJA (natural) | Max Tj | Strategy |
|-----------|-----------|---------|---------------|--------|----------|
| FPGA (U1) | 1.78 | caBGA-381 | 18 °C/W | 100°C | Thermal vias + enclosure contact |
| FT601Q (U2) | 0.60 | QFN-76 | 22 °C/W | 85°C | Thermal vias + copper pour |
| ADC (U3) | 0.85 | LFCSP-56 | 25 °C/W | 85°C | Thermal vias |
| VGA (U4) | 0.60 | WQFN-32 | 30 °C/W | 125°C | Thermal vias |
| DDR3 (U5) | 0.41 | FBGA-96 | 20 °C/W | 95°C | Thermal vias |
| TPS63070 (U6) | 0.30 | VQFN-15 | 35 °C/W | 125°C | Copper pour + vias |
| TPS65131 (U7) | 0.25 | VQFN-24 | 32 °C/W | 125°C | Copper pour + vias |

### 6.2 Thermal Simulation Estimates

```
Ambient temperature: 25°C (lab environment)
Worst-case ambient: 45°C (field use)

FPGA junction temperature:
  Tj = 45°C + 1.78W × 18°C/W = 77°C (well within 100°C limit)

ADC junction temperature:
  Tj = 45°C + 0.85W × 25°C/W = 66°C (within 85°C limit)

Board average temperature rise: ~15°C above ambient
```

### 6.3 Enclosure Thermal Interface

```
Aluminum enclosure (120×80×30 mm):
  - Internal height: 28 mm
  - PCB mounted on 5 mm standoffs
  - Thermal gap pad (Bergquist Gap Pad 1500, 2 mm thick, 1.5 W/m·K)
    between FPGA top and enclosure lid
  - Gap pad area: 17×17 mm (covers entire FPGA package)
  - Thermal resistance pad-to-enclosure: ~4.5 °C/W
  - Enclosure-to-ambient: ~15 °C/W (natural convection, 120×80 mm surface)

With thermal pad:
  FPGA Tj = 45°C + 1.78W × (18 || (4.5+15)) = 45°C + 1.78W × 9.4°C/W = 62°C
  (Significant improvement over no-pad scenario)
```

### 6.4 Copper Pour Strategy for Heat Spreading

```
L1 (Top):
  - GND copper pours in all unused areas
  - Connected to L2 GND with stitching vias every 5 mm
  - Under high-power components: solid GND copper (no splits)

L2 (GND):
  - Solid plane, maximum copper coverage
  - Acts as primary heat spreader

L3 (PWR):
  - Split planes, but maximize copper in each region
  - No isolated copper islands (all connected to their respective sources)

L4 (Bottom):
  - GND copper pours in unused areas
  - Connected to L2 GND with stitching vias
```

## 7. Clearance and Creepage

### 7.1 Voltage-Based Clearances (IPC-2221B)

| Voltage Difference | Minimum Clearance | Applied To |
|-------------------|-------------------|------------|
| 0–15V | 0.13 mm | Most signals |
| 15–30V (+25V SRD bias) | 0.25 mm | SRD bias trace to GND |
| 30–50V | 0.50 mm | Not applicable |
| USB VBUS (5V) to GND | 0.13 mm | Power input area |

### 7.2 Special Clearances

```
SMA connector center pin to GND:
  - 1.5 mm clearance on L1 (RF isolation)
  - 0.5 mm clearance on inner layers

USB-C VBUS to GND:
  - 0.25 mm (standard for 5V power)

Crystal oscillator (Y1) to any signal:
  - 2.0 mm clearance (EMI prevention)

Edge clearance (all layers):
  - 0.50 mm from board edge to any copper
```

## 8. Manufacturing Notes

### 8.1 PCB Fabrication Requirements

```
Manufacturer class: IPC Class 2 (modified)
Minimum trace/space: 0.10 mm / 0.10 mm (4 mil / 4 mil)
Minimum hole size: 0.15 mm (6 mil) — mechanical drill
Aspect ratio: 10:1 max (0.15 mm hole in 1.6 mm board = 10.7:1 — borderline)
  → Use 0.20 mm minimum hole for reliability, or specify laser drilling for 0.15 mm

Controlled impedance: Yes, with TDR coupon on panel
  - Coupon 1: 50 Ω microstrip (L1)
  - Coupon 2: 100 Ω differential microstrip (L1)
  - Coupon 3: 90 Ω differential microstrip (L1)

Solder mask: Green LPI, both sides
  - Solder mask dams: 0.10 mm minimum (between pads)
  - Solder mask expansion: 0.05 mm (from pad edge)

Silkscreen: White, both sides
  - Minimum line width: 0.15 mm
  - Minimum text height: 1.0 mm

Surface finish: ENIG (electroless nickel immersion gold)
  - Ni: 3–5 µm, Au: 0.05–0.1 µm
  - Reason: Flat surface for BGA packages, good for RF

Via treatment:
  - All vias: tented (solder mask covered) on both sides
  - Except: thermal vias under exposed pads — leave open for solder wicking
  - VIPPO vias: filled and capped (conductive fill, plated flat)
```

### 8.2 Panelization

```
Single board per panel for prototype run
Panel size: 120 × 90 mm (10 mm border each side)
Tooling holes: 4× 3.0 mm at panel corners
Fiducials: 3× 1.0 mm global fiducials (L1 and L4 copper, solder mask opening)
  - Fid1: (5.0, 5.0)
  - Fid2: (115.0, 5.0)
  - Fid3: (5.0, 85.0)
Local fiducials: 2× near FPGA (for BGA placement accuracy)
Break-away tabs: 5 mm wide, mouse-bite perforations
```

### 8.3 Test Points

```
Essential test points (1.0 mm diameter pads, labeled on silkscreen):
  - TP1: +3.3V (near U6 output)
  - TP2: +1.2V (near U8 output)
  - TP3: +1.8V (near U9 output)
  - TP4: +1.35V (near U10 output)
  - TP5: +5V (near U7 output)
  - TP6: -5V (near U7 output)
  - TP7: +25V_SRD (near D1)
  - TP8: GND (multiple locations)
  - TP9: PULSE_TRIG (near Q1)
  - TP10: ADC_CLK_P (near U3, high-impedance probe point)
  - TP11: I2C_SCL (near U12)
  - TP12: I2C_SDA (near U12)

All test points on L4 (bottom layer) for easy access
```

## 9. Signal Integrity Simulation Notes

### 9.1 Critical Nets Requiring SI Simulation

```
1. ADC LVDS data pairs: Verify eye diagram at 250 MHz DDR (500 Mbps per lane)
   - Target: Eye opening > 80% UI, jitter < 50 ps pp
   - Tool: HyperLynx or OpenEMS

2. USB 3.0 SS pairs: Verify eye at 5 Gbps
   - Target: Compliant with USB 3.0 eye mask
   - Tool: HyperLynx

3. DDR3 DQ/DQS: Verify setup/hold margins
   - Target: > 50 ps setup, > 50 ps hold at 800 MT/s
   - Tool: HyperLynx or DDR3-specific IBIS simulation

4. SMA output pulse: Verify rise time preservation
   - Target: < 10 ps additional rise time degradation
   - Tool: OpenEMS (full-wave EM simulation)

5. FPGA ref clock: Verify jitter and amplitude
   - Target: < 5 ps added jitter, > 200 mVpp differential at FPGA ball
```

### 9.2 IBIS Models Required

```
- LFE5U-45F: Lattice ECP5 IBIS (available from Lattice)
- AD9230-250: Analog Devices IBIS (available from ADI)
- FT601Q: FTDI IBIS (request from FTDI)
- MT41K128M16JT: Micron DDR3 IBIS (available from Micron)
- LMH6517: TI IBIS (available from TI)
```

---

*End of Phase 3 — PCB Blueprints & Layout*
