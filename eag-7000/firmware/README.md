# EAG-7000 — Firmware

Firmware for the EAG-7000 Edge AI Gateway targeting the **Cortex-M4F** core of the NXP i.MX8M Plus SoC.

The M4F core runs **FreeRTOS** and handles real-time tasks that cannot tolerate Linux latency:
- **CAN-FD message processing** (real-time <100µs response)
- **Modbus RTU slave** (UART2, 115200 baud)
- **Watchdog monitoring** of A72 Linux via MU mailbox heartbeat
- **Power state control** (trigger A72 sleep/wake via MU mailbox)

## Architecture

```
┌─────────────────────────────────────────┐
│           M4F FreeRTOS Firmware         │
│                                         │
│  ┌──────────┐ ┌──────────┐ ┌────────┐ │
│  │ CAN-FD   │ │ Modbus   │ │  MU    │ │
│  │ Gateway  │ │ RTU      │ │ Mailbox│ │
│  │ (SPI2/3) │ │ (UART2)  │ │ (A72↔M4)│ │
│  └────┬─────┘ └────┬─────┘ └───┬────┘ │
│       │             │            │      │
│  ┌────┴─────────────┴────────────┴───┐ │
│  │        Shared Message Queue        │ │
│  │     (FreeRTOS Queue / Ring Buf)    │ │
│  └──────────────┬────────────────────┘ │
│                 │                        │
│  ┌──────────────┴────────────────────┐  │
│  │     Board Support Package (BSP)    │  │
│  │  - Clock init (CCM)               │  │
│  │  - GPIO init (IOMUX)              │  │
│  │  - SPI driver (ECSPI2/3)          │  │
│  │  - UART driver (UART1/2/3)         │  │
│  │  - I2C driver (I2C1/2)            │  │
│  └────────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

## Building

```bash
# Install ARM GNU Toolchain for Cortex-M4
sudo apt install gcc-arm-none-eabi

# Build firmware
make clean && make

# Output: build/eag7000-m4f.elf
# Flash via A72 Linux remoteproc:
#   echo build/eag7000-m4f.elf > /sys/class/remoteproc/remoteproc0/firmware
#   echo start > /sys/class/remoteproc/remoteproc0/state
```

## Wire Protocol

See `../app/utils/protocol.js` for the companion app protocol definition.
The M4F communicates with the A72 Linux side via the MU (Message Unit) mailbox
using 32-bit message registers. See `registers.h` for MU register addresses.

## File Structure

| File | Description |
|------|-------------|
| `Makefile` | Build system for ARM Cortex-M4F |
| `main.c` | Main application entry, FreeRTOS task creation |
| `board.h` | Board-level pin definitions, clock frequencies, LED macros |
| `registers.h` | MMIO register addresses (i.MX8MP), bit masks |
| `drivers/spi.c` | ECSPI2/3 driver for MCP2518FD CAN-FD controllers |
| `drivers/spi.h` | SPI driver header |
| `drivers/uart.c` | UART driver for debug console (UART1) and Modbus (UART2) |
| `drivers/uart.h` | UART driver header |
| `drivers/i2c.c` | I2C driver for PMIC (I2C1) and sensors (I2C2 via mux) |
| `drivers/i2c.h` | I2C driver header |
| `drivers/gpio.c` | GPIO and IOMUX initialization |
| `drivers/gpio.h` | GPIO driver header |
| `drivers/mu.c` | MU mailbox driver for A72↔M4 communication |
| `drivers/mu.h` | MU mailbox driver header |
| `drivers/mcp2518fd.c` | MCP2518FD CAN-FD controller driver (M4F side) |
| `drivers/mcp2518fd.h` | MCP2518FD driver header |