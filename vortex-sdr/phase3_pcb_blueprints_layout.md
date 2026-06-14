# Vortex-SDR Phase 3: PCB Blueprints & Layout

## 1. PCB Specifications

### 1.1 Board Overview

| Parameter | Value |
|---|---|
| Dimensions | 120 mm × 75 mm |
| Layers | 4 (2 signal + 2 planes) |
| Thickness | 1.6 mm |
| Material | FR-4 370HR (Isola) |
| Copper weight | 1 oz (35 µm) outer, 0.5 oz (17.5 µm) inner |
| Surface finish | ENIG (1 µ" Au over 3-5 µ" Ni) |
| Solder mask | Green (LPI) |
| Silkscreen | White |
| Minimum trace | 0.10 mm (4 mil) |
| Minimum space | 0.10 mm (4 mil) |
| Minimum via | 0.40 mm dia / 0.20 mm drill |
| Minimum hole | 0.30 mm |
| Impedance control | 50 Ω ±10% coplanar waveguide |
| Controlled impedance | Yes, on L1 and L4 |

### 1.2 Layer Stackup

```
Layer 1 (Top Signal):     35 µm copper — Signal + RF + Component placement
                          0.10 mm FR-4 370HR prepreg (εr=4.4, tanδ=0.013)
Layer 2 (Inner):          17.5 µm copper — Ground plane (split: analog/digital)
                          0.50 mm FR-4 370HR core (εr=4.4, tanδ=0.013)
Layer 3 (Inner):          17.5 µm copper — Power plane (VDD_3V3 + VDD_1V8)
                          0.10 mm FR-4 370HR prepreg (εr=4.4, tanδ=0.013)
Layer 4 (Bottom Signal):  35 µm copper — Signal + Display connector

Total thickness: 1.6 mm (0.063")
```

### 1.3 Impedance Calculations

#### Coplanar Waveguide (RF traces, 50 Ω)

Using Saturn PCB toolkit with FR-4 370HR (εr=4.4):

| Parameter | Value |
|---|---|
| Trace width (W) | 0.35 mm |
| Trace thickness (T) | 0.035 mm |
| Dielectric height (H) | 0.10 mm (to L2 GND) |
| Dielectric constant (εr) | 4.4 |
| Coplanar gap (G) | 0.15 mm |
| **Calculated impedance** | **49.8 Ω** |
| **Target** | **50 Ω ±10%** |

#### USB Differential Pair (90 Ω)

| Parameter | Value |
|---|---|
| Trace width (W) | 0.20 mm |
| Trace spacing (S) | 0.15 mm |
| Dielectric height (H) | 0.10 mm |
| **Calculated differential impedance** | **89.6 Ω** |
| **Target** | **90 Ω ±10%** |

#### LVDS Differential Pairs (100 Ω)

| Parameter | Value |
|---|---|
| Trace width (W) | 0.10 mm |
| Trace spacing (S) | 0.15 mm |
| Dielectric height (H) | 0.10 mm |
| **Calculated differential impedance** | **98.2 Ω** |
| **Target** | **100 Ω ±10%** |

## 2. Routing Rules

### 2.1 Net Classes

| Net Class | Trace Width | Clearance | Via Dia | Via Drill | Notes |
|---|---|---|---|---|---|
| Power_3V3 | 0.30 mm | 0.15 mm | 0.60 mm | 0.30 mm | VDD_3V3, VSRC |
| Power_1V8 | 0.25 mm | 0.15 mm | 0.50 mm | 0.25 mm | VDD_1V8 |
| Power_GND | 0.30 mm | 0.15 mm | 0.60 mm | 0.30 mm | GND (plane fills) |
| RF_50 | 0.35 mm | 0.25 mm | — | — | Coplanar waveguide |
| LVDS_100 | 0.10 mm | 0.10 mm | 0.40 mm | 0.20 mm | Diff pair, length-match |
| USB_90 | 0.20 mm | 0.10 mm | 0.40 mm | 0.20 mm | Diff pair |
| HighSpeed_SPI | 0.15 mm | 0.10 mm | 0.40 mm | 0.20 mm | FPGA, Flash SPI |
| Default | 0.10 mm | 0.10 mm | 0.40 mm | 0.20 mm | General signal |

