# Nebula Matrix — Professional LED Matrix Display Engine

**FPGA-driven 256×256 RGB LED controller with 16-bit color, 120 Hz refresh, and 16× HUB75E outputs.**

---

## Specifications

| Parameter | Value |
|-----------|-------|
| **Max Pixel Matrix** | 256 × 256 (65,536 RGB LEDs) |
| **HUB75E Ports** | 16 (each drives up to 64×64 panel) |
| **Refresh Rate** | 120 Hz at full resolution |
| **Color Depth** | 16 bits per channel (48 bpp, 65,536 levels) |
| **Frame Buffer** | 1 GB DDR3L (2× 512 MB, double-buffered) |
| **FPGA** | Xilinx Artix-7 XC7A100T-2FGG484C (101K LUTs) |
| **Co-Processor** | STM32H743VIT6 Cortex-M7 @ 480 MHz |
| **Ethernet** | Gigabit (Art-Net 4, sACN E1.31, custom binary protocol) |
| **HDMI Input** | 1080p60 via ADV7611 |
| **USB** | USB-C RNDIS for web configuration |
| **SD Card** | UHS-I microSD for standalone playback |
| **Power** | 12V DC, < 25W typical |
| **Board Size** | 170 × 170 mm (Mini-ITX) |
| **PCB Layers** | 6-layer controlled impedance |
| **Temperature Range** | -20°C to +70°C (industrial) |
| **Cooling** | 2× 40mm PWM fans with PID control |

---

## Directory Structure

```
nebula-matrix/
├── README.md                          # This file
├── phase1_conceptual_architecture.md  # System architecture & design goals
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists
├── phase3_pcb_blueprints_layout.md    # PCB stackup, routing rules, thermal
├── phase4_software_stack.md           # Boot, drivers, FPGA Verilog, FreeRTOS
├── kicad/
│   ├── device.kicad_pro               # Project file with net classes & DRC
│   ├── device.kicad_sch               # Full schematic (300+ lines)
│   └── device.kicad_pcb               # Board outline, stackup, keepout zones
├── firmware/
│   ├── Makefile                       # Cross-compile for ARM Cortex-M7
│   ├── main.c                         # SPL init, peripheral setup, main loop
│   ├── board.h                        # Complete pin definitions (150+ lines)
│   ├── registers.h                    # Full MMIO register map (150+ lines)
│   ├── usb_descriptors.h             # USB RNDIS descriptors (150+ lines)
│   └── drivers/
│       ├── hub75e_spi_driver.h/.c     # SPI DMA pixel stream driver (200+ lines)
│       └── fpga_uart_driver.h/.c      # UART command interface driver (200+ lines)
└── app/
    ├── package.json                   # React Native dependencies
    ├── App.js                         # Navigation & connection management
    ├── screens/
    │   ├── ControlScreen.js           # Main control panel (150+ lines)
    │   ├── MonitorScreen.js           # Live status dashboard (150+ lines)
    │   ├── SettingsScreen.js          # Configuration (150+ lines)
    │   └── FirmwareScreen.js          # OTA firmware update (150+ lines)
    ├── components/
    │   └── DeviceConnection.js        # Connection modal component (80+ lines)
    └── utils/
        └── protocol.js               # Binary wire protocol with CRC (300+ lines)
```

---

## Quick Start

### 1. Hardware Assembly

1. Fabricate the 6-layer PCB per `phase3_pcb_blueprints_layout.md`
2. Source components per BOM in `phase2_component_selection_schematics.md`
3. Assemble (BGA components require reflow + X-ray inspection)
4. Connect 12V DC power, Ethernet, and LED panels via HUB75E ribbon cables

### 2. FPGA Bitstream

```bash
# Build with Xilinx Vivado 2024.1+
source /opt/Xilinx/Vivado/2024.1/settings64.sh
vivado -mode batch -source build_fpga.tcl
# Output: nebula_matrix.bit, nebula_matrix.mcs
```

### 3. MCU Firmware

```bash
cd firmware/
# Requires: arm-none-eabi-gcc, STM32H7 HAL, FreeRTOS, LWIP, FatFS, TinyUSB
make clean
make -j$(nproc)
# Flash via ST-Link:
make flash
```

### 4. Companion App

```bash
cd app/
npm install
npx react-native start
# In another terminal:
npx react-native run-android  # or run-ios
```

### 5. First-Time Setup

1. Connect the companion app to the device (default IP: 192.168.1.100, port 6454)
2. Configure matrix dimensions and panel scan type in Settings
3. Select input source (Ethernet for Art-Net/sACN, HDMI, or SD Card)
4. Adjust brightness, contrast, and gamma to match your panels
5. Start streaming content!

---

## Input Sources

| Source | Protocol | Use Case |
|--------|----------|----------|
| **Ethernet** | Art-Net 4, sACN E1.31, Custom binary | Live lighting control, media servers |
| **HDMI** | 1080p60 RGB | Direct video feed, media players |
| **SD Card** | Raw 48bpp pixel stream | Standalone playback, digital signage |
| **Test Pattern** | Internal generator | Panel testing, alignment, burn-in |

---

## Communication Protocols

### Ethernet Binary Protocol (UDP, port 6454)

Frame format: `[Magic "NEBU" 4B] [CmdID 2B] [PayloadLen 2B] [Payload 0-1400B] [CRC-16 2B]`

Full API documented in `app/utils/protocol.js` and `phase4_software_stack.md`.

### UART Command Protocol (115200 8N1, STM32 ↔ FPGA)

Frame format: `[SYNC 0xA5] [CMD 1B] [ADDR_HI 1B] [ADDR_LO 1B] [DATA 0-4B] [CRC-8 1B]`

Register map documented in `firmware/registers.h`.

### SPI Pixel Stream (50 MHz, STM32 → FPGA)

32-bit words: `[CMD 4b] [X 12b] [Y 16b]` + `[R 16b] [G 16b]` + `[B 16b] [Reserved 16b]`

---

## Power Tree

```
12V DC Input
  ├── 5.0V @ 8A  → HUB75E level shifters, USB VBUS
  ├── 3.3V @ 3A  → FPGA I/O, STM32H7, PHYs, sensors
  ├── 1.8V @ 2A  → FPGA Aux, ADV7611, KSZ9031
  ├── 1.35V @ 3A → DDR3L VDDQ, FPGA VCCO_DDR
  ├── 1.0V @ 6A  → FPGA VCCINT (Core)
  └── 1.2V @ 1A  → FPGA VCCBRAM
```

---

## Key Components

| RefDes | Part | Function |
|--------|------|----------|
| U1 | XC7A100T-2FGG484C | FPGA — pixel processing, HUB75E waveform gen |
| U2 | STM32H743VIT6 | Co-processor — networking, USB, SD, management |
| U3 | TPS65218D0RSLR | PMIC — 6 regulators for all voltage rails |
| U4 | Si5351A-B-GT | Clock generator — 8 programmable outputs |
| U5 | KSZ9031RNXIC | Gigabit Ethernet PHY (RGMII) |
| U6 | USB3320C-EZK | USB 2.0 ULPI PHY |
| U7 | ADV7611BSWZ-P | HDMI 1.4 Receiver |
| U8, U9 | AS4C256M16D3LC-12BCN | DDR3L 4Gb (×2, 1 GB total) |
| U10-U25 | SN74LVCH16T245DGGR | 16-bit level shifters (3.3V → 5.0V) |

---

## License

MIT License — see LICENSE file for details.

---

*Designed by Nous Research — 2026*
*Part of the Hardware Design Lab collection*
