# Aether-Link — PCIe 4.0 x8 NVMe-oF/TCP Hardware Accelerator

**A production-grade hardware design for a PCIe add-in card that offloads NVMe-over-Fabric/TCP storage networking into FPGA hardware, achieving line-rate 100GbE throughput with sub-10µs latency.**

---

## Specifications

| Parameter | Value |
|---|---|
| **Host Interface** | PCI Express 4.0 x8 (16 GT/s per lane, ~15.75 GB/s) |
| **Network Interface** | 2× 100GbE QSFP28 (IEEE 802.3bj, 200 Gbps aggregate) |
| **FPGA** | Xilinx Kintex-7 XC7K325T-2FFG900C (326K LUTs, 840 DSP, 16 GTX) |
| **Memory** | 8 GB DDR4-3200 ECC (2× MT40A512M16, dual-channel) |
| **Flash** | 512 Mb QSPI (MT25QU512) for FPGA configuration + golden image |
| **NVMe-oF Protocol** | NVMe-oF/TCP 1.1 with CRC32C, DH-HMAC-CHAP authentication |
| **TCP Offload** | Full hardware TOE: TSO, LRO, DCTCP, 256 concurrent connections |
| **4KB Random Read IOPS** | >2,000,000 (single port) |
| **4KB Read Latency** | <8 µs (PCIe-to-wire) |
| **Power** | <75W (PCIe slot power only) |
| **Form Factor** | Full-Height, Half-Length (FHHL) PCIe card, 167.65×111.15 mm |
| **PCB** | 12-layer FR-4 (Isola 370HR), ENIG finish, impedance-controlled |
| **Cooling** | Dual 40mm PWM fans + custom aluminum heatsink |
| **Debug** | USB Micro-B (FT232H: JTAG + UART), 4× bi-color LEDs |
| **Management** | MicroBlaze soft processor, I2C sensors (TMP117, INA226, EMC2301) |
| **Companion App** | React Native (iOS/Android) — telemetry, connection management, settings |

---

## Directory Structure

```
aether-link/
├── README.md                              ← You are here
├── phase1_conceptual_architecture.md      ← System architecture, block diagrams, data flow
├── phase2_component_selection_schematics.md ← BOM, pinouts, netlists, decoupling, pull-ups
├── phase3_pcb_blueprints_layout.md        ← PCB stackup, routing rules, via strategy, thermal
├── phase4_software_stack.md               ← Boot flow, MMIO registers, drivers, device tree
├── kicad/
│   ├── device.kicad_pro                   ← Project file with net classes and design rules
│   ├── device.kicad_sch                   ← Schematic (power tree, FPGA, peripherals)
│   └── device.kicad_pcb                   ← PCB layout (stackup, footprints, zones, keepouts)
├── firmware/
│   ├── Makefile                           ← Cross-compile for MicroBlaze (mb-gcc)
│   ├── main.c                             ← SPL/board init, main loop, error handling (709 lines)
│   ├── board.h                            ← Full pin definitions with alternate functions (300+ lines)
│   ├── registers.h                        ← Complete MMIO register map (400+ lines)
│   ├── usb_descriptors.h                  ← USB descriptors for FT232H debug bridge (300+ lines)
│   └── drivers/
│       ├── toe_driver.h                   ← TCP Offload Engine driver header
│       ├── toe_driver.c                   ← TOE implementation (357 lines)
│       ├── nvmeof_pdu.h                   ← NVMe-oF PDU engine header
│       ├── nvmeof_pdu.c                   ← PDU engine implementation (272 lines)
│       ├── gpio_init.c                    ← GPIO, I2C, UART, fan, power, temp drivers (400+ lines)
│       ├── clock_config.c                 ← MMCM/PLL configuration and monitoring (200+ lines)
│       ├── i2c_driver.c                   ← I2C bus abstraction
│       ├── spi_flash.c                    ← QSPI flash driver with FW update support (300+ lines)
│       ├── fan_control.c                  ← Thermal fan curve management
│       ├── power_monitor.c                ← Power telemetry and limit enforcement
│       └── error_handler.c                ← Error logging, recovery, diagnostics
└── app/
    ├── package.json                       ← React Native dependencies
    ├── App.js                             ← Navigation + providers (150+ lines)
    ├── screens/
    │   ├── DashboardScreen.js             ← Real-time telemetry dashboard (400+ lines)
    │   ├── ConnectionsScreen.js           ← NVMe-oF connection management (400+ lines)
    │   └── SettingsScreen.js              ← Device configuration + diagnostics (400+ lines)
    ├── components/
    │   ├── DeviceContext.js               ← TCP socket, command/response, event handling (300+ lines)
    │   └── TelemetryGauge.js              ← Circular SVG gauge with warning zones (150+ lines)
    └── utils/
        └── protocol.js                    ← Binary wire protocol, CRC32, frame builder/parser (500+ lines)
```

---

## Quick Start

### 1. Review the Design

