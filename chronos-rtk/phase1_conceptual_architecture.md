# Chronos-RTK — Phase 1: Conceptual Architecture

## 1. System Purpose

Chronos-RTK is a **high-precision dual-frequency RTK GNSS receiver board** designed for centimeter-level positioning in surveying, precision agriculture, drone navigation, and structural monitoring applications. It receives L1/L2 signals from GPS, GLONASS, Galileo, and BeiDou constellations, applies RTK corrections received over LoRa or USB, and outputs a corrected PVT (Position, Velocity, Time) solution at 20 Hz. An onboard OLED display shows live coordinates and status; SPI flash logs raw observations for post-processing; a LoRa 868/915 MHz radio shares RTCM corrections between rover and base stations in a mesh network.

## 2. Performance Targets

| Parameter | Target |
|---|---|
| Position accuracy (RTK fixed) | ≤ 1 cm horizontal, ≤ 2 cm vertical |
| Position accuracy (standalone) | ≤ 1.5 m horizontal |
| Time To First Fix (cold) | ≤ 30 s |
| RTK convergence time | ≤ 60 s |
| Update rate | 20 Hz PVT, 1 Hz raw observations |
| Constellations | GPS L1/L2, GLONASS L1/L2, Galileo E1/E5b, BeiDou B1/B2 |
| LoRa link range | ≥ 5 km line-of-sight |
| Power consumption | ≤ 800 mW typical, ≤ 2 W peak |
| Operating temp | −40 °C to +85 °C |
| Supply voltage | 5 V USB or 3.7–5.5 V battery via on-board charger |

## 3. Design Constraints

- **Single-board design** ≤ 70 mm × 45 mm (compatible with common RTK survey rods)
- **MCQ (Minimum Component Quality)**: all ICs automotive or industrial grade (−40/+85 °C)
- **Dual-frequency mandatory**: L1 + L2 (or equivalent) for ionospheric error cancellation
- **LoRa mesh**: must relay RTCM v3.3 messages between base and rover without a PC
- **USB-C data + power**: enumerate as CDC-ACM for NMEA output; charge LiPo simultaneously
- **Open firmware**: all register maps and drivers in public repo

## 4. High-Level Block Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                        Chronos-RTK Board                        │
│                                                                  │
│  ┌─────────┐   RF   ┌──────────────────┐   UART   ┌──────────┐  │
│  │ L1/L2   ├───────►│ u-blox ZED-F9P  ├─────────►│ STM32    │  │
│  │ Antenna │        │ GNSS Receiver    │  I2C    │ G474RET  │  │
│  │ (MCX)   │        │                  ├◄────────►│ MCU      │  │
│  └─────────┘        │  LNA ● SAW ●    │  PPS    │          │  │
│                     │  LNA ● SAW ●    ├─────────►│          │  │
│                     └──────┬───────────┘         │  ┌──────┐ │  │
│                            │                      │  │SPI   │ │  │
│                     ┌──────▼──────┐               │  │Flash │ │  │
│                     │  LDO / DC-DC│               │  └──────┘ │  │
│                     │  Power Tree │               │          │  │
│                     └──────┬──────┘               │  ┌──────┐ │  │
│                            │                      │  │LoRa  │ │  │
│              ┌─────────────┼──────────────────────┤  │SX1262│ │  │
│              │             │                      │  └──┬───┘ │  │
│     ┌────▼────┐   ┌───────▼───────┐              │     │     │  │
│     │USB-C   │   │   Battery     │              └─────┼─────┘  │
│     │PD Port │   │   Charger     │                    │        │
│     └────┬───┘   │   MCP73871    │              ┌─────▼─────┐  │
│          │       └───────┬───────┘              │ LoRa Ant   │  │
│          │               │                      │ (SMA)      │  │
│          └───────┬───────┘                      └────────────┘  │
│                  │                                                │
│           ┌──────▼──────┐  I2C   ┌───────────┐                 │
│           │   3.3 V /    ├───────►│ OLED SSD1306│                │
│           │   1.8 V Rails│        │ 128×64 I2C │                │
│           └─────────────┘         └────────────┘                 │
└──────────────────────────────────────────────────────────────────┘
```

## 5. Data Flow

```
GPS/GLO/GAL/BDS L1+L2 RF
        │
   ┌────▼────┐   NMEA + RAW   ┌────────────┐   PVT + RTCM   ┌───────────┐
   │ ZED-F9P ├──────────────►│ STM32G474  ├──────────────►│ USB Host  │
   │         │   RTCM corr   │ (parser,    │   NMEA out    │ (CDC-ACM) │
   │         │◄──────────────│  filter,    │◄──────────────│           │
   └─────────┘   from LoRa   │  log, mesh) │   RTCM in     └───────────┘
                             └─────┬───────┘
                                   │
              ┌────────────────────┬┴───────────────────┐
              │                    │                     │
         ┌────▼─────┐      ┌──────▼──────┐      ┌──────▼──────┐
         │ SPI Flash│      │ LoRa SX1262 │      │ OLED SSD1306│
         │ (raw obs │      │ (RTCM tx/rx)│      │ (status     │
         │  logging)│      └─────────────┘      │  display)   │
         └──────────┘                            └─────────────┘