### 2.2 RF Routing Rules

1. **SMA input to LNA**: Keep trace length under 5 mm, 50 Ω coplanar waveguide with ground pour on both sides
2. **LNA to attenuator**: 50 Ω trace, via stitching every 0.5 mm along ground pour edge
3. **Attenuator to mixer**: 50 Ω trace, minimize bends (use 45° maximum), add ground via at every bend
4. **Mixer to SAW filter**: 50 Ω trace, shortest path possible, avoid crossing digital planes
5. **SAW filter to ADC driver**: 50 Ω trace, matched length within 0.1 mm
6. **LO path (ADF4351 to mixer)**: 50 Ω trace, keep away from RF input trace by at least 5 mm
7. **All RF traces**: No vias allowed (route on top layer only), maintain 50 Ω impedance throughout

### 2.3 LVDS Routing Rules

1. **Differential pairs**: Route D+/D- together, matched length within 0.1 mm
2. **Via count**: Maximum 2 vias per differential pair (one on each signal)
3. **Serpentine matching**: If length mismatch > 0.1 mm, add serpentine near source
4. **Ground reference**: All LVDS pairs reference L2 ground plane, no splits
5. **Termination**: 100 Ω differential termination at FPGA input (on-die in iCE40UP5K)

### 2.4 USB Routing Rules

1. **D+/D-**: Route as differential pair, matched length within 0.05 mm
2. **Route on top layer**: No vias on USB signals
3. **ESD placement**: TPD4E05U06 within 5 mm of USB-C connector
4. **Series resistors**: 47 Ω within 10 mm of MCU pins
5. **Shield**: Connect USB-C shield to chassis ground via ferrite bead

### 2.5 Power Routing Rules

1. **VDD_3V3**: Use 0.30 mm traces from TPS62A02 output, with 4× 100 µF ceramic capacitors at output
2. **VDD_1V8**: Use 0.25 mm traces from TLV75518 output, with 10 µF + 100 nF at each FPGA bank
3. **VDD_3V3A**: Route via LC filter (10 µH + 10 µF), separate from digital 3.3V
4. **VDD_RF**: Route via LC filter (10 µH + 10 µF), separate from digital 3.3V
5. **Ground**: Use continuous ground plane on L2, no splits under RF section
6. **Power plane**: L3 carries VDD_3V3 on left half, VDD_1V8 on right half

### 2.6 Signal Routing Rules

1. **Crystal oscillators**: Keep traces under 10 mm, guard ring with ground vias
2. **TCXO outputs**: Route on top layer, avoid crossing other signals
3. **Reset lines**: Add 10 kΩ pull-up and 100 nF decoupling at MCU pin
4. **Button inputs**: Add 10 kΩ pull-up and 100 pF noise filter at MCU pin
5. **SPI clock**: Route on top layer, avoid parallel coupling with other signals

## 3. Placement

### 3.1 Zone Layout

The PCB is divided into functional zones to minimize interference:

