# CyberGuard — Phase 3: PCB Blueprints & Layout

## 1. PCB Stackup

**6-layer stackup, 1.0 mm total thickness (ultra-thin dongle form factor)**

| Layer | Material | Thickness | Copper Weight | Function |
|-------|----------|-----------|--------------|----------|
| L1 (Top) | — | 0.035 mm | 1 oz (35 µm) | Signal + Component Placement |
| PP1 | FR-4 (TG150) | 0.10 mm | — | Prepreg |
| L2 | — | 0.0175 mm | ½ oz (17.5 µm) | Ground Plane (GND) |
| Core1 | FR-4 (TG150) | 0.20 mm | — | Core |
| L3 | — | 0.0175 mm | ½ oz (17.5 µm) | Power Plane (VDD_3V3) |
| PP2 | FR-4 (TG150) | 0.10 mm | — | Prepreg |
| L4 | — | 0.0175 mm | ½ oz (17.5 µm) | Signal (high-speed routing) |
| Core2 | FR-4 (TG150) | 0.20 mm | — | Core |
| L5 | — | 0.0175 mm | ½ oz (17.5 µm) | Ground Plane (GND) |
| PP3 | FR-4 (TG150) | 0.10 mm | — | Prepreg |
| L6 (Bottom) | — | 0.035 mm | 1 oz (35 µm) | Signal + Component Placement |

**Total: 1.0 mm ± 0.1 mm**

**Impedance targets:**
- L1-L2 microstrip: 50Ω single-ended (trace width ~4.5 mil for 0.10mm prepreg)
- L4 stripline: 90Ω differential USB (trace width ~4 mil, spacing ~5 mil)
- L1 BLE/NFC antenna feed: 50Ω coplanar waveguide

## 2. Board Outline

```
┌────────────────────────────────────────────────────────────────┐
│                         48 mm                                   │
│   ┌────────────────────────────────────────────────────────┐    │
│   │                                                        │    │
│   │  ┌──────┐                          ┌──────────────────┐│    │
│   │  │      │                          │                  ││    │
│   │  │ USB-C│                          │  CR2032 Battery  ││24  │
│   │  │ Conn │                          │  Holder          ││mm  │
│   │  │      │                          │                  ││    │
│   │  └──────┘                          └──────────────────┘│    │
│   │                                                        │    │
│   │  ┌────────────┐    ┌──────────┐    ┌──────────────┐   │    │
│   │  │ STM32L562  │    │ A71CH    │    │ PN7150 NFC   │   │    │
│   │  │ (Main MCU) │    │ (SE)     │    │ Controller   │   │    │
│   │  └────────────┘    └──────────┘    └──────────────┘   │    │
│   │                                                        │    │
│   │  ┌──────────┐  ┌───────────┐  ┌─────┐  ┌──────────┐ │    │
│   │  │ nRF52833 │  │ MX25R16   │  │ LEDs│  │ FPC1025  │ │    │
│   │  │ (BLE)    │  │ (Flash)   │  │ ×3  │  │ (FP)     │ │    │
│   │  └──────────┘  └───────────┘  └─────┘  └──────────┘ │    │
│   │                                                        │    │
│   │  ┌──────────────────────────────────────────────────┐ │    │
│   │  │         NFC Antenna (30mm diameter)              │ │    │
│   │  └──────────────────────────────────────────────────┘ │    │
│   │                                                        │    │
│   │   ┌──────┐                                             │    │
│   │   │BTN  │                                             │    │
│   │   └──────┘                                             │    │
│   └────────────────────────────────────────────────────────┘    │
│                              ▼                                  │
│                    Keychain attachment hole                     │
└────────────────────────────────────────────────────────────────┘
```

**Dimensions: 44 mm × 20 mm PCB (48 mm × 24 mm overall with enclosure)**
**Corners: 2 mm radius**
**Keychain hole: 4 mm diameter, centered 3 mm from bottom edge**