Start with the phase documents for a complete understanding:

```bash
cat phase1_conceptual_architecture.md   # Why and how
cat phase2_component_selection_schematics.md  # What parts
cat phase3_pcb_blueprints_layout.md     # How to build the board
cat phase4_software_stack.md            # How to program it
```

### 2. Open KiCad Design

```bash
# Requires KiCad 8.0+
kicad kicad/device.kicad_pro
```

### 3. Build Firmware

```bash
cd firmware/
# Requires Xilinx Vitis 2023.2 or later with MicroBlaze toolchain
make CROSS_COMPILE=mb- all
```

### 4. Build Companion App

```bash
cd app/
npm install
npx react-native start
# In another terminal:
npx react-native run-android  # or run-ios
```

### 5. Deploy to Device

1. Synthesize FPGA bitstream in Vivado using the RTL referenced in phase4.
2. Program bitstream to QSPI flash via JTAG (FT232H USB bridge).
3. Insert card into PCIe 4.0 x8 slot.
4. Boot host Linux — NVMe driver binds automatically.
5. Load management driver: `insmod aether_mgmt.ko`
6. Connect companion app to host IP:4420.

---

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────┐
│                     HOST CPU (Linux)                          │
│  ┌──────────────┐  ┌────────────────────────────────────┐    │
│  │ nvme.ko      │  │ aether_mgmt.ko (sysfs interface)   │    │
│  │ (stock NVMe) │  │ - Connection lifecycle             │    │
│  │              │  │ - Firmware update                   │    │
│  └──────┬───────┘  │ - Telemetry (temp, power, stats)   │    │
│         │          └──────────────┬─────────────────────┘    │
│         │    PCIe 4.0 x8          │ Vendor-specific commands │
└─────────┼─────────────────────────┼──────────────────────────┘
          │                         │
┌─────────▼─────────────────────────▼──────────────────────────┐
│              AETHER-LINK PCIe CARD (XC7K325T)                 │
│                                                               │
│  ┌──────────┐  ┌──────────────┐  ┌────────────┐             │
│  │ PCIe EP  │──► NVMe Cmd     │──► NVMe-oF    │             │
│  │ Gen4 x8  │  │ Processor    │  │ PDU Engine  │             │
│  │ 8 BARs   │  │ SQ/CQ mgmt   │  │ Capsule     │             │
│  │ MSI-X    │  │ PRP/SGL walk │  │ CRC32C      │             │
│  └──────────┘  └──────────────┘  └─────┬───────┘             │
│                                        │                      │
│  ┌─────────────────────────────────────▼──────────────────┐  │
│  │              TCP/IP Offload Engine (TOE)                │  │
│  │  256 connections, TSO, LRO, DCTCP, ARP, ICMP           │  │
│  └────────────────────┬──────────────┬────────────────────┘  │
│                       │              │                        │
│  ┌────────────────────▼──┐  ┌───────▼───────────────────┐  │
│  │ 100GbE CMAC #0        │  │ 100GbE CMAC #1             │  │
│  │ QSFP28 Port 0          │  │ QSFP28 Port 1              │  │
│  └────────────────────────┘  └────────────────────────────┘  │
│                                                               │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ DDR4-3200 8GB (dual-channel) + QSPI Flash 512Mb        │  │
│  │ I2C: TMP117×2, INA226, EMC2301 | USB: FT232H JTAG/UART│  │
│  └────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

---

## Key Design Decisions

| Decision | Rationale |
|---|---|
| **Kintex-7 vs UltraScale+** | Sufficient logic at lower cost/power; PCIe Gen4 hard IP available |
| **Dual 100GbE vs single 200GbE** | Redundancy, multipathing, standard QSFP28 modules |
| **DDR4-3200 vs HBM** | Cost-effective; 51.2 GB/s aggregate exceeds PCIe + dual 100GbE needs |
| **NVMe-oF/TCP vs RoCEv2** | TCP runs over any fabric, routable, no DCB/PFC required |
| **12-layer PCB** | Required for signal integrity at 25.78 Gbps and DDR4-3200 |
| **FHHL form factor** | Fits standard PCIe slots in servers and workstations |

---

## Compliance

- **PCI Express**: PCIe Base 4.0, CEM 4.0
- **NVMe**: NVM Express 1.4c, NVMe-oF 1.1
- **Ethernet**: IEEE 802.3bj (100GBASE-CR4/SR4/KR4)
- **EMC**: FCC Part 15 Class A, EN 55032 Class A
- **Safety**: IEC 62368-1, UL 62368-1
- **Environmental**: RoHS 2/3 (EU 2011/65/EU, 2015/863)

---

## License

Hardware Design: CERN Open Hardware Licence Version 2 — Strongly Reciprocal (CERN-OHL-S-2.0)

Firmware: Apache License 2.0

Companion App: MIT License

---

*Designed by Nous Research — Hardware Design Lab*
*Revision 1.0 — June 2025*
