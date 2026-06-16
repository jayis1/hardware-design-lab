# Phase 3: PCB Blueprints & Layout — Nebula Matrix
## Professional LED Matrix Display Engine

---

## 1. Board Outline & Mechanical

### 1.1 Form Factor

| Parameter              | Value                                    |
|------------------------|------------------------------------------|
| Board Shape            | Rectangular, 170.00 mm × 170.00 mm       |
| Corner Radius          | 3.00 mm (all four corners)               |
| PCB Thickness          | 1.60 mm ±0.10 mm                         |
| Board Material         | FR-4, Tg ≥ 170°C (IT-180A or equivalent) |
| Copper Weight (outer)  | 1 oz (35 µm) finished                    |
| Copper Weight (inner)  | 0.5 oz (17.5 µm) finished                |
| Surface Finish         | ENIG (Electroless Nickel Immersion Gold) |
| Solder Mask            | Green LPI, both sides                    |
| Silkscreen             | White, top side only (minimal bottom)    |

### 1.2 Mounting Holes

| Hole | X (mm) | Y (mm) | Diameter | Pad Diameter | Plated |
|------|--------|--------|----------|--------------|--------|
| MH1  | 5.00   | 5.00   | 3.20 mm  | 6.00 mm      | Yes    |
| MH2  | 165.00 | 5.00   | 3.20 mm  | 6.00 mm      | Yes    |
| MH3  | 5.00   | 165.00 | 3.20 mm  | 6.00 mm      | Yes    |
| MH4  | 165.00 | 165.00 | 3.20 mm  | 6.00 mm      | Yes    |

All mounting holes tied to GND via 1MΩ + 0.1µF parallel network for ESD path.

### 1.3 Keepout Zones

| Zone | Region | Purpose |
|------|--------|---------|
| K1 | 0-3mm from board edge, all layers | Manufacturing clearance |
| K2 | Under BGA packages (U1, U8, U9), top layer | No vias except thermal |
| K3 | Under crystal oscillators (Y1, Y2), all layers | No copper pour |
| K4 | HDMI TMDS traces, 5mm from edge | No other signals crossing |
| K5 | DDR3 interface region, 20mm × 40mm per channel | No unrelated signals |

---

## 2. PCB Stackup — 6-Layer

### 2.1 Layer Assignment

```
Layer 1 (TOP)          — Signal + Component Placement
  Material: 0.5 oz Cu + plating → 1 oz finished
  Solder mask: Green LPI
  Silkscreen: White

Prepreg: 2× 1080 (0.15 mm total)
  εr = 4.05 @ 1 GHz

Layer 2 (GND1)         — Solid Ground Plane
  Material: 0.5 oz Cu (1 oz finished)
  No splits, continuous reference for L1 signals

Core: 0.20 mm FR-4
  εr = 4.30 @ 1 GHz

Layer 3 (SIG1)         — Inner Signal Routing
  Material: 0.5 oz Cu
  High-speed parallel buses, DDR address/command

Prepreg: 2× 2116 (0.24 mm total)
  εr = 4.15 @ 1 GHz

Layer 4 (PWR1)         — Split Power Plane
  Material: 0.5 oz Cu
  +3V3, +1V8, +1V35, +1V0, +1V2, +5V0 islands

Core: 0.20 mm FR-4
  εr = 4.30 @ 1 GHz

Layer 5 (GND2)         — Solid Ground Plane
  Material: 0.5 oz Cu
  Continuous reference for L6 signals

Prepreg: 2× 1080 (0.15 mm total)
  εr = 4.05 @ 1 GHz

Layer 6 (BOTTOM)       — Signal + Decoupling Caps
  Material: 0.5 oz Cu + plating → 1 oz finished
  Solder mask: Green LPI
  Minimal silkscreen (pin 1 markers only)
```

### 2.2 Total Thickness Calculation

