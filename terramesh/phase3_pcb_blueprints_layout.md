# Terramesh — Phase 3: PCB Blueprints & Layout

**Author: jayis1**  
**Copyright © 2026 jayis1**

---

## 1. PCB Stackup

4-layer, 1.6 mm total thickness, FR-4 (Tg 170°C), ENIG finish.

| Layer | Name | Thickness | Copper Weight | Notes |
|-------|------|-----------|---------------|-------|
| L1 | Top Signal | 0.035 mm | 1 oz | Components, RF, USB, SPI |
| — | Prepreg | 0.200 mm | — | 2116, Er=4.2 |
| L2 | GND Plane | 0.035 mm | 1 oz | Solid ground, no splits |
| — | Core | 1.000 mm | — | FR-4, Er=4.5 |
| L3 | Power Plane | 0.035 mm | 1 oz | VBAT, VDD_3V3, VDD_1V8, VREF |
| — | Prepreg | 0.200 mm | — | 2116, Er=4.2 |
| L4 | Bottom Signal | 0.035 mm | 1 oz | Low-speed signals, debug |

## 2. PCB Dimensions

- **Board size**: 100 mm × 60 mm (fits inside 120 mm × 80 mm enclosure)
- **Board shape**: Rectangle with 4× M3 mounting holes at corners (3.2 mm diameter)
- **Edge clearance**: 5 mm from board edge to components
- **Keep-out zones**: 10 mm radius around SMA antenna connector

## 3. Routing Rules

| Parameter | Value | Applies To |
|-----------|-------|------------|
| Minimum trace width | 0.15 mm | General signals |
| Minimum trace spacing | 0.15 mm | General signals |
| RF trace width | 0.45 mm | LoRa 50 Ω path |
| RF trace clearance | 0.50 mm (to GND) | LoRa path |
| USB DP/DN width | 0.35 mm | USB differential pair |
| USB DP/DN gap | 0.15 mm | USB differential pair |
| Power trace width | 0.50 mm | VBAT, VDD_3V3 |
| Power trace width | 1.00 mm | Battery, LTE burst |
| Via diameter | 0.30 mm | Signal vias |
| Via diameter | 0.50 mm | Power vias |
| Thermal via pitch | 1.0 mm | Under PMIC, LDO |

## 4. Length Matching

| Net Group | Max Skew | Target Length | Notes |
|-----------|----------|---------------|-------|
| USB_DP / USB_DN | 1 mm | 30 mm | 90 Ω differential |
| SPI1_SCK / MOSI / MISO | 5 mm | 25 mm | 10 MHz, relaxed |
| SPI2_SCK / MOSI / MISO | 5 mm | 20 mm | 8 MHz, relaxed |
| SX1262 RF_HF → SMA | — | 15 mm max | 50 Ω, minimize loss |

## 5. Via Strategy

- **Signal vias**: 0.3 mm drill, 0.5 mm pad, tented on bottom
- **Power vias**: 0.5 mm drill, 0.8 mm pad, 2+ per power net
- **Thermal vias**: 0.3 mm drill, 0.5 mm pad, 0.5 mm pitch, array under TPS7A4700 and MAX20361
- **Via-in-pad**: Not used (all components on top layer only)

## 6. Component Placement

### Top Layer (All Components)

```
┌─────────────────────────────────────────────────────────────┐
│  J3(SMA)                                                    │
│  ┌──┐                                                       │
│  │  │  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  │
│  │  │  │ U2   │  │ U9   │  │ U4   │  │ U5   │  │ U6   │  │
│  │  │  │SX1262│  │Flash │  │ADXL  │  │SCL33 │  │BME68 │  │
│  │  │  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘  │
│  │  │                                                       │
│  │  │  ┌──────────────────────────────────────────────┐    │
│  │  │  │              U1 (STM32U5A5ZJT6Q)              │    │
│  │  │  │              LQFP144                          │    │
│  │  │  └──────────────────────────────────────────────┘    │
│  │  │                                                       │
│  │  │  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  │
│  │  │  │ U7   │  │ U8   │  │U10   │  │U11   │  │U12   │  │
│  │  │  │ADC#1 │  │ADC#2 │  │RTC   │  │PMIC  │  │NFC   │  │
│  │  │  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘  │
│  │  │                                                       │
│  │  │  ┌──────┐  ┌──────┐  ┌──────┐                       │
│  │  │  │U13   │  │U14   │  │ J1   │                       │
│  │  │  │3.3V  │  │2.5V  │  │USB-C │                       │
│  │  │  │LDO   │  │LDO   │  └──────┘                       │
│  │  │  └──────┘  └──────┘                                  │
│  │  │                                                       │
│  │  │  ┌──────┐  ┌──────┐  ┌──────┐                       │
│  │  │  │ J2   │  │ J4   │  │ SC1  │                       │
│  │  │  │SWD   │  │Batt  │  │Super │                       │
│  │  │  └──────┘  └──────┘  │cap   │                       │
│  │  │                       └──────┘                       │
│  └──┘                                                       │
└─────────────────────────────────────────────────────────────┘
```

## 7. Thermal Management

- **TPS7A4700 (3.3 V LDO)**: Max dissipation 0.5 W (200 mA × 2.5 V dropout). Thermal pad to GND plane with 9 thermal vias. Expected junction temp: +55°C at 70°C ambient.
- **MAX20361 (PMIC)**: Boost converter at 85% efficiency, 0.3 W dissipation. Thermal pad with 4 vias.
- **BG95-M3 (LTE)**: 2 W peak during transmit. Module has internal PA with exposed pad. 6 thermal vias to GND plane.
- **General**: No active cooling required. Enclosure acts as heatsink via thermal pad to 316L tube.

## 8. DFM Notes

- Minimum trace/space: 0.15 mm/0.15 mm (standard capability)
- Minimum drill: 0.3 mm
- Aspect ratio: 8:1 (1.6 mm / 0.2 mm min drill)
- Copper balancing: > 70% on all layers
- Silkscreen: White, top layer only, 0.8 mm text height
- Solder mask: Green, LPI, 0.1 mm web clearance
- Panelization: 5-up V-score, 2 mm tabs
