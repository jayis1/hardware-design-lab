# HexaScope — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

6-layer stackup, 1.6 mm total thickness, ENIG finish (ENIG over copper, 1 μm Au / 3–5 μm Ni).

| Layer | Material | Thickness | Copper Weight | Function |
|---|---|---|---|---|
| L1 (Top) | — | 0.035 mm | 1 oz (35 μm) | Signal — Analog, LVDS, components |
| PP1 | Isola 370HR | 0.10 mm | — | Prepreg |
| L2 | — | 0.035 mm | 1 oz (35 μm) | Ground plane (continuous) |
| Core1 | Isola 370HR | 0.50 mm | — | Core |
| L3 | — | 0.035 mm | 1 oz (35 μm) | Signal — DDR3 routing, FPGA breakout |
| PP2 | Isola 370HR | 0.10 mm | — | Prepreg |
| L4 | — | 0.035 mm | 1 oz (35 μm) | Power plane (1.1V, 1.35V, 1.8V splits) |
| Core2 | Isola 370HR | 0.50 mm | — | Core |
| L5 | — | 0.035 mm | 1 oz (35 μm) | Ground plane (continuous) |
| PP3 | Isola 370HR | 0.10 mm | — | Prepreg |
| L6 (Bottom) | — | 0.035 mm | 1 oz (35 μm) | Signal — USB, FPGA breakout, passives |

**Total**: 0.035 + 0.10 + 0.035 + 0.50 + 0.035 + 0.10 + 0.035 + 0.50 + 0.035 + 0.10 + 0.035 = **1.51 mm** (within 1.6 mm ± 10% tolerance)

### Impedance Targets

| Trace Type | Layer | Z₀ (single) | Z₀_diff | Trace Width | Trace Spacing |
|---|---|---|---|---|---|
| LVDS (100 Ω diff) | L1 | 50 Ω | 100 Ω | 0.10 mm | 0.15 mm |
| DDR3 (100 Ω diff) | L3 | 50 Ω | 100 Ω | 0.10 mm | 0.15 mm |
| USB SS (90 Ω diff) | L6 | 45 Ω | 90 Ω | 0.10 mm | 0.12 mm |
| DDR3 DQ (50 Ω single) | L3 | 50 Ω | — | 0.10 mm | — |
| CMOS (50 Ω single) | L1, L6 | 50 Ω | — | 0.15 mm | — |

Note: Impedance calculated with Er = 4.2 (Isola 370HR), using L2/L5 as reference planes.

---

## 2. High-Speed Routing Rules

### 2.1 LVDS (ADC → FPGA) — 250 MHz

- **Route on L1 only** with L2 as continuous reference ground.
- **Differential pair routing**: tightly coupled, length-matched within ±5 ps (±0.8 mm).
- **Within-pair skew**: ≤ 0.2 mm between P/N traces.
- **Between-pair skew**: all 9 pairs per ADC group matched within ±2 mm.
- **Via transitions**: max 2 per net, place ground stitching vias within 0.5 mm of each signal via.
- **No routing over split planes**.
- **Series termination**: 33 Ω resistors placed within 5 mm of ADC output pins.
- **LVDS bias**: 100 Ω termination between P/N at FPGA input, placed within 3 mm of FPGA ball.

### 2.2 DDR3L — 400 MHz (800 MT/s)

- **Route on L3 only** with L2 and L4 as reference planes.
- **Address/command group**: length-matched within ±2.5 mm of clock.
- **Data group (DQ[0:7] + DQS0 + DM0)**: length-matched within ±1 mm within the byte lane.
- **Data group (DQ[8:15] + DQS1 + DM1)**: length-matched within ±1 mm within the byte lane.
- **Between byte lanes**: skew between byte lanes ≤ ±2.5 mm.
- **Clock differential pair**: length-matched within ±0.2 mm.
- **Fly-by topology** for address/command: T-branch is NOT used; address/command signals route in a daisy-chain from FPGA → DDR3 with a short stub at the DDR3 (≤ 5 mm).
- **VREF routing**: 0.2 mm trace, isolated from switching signals, bypassed at both ends.
- **ODT**: set to 60 Ω in DDR3 mode register.