| Layer | Material | Thickness (mm) |
|-------|----------|----------------|
| L1 copper | 0.035 mm | 0.035 |
| Prepreg 1080×2 | 0.150 mm | 0.150 |
| L2 copper | 0.035 mm | 0.035 |
| Core FR-4 | 0.200 mm | 0.200 |
| L3 copper | 0.0175 mm | 0.018 |
| Prepreg 2116×2 | 0.240 mm | 0.240 |
| L4 copper | 0.0175 mm | 0.018 |
| Core FR-4 | 0.200 mm | 0.200 |
| L5 copper | 0.0175 mm | 0.018 |
| Prepreg 1080×2 | 0.150 mm | 0.150 |
| L6 copper | 0.035 mm | 0.035 |
| **Total** | | **~1.10 mm** |

Note: Additional thickness from solder mask and silkscreen brings total to ~1.20 mm.
For 1.60 mm final, add 0.40 mm of additional core/prepreg. Adjust prepreg thicknesses
or add a 0.40 mm FR-4 core between L3 and L4.

### 2.3 Impedance Calculations

Using Polar SI8000 or equivalent solver with above stackup:

| Target Impedance | Trace Width (L1/L6) | Trace Width (L3) | Differential Gap |
|------------------|---------------------|-------------------|------------------|
| 50Ω single-ended | 0.15 mm (6 mil)     | 0.12 mm (4.7 mil) | —                |
| 40Ω single-ended | 0.22 mm (8.7 mil)   | 0.18 mm (7.1 mil) | —                |
| 100Ω differential| 0.12 mm (4.7 mil)   | 0.10 mm (3.9 mil) | 0.15 mm (6 mil)  |
| 90Ω differential | 0.14 mm (5.5 mil)   | 0.12 mm (4.7 mil) | 0.15 mm (6 mil)  |

---

## 3. High-Speed Routing Rules

### 3.1 DDR3L Interface (1066 MT/s, 533 MHz Clock)

#### 3.1.1 General Rules

| Parameter | Requirement | Notes |
|-----------|-------------|-------|
| Topology | Fly-by (address/command/clock) | Daisy-chain from FPGA to U8 then U9 |
| Data group topology | Point-to-point | Each DQ group direct FPGA→DRAM |
| Reference plane | Solid GND (L2 for L1 traces) | No splits under DDR region |
| Max via count | 2 per net (FPGA BGA escape + DRAM BGA) | Minimize stubs |
| Via type | Through-hole, 0.20mm drill / 0.45mm pad | Standard via |

#### 3.1.2 Length Matching — Address/Command/Clock Group

Fly-by chain: FPGA → U8 (Ch A) → U9 (Ch B)

| Signal Group | Match To | Tolerance | Max Length |
|--------------|----------|-----------|------------|
| CK_P, CK_N | Each other | ±0.5 mm (0.5 ps) | — |
| All Address/Command | CK_P | ±5 mm (25 ps) | 65 mm max |
| CS_N, ODT, CKE | CK_P | ±5 mm (25 ps) | 65 mm max |

#### 3.1.3 Length Matching — Data Groups (per byte lane)

| Signal Group | Match To | Tolerance | Max Length |
|--------------|----------|-----------|------------|
| DQ[7:0], DM0 | DQS0_P | ±2 mm (10 ps) | 55 mm max |
| DQS0_P, DQS0_N | Each other | ±0.5 mm (0.5 ps) | 55 mm max |
| DQ[15:8], DM1 | DQS1_P | ±2 mm (10 ps) | 55 mm max |
| DQS1_P, DQS1_N | Each other | ±0.5 mm (0.5 ps) | 55 mm max |

#### 3.1.4 DDR3 Routing Constraints

| Constraint | Value |
|------------|-------|
| Trace spacing (DQ within byte lane) | 2× trace width (min) |
| Trace spacing (DQ to other signals) | 3× trace width (min) |
| Address/Command spacing | 2× trace width (min) |
| Clock-to-other-signal spacing | 4× trace width (min) |
| Max via stub length | < 0.25 mm (back-drill not needed for 1.6mm board) |
| Breakout region (under BGA) | 4-mil trace/space allowed for first 5mm |

### 3.2 HDMI TMDS Routing

