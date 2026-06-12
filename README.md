# EAG-7000 — Edge AI Gateway

A high-performance, fanless industrial Edge AI Gateway computer designed for real-time ML inference at the network edge. Targets smart factory, intelligent surveillance, and autonomous logistics applications with sub-10ms sensor-to-action latency.

## System Specifications

| Parameter | Value |
|---|---|
| **AI Throughput** | ≥30 TOPS INT8 (2.3 TOPS SoC + 26 TOPS Hailo-8) |
| **CPU** | NXP i.MX8M Plus — 2x Cortex-A72 @ 2.0 GHz + 1x Cortex-M4F @ 800 MHz |
| **Memory** | 2× Micron MT53D512M32D4DS — 8GB LPDDR4X @ 4267 MT/s |
| **Storage** | 32GB eMMC 5.1 (boot) + 128GB NVMe M.2 2242 (data) |
| **Networking** | 2× 2.5GbE (AQR111C) + 1× 5GbE USB-C |
| **Industrial IO** | 2× CAN-FD, 3× SPI, 4× I2C, 5× UART, 1× MIPI-CSI2 |
| **Power** | 12V DC / PoE+ (802.3at), ≤15W typical, 25W peak |
| **Form Factor** | 100mm × 72mm × 25mm, DIN-rail mountable |
| **Operating Temp** | -40°C to +85°C (industrial) |

## Documentation Structure

Each phase has its own detailed document:

| Phase | Document | Description |
|---|---|---|
| **Phase 1** | [`docs/phase1/conceptual_architecture.md`](docs/phase1/conceptual_architecture.md) | System requirements, block diagram, bus topology, power domains |
| **Phase 2** | [`docs/phase2/component_selection_schematics.md`](docs/phase2/component_selection_schematics.md) | BOM, pinouts, netlists, decoupling networks, impedance matching |
| **Phase 3** | [`docs/phase3/pcb_blueprints_layout.md`](docs/phase3/pcb_blueprints_layout.md) | PCB stackup, routing rules, DDR length matching, thermal management |
| **Phase 4** | [`docs/phase4/software_stack_implementation.md`](docs/phase4/software_stack_implementation.md) | Bootloader, Linux kernel, HAL, MCP2518FD CAN-FD driver, device tree |

## Quick Start

### Build the Software Stack

```bash
# Set up cross-compilation toolchain
export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64

# Build U-Boot SPL + U-Boot proper
make eag7000_defconfig
make -j$(nproc)

# Build Linux kernel with EAG-7000 patches
make imx8mp_eag7000_defconfig
make -j$(nproc) Image dtbs modules

# Build M4F firmware (FreeRTOS)
cd software/m4f-firmware/
make CROSS_COMPILE=arm-none-eabi-
```

### Flash to eMMC

```bash
# Via U-Boot console:
=> ums 0 mmc 0    # Expose eMMC as USB mass storage
# Then flash from host:
dd if=spl.bin of=/dev/sdX bs=1K seek=0
dd if=u-boot.bin of=/dev/sdX bs=1K seek=64
dd if=Image of=/dev/sdX4 bs=1M   # Kernel into userdata partition
```

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────────┐
│                     EAG-7000 SYSTEM ARCHITECTURE                │
│                                                                  │
│  ┌──────────────┐    AXI4 (256-bit)    ┌──────────────────┐    │
│  │  SoC:        │◄────────────────────►│  LPDDR4X 8GB     │    │
│  │  i.MX8M Plus │   25.6 GB/s          │  (2× MT53D512)   │    │
│  │              │                       └──────────────────┘    │
│  │  2× A72      │    PCIe Gen3 x1      ┌──────────────────┐    │
│  │  1× M4F       │◄────────────────────►│  Hailo-8 NPU     │    │
│  │  GC7000L GPU │   8 GT/s             │  26 TOPS INT8    │    │
│  │  2.3T NPU    │                       └──────────────────┘    │
│  └──────────────┘                                               │
│         │                                                        │
│  ┌──────┴───────────────────────────────────────────────────┐   │
│  │  2× 2.5GbE │ USB 3.1 │ 2× CAN-FD │ SPI │ I2C │ UART   │   │
│  └───────────────────────────────────────────────────────────┘   │
│  ┌───────────────────────────────────────────────────────────┐   │
│  │  PMIC (PCA9450) │ PoE+ (TPS2378) │ RTC (PCF2129)       │   │
│  └───────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
```

## Key Design Decisions

1. **i.MX8M Plus** over Rockchip RK3588 — 15-year industrial availability, verified -40°C to 85°C operation
2. **LPDDR4X** over DDR4 — 1.1V IO (vs 1.2V) critical for passive cooling at 15W TDP
3. **eMMC boot** over SD card — vibration reliability in industrial environments
4. **Hailo-8 M.2** accelerator — 26 TOPS at 2.5W, best TOPS/W in form factor
5. **Dual CAN-FD via SPI** (MCP2518FD) — M4F handles real-time fieldbus with deterministic <1ms response

## License

Hardware design files: CERN-OHL-S v2  
Software: GPL-2.0 (kernel drivers), MIT (userspace tools)