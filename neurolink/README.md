# NeuroLink — 32-Channel Biosignal Acquisition System

> A production-quality, credit-card-sized biosignal acquisition platform for EMG/EEG/ECG with real-time DSP, BLE 5.0 streaming, and USB-C connectivity.

## Specifications

| Parameter | Value |
|---|---|
| **ADC Channels** | 32 differential (8× ADS1299, 4 ch each, daisy-chained) |
| **ADC Resolution** | 24-bit ΔΣ |
| **Sample Rates** | 250, 500, 1000, 2000, 4000, 8000, 16000 SPS |
| **Input-Referred Noise** | < 1 µV rms (0.5–500 Hz) |
| **CMRR** | > 110 dB at 50/60 Hz |
| **Programmable Gain** | ×1, ×2, ×4, ×6, ×8, ×12, ×24 |
| **DSP Coprocessor** | Lattice iCE40UP5K (5280 LUTs, 128 Kb BRAM) |
| **Main MCU** | STM32H743ZIT6 (Cortex-M7 @ 480 MHz, 2 MB Flash) |
| **RAM** | 512 KB AXI SRAM + 128 KB SRAM1 + 128 KB SRAM2 + 512 MB DDR3L |
| **BLE** | nRF52840 (BLE 5.0, 2 Mbps PHY, ~1.3 Mbps effective) |
| **USB** | USB-C 2.0 HS (480 Mbps, galvanically isolated via ADuM3160) |
| **IMU** | LSM6DSOX (6-axis accelerometer + gyroscope) |
| **Storage** | W25Q128 (128 Mb QSPI Flash) + MicroSD (SD 3.0) |
| **Battery** | 3.7 V LiPo (2000 mAh), 8+ hours continuous |
| **Charging** | BQ25895 (USB-C, 1.6 A max, ICO) |
| **Power Consumption** | ≤ 1.5 W typical |
| **Board Size** | 85 × 54 mm (1.6 mm, 8-layer) |
| **Patient Isolation** | 5 kV (ADuM3160), IEC 60601-1 compliant design |
| **End-to-End Latency** | < 3 ms (electrode → host) |

## Directory Structure

```
neurolink/
├── README.md
├── phase1_conceptual_architecture.md
├── phase2_component_selection_schematics.md
├── phase3_pcb_blueprints_layout.md
├── phase4_software_stack.md
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
├── firmware/
│   ├── Makefile
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   ├── usb_descriptors.h
│   └── drivers/
│       ├── ads1299.h
│       ├── ads1299.c
│       ├── bq25895.h
│       ├── bq25895.c
│       ├── lsm6dsox.h
│       ├── lsm6dsox.c
│       ├── ice40_spi.h
│       └── ice40_spi.c
└── app/
    ├── package.json
    ├── App.js
    ├── screens/
    │   ├── StreamScreen.js
    │   ├── ChannelsScreen.js
    │   ├── SettingsScreen.js
    │   └── RecordingsScreen.js
    ├── components/
    │   ├── BleContext.js
    │   └── ChannelCard.js
    └── utils/
        └── protocol.js
```

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Clone and build
cd neurolink/firmware
make clean && make

# Flash via ST-Link
make flash
```

### App Build

```bash
cd neurolink/app
npm install
npx react-native run-android   # or run-ios
```

### KiCad

Open `kicad/device.kicad_pro` in KiCad 8.0+ to view the schematic and PCB layout.

## Key Design Decisions

1. **ADS1299 Daisy Chain**: 8 devices chained via SPI for simultaneous 32-channel sampling with no multiplexing skew
2. **iCE40UP5K FPGA**: Offloads real-time DSP (IIR filters, feature extraction) from the MCU, enabling <1ms processing latency
3. **ADuM3160 Isolation**: 5 kV galvanic isolation on USB meets IEC 60601-1 patient safety requirements
4. **DDR3L Expansion**: 512 MB DDR3L enables extended recording storage (>2 hours at 32ch/1kSPS)
5. **Dual Transport**: USB-C for high-throughput data logging, BLE for mobile app streaming

## Power Tree

```
USB-C 5V → BQ25895 → LiPo 3.7V → VSYS
                                   ├→ TPS62821 → VDD_3V3 (digital)
                                   ├→ TPS62822 → VDD_1V8 (DDR, I/O)
                                   ├→ LP5907-1.2 → VDD_1V2 (MCU core, FPGA core)
                                   └→ LP5907-2.5 → VDDA_2V5 (ADC reference)
```

## License

MIT License. See repository root for details.