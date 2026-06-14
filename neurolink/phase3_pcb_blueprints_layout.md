# NeuroLink — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

**8-layer, 1.6 mm total thickness, FR-4 (IT-180A) core + prepreg**

| Layer | Material          | Thickness  | Copper  | Function            |
|-------|-------------------|------------|---------|---------------------|
| L1    | Copper            | 0.035 mm   | 1 oz   | Signal (components) |
| PP1   | Prepreg 1080      | 0.10 mm    | —      | —                   |
| L2    | Copper            | 0.035 mm   | 1 oz   | GND (solid plane)   |
| Core1 | FR-4 IT-180A     | 0.20 mm    | —      | —                   |
| L3    | Copper            | 0.017 mm   | ½ oz   | Signal (high-speed) |
| PP2   | Prepreg 2116      | 0.12 mm    | —      | —                   |
| L4    | Copper            | 0.035 mm   | 1 oz   | Power (3V3, 1V8)    |
| Core2 | FR-4 IT-180A     | 0.50 mm    | —      | —                   |
| L5    | Copper            | 0.035 mm   | 1 oz   | Power (1V2, 2V5, VDDA) |
| PP3   | Prepreg 2116      | 0.12 mm    | —      | —                   |
| L6    | Copper            | 0.017 mm   | ½ oz   | Signal (DDR)         |
| Core3 | FR-4 IT-180A     | 0.20 mm    | —      | —                   |
| L7    | Copper            | 0.035 mm   | 1 oz   | GND (solid plane)   |
| PP4   | Prepreg 1080      | 0.10 mm    | —      | —                   |
| L8    | Copper            | 0.035 mm   | 1 oz   | Signal (bottom)     |

**Total: 1.6 mm ±0.1 mm**

### Impedance Control

| Layer Pair        | Target Z₀  | Trace Width | Spacing | Dielectric |
|-------------------|-----------|-------------|---------|------------|
| L1 over L2 (SE)   | 50 Ω     | 0.15 mm    | —       | 0.10 mm    |
| L1 over L2 (diff) | 100 Ω    | 0.10 mm    | 0.15 mm | 0.10 mm    |
| L1 diff (USB)     | 90 Ω     | 0.15 mm    | 0.25 mm | 0.10 mm    |
| L3 over L2/L4 (SE)| 50 Ω     | 0.12 mm    | —       | 0.10 mm    |
| L6 over L5/L7 (SE)| 50 Ω     | 0.12 mm    | —       | 0.10 mm    |
| L6 diff (DDR)     | 100 Ω    | 0.10 mm    | 0.15 mm | 0.12 mm    |

---

## 2. High-Speed Routing Rules

### 2.1 USB 2.0 High-Speed (480 Mbps)

- **Layer**: L1 only (no layer transitions if possible)
- **Impedance**: 90 Ω ±10% differential
- **Max via transitions**: 2 (one at connector, one at isolator)
- **Length matching**: DP/DM within 0.5 mm
- **Maximum trace length**: 80 mm (MCU to connector)
- **ESD devices**: Placed within 5 mm of connector
- **Guard trace**: 0.5 mm clearance from other signals, grounded via every 2 mm
- **No stubs**: ESD devices routed in-line, not branched

### 2.2 DDR3L-1600 (IS43TR16256, ×16)

- **Signal groups**: Data lanes (DQ[0:15], DQS0±, DQS1±, DM0, DM1), Address/Command, Clock, Control
- **Layers**: Data lanes on L6, Address/Command on L3, Clock diff on L6
- **Impedance**: 50 Ω single-ended, 100 Ω differential
- **Length matching**:
  - Data group DQ[0:7] to DQS0±: ±5 mil
  - Data group DQ[8:15] to DQS1±: ±5 mil
  - Address/Command to Clock: ±25 mil
  - All data groups within ±50 mil of each other
- **Slew rate**: ≥ 1 V/ns
- **Vref bypass**: 0.1 µF + 10 µF at Vref pin, center of BGA footprint
- **VTT termination**: 100 Ω to VTT at each BGA pin, 0.1 µF per 2 pins
- **Max trace length**: 50 mm (MCU to DDR3L BGA)

### 2.3 SPI Bus (ADS1299 Daisy Chain)