### 2.3 USB 3.0 SuperSpeed (90 Ω diff)

- **Route on L6 only** with L5 as continuous reference ground.
- **Differential pair**: tightly coupled, length-matched within ±0.1 mm.
- **AC coupling capacitors**: 0.01 µF (10 nF), placed symmetrically, within 10 mm of USB connector.
- **ESD protection**: ESD7004 placed within 5 mm of USB-C connector.
- **No vias** between connector and ESD/ISO7740 on SuperSpeed lines.
- **Via transition to ULPI**: only after digital isolator; use ground stitching vias.

### 2.4 General Rules

- **Minimum trace width**: 0.10 mm (4 mil) for signal, 0.25 mm for power.
- **Minimum drill**: 0.20 mm (8 mil) for signal vias, 0.30 mm for power vias.
- **Via annular ring**: 0.15 mm minimum.
- **Trace-to-trace spacing**: 0.10 mm minimum for same-net, 0.15 mm for different-net, 0.50 mm for high-speed differential to other signals.
- **Ground via stitching**: every 2 mm along high-speed routes, every 5 mm elsewhere.
- **No 90° bends**: use 45° miters or rounded corners.

---

## 3. DDR3 Length Matching

### 3.1 Signal Groups and Length Targets

| Group | Signals | Target Length | Tolerance |
|---|---|---|---|
| Clock | CLK_P, CLK_N | 45 mm | ±0.2 mm within pair |
| Address/Command | A[0:14], BA[0:2], CAS_N, RAS_N, WE_N, CS_N, CKE, ODT | 48 mm (fly-by) | ±2.5 mm to clock |
| Data Lane 0 | DQ[0:7], DQS0_P, DQS0_N, DM0 | 40 mm | ±1 mm within lane |
| Data Lane 1 | DQ[8:15], DQS1_P, DQS1_N, DM1 | 42 mm | ±1 mm within lane |

### 3.2 Length Matching Implementation

- **Serpentine routing** for shorter signals to match longest in each group.
- **Serpentine dimensions**: amplitude ≥ 3× trace width, length per serpentine bend ≤ 1.5 mm.
- **Data lane matching**: Each byte lane is independently matched; byte-to-byte skew absorbed by controller read leveling.

---

## 4. Via Strategy

| Via Type | Drill | Pad | Anti-Pad | Use |
|---|---|---|---|---|
| Signal via | 0.20 mm | 0.40 mm | 0.50 mm | General signal routing |
| Power via | 0.30 mm | 0.60 mm | 0.75 mm | Power/ground connections |
| Thermal via | 0.30 mm | 0.60 mm | 0.75 mm | Under thermal pads (FPGA, PMIC) |

### Via Placement Rules

1. **FPGA fanout**: Use dog-bone fanout on L1, then via to L3 for DDR3 signals. Maximum via stub length: L1→L3 (2 layers), no back-drilling required at 400 MHz.
2. **ADC fanout**: Route on L1 directly to FPGA; avoid vias if possible. If vias needed, use L1→L3 transition with ground via stitching.
3. **Power via count**: At least 4 power vias per FPGA VCC_CORE ball, 2 per other power ball.
4. **Ground via density**: Minimum 1 ground via per 3 mm² under FPGA thermal pad, plus perimeter stitching.
5. **DDR3**: Via-in-pad for BGA (0.8 mm pitch), filled and capped vias for DQ signals.

---

## 5. Thermal Management

### 5.1 Thermal Budget

| Component | Power (W) | θ_JA (°C/W) | Max T_J (°C) | Expected T_J @ 50°C ambient |
|---|---|---|---|---|
| U1 (FPGA) | 3.0 | 12 (with thermal pad) | 100 | 86°C |
| U3–U6 (ADC ×4) | 1.2 × 4 = 4.8 | 35 each | 105 | 92°C |
| U9 (DDR3L) | 0.5 | 40 | 95 | 70°C |
| U10 (USB PHY) | 0.3 | 45 | 85 | 64°C |
| U11 (PMIC) | 0.2 (dissipation) | 30 | 125 | 56°C |
| U2 (STM32) | 0.2 | 40 | 105 | 58°C |
| U13 (ESP32-C6) | 0.5 | 35 | 105 | 68°C |
| **Total** | **~9.5 W** | | | |