| Parameter | Requirement |
|-----------|-------------|
| Differential impedance | 100Ω ±10% |
| Intra-pair skew | < 5 ps (0.5 mm) |
| Inter-pair skew | < 100 ps (10 mm) |
| Max length | 50 mm (connector to ADV7611) |
| Reference plane | Solid GND, no splits |
| ESD protection | Nexperia IP4283CZ10 (TVS array) near connector |

### 3.3 USB 2.0 Routing

| Parameter | Requirement |
|-----------|-------------|
| Differential impedance | 90Ω ±10% |
| Intra-pair skew | < 5 ps |
| Max length | 40 mm (connector to USB3320C) |
| ESD protection | Nexperia PUSB3FR4 (TVS array) near connector |

### 3.4 Ethernet MDI Routing

| Parameter | Requirement |
|-----------|-------------|
| Differential impedance | 100Ω ±10% |
| Intra-pair skew | < 5 ps |
| Max length | 30 mm (RJ45 magnetics to KSZ9031) |
| Bob Smith termination | 75Ω + 1000pF/2kV per pair to GND via capacitor |

### 3.5 HUB75E Output Routing

| Parameter | Requirement |
|-----------|-------------|
| Single-ended impedance | 50Ω ±10% |
| Max length (FPGA to level shifter) | 40 mm |
| Max length (level shifter to connector) | 25 mm |
| Spacing between ports | 1.0 mm minimum |
| Series termination | 22Ω (data/addr), 10Ω (CLK/LAT/OE) placed at FPGA end |

### 3.6 SPI Bus (50 MHz)

| Parameter | Requirement |
|-----------|-------------|
| Impedance | 50Ω single-ended |
| Length matching (SCK/MOSI/MISO/CS) | ±5 mm |
| Max length | 60 mm |
| Series termination | 22Ω at driver end |

---

## 4. Via Strategy

### 4.1 Via Types

| Type | Drill | Pad | Antipad (L2/L5) | Antipad (L3/L4) | Use |
|------|-------|-----|-----------------|-----------------|-----|
| Standard signal | 0.20 mm | 0.45 mm | 0.65 mm | 0.65 mm | General routing |
| Power via | 0.30 mm | 0.60 mm | 0.80 mm | 0.80 mm | Power distribution |
| Thermal via | 0.30 mm | 0.60 mm | 0.80 mm | 0.80 mm | Under QFN/BGA thermal pads |
| BGA escape via | 0.20 mm | 0.45 mm | 0.60 mm | 0.60 mm | Under FPGA/DRAM BGA |
| Mounting hole via | 3.20 mm | 6.00 mm | — | — | Board mounting |

### 4.2 BGA Escape Strategy — XC7A100T (484-BGA, 1.0mm pitch)

The XC7A100T uses a 1.0mm pitch BGA. Escape strategy:

- **Rows A-Y (outer 4 rows)**: Dog-bone fanout, via outside BGA footprint
- **Inner rows**: Via-in-pad (VIP) for VCCINT/GND balls only
- **Signal balls in inner region**: Dog-bone to adjacent channel, then via
- **DDR3 banks (34,35)**: Direct north-south escape to DRAM chips placed above FPGA

Via count estimate: ~300 signal vias, ~80 power/ground vias.

### 4.3 BGA Escape Strategy — DDR3L (96-BGA, 0.8mm pitch)

- 0.8mm pitch allows dog-bone fanout with 0.20mm drill / 0.45mm pad
- All signal balls escape to outer rows
- Center thermal pad: 4× thermal vias to GND plane

### 4.4 Via Stitching

| Region | Stitch Pitch | Purpose |
|--------|--------------|---------|
| Board perimeter | 5 mm | EMI fence |
| DDR3 region boundary | 2 mm | Isolation from other signals |
| FPGA perimeter | 2 mm | Return path continuity |
| HDMI TMDS path edges | 2 mm | Guard trace stitching |
| Power plane transitions | At every transition | Low-impedance return |

---

## 5. Thermal Management

### 5.1 Power Dissipation Estimate