- **Clock frequency**: 32 MHz max
- **Layer**: L1 over L2 ground plane
- **Trace width**: 0.15 mm (50 Ω)
- **Series termination**: 33 Ω at source (MCU side)
- **Daisy chain routing**: Star from MCU CS/SCLK/MOSI, daisy-chain DOUT→DAISY_IN
- **Max stub length**: ≤ 5 mm per device
- **Length matching**: SCLK, MOSI, CS within 5 mm across all 8 devices

### 2.4 SPI Bus (iCE40 FPGA)

- **Clock frequency**: 30 MHz
- **Same rules as ADS1299 SPI, adapted for single slave**
- **Direct point-to-point routing, no stubs**

---

## 3. DDR3L Length Matching Detail

### 3.1 Byte Lane 0 (DQ[0:7] + DQS0± + DM0)

| Signal  | Net          | Pin (STM32) | Pin (DDR3L) | Target Length | Match Group |
|---------|--------------|-------------|--------------|---------------|-------------|
| DQ0     | DDR_DQ0      | PE0         | A2           | 25.0 mm       | Group 0     |
| DQ1     | DDR_DQ1      | PE1         | A3           | 25.0 mm       | Group 0     |
| DQ2     | DDR_DQ2      | PE2         | A5           | 25.0 mm       | Group 0     |
| DQ3     | DDR_DQ3      | PE3         | A7           | 25.0 mm       | Group 0     |
| DQ4     | DDR_DQ4      | PE4         | A8           | 25.0 mm       | Group 0     |
| DQ5     | DDR_DQ5      | PE5         | B2           | 25.0 mm       | Group 0     |
| DQ6     | DDR_DQ6      | PE6         | B4           | 25.0 mm       | Group 0     |
| DQ7     | DDR_DQ7      | PE7         | B6           | 25.0 mm       | Group 0     |
| DQS0+   | DDR_DQS0_P   | PE8         | B8           | 25.0 mm       | Group 0     |
| DQS0-   | DDR_DQS0_N   | PE9         | B9           | 25.0 mm       | Group 0     |
| DM0     | DDR_DM0      | PE10        | A9           | 25.0 mm       | Group 0     |

### 3.2 Byte Lane 1 (DQ[8:15] + DQS1± + DM1)

| Signal  | Net          | Pin (STM32) | Pin (DDR3L) | Target Length | Match Group |
|---------|--------------|-------------|--------------|---------------|-------------|
| DQ8     | DDR_DQ8      | PF0         | C2           | 26.0 mm       | Group 1     |
| DQ9     | DDR_DQ9      | PF1         | C3           | 26.0 mm       | Group 1     |
| DQ10    | DDR_DQ10     | PF2         | C5           | 26.0 mm       | Group 1     |
| DQ11    | DDR_DQ11     | PF3         | C7           | 26.0 mm       | Group 1     |
| DQ12    | DDR_DQ12     | PF4         | C8           | 26.0 mm       | Group 1     |
| DQ13    | DDR_DQ13     | PF5         | D2           | 26.0 mm       | Group 1     |
| DQ14    | DDR_DQ14     | PF6         | D4           | 26.0 mm       | Group 1     |
| DQ15    | DDR_DQ15     | PF7         | D6           | 26.0 mm       | Group 1     |
| DQS1+   | DDR_DQS1_P   | PF8         | D8           | 26.0 mm       | Group 1     |
| DQS1-   | DDR_DQS1_N   | PF9         | D9           | 26.0 mm       | Group 1     |
| DM1     | DDR_DM1      | PF10        | C9           | 26.0 mm       | Group 1     |

### 3.3 Address/Command/Clock Group

| Signal  | Net          | Pin (STM32) | Pin (DDR3L) | Match to Clock |
|---------|--------------|-------------|--------------|----------------|
| A0–A12  | DDR_A[0:12]  | PG0–PG12    | —            | ±25 mil        |
| BA0–BA2 | DDR_BA[0:2]  | PH0–PH2     | —            | ±25 mil        |
| CAS     | DDR_CAS_N    | PG13        | —            | ±25 mil        |
| RAS     | DDR_RAS_N    | PG14        | —            | ±25 mil        |
| WE      | DDR_WE_N     | PG15        | —            | ±25 mil        |
| CS      | DDR_CS_N     | PH3         | —            | ±25 mil        |
| CKE     | DDR_CKE      | PH4         | —            | ±25 mil        |
| CLK+    | DDR_CLK_P    | PC6 (alt)   | —            | Reference      |
| CLK-    | DDR_CLK_N    | PC7 (alt)   | —            | Reference      |
| ODT     | DDR_ODT      | PH5         | —            | ±25 mil        |
| RESET   | DDR_RESET_N  | PH6         | —            | ±25 mil        |