### 5.2 Thermal Mitigation

1. **FPGA**: Exposed thermal pad (8 mm × 8 mm) on bottom of BGA, soldered to copper pour on L2 with 16 thermal vias (0.3 mm drill) connecting to L5 ground pour.
2. **ADC**: Thermal pad connected to ground pour with 4 thermal vias per device.
3. **PMIC**: Thermal pad connected to ground pour with 9 thermal vias.
4. **Board-level**: Top-side copper pour on L1 (unused area) connected to ground via stitching. Bottom-side copper pour on L6 connected similarly.
5. **Airflow**: Passive convection through vent slots in aluminum enclosure. Estimated 0.5 m/s natural convection.
6. **Thermal interface**: 0.5 mm thermal pad between FPGA BGA pad and aluminum enclosure lid for heatsinking.

### 5.3 Thermal Via Pattern (FPGA)

```
┌─────────────────────────────────────────┐
│  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  │  ← Ground stitching vias
│  ○  ┌─────────────────────┐  ○        │
│  ○  │ ●  ●  ●  ●  ●  ●  ● │  ○        │  ← Thermal vias under
│  ○  │ ●  ●  ●  ●  ●  ●  ● │  ○        │     BGA thermal pad
│  ○  │ ●  ●  ●  ●  ●  ●  ● │  ○        │     (4×4 grid, 0.3mm drill)
│  ○  │ ●  ●  ●  ●  ●  ●  ● │  ○        │
│  ○  └─────────────────────┘  ○        │
│  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  │
└─────────────────────────────────────────┘
```

---

## 6. Clearance & Creepage

### 6.1 Mains Input Protection (BNC Channels)

| Parameter | Value | Standard |
|---|---|---|
| Rated voltage (CAT II) | 300 Vrms | IEC 61010-1 |
| Clearance (basic insulation) | 3.0 mm | IEC 61010-1, Table 3 |
| Creepage (basic insulation) | 4.0 mm | IEC 61010-1, Table 5 |
| Clearance (reinforced, USB isolation) | 6.0 mm | IEC 61010-1 |
| Creepage (reinforced, USB isolation) | 8.0 mm | IEC 61010-1 |

### 6.2 Implementation

- **BNC connector area**: 4 mm slot cut in PCB between BNC ground shells and adjacent low-voltage circuitry. BNC connectors use isolated mount (non-conductive bushings).
- **Attenuator relay area**: 3 mm minimum clearance between high-voltage traces (input divider) and low-voltage traces (op-amp inputs). Routing on opposite sides of slot.
- **USB isolation barrier**: 8 mm creepage slot between ISO7740 primary and secondary sides. No copper on any layer in the isolation slot zone (6 mm wide, L1 through L6).
- **ESD protection devices**: Placed within 3 mm of connector pins, on the connector side of isolation barrier.

### 6.3 Creepage Slot

```
                    Isolation Zone
           ┌─────────────────────────────┐
           │   (no copper on any layer)  │
           │                             │
    USB    │   8mm creepage slot         │  FPGA/USB PHY
    Side   │                             │  Side
  (host)   │   ISO7740 straddles slot    │  (device)
           │                             │
           └─────────────────────────────┘
```

---

## 7. Board Outline

### 7.1 Dimensions

- **PCB size**: 160 mm × 100 mm
- **Corner radius**: 2 mm (4 corners)
- **Mounting holes**: 4× M2.5, at (5, 5), (155, 5), (5, 95), (155, 95)
- **Thickness**: 1.6 mm ± 0.1 mm

### 7.2 Connector Placement

