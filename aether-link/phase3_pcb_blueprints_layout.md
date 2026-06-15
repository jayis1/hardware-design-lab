# Aether-Link: Phase 3 — PCB Blueprints and Layout

## 1. Board Overview

### 1.1 Form Factor
- **Standard**: PCI Express Card Electromechanical Specification 4.0
- **Form Factor**: Full-Height, Half-Length (FHHL)
- **Board Dimensions**: 167.65 mm (6.60") × 111.15 mm (4.376")
- **Board Thickness**: 1.60 mm (0.063") nominal
- **PCIe Edge Connector**: 98-position, 1.0 mm pitch, gold-plated (ENIG, 30µ" Au over 100µ" Ni)
- **Bracket**: Full-height PCIe bracket with cutouts for 2x QSFP28 cages and 1x USB Micro-B

### 1.2 Board Outline Drawing

```
┌────────────────────────────────────────────────────────────┐
│  BRACKET END (I/O panel)                                   │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  [QSFP28 #0]  [QSFP28 #1]  [USB]  [LEDs]  [Fans]   │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                                                       │  │
│  │   QSFP0 Cage    QSFP1 Cage                           │  │
│  │   ┌─────────┐   ┌─────────┐                          │  │
│  │   │         │   │         │   ┌──────────────────┐   │  │
│  │   │         │   │         │   │  FPGA U1         │   │  │
│  │   └─────────┘   └─────────┘   │  XC7K325T        │   │  │
│  │                                │  FFG900 (31mm)   │   │  │
│  │   ┌─────────┐   ┌─────────┐   │  + Heatsink      │   │  │
│  │   │ DDR4 A  │   │ DDR4 B  │   └──────────────────┘   │  │
│  │   │ U2      │   │ U2      │                          │  │
│  │   └─────────┘   └─────────┘   ┌──────────────────┐   │  │
│  │                                │  Power Stage     │   │  │
│  │   ┌─────────┐   ┌─────────┐   │  U12-U17         │   │  │
│  │   │ Flash   │   │ FT232H  │   │  TPS546D24A x6   │   │  │
│  │   │ U3      │   │ U4      │   └──────────────────┘   │  │
│  │   └─────────┘   └─────────┘                          │  │
│  │                                                       │  │
│  │   ┌──────────────────────────────────────────────┐   │  │
│  │   │  PCIe x8 Edge Connector (gold fingers)       │   │  │
│  │   └──────────────────────────────────────────────┘   │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  EDGE CONNECTOR END (inserts into PCIe slot)                │
└────────────────────────────────────────────────────────────┘
```

## 2. PCB Stackup

### 2.1 Layer Configuration — 12-Layer Board

| Layer | Name              | Material        | Thickness    | Copper Weight | Primary Function                    |
|-------|-------------------|-----------------|--------------|---------------|-------------------------------------|
| L1    | TOP               | Copper          | 1.5 mil (0.5oz + plating) | 1.5 oz | High-speed signals, components      |
| —     | Prepreg           | FR-4 2116       | 4.0 mil       | —             | Bonding                             |
| L2    | GND1              | Copper          | 1.2 mil (1 oz)| 1.0 oz        | Ground plane (PCIe/QSFP reference)  |
| —     | Core              | FR-4 0.005"     | 5.0 mil       | —             | Dielectric                          |
| L3    | SIG1              | Copper          | 1.2 mil (1 oz)| 1.0 oz        | DDR4 routing, general signals       |
| —     | Prepreg           | FR-4 2116       | 4.0 mil       | —             | Bonding                             |
| L4    | GND2              | Copper          | 1.2 mil (1 oz)| 1.0 oz        | Ground plane (DDR4 reference)       |
| —     | Core              | FR-4 0.005"     | 5.0 mil       | —             | Dielectric                          |
| L5    | PWR1              | Copper          | 1.2 mil (1 oz)| 1.0 oz        | Power plane (VCCINT, MGTAVCC split) |
| —     | Prepreg           | FR-4 2116       | 4.0 mil       | —             | Bonding                             |
| L6    | SIG2              | Copper          | 1.2 mil (1 oz)| 1.0 oz        | Internal routing, DDR4              |
| —     | Core              | FR-4 0.005"     | 5.0 mil       | —             | Dielectric                          |
| L7    | GND3              | Copper          | 1.2 mil (1 oz)| 1.0 oz        | Ground plane                        |
| —     | Prepreg           | FR-4 2116       | 4.0 mil       | —             | Bonding                             |
| L8    | PWR2              | Copper          | 1.2 mil (1 oz)| 1.0 oz        | Power plane (VCCO, VCCAUX split)   |
| —     | Core              | FR-4 0.005"     | 5.0 mil       | —             | Dielectric                          |
| L9    | SIG3              | Copper          | 1.2 mil (1 oz)| 1.0 oz        | Internal routing, DDR4              |
| —     | Prepreg           | FR-4 2116       | 4.0 mil       | —             | Bonding                             |
| L10   | GND4              | Copper          | 1.2 mil (1 oz)| 1.0 oz        | Ground plane                        |
| —     | Core              | FR-4 0.005"     | 5.0 mil       | —             | Dielectric                          |
| L11   | PWR3              | Copper          | 1.2 mil (1 oz)| 1.0 oz        | Power plane (3.3V, 2.5V, 1.8V)     |
| —     | Prepreg           | FR-4 2116       | 4.0 mil       | —             | Bonding                             |
| L12   | BOTTOM            | Copper          | 1.5 mil (0.5oz + plating) | 1.5 oz | Components, decoupling, low-speed   |

**Total thickness**: ~62.4 mil (1.58 mm) — within 1.60 mm ±10% spec.

### 2.2 Material Properties

| Property                  | Value                          |
|---------------------------|--------------------------------|
| FR-4 Dielectric Constant  | εr = 4.2 @ 1 GHz (Isola 370HR)|
| FR-4 Loss Tangent         | tan δ = 0.020 @ 1 GHz         |
| Glass Transition Temp     | Tg = 180°C (Isola 370HR)      |
| Decomposition Temp        | Td = 350°C                     |
| CTE (Z-axis)              | 45 ppm/°C (pre-Tg), 250 ppm/°C (post-Tg) |
| Surface Finish            | ENIG (Electroless Nickel Immersion Gold) |
| Solder Mask               | Green LPI, both sides          |
| Silkscreen                | White epoxy, top side only     |

### 2.3 Impedance Calculation Summary

Using Polar SI9000 with Isola 370HR parameters:

| Signal Type              | Target Z0   | Trace Width | Spacing   | Layer Stack          |
|--------------------------|-------------|-------------|-----------|----------------------|
| 100Ω differential (edge) | 100Ω ±10%   | 4.0 mil     | 6.0 mil   | L1→L2 (4mil prepreg) |
| 100Ω differential (inner)| 100Ω ±10%   | 3.5 mil     | 7.0 mil   | L3→L4 (4mil prepreg) |
| 90Ω differential (USB)   | 90Ω ±10%    | 5.0 mil     | 5.0 mil   | L1→L2                |
| 80Ω differential (DQS)   | 80Ω ±10%    | 4.5 mil     | 5.5 mil   | L3→L4                |
| 40Ω single-ended (DDR4)  | 40Ω ±10%    | 6.0 mil     | —         | L3→L4                |
| 50Ω single-ended (GPIO)  | 50Ω ±10%    | 5.0 mil     | —         | L1→L2                |

## 3. High-Speed Routing Rules

### 3.1 PCIe Gen4 (16 GT/s) Routing

**Critical constraints for 16 GT/s NRZ signaling:**

| Parameter              | Requirement                          |
|------------------------|--------------------------------------|
| Differential impedance | 100Ω ±10%                            |
| Intra-pair skew        | <1 ps (0.15 mil length difference)   |
| Inter-pair skew (x8)   | <5 ns (1000 mil length difference)   |
| Max trace length       | <4.0 inches (from FPGA to edge)      |
| AC coupling cap        | 0.1µF 0402, placed <200 mil from FPGA TX |
| Via count per trace    | ≤2 vias (prefer 0 on top layer)      |
| Reference plane        | Solid GND on L2, no splits under pairs|
| Spacing to other signals| ≥4× trace width (16 mil minimum)     |
| Spacing to other pairs | ≥5× intra-pair spacing (30 mil min)  |
| Bend angle             | 45° max (no 90° bends)               |
| Via stub length        | <10 mil (back-drill if needed)       |

**PCIe lane routing order (top side, L1):**
```
FPGA MGT115 ──► Edge connector
Lane 0: shortest path (outermost edge finger)
Lane 7: longest path (innermost edge finger)
All lanes length-matched to within 25 mil of each other.
```

**PCIe TX AC coupling placement:**
- Capacitors: 0.1µF 0402 (GRM155R71C104KA88D), 16V rating
- Placed on L1, within 200 mil of FPGA TX ball
- No vias between FPGA ball and capacitor pad
- Capacitor pad width matched to trace width (4.0 mil → 12 mil pad with neck-down)

### 3.2 100GbE (4×25.78125 Gbps) QSFP28 Routing

| Parameter              | Requirement                          |
|------------------------|--------------------------------------|
| Differential impedance | 100Ω ±10%                            |
| Intra-pair skew        | <0.5 ps (0.08 mil length difference) |
| Inter-pair skew (per port)| <2 ns (400 mil length difference) |
| Max trace length       | <3.0 inches (FPGA to QSFP28 cage)    |
| AC coupling cap        | 0.1µF 0402, placed <150 mil from FPGA TX |
| Via count per trace    | ≤2 vias                              |
| Reference plane        | Solid GND on L2                      |
| Spacing to other pairs | ≥5× intra-pair spacing               |
| Return loss            | <-10 dB up to 12.89 GHz              |
| Insertion loss         | <-6 dB at 12.89 GHz (Nyquist)       |

**QSFP28 lane mapping (L1 routing):**
```
QSFP0: FPGA MGT116 → QSFP0 Cage connector
  Lane 0 (RX1): shortest
  Lane 3 (RX4): longest
  All 4 TX lanes length-matched within 10 mil.
  All 4 RX lanes length-matched within 10 mil.

QSFP1: FPGA MGT117 → QSFP1 Cage connector
  Same constraints as QSFP0.
```

### 3.3 DDR4-3200 Routing

**Fly-by topology for Address/Command/Control (one rank per channel):**

| Parameter              | Requirement                          |
|------------------------|--------------------------------------|
| Single-ended impedance | 40Ω ±10% (ADDR/CMD/CTRL)             |
| Differential impedance | 80Ω ±10% (DQS), 100Ω ±10% (CLK)      |
| CLK to ADDR/CMD skew   | ±5 ps at DRAM ball                   |
| DQS to DQ skew (byte)  | ±2 ps within byte lane               |
| DQS to CLK skew        | ±10 ps at DRAM ball                  |
| Max trace length       | <2.5 inches (FPGA to DRAM)           |
| Via count per trace    | ≤1 via (prefer L3 routing)           |
| Reference plane        | Solid GND on L2 and L4               |
| Spacing DQ-to-DQ       | ≥2× trace width (12 mil minimum)     |
| Spacing DQ-to-DQS      | ≥3× trace width (18 mil minimum)     |
| Address mirroring      | Not required (single rank)           |

**DDR4 Channel A routing layers:**
- ADDR/CMD/CTRL: L3 (SIG1), referenced to L2 (GND1) and L4 (GND2)
- DQ[0:7] + DQS0 + DM0: L3, length-matched within 5 mil
- DQ[8:15] + DQS1 + DM1: L3, length-matched within 5 mil
- DQ[16:23] + DQS2 + DM2: L9 (SIG3), length-matched within 5 mil
- DQ[24:31] + DQS3 + DM3: L9, length-matched within 5 mil
- CLK0_P/N: L3, differential pair, length-matched to ADDR/CMD within 5 mil

**DDR4 Channel B routing layers:**
- ADDR/CMD/CTRL: L3 (SIG1)
- DQ byte lanes: L3 and L9, same constraints as Channel A

**DDR4 termination:**
- VTT island on L1 under DRAM devices
- 49.9Ω 0402 resistors to VTT on each ADDR/CMD line at DRAM end
- VTT plane on L5 (PWR1) under DRAM region, connected to TPS51200

### 3.4 USB 2.0 (480 Mbps) Routing

| Parameter              | Requirement                          |
|------------------------|--------------------------------------|
| Differential impedance | 90Ω ±10%                             |
| Intra-pair skew        | <50 ps (7.5 mil length difference)   |
| Max trace length       | <6.0 inches (FT232H to USB connector)|
| Reference plane        | Solid GND on L2                      |
| ESD protection         | USBLC6-2SC6 (ST) at connector        |

### 3.5 Clock Routing

**LVDS clocks (100Ω differential):**

| Clock              | Source          | Destination     | Length    | Layer |
|--------------------|-----------------|-----------------|-----------|-------|
| PCIE_REFCLK        | Edge A13/A14    | FPGA A34_P/N    | <2.0"     | L1    |
| DDR4_REFCLK        | U8 (SiT9121)    | FPGA B34_P/N    | <1.5"     | L1    |
| CMAC_REFCLK        | U9 (SiT9365)    | FPGA C34_P/N    | <1.5"     | L1    |
| QSFP0_REFCLK       | U10 (SiT9365)   | FPGA D34_P/N    | <1.5"     | L1    |
| QSFP1_REFCLK       | U11 (SiT9365)   | FPGA E34_P/N    | <1.5"     | L1    |

All LVDS clocks: 100Ω differential termination at FPGA input (internal DIFF_TERM enabled).
Series termination: none (LVDS drivers have internal source termination).

## 4. Via Strategy

### 4.1 Via Types

| Via Type       | Drill Diameter | Pad Diameter | Anti-pad Diameter | Use Case                    |
|----------------|----------------|--------------|-------------------|-----------------------------|
| Microvia (L1-L2)| 0.10 mm (4 mil)| 0.25 mm (10 mil)| 0.40 mm (16 mil) | High-speed signal transitions|
| Blind (L1-L3)  | 0.20 mm (8 mil)| 0.40 mm (16 mil)| 0.60 mm (24 mil) | DDR4 routing transitions    |
| Buried (L3-L10)| 0.20 mm (8 mil)| 0.40 mm (16 mil)| 0.60 mm (24 mil) | Internal layer transitions  |
| Through (L1-L12)| 0.30 mm (12 mil)| 0.55 mm (22 mil)| 0.80 mm (32 mil) | Power, ground, low-speed    |

### 4.2 Via Rules for High-Speed Signals

- **PCIe/QSFP28**: Use microvias (L1→L2) only when necessary. Prefer top-layer routing
  with zero vias. If a via is unavoidable, back-drill from L12 to remove stub.
- **DDR4**: Use blind vias (L1→L3 or L1→L9) for layer transitions. No through vias
  on DQ/DQS nets (stub degrades signal integrity above 1.6 GT/s).
- **Clocks**: Zero vias preferred. If needed, microvia L1→L2 only.
- **Ground stitching vias**: Place GND vias within 40 mil of every signal via transition.
  For differential pairs, place symmetric GND return vias on both sides.

### 4.3 Via Count Budget

| Signal Group     | Max Vias per Net |
|------------------|------------------|
| PCIe TX/RX pairs | 2 (prefer 0)     |
| QSFP28 TX/RX     | 2 (prefer 0)     |
| DDR4 DQ/DQS      | 1                 |
| DDR4 ADDR/CMD    | 1                 |
| DDR4 CLK         | 0                 |
| LVDS clocks      | 0                 |
| USB D+/D-        | 2                 |
| I2C, UART, GPIO  | 4                 |

## 5. Power Distribution Network (PDN) Design

### 5.1 Plane Assignments

| Layer | Plane Splits                                              |
|-------|-----------------------------------------------------------|
| L5    | VCCINT (60%), MGTAVCC (20%), VCCBRAM (10%), MGTAVTT (10%)|
| L8    | VCCO_1V5 (50%), VCCAUX (30%), DDR4_VDDQ (20%)            |
| L11   | VCC3V3 (60%), VCC2V5 (20%), VCC1V8 (20%)                 |

### 5.2 Plane Isolation

- **Moat width**: 40 mil minimum between different voltage planes on same layer.
- **Stitching caps**: 0.1µF + 0.01µF across moats every 200 mil for return current continuity.
- **Ferrite beads**: BLM18PG121SN1D on each voltage entry to isolate noise between domains.

### 5.3 Target Impedance (PDN)

| Rail      | Target Z (DC-100MHz) | Target Z (100MHz-1GHz) |
|-----------|----------------------|-------------------------|
| VCCINT    | <5 mΩ                | <50 mΩ                  |
| MGTAVCC   | <3 mΩ                | <30 mΩ                  |
| MGTAVTT   | <10 mΩ               | <100 mΩ                 |
| VCCBRAM   | <10 mΩ               | <100 mΩ                 |
| VCCAUX    | <15 mΩ               | <150 mΩ                 |
| VCCO_1V5  | <10 mΩ               | <100 mΩ                 |
| VCC3V3    | <20 mΩ               | <200 mΩ                 |

### 5.4 Decoupling Placement Strategy

**Per FPGA power ball cluster (BGA breakout):**
1. 0.01µF 0402 — directly on L1, within 50 mil of BGA ball, via to plane within 20 mil.
2. 0.1µF 0402 — on L1, within 100 mil of BGA ball.
3. 1.0µF 0402 — on L1, within 200 mil of BGA ball.
4. 47µF 0805 — on L12 (bottom), directly under FPGA, via array to planes.
5. 100µF 1206 — on L12, under FPGA center.
6. 330µF Tantalum — on L12, near FPGA edge.

**Per voltage regulator (TPS546D24A):**
1. Input: 22µF 0805 + 0.1µF 0402 within 100 mil of VIN pins.
2. Output: 47µF 0805 + 0.1µF 0402 within 100 mil of VOUT pins.
3. Bulk output: 100µF 1206 within 300 mil.
4. Bootstrap cap: 0.1µF 0402 within 50 mil of BOOT pin.

## 6. Thermal Management

### 6.1 Thermal Vias

- **FPGA thermal pad**: 9×9 array of 0.30 mm (12 mil) drill thermal vias under FPGA
  center pad, connecting L1 to L12. Filled and capped (IPC-4761 Type VII).
- **Power stage thermal pads**: 4×4 array of thermal vias under each TPS546D24A.
- **DDR4 thermal relief**: 2×2 array under each DRAM package.

### 6.2 Copper Pour for Heat Spreading

- L1: 2 oz copper pour (GND) in all unused areas, stitched to L2 GND with vias every 200 mil.
- L2, L4, L7, L10: Solid GND planes (1 oz) act as heat spreaders.
- L12: 2 oz copper pour (GND) in unused areas.

### 6.3 Heatsink Mounting

- **Heatsink**: Custom aluminum, 90mm × 80mm × 25mm, with copper heat pipes.
- **Attachment**: 4x M2.5 screws through PCB to backing plate (non-conductive).
- **Thermal interface**: Honeywell PTM7000 phase-change material (0.25 mm).
- **Keepout zone**: 95mm × 85mm around FPGA, no components taller than 2mm.
- **Fan mounting**: 2x 40mm fans on bracket, blowing across heatsink fins.

### 6.4 Thermal Simulation Targets

| Component      | Max Tj      | Ambient | θJA (with heatsink) | Expected Tj at 75W |
|----------------|-------------|---------|---------------------|---------------------|
| FPGA           | 100°C       | 55°C    | 0.4°C/W             | 85°C                |
| DDR4 (each)    | 95°C        | 55°C    | 15°C/W (still air)  | 70°C (1.5W each)    |
| TPS546D24A     | 125°C       | 55°C    | 18°C/W (PCB)        | 85°C (2W each)      |
| QSFP28 module  | 70°C (case) | 55°C    | Module-dependent    | 65°C (3.5W each)    |

## 7. Clearance and Creepage

### 7.1 Voltage Spacing (per IPC-2221B)

| Voltage Difference | Clearance (uncoated) | Creepage (uncoated) |
|--------------------|----------------------|---------------------|
| 0-15V (12V rail)   | 0.13 mm (5 mil)      | 0.13 mm (5 mil)     |
| 15-30V              | 0.25 mm (10 mil)     | 0.25 mm (10 mil)    |
| 30-50V              | 0.50 mm (20 mil)     | 0.60 mm (24 mil)    |

All voltages on Aether-Link are ≤12V, so 5 mil clearance/creepage applies.
However, for manufacturing robustness, we use:
- **12V to GND**: 20 mil clearance (conservative)
- **12V to other rails**: 30 mil clearance
- **Between different power planes**: 40 mil moat

### 7.2 PCIe Edge Connector Keepout

- No components within 3.0 mm (120 mil) of the gold finger area on L1.
- No copper on L2-L12 within 2.0 mm (80 mil) of edge connector bevel.
- Bevel angle: 20° ±5°, gold finger length: 3.5 mm.

## 8. Manufacturing Notes

### 8.1 PCB Fabrication

- **PCB Vendor Class**: IPC-6012 Class 3 (high-reliability)
- **Minimum trace/space**: 3.5 mil / 3.5 mil
- **Minimum drill**: 0.10 mm (4 mil) for microvias, 0.20 mm (8 mil) for mechanical
- **Aspect ratio**: ≤10:1 for through vias
- **Copper plating**: 1.0 oz base + plating to 1.5 oz on outer layers
- **Solder mask registration**: ±2 mil
- **Silkscreen minimum line width**: 6 mil
- **Impedance control**: ±10% on all controlled-impedance traces
- **Testing**: 100% flying probe electrical test, impedance TDR coupon on panel

### 8.2 Assembly

- **SMT placement accuracy**: ±1 mil (25 µm) for 0402, ±2 mil for BGA
- **BGA soldering**: Vapor phase reflow, SAC305 solder, nitrogen atmosphere
- **Through-hole**: Selective wave solder for QSFP28 cages, fan headers
- **Inspection**: Automated Optical Inspection (AOI) + X-ray for BGA
- **Conformal coating**: Optional acrylic (for harsh environments)

### 8.3 Panelization

- 2-up panel (2 cards per panel) for production efficiency
- V-score separation (no routing between cards)
- Tooling holes: 4x 3.2 mm at panel corners
- Fiducials: 3x 1.0 mm global, 2x 0.5 mm local per card

## 9. Design Rule Check (DRC) Summary

| Rule                          | Value              |
|-------------------------------|--------------------|
| Minimum trace width           | 3.5 mil            |
| Minimum spacing               | 3.5 mil            |
| Minimum via annular ring      | 5 mil              |
| Minimum solder mask sliver    | 4 mil              |
| Minimum silkscreen to pad     | 5 mil              |
| Minimum copper to board edge  | 20 mil             |
| Minimum drill to copper       | 8 mil              |
| Minimum hole to hole          | 12 mil             |
| Unconnected net check         | Enabled (error)    |
| Starved thermal check         | Enabled (warning)  |
| Soldermask registration check | Enabled (error)    |

## 10. Signal Integrity Simulation Plan

### 10.1 Pre-Layout Simulation (HyperLynx / SIwave)

- PCIe Gen4 channel: Extract S-parameters from proposed stackup, simulate eye diagram
  at 16 GT/s with IBIS-AMI models for Kintex-7 GTX transceivers.
- 100GbE channel: Simulate 25.78125 Gbps NRZ eye, verify >30% eye opening at receiver.
- DDR4-3200: Simulate DQ write/read eye, verify >150mV × 0.2UI eye at DRAM ball.
- PDN: Simulate Z-parameter vs frequency, verify below target impedance curves.

### 10.2 Post-Layout Verification

- Full-board 3D EM extraction (SIwave or PowerSI).
- DDR4 timing analysis with board delay + FPGA package delay + DRAM timing.
- PCIe compliance channel simulation per PCIe 4.0 CEM spec.
- Power integrity DC drop analysis: verify <2% drop on all rails.

## 11. EMI/EMC Design Considerations

- **Edge plating**: Copper plating along board edges connected to GND, creating Faraday cage.
- **Stitching vias**: GND vias every 200 mil along board perimeter.
- **Spread-spectrum clocking**: PCIe refclk supports SSC (±0.5% down-spread at 30-33 kHz).
- **QSFP28 cages**: Metal cages provide inherent shielding; connect cage to chassis GND.
- **Ferrite beads**: On all I/O leaving the board (USB, fan tach, I2C debug header).
- **Decoupling**: Aggressive decoupling network minimizes power plane resonances.