```

## 6. Bus Topology

| Bus | Master | Slaves | Speed | Purpose |
|---|---|---|---|---|
| UART1 | STM32G474 | ZED-F9P | 460800 baud | GNSS NMEA/UBX data |
| I2C1 | STM32G474 | SSD1306, BME280 (opt.) | 400 kHz | Display + weather sensor |
| SPI1 | STM32G474 | W25Q128 flash | 50 MHz | Observation logging |
| SPI2 | STM32G474 | SX1262 LoRa | 16 MHz | LoRa radio transceiver |
| USB | STM32G474 | USB-C host | 12 Mbps FS | Data + power |
| PPS | ZED-F9P → STM32 | GPIO interrupt | — | Time pulse synchronization |
| GPIO | STM32G474 | LDO EN, LED, buttons | — | Power gating, UI |

## 7. Power Tree

```
VSRC (USB 5 V / LiPo 3.7–4.2 V)
  │
  ├── MCP73871 (LiPo charger, USB-powered)
  │     └── LiPo 3.7 V nominal
  │
  ├── TPS62A02 (Step-down, 5→3.3 V, 2 A) ──► VDD_3V3 rail
  │     ├── STM32G474 VDD
  │     ├── SX1262 VDD
  │     ├── W25Q128 VDD
  │     ├── SSD1306 VDD
  │     └── ZED-F9P VCC (3.3 V via L_NA)
  │
  └── TLV75518 (LDO, 3.3→1.8 V, 150 mA) ──► VDD_1V8 rail
        └── ZED-F9P VCC_RF (1.8 V analog domain)
```

## 8. Key Design Decisions

1. **ZED-F9P as GNSS engine**: Industry-proven 184-channel dual-frequency receiver with built-in RTK engine. Outputs RTCM v3.3 natively — no host-side RTK computation needed.
2. **STM32G474 as system controller**: 170 MHz Cortex-M4F with FPU, rich peripheral set, hardware CRC for LoRa frames, 512 KB Flash / 128 KB RAM sufficient for protocol stack + logging.
3. **SX1262 for LoRa**: Sub-GHz LoRa transceiver with +22 dBm output, better sensitivity than SX1276, supports LR-FHSS for RTCM relay.
4. **W25Q128 for logging**: 16 MB SPI NOR flash stores ~24 hours of raw observation data at 1 Hz rate.
5. **Battery-backed operation**: MCP73871 manages charge from USB; STM32 monitors battery voltage via ADC and can shut down LoRa/display to extend runtime.