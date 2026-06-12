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
| 2 | *More designs added over time by automated cron* | ⏳ | Complex hardware devices with full open stack |

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

## License

| Component | License |
|---|---|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| Python tools & scripts | MIT |
| React Native companion apps | MIT |