```
┌──────────────────────────────────────────────────────────────────┐
│  ZONE A: RF Section (left)           │  ZONE B: Digital (right)  │
│                                      │                          │
│  ┌──────────────┐                    │  ┌──────────────────┐   │
│  │ SMA Input    │                    │  │  STM32H743       │   │
│  │ (50Ω edge)   │                    │  │  (MCU)           │   │
│  └──────┬───────┘                    │  │  LQFP-144        │   │
│         │ 50Ω trace                  │  └──────┬───────────┘   │
│  ┌──────▼───────┐                    │         │               │
│  │  LNA         │                    │  ┌──────▼───────────┐   │
│  │  SPF5189Z    │                    │  │  iCE40UP5K       │   │
│  └──────┬───────┘                    │  │  (FPGA)          │   │
│         │ 50Ω trace                  │  │  QFN-48          │   │
│  ┌──────▼───────┐                    │  └──────┬───────────┘   │
│  │  Mixer       │◄──LO──┐            │         │ SPI            │
│  │  ADL5801     │       │            │  ┌──────▼───────────┐   │
│  └──────┬───────┘       │            │  │  W25Q256         │   │
│         │ IF            │            │  │  (Flash)          │   │
│  ┌──────▼───────┐  ┌────▼───────┐   │  └──────────────────┘   │
│  │  SAW Filter  │  │  ADF4351   │   │                          │
│  │  10.7MHz     │  │  (PLL)     │   │  ┌──────────────────┐   │
│  └──────┬───────┘  └────────────┘   │  │  Display Conn.   │   │
│         │ IF                         │  │  (bottom side)   │   │
│  ┌──────▼───────┐                    │  └──────────────────┘   │
│  │  OPA364      │                    │                          │
│  │  ADC driver  │                    │  ┌──────────────────┐   │
│  └──────┬───────┘                    │  │  nRF52832        │   │
│         │                            │  │  (BLE)           │   │
│  ┌──────▼───────┐                    │  └──────────────────┘   │
│  │  AD9645      │                    │                          │
│  │  (ADC)       │───LVDS──►FPGA     │  ┌──────────────────┐   │
│  └──────────────┘                    │  │  USB-C Port      │   │
│                                      │  │  (right edge)    │   │
│  ┌──────────────┐                    │  └──────────────────┘   │
│  │  Power       │                    │                          │
│  │  TPS62A02    │                    │  ┌──────────────────┐   │
│  │  TLV75518    │                    │  │  Buttons + LED   │   │
│  │  MCP73871    │                    │  │  (bottom edge)   │   │
│  └──────────────┘                    │  └──────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
```

### 3.2 Component Placement Rules

1. **RF section** (left half of board):
   - SMA connector on left edge, centered vertically
   - LNA within 5 mm of SMA
   - Mixer centered between LNA and SAW
   - ADF4351 PLL in lower-left, with ground pour isolation
   - AD9645 in upper-left, close to SAW filter output
   - All RF traces on top layer, no vias

2. **Digital section** (right half of board):
   - MCU centered-right, with decoupling on perimeter
   - FPGA adjacent to MCU on the ADC side (short SPI path)
   - Flash near MCU (short SPI3 path)
   - BLE module in upper-right corner (antenna clear)
   - Display connector on bottom edge (for module mounting)

3. **Power section** (lower-left):
   - Buck regulator near battery connector
   - LDOs near their respective loads
   - Charger near USB-C connector

4. **Crystal/TCXO placement**:
   - 8 MHz MCU crystal within 5 mm of OSC pins
   - 32.768 kHz crystal near OSC32 pins
   - 61.44 MHz TCXO within 5 mm of ADC CLK pins
   - 25 MHz TCXO within 5 mm of ADF4351 REF_IN

### 3.3 Clearance Requirements

| Zone | Keep-Out | Clearance | Reason |
|---|---|---|---|
| RF section | Digital signals | ≥ 5 mm | Prevent digital noise coupling into RF path |
| Crystal/TCXO | All other traces | ≥ 2 mm | Prevent pulling and phase noise degradation |
| ADC | Switching supplies | ≥ 5 mm | Prevent switching noise on ADC reference |
| USB connector | All traces | ≥ 2 mm | ESD protection zone |
| BLE antenna | Copper (other than ground) | ≥ 5 mm | Antenna detuning prevention |

## 4. Thermal Management

### 4.1 Thermal Budget

| Component | Power | Θ_JA (°C/W) | T_J (60°C ambient) | Margin |
|---|---|---|---|---|
| STM32H743 | 350 mW | 38 | 73.3°C | 31.7°C |
| iCE40UP5K | 100 mW | 45 | 64.5°C | 40.5°C |
| AD9645 | 370 mW | 42 | 75.5°C | 29.5°C |
| ADF4351 | 500 mW | 35 | 77.5°C | 27.5°C |
| ADL5801 | 350 mW | 40 | 74.0°C | 31.0°C |
| TPS62A02 | 150 mW | 55 | 68.3°C | 36.7°C |
| Display | 300 mW | N/A | N/A | N/A |

