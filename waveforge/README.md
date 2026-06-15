# WaveForge — Polyphonic Digital Audio Synthesizer Engine

> A production-quality, open-hardware polyphonic synthesizer engine board featuring STM32H743 (480 MHz Cortex-M7), WM8778 24-bit audio codec, 32 MB QSPI wavetable flash, MIDI DIN I/O, 4× CV inputs, USB-C audio/MIDI, and a React Native companion app for patch editing.

![Board](https://img.shields.io/badge/PCB-100×80mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H743-orange) ![Codec](https://img.shields.io/badge/Codec-WM8778-green) ![License](https://img.shields.io/badge/License-MIT-yellow)

## Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32H743VIT6 — ARM Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB RAM |
| **Audio Codec** | WM8778SEDS — 24-bit, 48/96 kHz, 110 dB SNR, 2-in / 2-out + HP |
| **HP Amplifier** | TPA6130A2 — 128 dB SNR, 138 mW stereo headphone driver |
| **Wavetable Flash** | W25Q256JVEIQ — 32 MB QSPI, XIP-capable, 133 MHz |
| **EEPROM** | 24C256 — 256 Kbit I2C, patch config storage |
| **Polyphony** | 64 voices (wavetable), 32 voices (FM), 8 voices (layered) |
| **Sample Rate** | 48 kHz (default) / 96 kHz (HQ mode) |
| **Bit Depth** | 24-bit DAC output, 32-bit internal processing |
| **Latency** | ≤ 2 ms (MIDI-in to DAC-out @ 48 kHz) |
| **Dynamic Range** | ≥ 110 dB DAC SNR |
| **MIDI** | DIN-5 IN + THRU (opto-isolated), USB MIDI class-compliant |
| **CV Inputs** | 4 × 0–5 V analog (10-bit ADC, voltage-divided) |
| **USB** | USB-C 2.0 FS — Audio Class 2.0 + MIDI + Vendor |
| **Storage** | 256 patches in SPI flash, wavetable ROM |
| **Power** | USB-C 5 V / 500 mA, or barrel jack 7–12 V DC |
| **Form Factor** | 100 mm × 80 mm, 4-layer PCB, ENIG finish |
| **BOM Cost** | ~$25 at 1000 units |

## Directory Structure

```
waveforge/
├── README.md                              # This file
├── phase1_conceptual_architecture.md      # System architecture, blocks, data flow
├── phase2_component_selection_schematics.md # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md         # Stackup, routing rules, thermal, DRC
├── phase4_software_stack.md                # Boot, MMIO, drivers, device tree
├── kicad/
│   ├── device.kicad_pro                    # KiCad project file
│   ├── device.kicad_sch                    # Full schematic
│   └── device.kicad_pcb                    # Board layout
├── firmware/
│   ├── Makefile                            # Cross-compile build system
│   ├── main.c                              # SPL/board init, audio engine, MIDI parser
│   ├── board.h                             # Pin definitions and macros
│   ├── registers.h                         # MMIO register map
│   ├── usb_descriptors.h                   # USB Audio Class 2.0 + MIDI descriptors
│   └── drivers/
│       ├── i2c.h / i2c.c                  # I2C1 master driver (400 kHz)
│       ├── spi_flash.h / spi_flash.c       # W25Q256 QSPI flash driver (DMA)
│       ├── wm8778.h / wm8778.c             # WM8778 codec driver (I2C control)
│       └── tpa6130.h / tpa6130.c           # TPA6130A2 headphone amp driver
└── app/
    ├── package.json                        # React Native dependencies
    ├── App.js                              # Navigation entry point
    ├── screens/
    │   ├── MainScreen.js                   # Synth control (osc, filter, ADSR, keyboard)
    │   ├── SettingsScreen.js               # Patch management, MIDI, effects, audio config
    │   └── DataViewScreen.js               # CV monitor, voice allocator, protocol debug
    ├── components/
    │   └── SynthKnob.js                    # Reusable rotary knob component
    └── utils/
        └── protocol.js                     # Binary wire protocol with CRC-8, frame parser
```

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# Build firmware
cd waveforge/firmware
make clean
make

# Flash via ST-Link
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
    -c "program build/waveforge.elf verify reset exit"
```

### Companion App

```bash
cd waveforge/app
npm install

# Android
npx react-native run-android

# iOS (requires macOS)
npx react-native run-ios
```

### Hardware Connections

| Connector | Function | Pinout |
|---|---|---|
| J1 (USB-C) | Power + USB data | VBUS, D+, D−, GND, CC1, CC2 |
| J2 (DIN-5) | MIDI IN | Pins 4,5 (opto-isolated current loop) |
| J3 (DIN-5) | MIDI THRU | Pins 4,5 (current loop output) |
| J4 (Barrel) | 7–12 V DC input | Center positive, 2.1 mm |
| J5 (3.5 mm) | Line Out Left | Tip=L, Ring=GND, Sleeve=GND |
| J6 (3.5 mm) | Line Out Right | Tip=R, Ring=GND, Sleeve=GND |
| J7 (3.5 mm) | Headphone Out | Tip=L, Ring=R, Sleeve=GND |
| J8 (3.5 mm) | Line In Left | Tip=L, Ring=GND, Sleeve=GND |
| J9 (3.5 mm) | Line In Right | Tip=R, Ring=GND, Sleeve=GND |
| J10 (2×5 header) | CV Inputs | CV1–CV4 + GND + 5V |
| J11 (2×5 header) | SWD Debug | VCC, SWDIO, SWCLK, SWO, GND, NRST |

## Wire Protocol

The companion app communicates with WaveForge over BLE or USB using a binary protocol:

```
Frame: [0xAA] [LEN] [CMD] [DATA...] [CRC8]
  0xAA   = Frame start marker
  LEN    = Number of bytes following (CMD + DATA)
  CMD    = Command ID (1 byte)
  DATA   = Payload (0–252 bytes)
  CRC8   = CRC-8 over [LEN, CMD, DATA...], polynomial 0x07
```

See `app/utils/protocol.js` for full command IDs and parser implementation.

## Key Design Decisions

1. **STM32H743** provides 480 MHz Cortex-M7 with hardware double-precision FPU — enough for 64-voice wavetable synthesis with per-voice filter + envelope entirely in software.
2. **WM8778** codec offers 110 dB SNR, 24-bit, 96 kHz — professional audio quality in a small QFN-28 package.
3. **QSPI-mapped wavetable flash** (W25Q256, 32 MB) enables zero-copy XIP access to 2048-sample wavetables from the synthesis engine.
4. **No on-board display** — all UI handled via MIDI CC, CV inputs, or the companion app. This reduces cost, power, and firmware complexity.
5. **Analog/digital ground split** with single-point bridging near the codec minimizes digital noise coupling into the audio path.

## License

MIT License — see [LICENSE](../LICENSE) for details.

Copyright © 2026 WaveForge Open Hardware Project