---

## 4. Via Strategy

### 4.1 Via Types

| Via Type   | Drill  | Pad   | Anti-Pad | Usage                          |
|-----------|--------|-------|----------|--------------------------------|
| Standard  | 0.20mm | 0.40mm| 0.50mm   | General signal routing          |
| Micro     | 0.10mm | 0.25mm| 0.35mm   | DDR BGA fanout, fine-pitch     |
| Power     | 0.30mm | 0.60mm| 0.75mm   | Power/ground transitions       |
| Thermal   | 0.40mm | 0.80mm| 1.00mm   | Power pad thermal relief        |

### 4.2 Via Placement Rules

- **No vias under BGA pads**: Route fanout with dog-bone pattern
- **BGA fanout**: Micro-via from pad → L2 (GND) or L3 (signal), then standard via to destination layer
- **Via-in-pad**: Allowed for DDR3L 96-BGA with micro-vias (filled and plated over)
- **Ground via adjacency**: Every signal via has a ground via within 1 mm (via stitching)
- **Power via clustering**: Minimum 3 power vias per power pin, clustered around pad
- **Analog isolation**: No digital signal vias in the analog section; separate ground via clusters

### 4.3 Via Stitching

- **Ground via fence**: Around analog section perimeter, every 2 mm
- **Ground via stitching**: 5 mm grid on L2/L7 across entire board
- **Power via stitching**: At every decoupling cap, via to appropriate power plane within 0.5 mm

---

## 5. Thermal Management

### 5.1 Thermal Budget

| Component     | Power (mW) | Θ_JA (°C/W) | ΔT above ambient | Max T_J |
|---------------|------------|-------------|-------------------|---------|
| STM32H743     | 800        | 30 (w/ pad) | 24°C              | 105°C   |
| ADS1299 (×8)  | 45 each=360| 85          | 30.6°C            | 105°C   |
| iCE40UP5K     | 150        | 55          | 8.3°C             | 85°C    |
| DDR3L         | 200        | 60          | 12°C              | 95°C    |
| nRF52840      | 120        | 75          | 9°C               | 85°C    |
| Total         | ~1630      | —           | —                 | —       |

### 5.2 Thermal Via Strategy

- **STM32H743**: 4×4 thermal via array (0.3mm drill) under exposed pad, connected to L2/L7 ground planes
- **ADS1299**: 2×2 thermal via array per device under exposed pad
- **DDR3L**: 3×3 thermal via array under BGA
- **Power regulators (TPS62821/22)**: 2×3 thermal via array under each IC pad, connected to ground plane

### 5.3 Copper Pour

- **Top layer (L1)**: Analog ground pour in electrode area, no pour under digital ICs
- **L2**: Solid ground plane (no splits)
- **L4**: 3V3 power pour (80% coverage) + 1V8 islands for DDR
- **L5**: 1V2 power pour (MCU core) + 2V5 island (ADS1299 VREF)
- **L7**: Solid ground plane (no splits)
- **L8**: Bottom ground pour + keepout under antenna

---

## 6. Clearance & Creepage

### 6.1 Patient-Connected Isolation (IEC 60601-1)

| Path                          | Clearance | Creepage | Method                  |
|-------------------------------|-----------|----------|-------------------------|
| Electrode inputs to USB       | 4.0 mm    | 5.0 mm   | Slot + ADuM3160 barrier |
| Electrode inputs to digital   | 2.0 mm    | 3.0 mm   | Ground plane isolation  |
| Battery to electrode inputs   | 2.0 mm    | 3.0 mm   | Keepout zone             |
| Mains (USB VBUS) to patient   | 4.0 mm    | 8.0 mm   | ADuM3160 (5kV rated)    |

### 6.2 General Clearance Rules