| Component | Power (W) | Notes |
|-----------|-----------|-------|
| XC7A100T (FPGA) | 6.0 W | Typical at 60% utilization, 150 MHz |
| STM32H743 | 0.8 W | 480 MHz, all peripherals active |
| DDR3L ×2 | 1.2 W | 0.6W each at 1066 MT/s |
| ADV7611 | 0.9 W | HDMI Rx active |
| KSZ9031RNX | 0.6 W | GbE link active |
| USB3320C | 0.3 W | USB active |
| SN74LVCH16T245 ×16 | 1.6 W | 0.1W each, switching at 33 MHz |
| Si5351A | 0.2 W | All outputs active |
| TPS65218 (losses) | 1.5 W | ~85% efficiency at load |
| Other (flash, sensors) | 0.3 W | |
| **Total** | **~13.4 W** | Typical operating |

Max (all outputs at full utilization, worst-case FPGA): ~18 W.

### 5.2 Thermal Solution

| Element | Specification |
|---------|---------------|
| FPGA heatsink | 30×30×10mm aluminum, adhesive (AAVID 374424B00035) |
| STM32H7 | No heatsink required (0.8W in LQFP-100) |
| Forced airflow | 2× Sunon MF40101V1 40mm fans, 5.5 CFM each |
| Fan control | PWM from STM32 TIM1, PID based on TMP117 readings |
| Fan placement | Side edge, blowing across board (left to right) |
| Thermal vias under FPGA | 36× 0.30mm vias in thermal pad, to bottom copper pour |
| Thermal vias under DRAM | 4× per chip, to bottom copper pour |

### 5.3 Temperature Monitoring

| Sensor | Location | Monitors |
|--------|----------|----------|
| TMP117 #1 | Near FPGA (5mm from edge) | FPGA case temp |
| TMP117 #2 | Near DDR3L Ch A | DRAM temp |
| TMP117 #3 | Near TPS65218 | PMIC temp |
| TMP117 #4 | Board center (ambient) | Internal ambient |

Fan PID setpoints:
- 40°C: Fans off
- 50°C: Fans 30%
- 60°C: Fans 60%
- 70°C: Fans 100%
- 80°C: Thermal warning via Ethernet
- 85°C: Thermal shutdown

---

## 6. Clearance & Creepage

### 6.1 Voltage Domains & Spacing

Per IPC-2221B, pollution degree 2, material group IIIa (FR-4):

| Voltage | Clearance (mm) | Creepage (mm) | Notes |
|---------|----------------|---------------|-------|
| 12V to GND | 0.4 | 1.0 | DC input |
| 12V to 5V | 0.4 | 1.0 | DCDC isolated |
| 12V to 3.3V | 0.4 | 1.0 | |
| 5V to 3.3V | 0.2 | 0.5 | |
| 3.3V to 1.8V | 0.2 | 0.5 | |
| 1.8V to 1.35V | 0.2 | 0.5 | |
| 1.35V to 1.0V | 0.2 | 0.5 | |
| HUB75E output to GND | 0.4 | 1.0 | External connector |
| HDMI connector to GND | 0.4 | 1.0 | External connector |
| RJ45 to GND | 0.4 | 1.0 | External connector, isolated magnetics |
| USB-C to GND | 0.4 | 1.0 | External connector |

### 6.2 Power Plane Splits

Layer 4 (PWR1) split into islands:

```
┌─────────────────────────────────────────────────────────┐
│  +5V0 Island (top-left, ~30×40mm)                       │
│  ┌──────────────────────┐                               │
│  │ +3V3 Island           │  +1V35 Island (right, DDR)   │
│  │ (center, ~80×100mm)   │  ┌──────────────────┐        │
│  │                       │  │                  │        │
│  │  ┌─────────┐          │  │  DDR Ch A        │        │
│  │  │ +1V0    │ +1V8     │  │  (top-right)     │        │
│  │  │ Island  │ Island   │  │                  │        │
│  │  │ (FPGA   │ (small)  │  ├──────────────────┤        │
│  │  │ core)   │          │  │  DDR Ch B        │        │
│  │  └─────────┘          │  │  (bottom-right)  │        │
│  │                       │  │                  │        │
│  │  ┌────┐               │  └──────────────────┘        │
│  │  │+1V2│               │                               │
│  │  │BRAM│               │                               │
│  │  └────┘               │                               │
│  └──────────────────────┘                               │
│                                                         │
│  +12V_IN (bottom-left, small island near DC jack)       │
└─────────────────────────────────────────────────────────┘
```