### 4.2 Thermal Mitigation

1. **STM32H743**: Use exposed pad (LQFP-144) connected to L2 ground plane via 9 thermal vias (0.40 mm dia)
2. **iCE40UP5K**: Use exposed pad (QFN-48) connected to L2 ground via 4 thermal vias
3. **AD9645**: Use exposed pad (LQFP-64) connected to L2 ground via 6 thermal vias
4. **ADF4351**: Use ground paddle (LFCSP-32) connected to L2 ground via 4 thermal vias
5. **ADL5801**: Use ground paddle (LFCSP-24) connected to L2 ground via 4 thermal vias
6. **TPS62A02**: Use exposed pad (VSON-8) connected to L2 ground via 2 thermal vias
7. **All thermal vias**: 0.40 mm dia / 0.20 mm drill, filled with solder mask

### 4.3 Thermal Vias

| Component | Vias | Pattern | Via Size | Connection |
|---|---|---|---|---|
| STM32H743 | 9 | 3×3 grid under pad | 0.40/0.20 mm | L2 GND |
| iCE40UP5K | 4 | 2×2 grid under pad | 0.40/0.20 mm | L2 GND |
| AD9645 | 6 | 2×3 grid under pad | 0.40/0.20 mm | L2 GND |
| ADF4351 | 4 | 2×2 grid under pad | 0.40/0.20 mm | L2 GND |
| ADL5801 | 4 | 2×2 grid under pad | 0.40/0.20 mm | L2 GND |
| TPS62A02 | 2 | 1×2 under pad | 0.40/0.20 mm | L2 GND |

## 5. Grounding Strategy

### 5.1 Ground Plane Architecture

- **L2 (inner ground)**: Solid ground plane with controlled splits
  - Digital ground: Right 2/3 of board (MCU, FPGA, display)
  - RF ground: Left 1/3 of board (LNA, mixer, ADC, PLL)
  - Connected at single point near TPS62A02 output (star ground)
  - No slots or cuts under RF traces

- **Via stitching**: Every 2 mm along ground pour edges, every 1 mm near RF traces

### 5.2 Ground Split Strategy

```
┌─────────────────────────────┬──────────────────────────────┐
│                             │                              │
│    RF GROUND ZONE           │    DIGITAL GROUND ZONE       │
│                             │                              │
│    (LNA, mixer, SAW,        │    (MCU, FPGA, display,     │
│     ADC, PLL, TCXOs)        │     flash, BLE, USB)        │
│                             │                              │
│         ┌──────┐            │                              │
│         │  ★   │            │                              │
│         │ STAR │            │                              │
│         │ GND  │            │                              │
│         └──────┘            │                              │
│                             │                              │
└─────────────────────────────┴──────────────────────────────┘
                 Single connection point at TPS62A02 GND
```

### 5.3 Ferrite Bead Isolation

| Net | Ferrite Bead | Purpose |
|---|---|---|
| VDD_RF | FB1 (600 Ω @ 100 MHz) | Isolate RF section from digital noise |
| VDD_3V3A | FB2 (600 Ω @ 100 MHz) | Isolate analog section from digital noise |
| VDD_PLL | FB3 (600 Ω @ 100 MHz) | Isolate PLL from switching noise |
| USB shield | FB4 (600 Ω @ 100 MHz) | Isolate USB shield from chassis ground |

## 6. DFM Notes

### 6.1 Assembly

1. **SMT only**: No through-hole components except connectors (JST-PH, SWD header)
2. **Stencil**: 0.10 mm (4 mil) for fine-pitch QFN/LGA packages
3. **Reflow**: Lead-free SAC305 profile (260°C peak)
4. **Inspection**: AOI required for U1 (LQFP-144), U2 (QFN-48), U3 (LQFP-64), U5 (LFCSP-32)
5. **Rework**: U1, U3, U5 require hot-air rework station
6. **Double-sided**: Display connector and touch controller on bottom side

### 6.2 Panelization