| Item                           | Clearance | Notes                          |
|--------------------------------|-----------|--------------------------------|
| Trace-to-trace (outer)         | 0.15 mm   | Standard signal                |
| Trace-to-trace (inner)         | 0.10 mm   | Inner layer, buried            |
| High-voltage (>30V)            | 0.50 mm   | USB VBUS isolation area        |
| BGA pitch (0.5mm)              | 0.08 mm   | Neck-down under BGA            |
| DDR3L BGA pitch (0.8mm)        | 0.12 mm   | Standard neck-down             |
| Copper pour clearance           | 0.20 mm   | From signal traces             |
| Antenna keepout                 | 5.0 mm    | No copper on any layer under antenna |

---

## 7. Board Outline

### 7.1 Dimensions

- **Overall**: 85.0 mm × 54.0 mm
- **Corner radius**: 2.0 mm (4 corners)
- **Board thickness**: 1.6 mm (±0.1 mm)
- **Copper weight**: 1 oz (35 µm) outer, ½ oz (17.5 µm) inner signal, 1 oz inner power/ground

### 7.2 Connector Placement

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  J2 (40-pin electrode)                        J3 (40-pin)  │
│  ┌──────────────────┐                        ┌──────────┐  │
│  │ ●●●●●●●●●●●●●●●●│                        │●●●●●●●●●●│  │
│  │ ●●●●●●●●●●●●●●●●│                        │●●●●●●●●●●│  │
│  └──────────────────┘                        └──────────┘  │
│                                                             │
│   ┌───────────────────────────────────────────┐            │
│   │              STM32H743                     │            │
│   │              (LQFP-144)                    │            │
│   └───────────────────────────────────────────┘            │
│                                                             │
│  ┌──────┐  ┌──────────┐  ┌─────────┐  ┌────────┐         │
│  │ADS ×8│  │iCE40UP5K │  │ DDR3L   │  │nRF52840│         │
│  │(analog│  │(DSP)     │  │(512Mb)  │  │(BLE)   │         │
│  │ zone)│  └──────────┘  └─────────┘  └────────┘         │
│  └──────┘                                                  │
│                                                             │
│  ┌──────────┐    ┌────┐    ┌─────┐    ┌──────┐            │
│  │PMIC area │    │IMU │    │Temp │    │RGB×4 │            │
│  └──────────┘    └────┘    └─────┘    └──────┘            │
│                                                             │
│         ┌──────┐                               ┌────────┐ │
│         │µSD   │                               │USB-C   │ │
│         │slot  │                               │(J1)    │ │
│         └──────┘                               └────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 7.3 Keepout Zones

1. **Analog Keepout**: Region from left edge to 25mm in, L1 only analog signals, no digital routing on any layer
2. **USB Isolation Keepout**: 5mm slot between ADuM3160 primary and secondary sides, extending through all layers
3. **Antenna Keepout**: 5mm radius around nRF52840 antenna trace, no copper on any layer
4. **Crystal Keepout**: No high-speed traces within 3mm of any crystal
5. **Battery Keepout**: No traces or vias under battery connector footprint (L8 only)

### 7.4 Mounting Holes

- 4× M2 mounting holes at corners (inside corner radius)
- Hole diameter: 2.2 mm
- Annular ring: 4.0 mm
- Connected to ground plane through via stitching

---

## 8. Design Rules Summary

| Parameter                     | Value                                    |
|-------------------------------|------------------------------------------|
| Minimum trace width            | 0.10 mm (0.08 mm under BGA)              |
| Minimum trace spacing          | 0.10 mm (0.08 mm under BGA)              |
| Minimum via drill              | 0.10 mm (micro), 0.20 mm (standard)      |
| Minimum via pad                | 0.25 mm (micro), 0.40 mm (standard)       |
| Minimum via anti-pad            | 0.35 mm (micro), 0.50 mm (standard)       |
| Solder mask clearance           | 0.05 mm (0.03 mm for fine pitch)         |
| Solder mask web                 | 0.08 mm minimum                           |
| Silkscreen line width          | 0.12 mm                                   |
| Silkscreen text height         | 0.80 mm minimum                           |
| Board edge copper clearance    | 0.50 mm                                   |
| Internal layer copper clearance | 0.20 mm                                   |
| Differential pair tolerance     | ±10% impedance (measured)                 |
| Length matching tolerance       | ±5 mil (DDR), ±2 mm (SPI)                |
| Maximum via count per net      | 4 (USB), 6 (DDR), 2 (SPI)                |