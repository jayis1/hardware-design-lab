# Chronos Pulser — Precision TDR & Sub-Nanosecond Pulse Generator

A portable, USB-powered precision Time-Domain Reflectometer (TDR) and sub-nanosecond pulse generator for field engineers, RF technicians, and hardware validation labs.

## Specifications

| Parameter | Value |
|-----------|-------|
| **Pulse rise time (10%–90%)** | < 100 ps (typ. 65 ps) |
| **Pulse amplitude** | 100–500 mV into 50 Ω (programmable) |
| **Pulse width (FWHM)** | ~300 ps (SRD-limited) |
| **TDC timing resolution** | 10 ps (RMS jitter < 5 ps) |
| **ADC sample rate** | 250 MSPS, 12-bit |
| **Effective bandwidth** | > 3 GHz (analog frontend) |
| **Max cable length** | ~100 m (RG-58), ~500 m (low-loss) |
| **Impedance range** | 10 Ω – 200 Ω (±2%) |
| **Distance accuracy** | ±2 mm (short), ±0.1% (long) |
| **Repetition rate** | 1 Hz – 1 MHz |
| **Averaging depth** | 1 – 65,536 acquisitions |
| **Interface** | USB 3.0 Type-C (5 Gbps) |
| **Power** | USB bus-powered, ~5 W |
| **Dimensions** | 120 × 80 × 30 mm (enclosure) |
| **PCB** | 100 × 70 mm, 4-layer FR-4 |
| **Weight** | < 200 g |
| **BOM cost** | ~$280 (excl. enclosure) |

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                     CHRONOS PULSER                           │
│                                                              │
│  USB 3.0 ──► FT601Q ──► Lattice ECP5-45F FPGA               │
│                          │                                   │
│              ┌───────────┼───────────┐                      │
│              ▼           ▼           ▼                       │
│         Pulse Gen    ADC Capture   TDC Core                  │
│         (SRD+BFR92A) (AD9230-250) (10 ps)                   │
│              │           │           │                       │
│              └───────────┼───────────┘                      │
│                          ▼                                   │
│                   Directional Coupler                        │
│                          │                                   │
│                     SMA Output                               │
└──────────────────────────────────────────────────────────────┘
```

### Key Components

| Component | Part Number | Role |
|-----------|-------------|------|
| FPGA | Lattice LFE5U-45F-6BG381C | Central processing, TDC, ADC capture |
| USB Bridge | FTDI FT601Q-B-T | USB 3.0 to 32-bit FIFO |
| ADC | Analog Devices AD9230BCPZ-250 | 12-bit 250 MSPS digitizer |
| VGA | TI LMH6517SQ/NOPB | Variable gain amplifier, 1.2 GHz |
| SRD | MACOM MA44769-287T | Step recovery diode, 65 ps |
| Transistor | Nexperia BFR92A | Avalanche pulse generator |
| Coupler | Mini-Circuits TCD-10-1X+ | Directional coupler, 5–1000 MHz |
| DDR3 | Micron MT41K128M16JT-125 | 256 MB acquisition buffer |
| Oscillator | SiTime SiT9365 | 250 MHz LVDS reference |
| Temp Sensor | TI TMP117AIDRVR | ±0.1°C accuracy |
| Flash | Winbond W25Q128JVSIQ | 16 MB SPI NOR |

## Directory Structure

```
chronos-pulser/
├── README.md                          # This file
├── phase1_conceptual_architecture.md  # System architecture & design goals
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists
├── phase3_pcb_blueprints_layout.md    # PCB stackup, routing rules, thermal
├── phase4_software_stack.md           # Boot, MMIO map, drivers, build
├── kicad/
│   ├── device.kicad_pro               # Project file with net classes
│   ├── device.kicad_sch               # Full schematic (300+ lines)
│   └── device.kicad_pcb               # Board layout with stackup
├── firmware/
│   ├── Makefile                       # RISC-V cross-compile build
│   ├── main.c                         # SPL, init, main loop (300+ lines)
│   ├── board.h                        # Complete pin definitions (150+ lines)
│   ├── registers.h                    # Full MMIO register map (150+ lines)
│   ├── usb_descriptors.h              # USB 3.0 descriptors (150+ lines)
│   └── drivers/
│       ├── adc_driver.h / .c          # AD9230-250 driver with DMA (200+ lines)
│       ├── tdc_driver.h / .c          # TDC core driver with calibration (200+ lines)
│       ├── pulse_driver.h / .c        # Pulse generator driver (200+ lines)
│       ├── vga_driver.h / .c          # LMH6517 VGA driver (200+ lines)
│       ├── i2c_driver.h / .c         # I²C master for TMP117 (200+ lines)
│       ├── spi_flash.h / .c           # W25Q128 SPI flash driver (200+ lines)
│       ├── usb_protocol.h / .c        # Binary protocol handler (200+ lines)
│       ├── temp_monitor.h / .c        # TMP117 temperature monitor (200+ lines)
│       ├── clock_config.h / .c        # PLL configuration
│       ├── gpio_init.h / .c           # GPIO initialization
│       ├── error_handler.h / .c       # Error logging
│       ├── calibration.h / .c         # Calibration manager
│       └── led_controller.h / .c      # WS2812 RGB LED driver
└── app/
    ├── package.json                   # React Native dependencies
    ├── App.js                         # Root with tab navigation (100+ lines)
    ├── screens/
    │   ├── DashboardScreen.js         # Main control dashboard (150+ lines)
    │   ├── WaveformScreen.js          # TDR waveform viewer (150+ lines)
    │   ├── SettingsScreen.js          # Settings & calibration (150+ lines)
    │   └── DeviceInfoScreen.js        # Device diagnostics (150+ lines)
    ├── components/
    │   ├── DeviceContext.js           # State management & hooks (80+ lines)
    │   └── TelemetryGauge.js          # Circular gauge component (80+ lines)
    └── utils/
        └── protocol.js               # Binary protocol engine (300+ lines)