- **Panel size**: 3-up on 150 mm × 200 mm panel
- **Breakaway**: 2 mm mouse bites (5 per side)
- **Fiducials**: 3 on panel edge, 1 per board
- **Tooling holes**: 4× 3.0 mm holes on panel corners

### 6.3 Testing

1. **ICT fixture**: Bed-of-nails with probe points on test pads
2. **Test points**: 1 mm diameter test pads on all power rails and critical signals
3. **Minimum test coverage**: 95% of nets
4. **Boundary scan**: STM32H743 JTAG for digital I/O verification
5. **RF test**: SMA connector → network analyzer for S-parameter verification
6. **Functional test**: Power-on self-test with FPGA bitstream load + ADC verification

### 6.4 Design for Reliability

1. **Conformal coating**: Apply acrylic conformal coating to all components except:
   - SMA connector (RF port)
   - USB-C connector (data port)
   - Display connector (flex cable)
   - Antenna area (BLE module)
2. **ESD protection**: TPD4E05U06 on USB, 100 kΩ series resistors on all external buttons
3. **Vibration**: All components > 5 mm² have additional adhesive dots
4. **Thermal cycling**: Components rated to -40°C to +85°C (industrial grade)

## 7. Via Strategy

### 7.1 Via Types

| Type | Diameter | Drill | Use |
|---|---|---|---|
| Standard | 0.40 mm | 0.20 mm | General signal routing |
| Power | 0.60 mm | 0.30 mm | Power rails (VDD_3V3, VDD_1V8, GND) |
| Thermal | 0.40 mm | 0.20 mm | Under IC pads for heat dissipation |
| RF ground | 0.30 mm | 0.15 mm | Ground stitching near RF traces |

### 7.2 Via Stitching Rules

1. **Ground pour edges**: Via every 2 mm along all ground pour edges
2. **RF section**: Via every 1 mm along ground pour edges adjacent to RF traces
3. **Power via**: Minimum 2 vias per power pin (except IC pads)
4. **Thermal pads**: Via grid under all QFN/LFCSP ground pads
5. **No vias**: On RF signal traces (L1 only), on USB differential pairs

## 8. Silkscreen & Markings

### 8.1 Required Silkscreen

- Reference designators (U1, C1, R1, etc.)
- Polarity marks on polarized components (diodes, electrolytics)
- Pin 1 indicators on all ICs
- Test point labels (TP1, TP2, etc.)
- Board name: "VORTEX-SDR"
- Board revision: "REV 1.0"
- Date code placeholder: "YYYY-WW"
- Logo placeholder (front and back)

### 8.2 Bottom Silkscreen

- Battery polarity (+/-) near J3
- USB-C port label
- Regulatory marks (CE, FCC) placeholder
- QR code linking to documentation

## 9. Board Outline

```
                    75 mm
    ┌────────────────────────────────┐
    │                                │ 120
    │  SMA                           │ mm
    │  ──►[LNA]──[MIX]──[SAW]──►    │
    │          [PLL]  [ADC]          │
    │                                │
    │  [MCU]     [FPGA]  [FLASH]    │
    │                                │
    │  [DISP]    [BLE]   [USB-C]    │
    │                                │
    │  [POWER]  [CHG]   [BAT]       │
    │                                │
    │  [SW1][SW2][SW3][SW4][SW5][SW6]│
    └────────────────────────────────┘
    
    Mounting holes: 4× M2.5 in corners (3.5 mm from edges)
    Board edge: 0.5 mm clearance from copper
```

### 9.1 Dimensions

| Feature | X Position | Y Position | Size |
|---|---|---|---|
| Board outline | 0, 0 | 120, 75 | 120 × 75 mm |
| SMA connector | 0, 37.5 | — | 5 mm from left edge |
| USB-C connector | 120, 50 | — | 5 mm from right edge |
| Display connector | 60, 75 | — | 30 mm wide, bottom edge |
| Battery connector | 30, 10 | — | 5 mm from top edge |
| Mounting holes | (3.5, 3.5), (116.5, 3.5), (3.5, 71.5), (116.5, 71.5) | — | M2.5 |
| SWD header | 115, 5 | — | 5-pin, top-right |