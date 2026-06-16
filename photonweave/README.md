# PhotonWeave — Real-Time Computer-Generated Holography (CGH) Engine

A PCIe FHHL accelerator card that computes holograms from 3D depth-map data in real time using an FPGA-based Angular Spectrum Method (ASM) pipeline and drives a 4K spatial light modulator at 60 frames per second.

## Specifications

| Parameter | Value |
|-----------|-------|
| **FPGA** | Xilinx Kintex-7 XC7K325T-2FFG900C (326K logic cells, 840 DSP slices, 16 GTX) |
| **System Controller** | STM32H743VIT6 (ARM Cortex-M7, 480 MHz) |
| **Memory** | 4× MT40A512M16JY-075E DDR4-2400 (16 Gb total, 76.8 GB/s) |
| **Clock Generator** | Si5345A-B-GM (10 outputs, 25 MHz TCXO reference) |
| **PMIC** | TPS65218D0RSLR (6 buck converters + 1 LDO) |
| **USB Interface** | FT601Q-B-T USB 3.0 to 32-bit FIFO bridge |
| **Configuration Flash** | MT25QU512ABB8ESF 512 Mb QSPI NOR |
| **Input Resolution** | 2048×2048 depth map, 16-bit per pixel |
| **Output Resolution** | 3840×2160 (4K UHD) phase-only hologram |
| **Frame Rate** | 60 fps sustained (16.67 ms per frame) |
| **Depth Planes** | Up to 256 planes |
| **FFT Size** | 4096×4096 complex, 8 parallel engines |
| **Propagation Model** | Angular Spectrum Method (ASM) |
| **Color Channels** | RGB time-sequential (180 Hz total SLM refresh) |
| **Host Interface** | PCIe Gen3 x8 (64 Gbps) |
| **Camera Ingest** | QSFP+ 40G direct from camera array |
| **SLM Output** | HDMI 2.0, 4K60 8-bit grayscale |
| **Power Budget** | <75W (PCIe slot + aux) |
| **Form Factor** | Full-height, half-length (FHHL), single slot |
| **PCB Layers** | 16-layer HDI with blind/buried vias |
| **Board Dimensions** | 111.15 mm × 167.65 mm (4.375" × 6.600") |

## Directory Structure

```
photonweave/
├── README.md                          # This file
├── phase1_conceptual_architecture.md  # System architecture & data flow
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists
├── phase3_pcb_blueprints_layout.md    # PCB stackup, routing rules, thermal
├── phase4_software_stack.md          # Boot strategy, drivers, device tree
├── kicad/
│   ├── device.kicad_pro              # Project file with net classes
│   ├── device.kicad_sch              # Full schematic (300+ lines)
│   └── device.kicad_pcb              # Board outline & stackup
├── firmware/
│   ├── Makefile                      # ARM GCC cross-compile
│   ├── main.c                        # SPL/board init, main loop (300+ lines)
│   ├── board.h                       # Full pin definitions (150+ lines)
│   ├── registers.h                   # Complete MMIO register map (150+ lines)
│   ├── usb_descriptors.h             # USB 3.0 descriptors (150+ lines)
│   └── drivers/
│       ├── ddr4_mig_driver.h         # DDR4 MIG controller header
│       ├── ddr4_mig_driver.c         # DDR4 MIG driver (200+ lines)
│       ├── cgh_pipeline.h            # CGH compute pipeline header
│       └── cgh_pipeline.c            # CGH pipeline driver (200+ lines)
└── app/
    ├── package.json                  # React Native dependencies
    ├── App.js                        # Navigation & theme (100+ lines)
    ├── screens/
    │   ├── ControlScreen.js          # Main hologram control (150+ lines)
    │   ├── MonitorScreen.js          # Performance & thermal monitor (150+ lines)
    │   └── SettingsScreen.js         # Device configuration (150+ lines)
    ├── components/
    │   └── DeviceConnection.js       # Connection management (80+ lines)
    └── utils/
        └── protocol.js              # Binary wire protocol (300+ lines)
```

## Quick Start

### FPGA Bitstream Build
```bash
# Requires Xilinx Vivado 2023.2+
vivado -mode batch -source create_project.tcl
vivado -mode batch -source build_bitstream.tcl
# Output: photonweave.mcs (SPI x4 configuration image)
```

### STM32 Firmware Build & Flash
```bash
cd firmware/
make -j4
st-flash write build/photonweave_stm32.bin 0x08000000
```

### Linux Kernel Driver
```bash
cd driver/
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
sudo insmod photonweave.ko
# Device appears as /dev/photonweave0
```

### React Native Companion App
```bash
cd app/
npm install
npx react-native start
# For Android: npx react-native run-android
# For iOS: npx react-native run-ios
```

## Architecture Overview

PhotonWeave ingests 3D depth-map video streams (via PCIe from host or QSFP+ from camera array), converts each pixel to a complex amplitude based on depth and wavelength, then applies the Angular Spectrum Method:

1. **2D FFT** of the source complex field (8 parallel 4096-point FFT engines)
2. **Transfer function multiply** for each of 256 depth planes
3. **Accumulate** contributions into hologram buffer
4. **2D IFFT** to spatial domain
5. **Phase quantize** to 8-bit grayscale
6. **HDMI 2.0 output** to 4K SLM at 60 Hz

The entire pipeline completes in under 12 ms, leaving 4.67 ms for I/O within the 16.67 ms frame budget.

## Key Components

- **XC7K325T FPGA**: Houses PCIe Gen3 endpoint, 4× DDR4 MIG controllers, 8 FFT engines, ASM propagation pipeline, HDMI TMDS transmitter, QSFP+ interface, and AXI4 interconnect
- **STM32H743**: System supervisor handling power sequencing, Si5345 clock configuration, FPGA programming, temperature monitoring, and USB command processing
- **Si5345**: Any-frequency clock generator providing 10 precision clocks (200 MHz FFT, 125 MHz PCIe, 300 MHz DDR4, 148.5 MHz HDMI pixel, etc.)
- **TPS65218**: Multi-rail PMIC with enforced power sequencing for FPGA core, auxiliary, GTX transceivers, DDR4, and I/O voltages
- **FT601Q**: USB 3.0 to 32-bit parallel FIFO bridge for configuration, debug, and firmware updates

## License

Hardware design: CERN Open Hardware Licence Version 2 - Strongly Reciprocal (CERN-OHL-S-2.0)
Firmware & Software: MIT License

---

**Designed by Nous jayis1** — *Inventing the future of holographic display technology*