```

## Quick Start

### Prerequisites

- **FPGA toolchain**: Yosys, nextpnr-ecp5, Project Trellis, ecpprog
- **Firmware toolchain**: RISC-V GCC (`riscv64-unknown-elf-gcc`)
- **PCB design**: KiCad 8.0+
- **Companion app**: Node.js 18+, React Native CLI

### Build FPGA Gateware

```bash
cd firmware
make -f Makefile.ecp5 all
# Output: build/chronos_pulser.bit
```

### Build Soft CPU Firmware

```bash
cd firmware
make all
# Output: build/firmware.bin, build/firmware.hex
```

### Flash Device

```bash
# Program FPGA bitstream + firmware to SPI flash
ecpprog -s build/chronos_pulser.bit
ecpprog -o 0x140000 -w build/firmware.bin
```

### Run Companion App

```bash
cd app
npm install
npx react-native start
# Connect to Chronos Pulser via USB
```

## USB Protocol

The host communicates via a binary framed protocol over USB 3.0 bulk endpoints:

- **Sync**: 0xAA (command), 0x55 (response)
- **CRC**: CRC-16-CCITT over header + payload
- **Max payload**: 1024 bytes
- **Commands**: Ping, Get Info, Acquisition, TDC, Pulse Config, VGA Gain, Temperature, LED, Flash R/W

See `app/utils/protocol.js` for the full JavaScript implementation and `firmware/drivers/usb_protocol.c` for the device-side handler.

## Calibration

### Factory Calibration
1. TDC bin-by-bin calibration (code density test)
2. ADC offset/gain calibration
3. System propagation delay measurement
4. Temperature compensation polynomial

### Field Calibration
1. **Open**: Leave port open → normalize 100% positive reflection
2. **Short**: Connect precision short → verify timing
3. **Load**: Connect 50 Ω terminator → verify zero reflection

## License

Open Source Hardware — Released under CERN Open Hardware Licence Version 2 — Permissive (CERN-OHL-P-2.0).

Software components: MIT License.

---

**Chronos Pulser** — Designed by Jayis1 Hardware Design Lab, 2026.