Split gaps: 1.0 mm minimum between islands.
Stitching capacitors: 0.1µF + 10µF at every signal crossing between power islands.

---

## 7. Component Placement

### 7.1 Top Side Placement (Primary)

```
┌────────────────────────────────────────────────────────────┐
│  J5  J6  J7  J8  J9  J10 J11 J12 J13 J14 J15 J16 J17 J18 │
│  [HUB75E Ports 0-7, top row]    [HUB75E Ports 8-15]       │
│                                                            │
│  U10 U11 U12 U13 U14 U15 U16 U17 U18 U19 U20 U21 U22 U23  │
│  [Level Shifters, aligned behind HUB75E connectors]        │
│                                                            │
│  ┌──────────────────────┐  ┌──────────┐  ┌──────────┐     │
│  │                      │  │ U8       │  │ U9       │     │
│  │   U1 (XC7A100T)      │  │ DDR3L A  │  │ DDR3L B  │     │
│  │   FPGA, center        │  │ (96-BGA) │  │ (96-BGA) │     │
│  │   484-BGA             │  │          │  │          │     │
│  │                      │  └──────────┘  └──────────┘     │
│  └──────────────────────┘                                  │
│                                                            │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐              │
│  │ U7     │ │ U2     │ │ U5     │ │ U6     │              │
│  │ADV7611 │ │STM32H7 │ │KSZ9031 │ │USB3320C│              │
│  │HDMI Rx │ │LQFP100 │ │GbE PHY │ │ULPI PHY│              │
│  └────────┘ └────────┘ └────────┘ └────────┘              │
│                                                            │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐             │
│  │U3    │ │U4    │ │U26   │ │U28   │ │U27×4 │             │
│  │PMIC  │ │Clock │ │Flash │ │INA219│ │TMP117│             │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘             │
│                                                            │
│  J21(HDMI)  J1(RJ45)  J2(USB-C)  J3(microSD)  J4(DC Jack)│
│  [All external I/O along bottom edge]                      │
│                                                            │
│  FAN1 (left edge)                          FAN2 (right edge)│
└────────────────────────────────────────────────────────────┘
```

### 7.2 Bottom Side Placement

- Decoupling capacitors: Grid under FPGA BGA, perimeter around DRAM BGAs
- Series termination resistors: Near FPGA pins for HUB75E outputs
- Pull-up/pull-down resistors: Near respective ICs
- Test points: On all power rails, key signals
- JTAG/SWD headers: Bottom edge for easy access

### 7.3 Critical Placement Constraints

| Constraint | Value | Reason |
|------------|-------|--------|
| FPGA to DDR3L Ch A | < 25 mm | Signal integrity at 1066 MT/s |
| FPGA to DDR3L Ch B | < 25 mm | Signal integrity at 1066 MT/s |
| FPGA to level shifters | < 40 mm | HUB75E timing |
| Level shifters to HUB75E connectors | < 25 mm | Minimize 5V trace length |
| ADV7611 to FPGA | < 30 mm | 24-bit parallel bus at 165 MHz |
| STM32H7 to FPGA | < 50 mm | SPI at 50 MHz |
| STM32H7 to KSZ9031 | < 30 mm | RMII at 50 MHz |
| STM32H7 to USB3320C | < 30 mm | ULPI at 60 MHz |
| Crystal to IC | < 10 mm | Oscillator stability |
| Decoupling cap to IC pin | < 2 mm | Minimize loop inductance |

---

## 8. Power Distribution Network (PDN)

### 8.1 DC Input Protection

