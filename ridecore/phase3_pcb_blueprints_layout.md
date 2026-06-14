# RideCore — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

6-layer stackup on 1.6 mm total thickness, FR-4 TG-170 (Isola 370HR or equivalent).

| Layer | Type | Material | Thickness | Copper Weight | Purpose |
|---|---|---|---|---|---|
| L1 (Top) | Signal | Cu + FR-4 prepreg | 0.035 mm | 1 oz (35 µm) | Component placement, high-speed routing, phase pours |
| L2 | Ground | Cu + FR-4 core | 0.035 mm | 1 oz (35 µm) | Solid GND plane (LV side) |
| L3 | Signal/Power | Cu + FR-4 prepreg | 0.035 mm | 1 oz (35 µm) | Inner routing, power traces (3V3, 5V) |
| L4 | Power | Cu + FR-4 core | 0.035 mm | 2 oz (70 µm) | HV bus pour, phase pours (high current) |
| L5 | Ground | Cu + FR-4 prepreg | 0.035 mm | 1 oz (35 µm) | Solid GND plane (HV side) |
| L6 (Bottom) | Signal | Cu + FR-4 solder mask | 0.035 mm | 1 oz (35 µm) | Bottom-side components, phase pads |

**Prepreg dielectric constant (Dk):** 4.2 @ 1 GHz  
**Loss tangent:** 0.015 @ 1 GHz  
**Glass transition temp (Tg):** 170 °C  

### Controlled Impedance Calculations

| Trace Type | Layer | Width | Spacing | Zdiff | Z0 | Reference Plane |
|---|---|---|---|---|---|---|
| USB 90Ω diff pair | L1 | 0.15 mm | 0.10 mm | 90 Ω | 53 Ω | L2 GND |
| CAN FD 120Ω diff pair | L1 | 0.20 mm | 0.15 mm | 120 Ω | 70 Ω | L2 GND |
| BLE 50Ω single-ended | L1 | 0.38 mm | — | — | 50 Ω | L2 GND |
| High-current phase (2 oz equiv.) | L4/L6 | 5.0 mm pour | — | — | — | L5 GND |

## 2. Board Outline

```
  80.0 mm
 ┌────────────────────────────────────────┐
 │                                        │ 60.0 mm
 │   ┌──────┐          ┌──────┐           │
 │   │ U1   │          │ U5   │           │
 │   │STM32 │          │nRF   │           │
 │   └──────┘          └──────┘           │
 │                                        │
 │  ┌──────────────────────────────┐      │
 │  │     Q1  Q2   Q3  Q4  Q5 Q6  │      │
 │  │     ─────────────────────── │      │
 │  │      Power Stage (L4/L6)    │      │
 │  └──────────────────────────────┘      │
 │                                        │
 │  J1          J2     J3    J4          │
 │  PHASE      CAN    USB   HALL         │
 └────────────────────────────────────────┘
 Mounting holes: 4× M3 at corners (3.5 mm from edges)
```

- **Board size:** 80.0 mm × 60.0 mm × 1.6 mm
- **Mounting:** 4× M3 holes, 3.5 mm center-to-edge, plated (thermal vias to GND)
- **Connector placement:**
  - J1 (Phase/HV): Right edge, 15 mm from top
  - J2 (CAN): Bottom edge, 10 mm from left
  - J3 (USB-C): Bottom edge, 40 mm from left
  - J4 (Hall): Top edge, 60 mm from left

## 3. High-Speed Routing Rules

### 3.1 USB 2.0 Full-Speed (12 Mbps)
- Differential pair: USB_DP / USB_DM
- Length match: ±0.5 mm within pair
- Max via count: 2 per trace (entry/exit only)
- No 90° bends; use 45° or curved corners
- Guard trace: 0.2 mm clearance to other signals, pour GND between pair and adjacent traces
- ESD diode (D4) placed within 5 mm of connector

### 3.2 CAN FD (5 Mbps)
- Differential pair: CAN_H / CAN_L
- Length match: ±1.0 mm within pair
- Route on L1 (top) with reference to L2 GND plane
- Termination resistor (R12: 120 Ω) placed at connector entry
- TVS (D3) within 10 mm of J2 connector
- No stubs: daisy-chain if multiple nodes

### 3.3 BLE Antenna (2.4 GHz)
- 50 Ω microstrip from U5 pin 45 to inverted-F PCB antenna
- Antenna clearance: ≥ 5 mm keepout (no copper on any layer beneath antenna)
- Pi-network matching: 0.5 pF / 2.7 nH / 0.5 pF (tunable per board tune)
- Route on L1 only; no vias on RF path

### 3.4 SPI1 (20 MHz)
- SPI1_SCK, SPI1_MISO, SPI1_MOSI: length match ±2.5 mm
- Route on L1 or L3, avoid crossing power plane splits
- CS lines (CAN_CS, FLASH_CS) can be longer, no critical matching

## 4. DDR Length Matching

No DDR memory on this design. However, the SPI bus and gate drive signals have timing-critical matching requirements:

