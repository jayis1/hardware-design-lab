# Chronos-RTK — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

6-layer stackup (1.6 mm total thickness, ENIG finish, 1 oz copper outer / 0.5 oz inner):

| Layer | Material | Thickness | Copper | Role |
|---|---|---|---|---|
| L1 (Top) | — | 0.035 mm | 1 oz | Component placement, signal routing, RF traces |
| PP1 | FR-4 370HR | 0.10 mm | — | Prepreg |
| L2 | — | 0.0175 mm | 0.5 oz | GND plane (unbroken reference for L1 RF) |
| Core1 | FR-4 370HR | 0.50 mm | — | Core |
| L3 | — | 0.0175 mm | 0.5 oz | VDD_3V3 power plane + splits |
| PP2 | FR-4 370HR | 0.10 mm | — | Prepreg |
| L4 | — | 0.0175 mm | 0.5 oz | VDD_1V8 / VSRC power splits |
| Core2 | FR-4 370HR | 0.50 mm | — | Core |
| PP3 | FR-4 370HR | 0.10 mm | — | Prepreg |
| L5 | — | 0.0175 mm | 0.5 oz | GND plane (unbroken reference for L6) |
| PP4 | FR-4 370HR | 0.10 mm | — | Prepreg |
| L6 (Bottom) | — | 0.035 mm | 1 oz | Component placement (BME280, buttons), signal routing |

**Dielectric constant (Dk)**: 4.4 (370HR at 1 GHz)  
**Loss tangent**: 0.013  
**Impedance target**: 50 Ω single-ended, 90 Ω differential

## 2. Board Outline

- **Dimensions**: 70 mm × 45 mm  
- **Corners**: 4 mounting holes, M2.5, 3.5 mm from edges  
- **Connector placement**:
  - J1 (USB-C): top edge, center, flush
  - J2 (SMA LoRa): bottom-right, edge-mount
  - J3 (MCX GNSS): bottom-left, edge-mount
  - J4 (JST-PH battery): right edge
  - OLED flex: left edge, ZIF connector
- **Keepout**: 5 mm border around GNSS antenna connector, no copper fill

## 3. High-Speed Routing Rules

### 3.1 USB 2.0 FS (12 Mbps)

| Parameter | Value |
|---|---|
| Impedance | 90 Ω ±10% differential |
| Pair gap | 0.15 mm |
| Trace width | 0.20 mm (each) |
| Length matching | ΔL ≤ 150 mils (3.8 mm) |
| ESD protection | D1 (TVS) within 5 mm of connector |
| Via count | ≤ 2 per signal |
| Layer | L1 only, referenced to L2 GND |

### 3.2 SPI1 (50 MHz to W25Q128)

| Parameter | Value |
|---|---|
| Trace width | 0.15 mm |
| Spacing | 0.15 mm minimum |
| Length matching | ΔL ≤ 500 mils across all 4 SPI signals |
| Layer | L1 preferred, L6 allowed with stitched GND vias |
| Series termination | 22 Ω on CLK, MOSI near MCU |

### 3.3 SPI2 (16 MHz to SX1262)

| Parameter | Value |
|---|---|
| Trace width | 0.15 mm |
| Spacing | 0.15 mm |
| Series termination | 33 Ω on SCK, MOSI near MCU |

### 3.4 UART1 (460800 baud)

| Parameter | Value |
|---|---|
| Trace width | 0.15 mm |
| Series termination | 100 Ω near source |
| EMI consideration | Route away from RF traces, parallel ground trace |

## 4. RF Routing

### 4.1 GNSS RF Path (MCX → ZED-F9P)

- **Trace**: Coplanar waveguide, 50 Ω
- **Width**: 0.35 mm (calculated for stackup, εr=4.4, 0.1 mm prepreg to L2 GND)
- **Side gap**: 0.25 mm (coplanar ground on L1)
- **Length**: Minimize, target ≤ 15 mm
- **DC block**: C10 (56 pF C0G, 0402) series, near RF_IN pin
- **ESD**: GNSS-specific TVS (PRTR5V0U2X) at connector
- **Stitching vias**: Every 0.5 mm along coplanar GND, connecting L1 fence to L2/L5 GND

### 4.2 LoRa RF Path (SX1262 → SMA)

- **Trace**: Coplanar waveguide, 50 Ω
- **Width**: 0.35 mm
- **Matching**: π-network (C1=2.2 pF, L1=3.3 nH, C2=2.7 pF) at 0402, placed within 3 mm of IC
- **TX power path**: RFI_HP pin through matching → SMA
- **Antenna switch**: DIO2 controls internal MOSFET switch (no external switch needed for single antenna)
- **Stitching vias**: Every 0.5 mm along coplanar GND

## 5. DDR Length Matching

Not applicable — no DDR memory on this board. SPI flash is asynchronous and does not require length matching beyond reasonable skew (< 1 ns).

## 6. Via Strategy

| Via Type | Drill | Pad | Usage |
|---|---|---|---|
| Standard via | 0.2 mm | 0.4 mm | Signal routing, power |
| GND stitching via | 0.2 mm | 0.4 mm | RF ground stitching, plane connection |
| Thermal via | 0.3 mm | 0.6 mm | Under TPS62A02, ZED-F9P thermal pads |
| Via-in-pad | 0.1 mm | 0.25 mm | BGA-style escapes (not used; LQFP/QFN only) |