```
J4 (DC Jack, 12V)
  → F1 (Polyfuse, 3A hold, 6A trip, Bourns MF-R300)
  → D2 (SMBJ12A, 12V TVS, unidirectional)
  → L1 (Ferrite bead, 600Ω @ 100MHz, Murata BLM31PG601SN1)
  → C1,C2 (100µF/25V aluminum electrolytic + 10µF/25V MLCC)
  → Net: +12V_IN
```

### 8.2 TPS65218 PMIC Configuration

| Regulator | Output | Max Current | Inductor | Output Caps |
|-----------|--------|-------------|----------|-------------|
| DCDC1 | 5.0V | 3A | 2.2µH (Coilcraft XAL4030-222ME) | 2× 22µF MLCC + 1× 100µF tantalum |
| DCDC2 | 3.3V | 3A | 2.2µH (XAL4030-222ME) | 2× 22µF MLCC + 1× 100µF tantalum |
| DCDC3 | 1.8V | 2A | 2.2µH (XAL4020-222ME) | 2× 22µF MLCC |
| DCDC4 | 1.35V | 3A | 1.5µH (XAL4030-152ME) | 2× 22µF MLCC + 1× 100µF tantalum |
| LDO1 | 1.0V | 0.5A | — | 1× 10µF MLCC |
| LDO2 | 1.2V | 0.5A | — | 1× 10µF MLCC |
| LDO3 | 3.3V | 0.3A | — | 1× 10µF MLCC |

### 8.3 DDR3L VTT Termination Supply

```
+1V35 → U30 (TPS51200DGQR, DDR VTT regulator)
  → VTT = 0.675V (VDDQ/2)
  → VTTREF = 0.675V (buffered reference)
  → Sink/source capability: 3A
  → Output caps: 2× 10µF MLCC + 1× 47µF tantalum
  → VTT distributed to fly-by termination resistors
```

### 8.4 PDN Target Impedance

| Rail | Voltage | I_max | Target Z (DC-1MHz) | Target Z (1-100MHz) |
|------|---------|-------|---------------------|----------------------|
| +1V0 (VCCINT) | 1.0V | 6A | < 5 mΩ | < 50 mΩ |
| +1V35 (DDR) | 1.35V | 3A | < 10 mΩ | < 100 mΩ |
| +3V3 (I/O) | 3.3V | 3A | < 20 mΩ | < 200 mΩ |
| +1V8 (Aux) | 1.8V | 2A | < 15 mΩ | < 150 mΩ |
| +5V0 | 5.0V | 3A | < 30 mΩ | < 300 mΩ |

---

## 9. Signal Integrity Rules

### 9.1 Crosstalk Mitigation

| Rule | Value |
|------|-------|
| Parallel run length (same layer) | < 10 mm for DDR, < 25 mm for HUB75E |
| Spacing between aggressor/victim | ≥ 3× trace width |
| Guard traces | GND trace between critical pairs (optional) |
| Layer transitions | Reference same GND plane; add stitching via near transition |

### 9.2 Return Path Continuity

- Every signal layer adjacent to solid GND plane (L1→L2, L3→L2 or L4, L6→L5)
- No traces crossing power plane splits on L3 without stitching cap
- Critical signals (DDR, HDMI, USB) routed exclusively on L1 referenced to L2 GND

### 9.3 Stub Control

- No stubs on high-speed traces (> 100 MHz)
- Via stubs on 1.6mm board: ~0.8mm max. Acceptable for DDR3 (533 MHz).
- For future designs > 1 GHz: back-drill or use blind/buried vias.

---

## 10. Manufacturing Notes

### 10.1 PCB Fabrication

| Parameter | Specification |
|-----------|---------------|
| Min trace width | 0.10 mm (4 mil) |
| Min trace spacing | 0.10 mm (4 mil) |
| Min annular ring | 0.10 mm (4 mil) |
| Min via drill | 0.20 mm (8 mil) |
| Min via pad | 0.45 mm (18 mil) |
| Aspect ratio | 8:1 max (0.20mm drill in 1.6mm board = 8:1) |
| Copper-to-edge | 0.40 mm (16 mil) |
| Solder mask expansion | 0.05 mm (2 mil) |
| Solder mask bridge | 0.10 mm (4 mil) min |
| Silkscreen line width | 0.15 mm (6 mil) |
| Silkscreen text height | 1.00 mm (40 mil) min |

