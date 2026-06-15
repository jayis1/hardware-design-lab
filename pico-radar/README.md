# PicoRadar — 60GHz FMCW Radar Development Platform

> A credit-card-sized 60GHz FMCW radar development platform with on-package antennas, STM32H7 host processor, WiFi/BLE, Ethernet+PoE, IMU, and OLED display — for gesture recognition, presence detection, people counting, and short-range imaging.

## Specifications

| Parameter | Value |
|---|---|
| **Radar SoC** | TI IWR6843AOP (3TX/4RX, 60–64 GHz ISM) |
| **Antennas** | On-package AiP (antenna-in-package, no external antenna) |
| **Range resolution** | 3.75 cm (4 GHz sweep BW) |
| **Max range** | 12 m (default), 50 m (extended profile) |
| **Angular resolution** | 15° (3-TX MIMO) |
| **Point cloud rate** | 20 fps (configurable up to 30 fps) |
| **Host CPU** | STM32H743VIT6 (Cortex-M7 @ 480 MHz, 1 MB SRAM) |
| **Connectivity** | WiFi 4 + BLE 5 (ESP32-C3), Ethernet 10/100 (LAN8720A), USB-C CDC |
| **PoE** | 802.3af via TPS2378 (48V input, single-cable install) |
| **IMU** | ICM-42688-P (6-axis, accel ±16g + gyro ±2000°/s) |
| **Display** | 1.3" 128×64 OLED (SH1106, I2C) |
| **Power** | USB-C 5V/1.5A or PoE 48V; <3.5W active, <0.5W idle |
| **Board size** | 85 mm × 55 mm × 1.6 mm (8-layer FR4) |
| **Temperature** | –20 °C to +60 °C (industrial) |
| **BOM cost** | ~$83 @ 1K units |

## Directory Structure

```
pico-radar/
├── README.md                              # This file
├── phase1_conceptual_architecture.md      # System purpose, block diagram, data flow, bus topology
├── phase2_component_selection_schematics.md # BOM, pinouts, netlists, decoupling, impedance
├── phase3_pcb_blueprints_layout.md         # Stackup, routing rules, vias, thermal, clearances
├── phase4_software_stack.md                # Boot, registers, clock, GPIO, drivers, device tree, build
├── kicad/
│   ├── device.kicad_pro                    # KiCad project file with net classes & design rules
│   ├── device.kicad_sch                    # Full schematic with all ICs & annotations
│   └── device.kicad_pcb                    # Board outline, placement, stackup, keepout zones
├── firmware/
│   ├── Makefile                            # Cross-compile Makefile (arm-none-eabi-gcc)
│   ├── main.c                              # SPL/board init, FreeRTOS tasks, peripheral startup
│   ├── board.h                             # Pin definitions for all peripherals
│   ├── registers.h                         # MMIO register map (STM32H743, ICM-42688, SH1106, LAN8720)
│   ├── usb_descriptors.h                  # USB CDC device & config descriptors
│   └── drivers/
│       ├── imu_icm42688.h                  # ICM-42688-P IMU driver header (SPI1 + DMA)
│       ├── imu_icm42688.c                  # IMU driver implementation with DMA FIFO read
│       ├── oled_sh1106.h                  # SH1106 OLED driver header (I2C1)
│       └── oled_sh1106.c                  # OLED driver with framebuffer, font, drawing primitives
└── app/
    ├── package.json                        # React Native dependencies
    ├── App.js                              # Navigation (bottom tabs: Radar, Data, Settings)
    ├── screens/
    │   ├── RadarScreen.js                  # Real-time point cloud visualization
    │   ├── ConnectionScreen.js             # BLE device scanning & connection
    │   ├── DataScreen.js                   # IMU data, session stats, logging
    │   └── SettingsScreen.js               # Chirp profiles, WiFi, display, system config
    ├── components/
    │   ├── RadarPlot.js                    # 2D polar radar plot (SVG, color by velocity)
    │   └── ImuGauge.js                     # Reusable IMU axis gauge component
    └── utils/
        └── protocol.js                    # Binary wire protocol (CRC-16, frame builder/parser)
```

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi openocd

# Build
cd firmware/
make clean && make all

# Flash via ST-Link SWD
make flash
```

### Radar Firmware (IWR6843AOP)

1. Set SOP0=LOW, SOP1=HIGH on PicoRadar (flash mode)
2. Use TI Uniflash or mmWave Studio to flash firmware via UART XMODEM
3. Set SOP0=HIGH, SOP1=LOW (functional mode)
4. Reset radar — it will boot from on-board W25Q128 SPI flash

### Companion App

```bash
cd app/
npm install
npx react-native run-android   # or run-ios
```

1. Open the app → tap **Connect** → scan for `PicoRadar` BLE device
2. Connect → point cloud data streams to the Radar tab
3. Configure chirp profile, WiFi, display in Settings tab
4. View IMU data and session statistics in Data tab

### USB Serial Console

Connect USB-C to host, open serial port at 115200 baud:
```
/minicom -D /dev/ttyACM0 -b 115200
```

Send mmWave SDK CLI commands (e.g., `sensorStart`, `sensorStop`, `profileCfg`).

## Key Design Decisions

- **IWR6843AOP with AiP** — No RF layout expertise needed; antenna-in-package eliminates external antenna design and FCC certification complexity
- **STM32H743 bare-metal FreeRTOS** — Deterministic <5ms radar-to-point-cloud latency; no Linux kernel overhead
- **Dual connectivity** — WiFi/BLE for mobile/cloud apps + Ethernet/PoE for ceiling/wall deployment
- **Integrated IMU** — Enables radar + inertial sensor fusion for robotics and navigation
- **Binary wire protocol** — CRC-16 protected frames with command IDs; efficient for BLE MTU constraints

## License

This design is released under the **CERN Open Hardware Licence v2.0 (Strongly Reciprocal)** — see LICENSE file.

Hardware design files (KiCad), firmware source, and app source are all open source.

---

*Designed by Jayis1 2026*
