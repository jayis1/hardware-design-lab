# HexaScope — 6-Channel Mixed-Signal Oscilloscope / Logic Analyzer

A production-quality, portable mixed-signal oscilloscope featuring 4 analog channels (250 MS/s, 100 MHz bandwidth), 2 digital channels (200 MS/s), FPGA-based real-time triggering and protocol decode, USB-C PD power, Wi-Fi 6 wireless streaming, and a React Native companion app.

---

## Specifications

| Parameter | Value |
|---|---|
| **Analog Channels** | 4 (individally switchable single-ended) |
| **Digital Channels** | 2 (threshold-adjustable) |
| **Analog Bandwidth** | 100 MHz per channel |
| **Sample Rate (Analog)** | 250 MS/s, 8-bit |
| **Sample Rate (Digital)** | 200 MS/s, 1-bit |
| **Input Range** | ±20 mV to ±40 V (1-2-5 sequence, 9 ranges) |
| **Input Impedance** | 1 MΩ ∥ 13 pF (BNC), 50 Ω switchable |
| **Memory Depth** | 128 Mpts per channel (shared 256 MB DDR3L) |
| **Trigger Types** | Edge, Pulse Width, Timeout, Runt, Window, Logic, Serial Pattern |
| **Protocol Decode** | UART, SPI, I²C, CAN, LIN (FPGA-based) |
| **Trigger Latency** | < 50 ns |
| **USB Transfer Rate** | ≥ 320 Mbps sustained (USB 3.0) |
| **Wi-Fi Throughput** | ≥ 80 Mbps sustained (Wi-Fi 6) |
| **Power** | USB-C PD 15 V @ 0.8 A (12 W max) |
| **Form Factor** | 170 × 110 × 25 mm, passive cooling |
| **FPGA** | Lattice ECP5-5G, 45K LUTs |
| **MCU** | STM32G474VET6, Cortex-M4F @ 170 MHz |
| **ADC** | TI ADS61B49 × 4, 250 MS/s 8-bit LVDS |
| **Memory** | Micron MT41K256M8TW, 256 MB DDR3L |
| **USB** | USB3340 ULPI PHY + ISO7740 digital isolator |
| **Wi-Fi** | ESP32-C6-WROOM-1, 802.11ax |
| **PMIC** | Dialog DA9063, 6-output |
| **BOM Cost** | ~$124 @ 1K units |

---

## Directory Structure

```
hexascope/
├── README.md                              This file
├── phase1_conceptual_architecture.md      System purpose, targets, block diagram, data flow
├── phase2_component_selection_schematics.md BOM, pinouts, netlists, decoupling, impedance
├── phase3_pcb_blueprints_layout.md         Stackup, routing rules, DDR matching, thermal, DFM
├── phase4_software_stack.md                Boot, MMIO, clocks, GPIO, drivers, device tree
├── kicad/
│   ├── device.kicad_pro                    KiCad project with net classes and design rules
│   ├── device.kicad_sch                    Full schematic (all ICs, passives, connectors)
│   └── device.kicad_pcb                    Board outline, placement, stackup, keepout zones
├── firmware/
│   ├── Makefile                            Cross-compile for ARM Cortex-M4
│   ├── main.c                              SPL/board init entry point
│   ├── board.h                             Pin definitions
│   ├── registers.h                          MMIO register map (STM32 + FPGA + peripherals)
│   ├── usb_descriptors.h                   USB 2.0 HS device descriptors
│   └── drivers/
│       ├── pmic_da9063.h                   DA9063 PMIC driver header
│       ├── pmic_da9063.c                   DA9063 PMIC driver (I²C, 6-rail control)
│       ├── dac60508.h                      DAC60508 8-ch DAC driver header
│       └── dac60508.c                      DAC60508 driver (SPI, threshold/calibration)
└── app/
    ├── package.json                        React Native project manifest
    ├── App.js                              Main navigation (Waveform, Protocol, Settings)
    ├── screens/
    │   ├── WaveformScreen.js               Real-time waveform display with controls
    │   ├── SettingsScreen.js                Channel config, Wi-Fi, calibration
    │   └── ProtocolScreen.js                Protocol decoder (UART/SPI/I²C/CAN/LIN)
    ├── components/
    │   ├── WaveformView.js                 SVG oscilloscope trace renderer
    │   └── TriggerControls.js              Trigger level/channel/type selector
    └── utils/
        └── protocol.js                     Binary wire protocol (CRC-16-CCITT, frame builder/parser)
```

---

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt install gcc-arm-none-eabi

# Build firmware
cd firmware
make

# Flash via ST-Link
make flash
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

### KiCad Design

Open `kicad/device.kicad_pro` in KiCad 8.0+ to view the schematic and PCB layout.

---

## License

Hardware design files: CERN-OHL-S v2 (Strongly Reciprocal)
Firmware source code: MIT License
Companion app source code: MIT License
Documentation: CC-BY-4.0