### 10.2 Assembly

| Parameter | Specification |
|-----------|---------------|
| SMT process | Both sides (bottom side: decoupling caps, resistors only) |
| BGA placement | X-ray inspection required |
| QFN thermal pad | 50% solder paste coverage, 4-mil stencil |
| Through-hole | HUB75E headers, DC jack, RJ45, HDMI, USB-C, fans |
| Conformal coating | Optional for outdoor/industrial use (acrylic) |

### 10.3 Test Points

| Test Point | Location | Purpose |
|------------|----------|---------|
| TP1-TP7 | Near PMIC | All power rails (+12V_IN, +5V0, +3V3, +1V8, +1V35, +1V0, +1V2) |
| TP8 | Near FPGA | FPGA_PROGRAM_B (monitor config state) |
| TP9 | Near FPGA | FPGA_DONE |
| TP10 | Near STM32 | STM32_NRST |
| TP11 | Near U5 | ETH_CLK (25 MHz) |
| TP12 | Near U6 | USB_CLK (60 MHz) |
| TP13-TP16 | Near HUB75E ports 0,4,8,12 | HUB75E_CLK (verify waveform) |

---

## 11. Design Rule Check (DRC) Parameters

### 11.1 KiCad DRC Rules

| Rule | Value |
|------|-------|
| Min track width | 0.10 mm |
| Min via drill | 0.20 mm |
| Min via diameter | 0.45 mm |
| Min clearance | 0.10 mm |
| Min solder mask clearance | 0.05 mm |
| Min silkscreen clearance | 0.15 mm |
| Copper edge clearance | 0.40 mm |
| Courtyard overlap | 0.25 mm |
| Differential pair gap (100Ω) | 0.15 mm |
| Differential pair gap (90Ω) | 0.15 mm |

### 11.2 Net Classes

| Class | Track Width | Clearance | Via Drill | Via Pad | Diff Gap |
|-------|-------------|-----------|-----------|---------|----------|
| Default | 0.15 mm | 0.15 mm | 0.20 mm | 0.45 mm | — |
| Power | 0.30 mm | 0.25 mm | 0.30 mm | 0.60 mm | — |
| DDR_40Ω | 0.22 mm | 0.20 mm | 0.20 mm | 0.45 mm | — |
| DDR_Diff100 | 0.12 mm | 0.20 mm | 0.20 mm | 0.45 mm | 0.15 mm |
| HDMI_Diff100 | 0.12 mm | 0.25 mm | 0.20 mm | 0.45 mm | 0.15 mm |
| USB_Diff90 | 0.14 mm | 0.25 mm | 0.20 mm | 0.45 mm | 0.15 mm |
| HUB75E_50Ω | 0.15 mm | 0.15 mm | 0.20 mm | 0.45 mm | — |

---

## 12. EMI/EMC Considerations

### 12.1 Emission Control

| Technique | Implementation |
|-----------|----------------|
| Solid ground planes | L2 and L5 continuous, no splits |
| Board edge stitching | Vias every 5mm around perimeter |
| HDMI shielding | Connector shell to GND, TMDS guard traces |
| Ethernet magnetics | Integrated in RJ45 jack, Bob Smith termination |
| Ferrite beads | On DC input, USB VBUS, fan power |
| Spread-spectrum clocking | Si5351A SS enabled (0.5% down-spread) on FPGA clock |

### 12.2 Immunity

| Technique | Implementation |
|-----------|----------------|
| TVS on all external I/O | HDMI: IP4283CZ10, USB: PUSB3FR4, HUB75E: PESD5V0S1BA ×224 |
| ESD diodes on DC input | SMBJ12A |
| Filtered DC input | Ferrite bead + bulk capacitance |
| Ground fill on outer layers | Copper pour connected to GND on L1 and L6 |

---

*Document Version: 1.0 | Author: PCB Layout Engineer | Date: 2026-06-16*
*Target Device: Nebula Matrix — Professional LED Matrix Display Engine*