| Signal Group | Matched Length | Tolerance |
|---|---|---|
| PWM_A/B/C_HIGH (TIM1 CH1/2/3) | Equal within | ±1.0 mm |
| PWM_A/B/C_LOW (TIM1 CH1N/2N/3N) | Equal within | ±1.0 mm |
| HIGH to LOW skew (same phase) | ≤ 2.0 mm | Dead-time set in firmware, not layout |

## 5. Via Strategy

| Via Type | Drill | Pad | Purpose |
|---|---|---|---|
| Standard via | 0.3 mm | 0.6 mm | Signal routing, GND stitching |
| Power via | 0.5 mm | 1.0 mm | Power plane transitions, phase outputs |
- Thermal via array under MOSFETs: 4× 0.5 mm vias per MOSFET drain pad, filled with epoxy and copper-capped
- GND stitching vias: 0.3 mm, placed every 2.5 mm along board edges, around pours, and near connector GND pins
- No vias on high-speed differential pair traces (USB, CAN) except at layer transitions
- Via-in-pad allowed for QFN thermal pads (U1, U5) — filled and capped

## 6. Thermal Management

### 6.1 MOSFET Thermal Path
- Q1–Q6 (IPT015N10N5ATMA1): RθJA = 40 °C/W (junction to ambient with 1 sq in copper)
- Each MOSFET drain tab soldered to 5 mm × 5 mm copper pour on L4/L6
- 4× thermal vias per drain pad connecting L4 ↔ L5 ↔ L6 pours
- Top-side heatsink mounting: M2 threaded inserts at 4 points around power stage
- Thermal interface material: 0.5 mm gap pad (1.5 W/mK) between MOSFET tops and heatsink

### 6.2 Gate Driver Thermal
- U7–U9 (IRS2186): ≤ 1 W dissipation each; standard copper pour sufficient

### 6.3 PMIC Thermal
- U2 (TPS6521801): 2 W max; exposed pad (WQFN-32) with 6× thermal vias to GND

### 6.4 Board-Level Thermal Analysis
- Max ambient: 40 °C
- Max junction (MOSFETs): 150 °C → allowable ΔT = 110 °C
- At 120 A continuous: total conduction loss ≈ 120² × 1.5 mΩ × 6 = 129.6 W (all FETs)
- Switching loss at 20 kHz: ≈ 5 W per FET pair × 3 = 15 W
- Total dissipation: ~145 W → with heatsink (RθSA = 0.3 °C/W): ΔT = 145 × 0.3 = 43.5 °C
- Junction temp: 40 + 43.5 + (145 / 6 × 0.04) = 40 + 43.5 + 1.0 = 84.5 °C ✅

## 7. Clearance & Creepage

### 7.1 HV/LV Isolation Barrier
- Isolation boundary runs between digital isolators (U10–U12) and gate drivers (U7–U9)
- Minimum clearance across barrier: 3.0 mm (basic insulation at 72 V)
- Minimum creepage across barrier: 4.0 mm (pollution degree 2, material group IIIa)
- Slot (milled): 1.0 mm wide slot in PCB across isolation barrier for enhanced creepage

### 7.2 HV Internal Clearances
| Voltage | Clearance | Creepage | Standard |
|---|---|---|---|
| 60V–72V (HV bus) | 1.0 mm | 1.5 mm | IEC 60664-1, OVC II |
| 3.3V/5V (LV) | 0.1 mm | 0.2 mm | Standard PCB |

### 7.3 Phase Output Clearance
- Phase traces (L4/L6 pours): 2.0 mm minimum from HV bus and other phase pours
- Phase connector (J1): 2.5 mm pin-to-pin spacing minimum

## 8. Keepout Zones

| Zone | Location | Clearance | Reason |
|---|---|---|---|
| Antenna keepout | Top-right corner, 15×15 mm | No copper on any layer | BLE 2.4 GHz antenna |
| Isolation slot | Between U10–U12 and U7–U9 | 1.0 mm milled slot | HV/LV galvanic isolation |
| Mounting holes | 4× M3 corner positions | 1.5 mm annular ring clear of signals | Mechanical fastening, GND tie |
| Heatsink screw zone | 4× M2 positions around power stage | 0.5 mm from MOSFET pours | Mechanical mounting |

## 9. Design Rules Summary

| Rule | Value |
|---|---|
| Min trace width (LV signal) | 0.10 mm |
| Min trace width (LV power) | 0.25 mm |
| Min trace width (HV bus) | 1.0 mm |
| Phase copper pour | 5.0 mm (2 oz equivalent with L4+L6) |
| Min trace spacing (LV) | 0.10 mm |
| Min trace spacing (HV to LV) | 1.0 mm |
| Min via drill | 0.3 mm |
| Min pad size | 0.6 mm (0.3 mm drill) |
| Min solder mask dam | 0.05 mm |
| Silkscreen line width | 0.12 mm |
| Board thickness | 1.6 mm ± 0.1 mm |
| Surface finish | ENIG (Au 0.05 µm / Ni 3 µm) |
| Solder mask color | Black (power stage), Green (logic) — or uniform black |