```
                    FRONT (connector side)
        ┌──────────────────────────────────────┐
        │  BNC1    BNC2    BNC3    BNC4        │
        │  ○       ○       ○       ○           │
        │                                       │
        │  MCX1         MCX2                    │
        │  ○            ○                       │
        │                                       │
        │    ┌──────────┐  ┌──────┐             │
        │    │   FPGA   │  │ DDR3 │             │
        │    └──────────┘  └──────┘             │
        │                                       │
        │   ┌─────┐  ┌─────┐  ┌────┐           │
        │   │ADC1 │  │ADC2 │  │ADC3│ ADC4      │
        │   └─────┘  └─────┘  └────┘ ┌────┐   │
        │                               │ADC4│  │
        │                               └────┘  │
        │                                       │
  LEFT  │   ┌──────┐                ┌──────┐   │  RIGHT
        │   │STM32 │                │USB-C │   │
        │   └──────┘                └──────┘   │
        │                              ○ SMA   │
        │                           ┌──────┐   │
        │                           │microSD│  │
        │                           └──────┘   │
        │                                       │
        │   [BTN] [BTN] [BTN] [BTN]  [LED]×4   │
        │                                       │
        └──────────────────────────────────────┘
                    REAR (USB-C, SMA, SD)
```

### 7.3 Keepout Zones

| Zone | Description | Restriction |
|---|---|---|
| Z1 | Analog input area (BNC to ADC) | No digital signals, no power planes except ground |
| Z2 | Isolation barrier (ISO7740) | No copper on any layer, 8 mm wide |
| Z3 | Crystal area (Y1, Y2) | No high-speed signals within 3 mm, ground pour on L2 |
| Z4 | USB connector area | No high-speed signals except USB, ground pour |
| Z5 | DDR3 zone | Controlled-length routing only, no unrelated signals |
| Z6 | FPGA BGA area | No traces on L1 under BGA (fanout only), via-in-pad |

---

## 8. Power Plane Split (L4)

Layer 4 (power plane) is split into regions:

```
┌─────────────────────────────────────────────────┐
│          VDD_CORE (1.1V)                        │
│   (FPGA core supply, left 60% of board)          │
│                                                  │
│   ┌──────────────────────────────────┐           │
│   │                                  │           │
│   │     VDD_DDR (1.35V)              │           │
│   │     (DDR3 area, center)          │           │
│   │                                  │           │
│   └──────────────────────────────────┘           │
│                                                  │
│          VDD_IO18 (1.8V)                         │
│   (ADC/FPGA IO, right 40% of board)              │
│                                                  │
└─────────────────────────────────────────────────┘

L4 is split into 3 islands:
- VDD_CORE (1.1V): Left 55% — powers FPGA core, bulk caps
- VDD_DDR (1.35V): Center 20% — powers DDR3L, VREF divider
- VDD_IO18 (1.8V): Right 25% — powers ADC digital IO, FPGA IO banks

Split gaps: 0.5 mm minimum between islands.
Each island has star-point connection to DA9063 output.
```

L5 is a continuous ground plane with no splits (except isolation barrier zone).

---

## 9. Design for Manufacturing (DFM) Notes

1. **Minimum trace/space**: 0.10 mm / 0.10 mm (4/4 mil) — achievable at most PCB fabs.
2. **Minimum drill**: 0.20 mm (8 mil) — standard capability.
3. **Via-in-pad**: Required for FPGA BGA (0.8 mm pitch) and DDR3 BGA (0.65 mm pitch). Non-filled micro-vias for signal, filled/capped for power and DQ.
4. **Solder mask**: Green, LPI, 0.10 mm over copper.
5. **Silkscreen**: White, top and bottom, minimum 0.10 mm line width.
6. **Surface finish**: ENIG (1 μm Au / 3–5 μm Ni) — required for BGA and fine-pitch components.
7. **Panelization**: V-score, single panel, 160 × 100 mm.
8. **Electrical testing**: 100% netlist comparison, flying probe.
9. **Impedance testing**: Coupon included on panel edge, measured with TDR.
10. **IPC class**: Class 2 (for test & measurement equipment).