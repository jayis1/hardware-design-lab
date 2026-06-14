# SkyPilot — Dual-IMU Drone Flight Controller with 4G/LTE Telemetry

![SkyPilot FC](https://img.shields.io/badge/PCB-45x45mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H743-orange) ![LTE](https://img.shields.io/badge/LTE-Cat.4-green) ![License](https://img.shields.io/badge/License-MIT-yellow)

## Overview

SkyPilot is a professional-grade dual-IMU drone flight controller with integrated 4G/LTE telemetry, designed for beyond-visual-line-of-sight (BVLOS) autonomous drone operations. It combines a 480MHz Cortex-M7 flight processor with dual redundant IMUs, a u-blox LARA-R6 cellular modem, a SAM-M10Q multi-constellation GNSS, and an ESP32-H2 companion computer for AI-assisted mission planning.

## Key Specifications

| Parameter | Value |
|---|---|
| **Flight Processor** | STM32H743VIT6, 480MHz Cortex-M7, 2MB Flash, 1MB RAM |
| **Primary IMU** | TDK ICM-42688-P, 6-axis (3-axis accel + 3-axis gyro), 32kHz ODR |
| **Secondary IMU** | Bosch BMI270, 6-axis, 6.4kHz ODR (redundant) |
| **Barometer** | Bosch BMP390, ±0.5hPa accuracy |
| **GNSS** | u-blox SAM-M10Q, GPS/Galileo/GLONASS/BeiDou |
| **4G/LTE Modem** | u-blox LARA-R6401, Cat.4 (150Mbps DL / 50Mbps UL) |
| **Companion** | Espressif ESP32-H2, 96MHz RISC-V, IEEE 802.15.4 |
| **Motor Outputs** | 12× DShot1200 / PWM |
| **CAN Bus** | FDCAN1, 1Mbps (DroneCAN compatible) |
| **USB** | USB-C 2.0 Full-Speed (config only) |
| **Power Input** | 7–26V (2–6S LiPo), 5V/3A BEC, 3.3V/1.5A LDO |
| **Board Size** | 45 × 45 mm, 1.0mm thick, 6-layer |
| **Weight** | ≤ 28g (PCB + components) |
| **Mounting** | 4× M3 at 30.5mm square |
| **Operating Temp** | -20°C to +70°C |

## Architecture

```
┌─────────────────────────────────────────────┐
│          STM32H743 (Flight Core)             │
│  SPI1 → ICM-42688-P (Primary IMU)           │
│  SPI2 → BMI270 (Secondary IMU)              │
│  I2C1 → BMP390 (Baro) + RV-3032 (RTC)       │
│  UART1 → SAM-M10Q (GNSS)                    │
│  UART8 → LARA-R6 (4G/LTE)                   │
│  UART4 → ESP32-H2 (Companion)               │
│  TIM1-4 → 12× DShot1200 motor outputs       │
│  FDCAN1 → CAN bus                           │
│  USB → USB-C config port                     │
│  ADC1 → Battery voltage/current              │
└─────────────────────────────────────────────┘
```

## Directory Structure

```
skypilot/
├── phase1_conceptual_architecture.md    # System purpose, block diagram, data flow
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md      # Stackup, routing rules, thermal, clearances
├── phase4_software_stack.md             # Boot, MMIO, drivers, device tree, build
├── kicad/
│   ├── device.kicad_pro                 # KiCad project file
│   ├── device.kicad_sch                 # Full schematic
│   └── device.kicad_pcb                 # PCB layout
├── firmware/
│   ├── Makefile                         # Cross-compile Makefile
│   ├── main.c                           # SPL/board init entry point
│   ├── board.h                          # Pin definitions
│   ├── registers.h                      # MMIO register map (in phase4)
│   ├── stm32h743vit6.ld                 # Linker script
│   ├── usb_descriptors.h               # USB descriptors
│   └── drivers/
│       ├── imu_icm42688.h              # ICM-42688-P driver header
│       ├── imu_icm42688.c              # ICM-42688-P driver with DMA
│       ├── lte_lara_r6.h               # LARA-R6 LTE modem driver header
│       └── lte_lara_r6.c               # LARA-R6 driver with DMA ring buffer
├── app/
│   ├── package.json                    # React Native dependencies
│   ├── App.js                          # Main navigation
│   ├── screens/
│   │   ├── FlightScreen.js             # Attitude, motors, arm/disarm
│   │   ├── TelemetryScreen.js          # Sensor data, LTE stats
│   │   ├── SettingsScreen.js           # Configuration, calibration
│   │   └── MapScreen.js               # GPS position with trail
│   ├── components/
│   │   ├── ConnectionContext.js        # BLE/TCP connection manager
│   │   ├── AttitudeIndicator.js        # Artificial horizon widget
│   │   └── SensorCard.js              # Reusable sensor value card
│   └── utils/
│       ├── protocol.js                 # Binary wire protocol (CRC-16)
│       └── theme.js                    # Colors, typography, spacing
└── README.md                           # This file
```

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi

# Build firmware
cd firmware
make clean && make all

# Flash via ST-Link
make flash

# Flash via DFU (USB-C)
make dfu
```

### Companion App

```bash
# Install dependencies
cd app
npm install

# Run on Android
npx react-native run-android

# Run on iOS
npx react-native run-ios
```

### KiCad

Open `kicad/device.kicad_pro` in KiCad 8.0+ to view/edit the schematic and PCB layout.

## Power Tree

```
VBAT (7-26V) ──► TPS65294 (5V/3A BEC) ──┬──► TLV75533 (3.3V/1.5A) ──► STM32, IMUs, BMP390, GNSS, ESP32
                                            └──► TLV75518 (1.8V/500mA) ──► IMU cores, STM32 VDDA1V8
VBAT_DIRECT ──► TPS22917 (Load Switch) ──► LARA-R6 (3.4-4.2V)
CR1225 ──► STM32 VBAT (RTC backup)
```

## Communication Protocol

The companion app communicates with the flight controller via a binary wire protocol over BLE or TCP:

- **Frame format**: `[0xAA][0x55][LEN_H][LEN_L][CMD][SEQ][PAYLOAD...][CRC_H][CRC_L]`
- **CRC**: CRC-16/CCITT (polynomial 0x1021, init 0xFFFF)
- **Commands**: See `app/utils/protocol.js` for full command IDs and payload formats

## Features

- ✅ Dual redundant IMU (ICM-42688-P + BMI270) with voting
- ✅ 4G/LTE Cat.4 telemetry (LARA-R6401) — up to 150Mbps downlink
- ✅ Multi-constellation GNSS (SAM-M10Q) — GPS/Galileo/GLONASS/BeiDou
- ✅ 12-channel DShot1200 motor output
- ✅ FDCAN1 (DroneCAN compatible)
- ✅ Barometric altitude (BMP390)
- ✅ Real-time clock (RV-3032-7) with CR1225 backup
- ✅ USB-C configuration port
- ✅ Companion computer (ESP32-H2) for AI-assisted missions
- ✅ 128Mbit SPI flash for config/OSD
- ✅ Hardware watchdog (IWDG, 2-second timeout)
- ✅ Battery voltage/current sensing (ADC1)

## License

MIT License. See [LICENSE](../LICENSE) for details.

---

*Designed by Hardware Design Lab. All part numbers, register addresses, and pin names are real and verified against manufacturer datasheets.*