## 3. High-Speed Routing Rules

### 3.1 USB 2.0 Full-Speed (12 Mbps)

| Parameter | Value |
|-----------|-------|
| Differential impedance | 90Ω ± 10% |
| Trace width | 4.0 mil (0.10 mm) |
| Trace spacing | 5.0 mil (0.127 mm) |
| Max trace length | 100 mm (actual ~15 mm) |
| Length matching | Within 0.5 mm |
| ESD protection | Place within 5 mm of connector |
| Via count | Max 2 per signal |
| Guard ground | Surround with GND pour, stitch vias every 2 mm |

### 3.2 SPI (NFC Controller)

| Parameter | Value |
|-----------|-------|
| Clock frequency | 10 MHz |
| Trace length | < 30 mm |
| Series termination | 22Ω on SCK |
| Length matching | Within 2 mm across all SPI lines |

### 3.3 QSPI Flash

| Parameter | Value |
|-----------|-------|
| Clock frequency | 80 MHz |
| Trace length | < 20 mm |
| Length matching | Within 3 mm across all QSPI lines |
| Impedance | 50Ω single-ended target |

### 3.4 BLE Antenna (2.4 GHz)

| Parameter | Value |
|-----------|-------|
| Trace impedance | 50Ω coplanar waveguide |
| PI matching network | Per nRF52833 reference design (C, L values) |
| Antenna | Meandered inverted-F on PCB edge |
| Clearance | No copper within 5 mm of antenna |
| Keepout | No traces on layers beneath antenna |

### 3.5 NFC Antenna (13.56 MHz)

| Parameter | Value |
|-----------|-------|
| Type | Circular loop, 30 mm diameter |
| Inductance target | 1.0–1.5 µH |
| Q factor | 15–30 |
| Tuning caps | 27 pF + 56 pF parallel (per PN7150 design guide) |
| Trace width | 0.8 mm |
| Clearance | 5 mm from metal components |

## 4. DDR Length Matching

N/A — no DDR interface on this design.

## 5. Via Strategy

| Via Type | Drill | Pad | Anti-pad | Usage |
|----------|-------|-----|----------|-------|
| Standard | 0.2 mm | 0.4 mm | 0.6 mm | General signal routing |
| Power | 0.3 mm | 0.6 mm | 0.8 mm | Power/ground connections |
| Thermal | 0.3 mm | 0.6 mm | 0.8 mm | IC ground/power pad connections |

**Via rules:**
- Max 2 vias on any high-speed signal (USB_DP/DN)
- Stitching vias around USB differential pairs every 2 mm
- Ground stitching vias every 3 mm along board edges
- No vias on BLE antenna trace (route on L1 only)
- NFC antenna routed on L6 (bottom) with vias at feed points only
- Via-in-pad allowed for QFN center pads (0.3 mm via, filled and capped)

## 6. Thermal Management

| Component | Power Dissipation | Thermal Solution |
|-----------|------------------|------------------|
| STM32L562 | ~50 mW (active) | Copper pour under IC, 4× thermal vias |
| PN7150 | ~150 mW (TX active) | Larger copper pour, 6× thermal vias to L2/L5 |
| nRF52833 | ~60 mW (TX active) | Copper pour, 4× thermal vias |
| A71CH | ~10 mW | Minimal — no special thermal |
| STPMIC1 | ~80 mW (buck) | Copper pour, 4× thermal vias |
| FPC1025 | ~30 mW | Minimal — no special thermal |

**Thermal vias:** 0.3 mm drill, placed in rows under exposed pads, connecting L2/L5 ground planes.

**Overall thermal resistance:** < 40 °C/W (PCB only, no heatsink)

**Max junction temperatures:**
- STM32L562: 105 °C
- PN7150: 125 °C
- nRF52833: 105 °C
- All others: 85–125 °C

## 7. Clearance & Creepage

