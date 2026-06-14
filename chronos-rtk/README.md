# Chronos-RTK — High-Precision Dual-Frequency RTK GNSS Receiver Board

> A single-board centimeter-accuracy positioning device with LoRa mesh correction relay, OLED status display, and USB/Bluetooth companion app.

## Specifications

| Parameter | Value |
|---|---|
| **GNSS Receiver** | u-blox ZED-F9P (184-channel) |
| **Constellations** | GPS L1/L2, GLONASS L1/L2, Galileo E1/E5b, BeiDou B1/B2 |
| **Position Accuracy (RTK Fixed)** | ≤ 1 cm horizontal, ≤ 2 cm vertical |
| **Position Accuracy (Standalone)** | ≤ 1.5 m horizontal |
| **Update Rate** | 20 Hz PVT, 1 Hz raw observations |
| **TTFF (Cold Start)** | ≤ 30 s |
| **RTK Convergence** | ≤ 60 s |
| **LoRa Radio** | Semtech SX1262, 868/915 MHz, SF7–SF12 |
| **LoRa TX Power** | +22 dBm (≈160 mW) |
| **LoRa Range** | ≥ 5 km LOS (SF7), ≥ 15 km (SF12) |
| **Microcontroller** | STM32G474RET6 (Cortex-M4F @ 170 MHz) |
| **Flash Storage** | Winbond W25Q128 (16 MB SPI NOR) |
| **Display** | SSD1306 OLED 128×64 (I2C) |
| **Connectivity** | USB-C (CDC-ACM), UART, LoRa |
| **Battery** | 3.7 V LiPo via MCP73871 charger (USB-powered) |
| **Power** | ≤ 800 mW typical, ≤ 2 W peak |
| **Board Size** | 70 × 45 mm |
| **Operating Temp** | −40 °C to +85 °C |
| **PCB** | 6-layer FR-4 370HR, ENIG, 1.6 mm |
| **BOM Cost (1 qty)** | ≈ $220 |

## Directory Structure

```
chronos-rtk/
├── phase1_conceptual_architecture.md   # System architecture, block diagram, data flow
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md     # Stackup, routing rules, thermal, DFM
├── phase4_software_stack.md            # Boot, drivers, USB, device tree
├── kicad/
│   ├── device.kicad_pro                # KiCad project (net classes, design rules)
│   ├── device.kicad_sch               # Full schematic
│   └── device.kicad_pcb               # Board layout, placement, zones
├── firmware/
│   ├── Makefile                        # ARM GCC cross-compile build
│   ├── main.c                          # SPL entry point, init, main loop
│   ├── board.h                         # Pin definitions, constants
│   ├── registers.h                     # MMIO register map (STM32G474)
│   ├── usb_descriptors.h               # USB CDC-ACM descriptors
│   └── drivers/
│       ├── sx1262.h / sx1262.c         # LoRa transceiver driver (SPI, IRQ, RTCM)
│       ├── w25q128.h / w25q128.c       # SPI flash driver (read, write, log)
│       └── ssd1306.h / ssd1306.c        # OLED display driver (I2C, font)
├── app/
│   ├── package.json                    # React Native dependencies
│   ├── App.js                          # Navigation root
│   ├── screens/
│   │   ├── MapScreen.js                # Live position & RTK status
│   │   ├── SettingsScreen.js           # LoRa, RTK, logging config
│   │   └── LogScreen.js                # Observation log viewer
│   ├── components/
│   │   ├── ChronosContext.js           # Device connection state provider
│   │   └── StatusCard.js               # Reusable UI card component
│   └── utils/
│       └── protocol.js                 # Binary wire protocol (CRC-16, frame parser)
└── README.md                           # This file
```

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi

# Build
cd firmware/
make clean && make -j$(nproc)

# Flash via ST-Link
openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
    -c "program build/chronos-rtk.elf verify reset exit"

# Or via USB DFU (hold BOOT0 at reset)
dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D build/chronos-rtk.bin
```

### Companion App

```bash
cd app/
npm install

# Android
npx react-native run-android

# iOS
npx react-native run-ios
```

### Connecting via USB (Linux)

The board enumerates as `/dev/ttyACM0` (CDC-ACM):

```bash
# View NMEA output
stty -F /dev/ttyACM0 460800 raw
cat /dev/ttyACM0

# Send UBX command to enable RTCM on UART
# (Use u-center or custom Python script)
```

### LoRa RTCM Relay Setup

1. **Base station**: Configure RTK mode to "base" via app or USB command
2. **Rover**: Configure RTK mode to "rover"
3. Both devices must be on the same LoRa frequency and spreading factor
4. Base station broadcasts RTCM v3.3 corrections via LoRa
5. Rover receives corrections and forwards to ZED-F9P for RTK solution

## Architecture Overview

```
GPS/GLO/GAL/BDS RF ──► ZED-F9P ──UART──► STM32G474 ──USB──► Host PC/App
                         │  ▲              │    ▲
                         │  │              │    │
                     NMEA/RAW          RTCM relay
                         │  │              │    │
                         ▼  │              ▼    │
                       ┌──┴──┐          ┌──┴──┐  │
                       │ SPI │          │ SPI │  │
                       │Flash│          │SX1262│ │
                       │(log)│          │(LoRa)│ │
                       └─────┘          └──┬──┘  │
                                           │     │
                                      LoRa Ant   │
                                        │         │
                                    LoRa mesh    │
                                        │         │
                                    Other ◄───────┘
                                    RTK nodes
```

## Key Components

| Ref | Part | Function |
|---|---|---|
| U1 | u-blox ZED-F9P | Dual-frequency RTK GNSS receiver |
| U2 | STM32G474RET6 | System MCU (Cortex-M4F @ 170 MHz) |
| U3 | Semtech SX1262 | Sub-GHz LoRa transceiver |
| U4 | Winbond W25Q128JVSIM | 16 MB SPI NOR flash |
| U5 | SSD1306 | 128×64 OLED controller (I2C) |
| U6 | TI TPS62A02 | 5→3.3 V buck converter |
| U7 | TI TLV75518 | 3.3→1.8 V LDO |
| U8 | Microchip MCP73871 | LiPo battery charger |

## License

This project is released under the **CERN-OHL-S v2** (hardware) and **MIT License** (firmware + app).

- Hardware: [CERN-OHL-S v2](https://ohwr.org/cern_ohl_s_v2.txt)
- Firmware: [MIT License](https://opensource.org/licenses/MIT)
- App: [MIT License](https://opensource.org/licenses/MIT)

---

*Chronos-RTK v1.0 · June 2026 · Chronos Systems*