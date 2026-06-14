# Hardware Design Lab

Full-cycle, open-source hardware device designs — from concept to KiCad schematics to production firmware to companion mobile apps. Every device is fully open: schematics, board layouts, C/Python firmware, and React Native apps. Each device lives in its own subfolder with complete deliverables.

## What's Inside Every Device

Every device folder contains the full stack — not just docs, but real buildable artifacts:

| Category | Files | Description |
|---|---|---|
| **Engineering Docs** | `phase1_conceptual_architecture.md` → `phase4_software_stack.md` | 4-phase design process: requirements, BOM & netlists, PCB layout rules, software stack |
| **KiCad Schematics** | `kicad/*.kicad_pro`, `*.kicad_sch`, `*.kicad_pcb` | Full schematic with real component symbols, net labels, power flags, and board outline with placed footprints |
| **C/Python Firmware** | `firmware/main.c`, `registers.h`, `board.h`, `drivers/`, `Makefile` | Production-quality MCU firmware — init code, drivers with DMA, USB descriptors, wire protocol |
| **React Native App** | `app/App.js`, `screens/`, `components/`, `utils/protocol.js`, `package.json` | Companion mobile app for device control, configuration, and status display |

## Design Methodology

Every device follows the same rigorous 4-phase process:

| Phase | Document | Contents |
|---|---|---|
| **Phase 1** | `phase1_conceptual_architecture.md` | Purpose, performance targets, constraints, block diagram, bus topology, power domains |
| **Phase 2** | `phase2_component_selection_schematics.md` | BOM with real part numbers, pinouts/MUX tables, decoupling networks, netlists, impedance matching |
| **Phase 3** | `phase3_pcb_blueprints_layout.md` | PCB stackup, routing rules, length matching, via strategy, thermal management, DFM notes |
| **Phase 4** | `phase4_software_stack.md` | MMIO/register defs, clock & GPIO init (production C), critical device driver, device tree, bootloader |

## Devices

| # | Device | Status | Description |
|---|---|---|---|
| 1 | [`eag-7000/`](eag-7000/) — Edge AI Gateway | ✅ Docs complete, KiCad/firmware/app pending | i.MX8MP + Hailo-8 NPU, 30 TOPS, fanless industrial, 2×2.5GbE, CAN-FD |
| 2 | [`chronos-rtk/`](chronos-rtk/) — Dual-Frequency RTK GNSS Receiver | ✅ Complete | ZED-F9P + STM32L4 + LoRa mesh, cm-level positioning, 868/915 MHz ISM |
| 3 | [`vortex-sdr/`](vortex-sdr/) — Portable SDR Spectrum Analyzer | ✅ Complete | STM32H743 + iCE40UP5K FPGA + ADF4351 PLL, 100kHz–6GHz, dual-FFT waterfall |

---

## EAG-7000 — Edge AI Gateway

A high-performance, fanless industrial Edge AI Gateway for real-time ML inference at the network edge.

### System Specifications

| Parameter | Value |
|---|---|
| **AI Throughput** | ≥30 TOPS INT8 (2.3 TOPS SoC + 26 TOPS Hailo-8) |
| **CPU** | NXP i.MX8M Plus — 2× Cortex-A72 @ 2.0 GHz + 1× Cortex-M4F @ 800 MHz |
| **Memory** | 2× Micron MT53D512M32D4DS — 8GB LPDDR4X @ 4267 MT/s |
| **Storage** | 32GB eMMC 5.1 (boot) + 128GB NVMe M.2 2242 (data) |
| **Networking** | 2× 2.5GbE (AQR111C) + 1× 5GbE USB-C |
| **Industrial IO** | 2× CAN-FD, 3× SPI, 4× I2C, 5× UART, 1× MIPI-CSI2 |
| **Power** | 12V DC / PoE+ (802.3at), ≤15W typical, 25W peak |
| **Form Factor** | 100mm × 72mm × 25mm, DIN-rail mountable |
| **Operating Temp** | -40°C to +85°C (industrial) |

### Key Design Decisions

1. **i.MX8M Plus** over Rockchip RK3588 — 15-year industrial availability, verified -40°C to 85°C
2. **LPDDR4X** over DDR4 — 1.1V IO for passive cooling at 15W TDP
3. **eMMC boot** over SD card — vibration reliability in industrial environments
4. **Hailo-8 M.2** accelerator — 26 TOPS at 2.5W, best TOPS/W in form factor
5. **Dual CAN-FD via SPI** (MCP2518FD) — M4F handles real-time fieldbus with <1ms response

## Vortex-SDR — Portable SDR Spectrum Analyzer

A portable, handheld spectrum analyzer covering 100 kHz to 6 GHz with dual-channel FFT, real-time waterfall display, capacitive touchscreen, and BLE companion app. Designed for RF engineers, ham radio operators, and EMC compliance testing.

### System Specifications

| Parameter | Value |
|---|---|
| **Frequency Range** | 100 kHz – 6 GHz (dual-conversion superheterodyne) |
| **MCU** | STM32H743ZIT6 — Cortex-M7 @ 480 MHz, 1MB Flash, 1MB RAM |
| **FPGA** | Lattice iCE40UP5K — FFT/waterfall acceleration |
| **PLL Synthesizer** | ADF4351 — 35 MHz to 4.4 GHz (6 GHz with ADF5002 prescaler) |
| **ADC** | AD9645 — 14-bit, 61.44 MSPS dual-channel |
| **Display** | ILI9341 — 2.8" 320×240 TFT with capacitive touch |
| **BLE** | nRF52840 — 2 Mbps BLE 5.0 companion link |
| **Power** | 3.7V LiPo (TPS62A02 buck + AMS1117 LDO), USB-C charging |
| **Form Factor** | 120mm × 75mm × 18mm, handheld |
| **Operating Temp** | -20°C to +55°C (commercial) |

### Key Design Decisions

1. **STM32H743** over STM32F4 — Cortex-M7 @ 480 MHz for real-time FFT processing, dual-issue FPU
2. **iCE40UP5K FPGA** for FFT offload — 5280 LUTs, on-chip RAM for windowing and averaging
3. **Dual-conversion architecture** — 1st LO (ADF4351) + 10.7 MHz IF for image rejection
4. **AD9645 dual ADC** — simultaneous I/Q sampling, 14-bit resolution for dynamic range
5. **Battery-powered with USB-C** — TPS62A02 2A buck for efficiency, MCP73871 charger

## License

| Component | License |
|---|---|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| Python tools & scripts | MIT |
| React Native companion apps | MIT |