| Parameter | Value |
|-----------|-------|
| Minimum trace spacing (signal) | 0.10 mm (4 mil) |
| Minimum trace spacing (power) | 0.20 mm (8 mil) |
| USB-C connector creepage | 0.5 mm min ( reinforced insulation ) |
| Mains clearance (if applicable) | N/A (battery/USB powered, < 60V) |
| NFC antenna clearance | 5 mm from any copper / metal |
| BLE antenna clearance | 5 mm from any copper / metal |
| Solder mask | 0.05 mm (2 mil) dam between pads |
| Minimum drill-to-trace | 0.20 mm |

**Safety notes:**
- Device is SELV (< 60V), no primary creepage requirements
- USB-C pins: CC resistors placed < 5mm from connector
- TVS (D1) placed < 5mm from USB-C connector pins
- ESD ground vias placed immediately adjacent to TVS pads

## 8. Component Placement (Top View)

```
                USB-C Connector (J1)
                    ┌──────┐
    ESD (D1) ──────│      │────── FB1 (VBUS filter)
                    │      │
    ┌───────────────└──────┘
    │
    │   C1-C4 (MCU decoupling)
    │   ┌────────────────────┐
    │   │  STM32L562QE (U1)  │
    │   │  UFQFPN48          │
    │   └────────────────────┘
    │          │
    │   ┌──────────┐     ┌──────────┐     ┌──────────┐
    │   │ A71CH   │     │ STPMIC1 │     │ MX25R16  │
    │   │ (U2)    │     │ (U7)    │     │ (U6)     │
    │   └──────────┘     └──────────┘     └──────────┘
    │          │
    │   ┌──────────┐     ┌──────────────┐
    │   │nRF52833 │     │ FPC1025      │
    │   │ (U5)    │     │ Fingerprint  │
    │   └──────────┘     └──────────────┘
    │          │
    │   ┌──────┐ ┌──────┐ ┌──────┐
    │   │LED_R │ │LED_G │ │LED_B │
    │   └──────┘ └──────┘ └──────┘
    │          │
    │   ┌──────────┐     ┌─────────────────────────────────┐
    │   │ PN7150  │     │ NFC Antenna (30mm loop)          │
    │   │ (U3)    │     │ (bottom layer)                    │
    │   └──────────┘     └─────────────────────────────────┘
    │
    │   ┌──────────────┐     ┌──────┐
    │   │ CR2032 (BT1) │     │ SW1  │
    │   └──────────────┘     └──────┘
    │                              │
    └──────────────────────────────┘
                        │  Keychain hole (4mm)
                    ○
```

## 9. Grounding Strategy

- **L2 and L5**: Continuous ground planes — no splits
- **L3**: Split power plane — VDD_3V3 (80%), VDD_1V8 (20%)
- **Ground stitching**: Vias every 3 mm along board perimeter
- **Star ground**: All IC grounds connect to ground plane via vias; no daisy-chaining
- **Analog ground**: VDDA has dedicated filter (ferrite bead + cap) before joining ground plane
- **ESD ground**: TVS diode ground connects directly to ground plane with 2+ vias

## 10. Design for Manufacturing (DFM)

| Parameter | Value |
|-----------|-------|
| Minimum trace width | 0.10 mm (4 mil) |
| Minimum via drill | 0.2 mm |
| Minimum pad size | 0.4 mm |
| Solder mask | Green LPI, 0.05 mm dams |
| Silkscreen | White, minimum 0.10 mm line width |
| Surface finish | ENIG (Au 0.03 µm / Ni 3 µm) |
| Board thickness | 1.0 mm ± 0.1 mm |
| Copper weight | 1 oz outer, ½ oz inner |
| Panel size | 100 × 100 mm (4-up panel) |
| V-scoring | Between boards, 0.3 mm residual |
| Test points | 6 (VBUS, VDD_3V3, VDD_1V8, GND, SE_SDA, SE_SCL) |