**Via rules**:
- No vias on RF traces (maintain on L1 only)
- No vias on differential USB pair except at pin escape (max 2 per signal)
- GND stitching vias: placed every 2 mm along RF trace coplanar ground
- Thermal vias: 4× under TPS62A02, 9× under ZED-F9P center pad
- All power pins transition to power plane within 1 mm of IC pin

## 7. Thermal Management

| Component | Power Dissipation | Cooling Method |
|---|---|---|
| ZED-F9P | ~300 mW | 9× thermal vias to L2 GND pour; board acts as heatsink |
| TPS62A02 | ~150 mW (eff ~92%) | 4× thermal vias under pad; copper pour on L1 |
| SX1262 | ~440 mW (TX @ +22 dBm) | 6× thermal vias under pad; thermal relief on L2 |
| STM32G474 | ~60 mW | Minimal; copper pour on L1 sufficient |

**Thermal via pattern under ZED-F9P**:
```
  ○  ○  ○
  ○  ○  ○
  ○  ○  ○
```
0.8 mm pitch, connecting center ground pad to L2/L5 ground planes.

## 8. Clearance & Creepage

| Parameter | Value | Standard |
|---|---|---|
| Minimum trace spacing | 0.10 mm (4 mil) | IPC-2221B |
| Minimum trace width | 0.10 mm (4 mil) | IPC-2221B |
| USB connector clearance | 0.5 mm | USB-IF spec |
| Mains isolation | N/A (≤ 5 V system) | — |
| Creepage (5 V domain) | 0.5 mm | IPC-2221B, pollution degree 2 |
| RF trace clearance to non-RF | 1.0 mm minimum | Minimize coupling |
| Antenna keepout | 5 mm radius, no copper | Prevent detuning |

## 9. Ground Plane Strategy

- **L2**: Solid ground plane, no splits. Reference for L1 signal traces and RF.
- **L5**: Solid ground plane, no splits. Reference for L6 signal traces.
- **L3**: VDD_3V3 power plane with splits for VSRC, VDD_1V8 where needed. 3.3 V pour covers 80% of area.
- **L4**: VSRC (5 V) pour near USB connector; VDD_1V8 pour near ZED-F9P.
- **Stitching**: Ground planes L2 and L5 stitched with vias every 3 mm grid across the board.

## 10. Design for Manufacturing

| Parameter | Value |
|---|---|
| Minimum drill | 0.2 mm |
| Minimum annular ring | 0.1 mm |
| Solder mask | LPI green, 0.065 mm dam |
| Silkscreen | White on L1/L6, 0.12 mm line min |
| Surface finish | ENIG (Au 0.03 µm / Ni 3 µm over Cu) |
| Board thickness | 1.6 mm ±10% |
| Copper weight | 1 oz outer, 0.5 oz inner |
| IPC class | Class 2 |
| Assembly | SMT only, no wave solder (bottom components ≤ 5) |
| Panel | 2-up with mousebites, 3 mm rail |
| Reflow | Lead-free SAC305, 260 °C peak |

## 11. Placement Guidelines

```
┌────────────────────────────────────────────────────┐
│                    TOP SIDE (L1)                   │
│                                                    │
│  ┌──────────┐        ┌────────────────┐           │
│  │  USB-C   │        │   STM32G474    │           │
│  │  J1      │        │   U2 (LQFP-64) │           │
│  └──────────┘        └────────────────┘           │
│                                                    │
│  ┌──────────────┐  ┌──────┐  ┌──────────────┐   │
│  │ TPS62A02 U6  │  │Y2 32M│  │  W25Q128 U4  │   │
│  └──────────────┘  └──────┘  └──────────────┘   │
│                                                    │
│  ┌──────────────────────────┐  ┌──────────────┐  │
│  │    ZED-F9P U1            │  │  SX1262 U3   │  │
│  │    (LGA 24, center-pad) │  │  (QFN-24)    │  │
│  └──────────────────────────┘  └──────┬───────┘  │
│                                          │          │
│  ┌──────┐  ┌─────────┐  ┌───────┐   ┌──▼──┐    │
│  │TLV   │  │MCP73871 │  │BME280 │   │LoRa │    │
│  │U7    │  │U8        │  │U9     │   │π-net│    │
│  └──────┘  └─────────┘  └───────┘   └─────┘    │
│                                                    │
│  [MCX]                                    [SMA]    │
│  GNSS                                      LoRa    │
└────────────────────────────────────────────────────┘
```

**Key placement rules**:
1. GNSS RF trace: MCX connector → shortest path → ZED-F9P RF_IN (≤ 15 mm)
2. LoRa RF trace: SX1262 RFI_HP → matching → SMA connector (≤ 20 mm)
3. Crystal Y2 (32 MHz): ≤ 5 mm from STM32, guard ring of GND vias
4. Crystal Y3 (32 MHz TCXO): ≤ 5 mm from SX1262 TCXO_IN pin
5. Decoupling caps: directly adjacent to IC power pins
6. USB ESD (D1): within 5 mm of USB-C connector