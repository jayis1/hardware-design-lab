# Phase 4: Software Stack — Chronos Pulser

## 1. Overview

The Chronos Pulser firmware stack spans three execution domains:

1. **FPGA Gateware** (Verilog, Yosys/nextpnr-ecp5 flow): TDC core, ADC capture pipeline, DDR3 controller, USB bridge, pulse generator sequencer
2. **Soft CPU Firmware** (C, RISC-V RV32IM, GCC): VexRiscv-based command processor running calibration, health monitoring, and protocol handling
3. **Host Companion App** (React Native / JavaScript): UI, waveform rendering, measurement database

This document covers the FPGA gateware register map, soft CPU firmware architecture, boot sequence, device drivers, and build system.

## 2. Boot Strategy

### 2.1 FPGA Configuration Boot Flow

```
Power-On / Reset
    │
    ▼
FPGA internal POR (Power-On Reset) — 50 ms
    │
    ▼
CFG_INITN goes high — FPGA samples CFG_M[2:0] pins
    │  CFG_M[2:0] = 010 (SPI Master, 4-wire)
    ▼
FPGA drives CFG_CSN low, CFG_MCLK starts (2.08 MHz default)
    │
    ▼
FPGA reads bitstream from U13 (W25Q128JVSIQ) via SPI
    │  Bitstream size: ~4.2 Mbit (ECP5-45F)
    │  Configuration time: ~2.0 seconds at 2.08 MHz
    │  (Can be accelerated to ~0.2s with 25 MHz CCLK after initial header)
    ▼
CFG_DONE goes high — FPGA enters user mode
    │
    ▼
Internal global reset released
    │
    ▼
Soft CPU (VexRiscv) boots from internal block RAM
    │  Boot ROM: 8 KB, contains first-stage bootloader
    ▼
Bootloader initializes:
    1. DDR3 controller (LiteDRAM) — calibration run (~100 ms)
    2. SPI flash filesystem (littlefs) — mount
    3. Load main firmware from SPI flash to DDR3 (256 KB)
    4. Jump to DDR3 firmware entry point
    │
    ▼
Main firmware initializes:
    1. Clock tree and PLLs
    2. ADC SPI configuration (gain, offset, test mode off)
    3. VGA SPI configuration (gain setting, bandwidth)
    4. TDC calibration LUT load from flash
    5. USB bridge enumeration (FT601)
    6. Pulse generator (safe defaults: 1 kHz, 250 mV)
    7. Temperature sensor (TMP117) start continuous conversion
    8. Enable interrupts, start main loop
    │
    ▼
Main loop: Process USB commands, manage acquisitions, monitor health
```

### 2.2 Bitstream Storage Layout (W25Q128JVSIQ, 16 MB)

| Offset | Size | Content |
|--------|------|---------|
| 0x000000 | 512 KB | FPGA bitstream (golden image) |
| 0x080000 | 512 KB | FPGA bitstream (update image) |
| 0x100000 | 256 KB | Soft CPU bootloader |
| 0x140000 | 512 KB | Main firmware image |
| 0x1C0000 | 64 KB | TDC calibration LUT |
| 0x1D0000 | 64 KB | Factory calibration data |
| 0x1E0000 | 128 KB | littlefs filesystem (user settings, waveforms) |
| 0x200000 | 14 MB | Reserved (future expansion) |

## 3. MMIO Register Map

The soft CPU accesses all peripherals through a 32-bit Wishbone bus. The address space is 256 MB (28-bit address).

### 3.1 Top-Level Address Map

| Base Address | Size | Peripheral |
|-------------|------|------------|
| 0x00000000 | 256 MB | DDR3 SDRAM (via LiteDRAM) |
| 0x10000000 | 4 KB | Boot ROM (internal BRAM) |
| 0x20000000 | 4 KB | System Controller (SYS) |
| 0x20001000 | 4 KB | UART (debug console) |
| 0x20002000 | 4 KB | Timer / Timestamp Counter |
| 0x20003000 | 4 KB | GPIO Controller |
| 0x20004000 | 4 KB | I²C Master |
| 0x20005000 | 4 KB | SPI Master (flash) |
| 0x20006000 | 4 KB | SPI Master (ADC/VGA config) |
| 0x30000000 | 16 KB | TDC Core |
| 0x30004000 | 16 KB | ADC Capture Engine |
| 0x30008000 | 16 KB | Pulse Generator Controller |
| 0x3000C000 | 16 KB | USB Bridge (FT601 FIFO) |
| 0x30010000 | 4 KB | Acquisition Sequencer |
| 0x40000000 | 64 KB | VexRiscv CPU CSRs (internal) |

### 3.2 System Controller Registers (SYS — 0x20000000)

```
Offset  | Name            | Access | Width | Description
--------|-----------------|--------|-------|-----------------------------
0x00    | SYS_ID          | RO     | 32    | System identifier
        |                 |        |       | [31:16] Device ID = 0x4350 ("CP")
        |                 |        |       | [15:8]  Revision = 0x01
        |                 |        |       | [7:0]   Variant = 0x00
0x04    | SYS_STATUS      | RO     | 32    | System status
        |                 |        |       | [0]     FPGA config done
        |                 |        |       | [1]     DDR3 calibrated
        |                 |        |       | [2]     USB enumerated
        |                 |        |       | [3]     TDC calibrated
        |                 |        |       | [4]     Overtemperature warning
        |                 |        |       | [5]     Overtemperature shutdown
        |                 |        |       | [8]     Power good (all rails)
        |                 |        |       | [16:19] Boot source (0=golden,1=update)
0x08    | SYS_RESET       | WO     | 32    | Software reset control
        |                 |        |       | [0]     Reset TDC core
        |                 |        |       | [1]     Reset ADC capture
        |                 |        |       | [2]     Reset pulse gen
        |                 |        |       | [3]     Reset USB bridge
        |                 |        |       | [31]    Full system reset
0x0C    | SYS_CLK_CTRL    | RW     | 32    | Clock control
        |                 |        |       | [0]     System clock enable
        |                 |        |       | [1]     ADC clock enable (250 MHz)
        |                 |        |       | [2]     TDC clock enable (500 MHz)
        |                 |        |       | [3]     USB clock enable (100 MHz)
        |                 |        |       | [8:15]  ADC clock divider (0=bypass)
0x10    | SYS_LED_CTRL    | RW     | 32    | LED control
        |                 |        |       | [7:0]   LED0 red
        |                 |        |       | [15:8]  LED0 green
        |                 |        |       | [23:16] LED0 blue
        |                 |        |       | [24]    LED update strobe
0x14    | SYS_TEMP        | RO     | 32    | Temperature (from TMP117)
        |                 |        |       | [15:0]  Temperature in 0.0078125°C LSB
        |                 |        |       | [31]    Sign bit (0=positive)
0x18    | SYS_UPTIME_LO   | RO     | 32    | Uptime counter low (microseconds)
0x1C    | SYS_UPTIME_HI   | RO     | 32    | Uptime counter high
0x20    | SYS_SCRATCH     | RW     | 32    | Scratch register (bootloader→firmware)
0x24    | SYS_BOOT_SRC    | RW     | 32    | Boot source for next reset
        |                 |        |       | [0]     0=golden, 1=update
        |                 |        |       | [31]    Commit (write 0xACCE55 to commit)
0x28    | SYS_INT_EN      | RW     | 32    | Interrupt enable mask
0x2C    | SYS_INT_STATUS  | RO     | 32    | Interrupt status (write 1 to clear)
0x30    | SYS_WATCHDOG    | RW     | 32    | Watchdog timer
        |                 |        |       | [23:0]  Timeout in ms (0=disabled)
        |                 |        |       | [31]    Watchdog enable
0x34    | SYS_WDT_KICK    | WO     | 32    | Watchdog kick (write 0x5AFE0001)
```

### 3.3 TDC Core Registers (TDC — 0x30000000)

```
Offset  | Name            | Access | Width | Description
--------|-----------------|--------|-------|-----------------------------
0x00    | TDC_CTRL        | RW     | 32    | TDC control
        |                 |        |       | [0]     TDC enable
        |                 |        |       | [1]     Continuous mode
        |                 |        |       | [2]     Single-shot mode (self-clearing)
        |                 |        |       | [3]     Calibration mode
        |                 |        |       | [4]     Clear FIFOs
        |                 |        |       | [8:11]  Trigger source (0=internal,1=external,2=ADC_OR)
        |                 |        |       | [16:23] Averaging depth log2 (0=1, 8=256)
0x04    | TDC_STATUS      | RO     | 32    | TDC status
        |                 |        |       | [0]     TDC busy
        |                 |        |       | [1]     FIFO empty
        |                 |        |       | [2]     FIFO full
        |                 |        |       | [3]     FIFO overflow (sticky)
        |                 |        |       | [8:15]  FIFO count
        |                 |        |       | [16]    Calibration done
0x08    | TDC_FIFO_RD     | RO     | 32    | TDC timestamp FIFO read
        |                 |        |       | [15:0]  Fine timestamp (0–65535, ~10 ps LSB)
        |                 |        |       | [31:16] Coarse timestamp (0–65535, 4 ns LSB)
        |                 |        |       | Reading auto-pops FIFO
0x0C    | TDC_CAL_ADDR    | RW     | 32    | Calibration LUT address
        |                 |        |       | [7:0]   Bin index (0–255)
        |                 |        |       | [31]    Write enable
0x10    | TDC_CAL_DATA    | RW     | 32    | Calibration LUT data
        |                 |        |       | [15:0]  Bin width in femtoseconds
0x14    | TDC_COARSE_OVF  | RO     | 32    | Coarse counter overflow count
0x18    | TDC_TRIG_COUNT  | RO     | 32    | Total trigger count
0x1C    | TDC_CFG         | RW     | 32    | TDC configuration
        |                 |        |       | [0]     Rising edge detect
        |                 |        |       | [1]     Falling edge detect
        |                 |        |       | [2]     Both edges
        |                 |        |       | [8:15]  Hit threshold (min pulse width in coarse ticks)
        |                 |        |       | [16:23] Dead time in coarse ticks
0x20    | TDC_VERSION     | RO     | 32    | TDC core version
        |                 |        |       | [7:0]   Minor version
        |                 |        |       | [15:8]  Major version
        |                 |        |       | [31:16] Core ID = 0x5444 ("TD")
```

### 3.4 ADC Capture Engine Registers (ADC_CAP — 0x30004000)

```
Offset  | Name            | Access | Width | Description
--------|-----------------|--------|-------|-----------------------------
0x00    | ADC_CTRL        | RW     | 32    | ADC capture control
        |                 |        |       | [0]     Capture enable
        |                 |        |       | [1]     Continuous capture
        |                 |        |       | [2]     Single-shot (self-clearing)
        |                 |        |       | [3]     Decimation enable
        |                 |        |       | [4]     Averaging enable
        |                 |        |       | [8:11]  Decimation factor (0=bypass, 1=/2, 2=/4, ...)
        |                 |        |       | [16:23] Averaging depth log2
        |                 |        |       | [24]    Triggered mode (sync to pulse gen)
        |                 |        |       | [25]    Free-running mode
0x04    | ADC_STATUS      | RO     | 32    | ADC status
        |                 |        |       | [0]     ADC busy
        |                 |        |       | [1]     FIFO empty
        |                 |        |       | [2]     FIFO full
        |                 |        |       | [3]     FIFO overflow (sticky)
        |                 |        |       | [4]     ADC over-range detected
        |                 |        |       | [8:23]  FIFO sample count
0x08    | ADC_FIFO_RD     | RO     | 32    | ADC sample FIFO read
        |                 |        |       | [11:0]  ADC sample (12-bit, offset binary)
        |                 |        |       | [12]    Over-range flag for this sample
        |                 |        |       | [15:13] Reserved
        |                 |        |       | [31:16] Sample index (in current burst)
0x0C    | ADC_SAMPLES     | RW     | 32    | Samples per acquisition
        |                 |        |       | [23:0]  Number of samples (max 16,777,215)
0x10    | ADC_TRIG_DELAY  | RW     | 32    | Trigger delay (post-trigger samples)
        |                 |        |       | [15:0]  Delay in sample clocks (4 ns each)
0x14    | ADC_DMA_BASE    | RW     | 32    | DMA base address in DDR3
0x18    | ADC_DMA_LEN     | RW     | 32    | DMA transfer length (bytes)
0x1C    | ADC_DMA_STATUS  | RO     | 32    | DMA status
        |                 |        |       | [0]     DMA busy
        |                 |        |       | [1]     DMA done
        |                 |        |       | [2]     DMA error
        |                 |        |       | [31:16] Bytes transferred
0x20    | ADC_SPI_TX      | RW     | 32    | ADC SPI transmit data
        |                 |        |       | [7:0]   SPI data byte
        |                 |        |       | [15:8]  Register address
        |                 |        |       | [16]    Read (0) / Write (1)
        |                 |        |       | [31]    Start transaction (self-clearing)
0x24    | ADC_SPI_RX      | RO     | 32    | ADC SPI receive data
        |                 |        |       | [7:0]   Received data byte
        |                 |        |       | [8]     Transaction done
0x28    | ADC_CAL_OFFSET  | RW     | 32    | ADC offset calibration
        |                 |        |       | [11:0]  Offset value (signed, 2's complement)
0x2C    | ADC_CAL_GAIN    | RW     | 32    | ADC gain calibration
        |                 |        |       | [15:0]  Gain correction (1.0 = 0x8000, 16.16 fixed)
```

### 3.5 Pulse Generator Registers (PULSE — 0x30008000)

```
Offset  | Name            | Access | Width | Description
--------|-----------------|--------|-------|-----------------------------
0x00    | PULSE_CTRL      | RW     | 32    | Pulse generator control
        |                 |        |       | [0]     Pulse generator enable
        |                 |        |       | [1]     Continuous mode
        |                 |        |       | [2]     Single pulse (self-clearing)
        |                 |        |       | [3]     External trigger enable
        |                 |        |       | [4]     Burst mode
        |                 |        |       | [8:15]  Burst count (1–255)
0x04    | PULSE_STATUS    | RO     | 32    | Pulse generator status
        |                 |        |       | [0]     Pulse generator busy
        |                 |        |       | [1]     Burst in progress
        |                 |        |       | [16:31] Pulses emitted (this burst)
0x08    | PULSE_PERIOD    | RW     | 32    | Pulse period
        |                 |        |       | [31:0]  Period in 4 ns ticks (250 MHz clock)
        |                 |        |       | 1 kHz = 250,000 ticks
        |                 |        |       | 1 MHz = 250 ticks
0x0C    | PULSE_AMPLITUDE | RW     | 32    | Pulse amplitude control
        |                 |        |       | [7:0]   SRD bias DAC value (0–255)
        |                 |        |       | [8:15]  VGA gain setting (0–255)
        |                 |        |       | Amplitude mapping: DAC=0→100mV, DAC=255→500mV
0x10    | PULSE_DELAY     | RW     | 32    | Pulse delay from trigger
        |                 |        |       | [15:0]  Delay in 4 ns ticks
0x14    | PULSE_WIDTH     | RW     | 32    | Pulse width control (fine trim)
        |                 |        |       | [7:0]   SRD bias trim (affects transition speed)
0x18    | PULSE_CAL       | RW     | 32    | Pulse calibration
        |                 |        |       | [15:0]  Propagation delay compensation (ps)
        |                 |        |       | [31:16] Amplitude calibration factor (16.16 fixed)
```

### 3.6 USB Bridge Registers (USB — 0x3000C000)

```
Offset  | Name            | Access | Width | Description
--------|-----------------|--------|-------|-----------------------------
0x00    | USB_CTRL        | RW     | 32    | USB bridge control
        |                 |        |       | [0]     Bridge enable
        |                 |        |       | [1]     FT601 reset (active high)
        |                 |        |       | [2]     Wake up FT601
        |                 |        |       | [8]     Data direction (0=host→dev, 1=dev→host)
0x04    | USB_STATUS      | RO     | 32    | USB bridge status
        |                 |        |       | [0]     FT601 configured
        |                 |        |       | [1]     USB suspended
        |                 |        |       | [2]     TX FIFO empty (TXE#)
        |                 |        |       | [3]     RX FIFO full (RXF#)
        |                 |        |       | [4]     USB 3.0 connection active
        |                 |        |       | [5]     USB 2.0 fallback active
0x08    | USB_TX_LEN      | RW     | 32    | TX transfer length (bytes)
0x0C    | USB_TX_BASE     | RW     | 32    | TX DMA base address in DDR3
0x10    | USB_TX_START    | WO     | 32    | Start TX DMA (write 1)
0x14    | USB_TX_STATUS   | RO     | 32    | TX DMA status
        |                 |        |       | [0]     TX busy
        |                 |        |       | [1]     TX done
        |                 |        |       | [2]     TX error
0x18    | USB_RX_LEN      | RW     | 32    | RX transfer length (bytes)
0x1C    | USB_RX_BASE     | RW     | 32    | RX DMA base address in DDR3
0x20    | USB_RX_START    | WO     | 32    | Start RX DMA (write 1)
0x24    | USB_RX_STATUS   | RO     | 32    | RX DMA status
0x28    | USB_EP0_DATA    | RW     | 32    | Endpoint 0 data register
0x2C    | USB_EP0_CTRL    | RW     | 32    | Endpoint 0 control
        |                 |        |       | [0]     Setup packet received
        |                 |        |       | [1]     IN transfer complete
        |                 |        |       | [2]     OUT transfer complete
        |                 |        |       | [8:15]  Setup packet bRequest
        |                 |        |       | [16:23] Setup packet wValue low
        |                 |        |       | [24:31] Setup packet wValue high
```

### 3.7 Acquisition Sequencer Registers (ACQ — 0x30010000)

```
Offset  | Name            | Access | Width | Description
--------|-----------------|--------|-------|-----------------------------
0x00    | ACQ_CTRL        | RW     | 32    | Acquisition sequencer control
        |                 |        |       | [0]     Start acquisition
        |                 |        |       | [1]     Abort acquisition
        |                 |        |       | [2]     Continuous acquisition mode
        |                 |        |       | [8:15]  Repetitions (0=infinite in continuous)
0x04    | ACQ_STATUS      | RO     | 32    | Acquisition status
        |                 |        |       | [0]     Acquisition running
        |                 |        |       | [1]     Acquisition complete
        |                 |        |       | [2]     Acquisition error
        |                 |        |       | [8:15]  Repetitions completed
0x08    | ACQ_CONFIG      | RW     | 32    | Acquisition configuration
        |                 |        |       | [0]     Enable TDC capture
        |                 |        |       | [1]     Enable ADC capture
        |                 |        |       | [2]     Enable both (TDC + ADC synchronized)
        |                 |        |       | [3]     Auto-upload to USB
        |                 |        |       | [8:15]  Pre-trigger samples (ADC)
        |                 |        |       | [16:23] Post-trigger samples (ADC)
0x0C    | ACQ_TDC_BASE    | RW     | 32    | TDC data DMA base address
0x10    | ACQ_ADC_BASE    | RW     | 32    | ADC data DMA base address
0x14    | ACQ_RESULT      | RO     | 32    | Acquisition result summary
        |                 |        |       | [0]     Valid reflection detected
        |                 |        |       | [1]     Open circuit detected
        |                 |        |       | [2]     Short circuit detected
        |                 |        |       | [3]     Multiple reflections
        |                 |        |       | [15:8]  Reflection polarity (0=negative, 255=positive)
        |                 |        |       | [31:16] Reflection amplitude (arbitrary units)
```

### 3.8 GPIO Controller Registers (GPIO — 0x20003000)

```
Offset  | Name            | Access | Width | Description
--------|-----------------|--------|-------|-----------------------------
0x00    | GPIO_IN         | RO     | 32    | GPIO input data
0x04    | GPIO_OUT        | RW     | 32    | GPIO output data
0x08    | GPIO_OE         | RW     | 32    | GPIO output enable (1=output)
0x0C    | GPIO_INT_EN     | RW     | 32    | Interrupt enable per pin
0x10    | GPIO_INT_STATUS | RO     | 32    | Interrupt status (write 1 to clear)
0x14    | GPIO_INT_TYPE   | RW     | 32    | Interrupt type (0=level, 1=edge)
0x18    | GPIO_INT_POL    | RW     | 32    | Interrupt polarity (0=low/falling, 1=high/rising)
0x1C    | GPIO_PULL_EN    | RW     | 32    | Pull-up/down enable
0x20    | GPIO_PULL_SEL   | RW     | 32    | Pull select (0=pull-down, 1=pull-up)
```

### 3.9 I²C Master Registers (I2C — 0x20004000)

```
Offset  | Name            | Access | Width | Description
--------|-----------------|--------|-------|-----------------------------
0x00    | I2C_CTRL        | RW     | 32    | I²C control
        |                 |        |       | [0]     I²C enable
        |                 |        |       | [1]     Interrupt enable
        |                 |        |       | [7:0]   Clock prescaler (400 kHz = 62 @ 100 MHz sys)
0x04    | I2C_STATUS      | RO     | 32    | I²C status
        |                 |        |       | [0]     Bus busy
        |                 |        |       | [1]     Transfer in progress
        |                 |        |       | [2]     Transfer complete
        |                 |        |       | [3]     NACK received
        |                 |        |       | [4]     Arbitration lost
0x08    | I2C_TX          | RW     | 32    | I²C transmit data
        |                 |        |       | [7:0]   Data byte
        |                 |        |       | [8]     Start condition
        |                 |        |       | [9]     Stop condition
        |                 |        |       | [10]    Read (1) / Write (0)
        |                 |        |       | [31]    Start transaction
0x0C    | I2C_RX          | RO     | 32    | I²C receive data
        |                 |        |       | [7:0]   Received data byte
        |                 |        |       | [8]     RX FIFO empty
        |                 |        |       | [9]     RX FIFO full
0x10    | I2C_ADDR        | RW     | 32    | I²C target address
        |                 |        |       | [6:0]   7-bit address
        |                 |        |       | [7]     10-bit address mode
```

## 4. Clock Configuration Code

### 4.1 FPGA PLL Configuration (Verilog)

The ECP5 FPGA has 4 PLLs. We use 3:

```verilog
// PLL0: 250 MHz reference → 250 MHz ADC clock + 500 MHz TDC clock
// PLL1: 250 MHz reference → 100 MHz system clock + 100 MHz FT601 clock
// PLL2: 250 MHz reference → 400 MHz DDR3 controller clock

// PLL0 configuration (EHXPLLL primitive)
// Fin = 250 MHz, Fout0 = 250 MHz (ADC), Fout1 = 500 MHz (TDC)
// DIVR=0, DIVF=15, DIVQ=4 → FVCO = 250 * 16 = 4000 MHz
// Fout0 = 4000 / 16 = 250 MHz (CLKOP)
// Fout1 = 4000 / 8 = 500 MHz (CLKOS)

// PLL1 configuration
// Fin = 250 MHz, Fout0 = 100 MHz (system), Fout1 = 100 MHz (FT601)
// DIVR=0, DIVF=19, DIVQ=4 → FVCO = 250 * 20 = 5000 MHz
// Fout0 = 5000 / 50 = 100 MHz (CLKOP)
// Fout1 = 5000 / 50 = 100 MHz (CLKOS)

// PLL2 configuration
// Fin = 250 MHz, Fout0 = 400 MHz (DDR3)
// DIVR=0, DIVF=23, DIVQ=4 → FVCO = 250 * 24 = 6000 MHz
// Fout0 = 6000 / 15 = 400 MHz (CLKOP)
```

### 4.2 Soft CPU Clock Initialization (C)

```c
// clock_config.c — Clock tree initialization for Chronos Pulser
// This runs on the VexRiscv soft CPU after FPGA configuration

#include "registers.h"
#include "board.h"

// PLL lock timeout: 100 µs at 100 MHz = 10,000 cycles
#define PLL_LOCK_TIMEOUT  10000
#define PLL_LOCK_POLL_DELAY 100

typedef enum {
    CLOCK_SOURCE_REF_250MHZ = 0,
    CLOCK_SOURCE_PLL0_250MHZ = 1,
    CLOCK_SOURCE_PLL0_500MHZ = 2,
    CLOCK_SOURCE_PLL1_100MHZ = 3,
    CLOCK_SOURCE_PLL2_400MHZ = 4
} clock_source_t;

typedef struct {
    uint32_t frequency_hz;
    clock_source_t source;
    uint8_t  pll_instance;   // 0, 1, or 2
    uint8_t  pll_output;    // 0=CLKOP, 1=CLKOS, 2=CLKOS2, 3=CLKOS3
    uint8_t  divider;       // Post-divider (1–128)
    uint8_t  enabled;
} clock_domain_t;

// System clock domains
static clock_domain_t clock_domains[CLOCK_DOMAIN_COUNT] = {
    [CLOCK_SYS]   = { .frequency_hz = 100000000, .source = CLOCK_SOURCE_PLL1_100MHZ,
                      .pll_instance = 1, .pll_output = 0, .divider = 1, .enabled = 0 },
    [CLOCK_ADC]   = { .frequency_hz = 250000000, .source = CLOCK_SOURCE_PLL0_250MHZ,
                      .pll_instance = 0, .pll_output = 0, .divider = 1, .enabled = 0 },
    [CLOCK_TDC]   = { .frequency_hz = 500000000, .source = CLOCK_SOURCE_PLL0_500MHZ,
                      .pll_instance = 0, .pll_output = 1, .divider = 1, .enabled = 0 },
    [CLOCK_USB]   = { .frequency_hz = 100000000, .source = CLOCK_SOURCE_PLL1_100MHZ,
                      .pll_instance = 1, .pll_output = 1, .divider = 1, .enabled = 0 },
    [CLOCK_DDR3]  = { .frequency_hz = 400000000, .source = CLOCK_SOURCE_PLL2_400MHZ,
                      .pll_instance = 2, .pll_output = 0, .divider = 1, .enabled = 0 },
};

// PLL register offsets within SYS_CLK_CTRL space
#define PLL0_CTRL_OFFSET    0x40
#define PLL1_CTRL_OFFSET    0x48
#define PLL2_CTRL_OFFSET    0x50
#define PLL_STATUS_OFFSET   0x60

// PLL control register bit definitions
#define PLL_CTRL_RESET      (1 << 0)
#define PLL_CTRL_PWRDN      (1 << 1)
#define PLL_CTRL_BYPASS     (1 << 2)
#define PLL_CTRL_LOCK       (1 << 31)  // Read-only

// PLL divider register (at CTRL+4)
#define PLL_DIV_M_MASK      0x3F       // Feedback divider (1–64)
#define PLL_DIV_N_MASK      0x3F       // Input divider (1–64)
#define PLL_DIV_M_SHIFT     0
#define PLL_DIV_N_SHIFT     8

// PLL output divider register (at CTRL+8)
#define PLL_CLKOP_DIV_MASK  0x7F       // CLKOP divider (1–128)
#define PLL_CLKOS_DIV_MASK  0x7F       // CLKOS divider
#define PLL_CLKOS2_DIV_MASK 0x7F       // CLKOS2 divider
#define PLL_CLKOS3_DIV_MASK 0x7F       // CLKOS3 divider

static volatile uint32_t *sys_clk_base = (volatile uint32_t *)SYS_CLK_CTRL_BASE;

// Wait for PLL lock with timeout
static int pll_wait_lock(int pll_index, uint32_t timeout_cycles) {
    volatile uint32_t *pll_ctrl = sys_clk_base + (PLL0_CTRL_OFFSET / 4)
                                  + (pll_index * 2);  // 2 words per PLL
    uint32_t waited = 0;

    while (!(*pll_ctrl & PLL_CTRL_LOCK)) {
        for (volatile int d = 0; d < PLL_LOCK_POLL_DELAY; d++) {
            asm volatile ("nop");
        }
        waited += PLL_LOCK_POLL_DELAY;
        if (waited >= timeout_cycles) {
            return -1;  // Timeout
        }
    }
    return 0;
}

// Configure a PLL with given parameters
// Fin = reference_freq, Fvco = Fin * (DIVF+1) / (DIVR+1)
// Fout = Fvco / DIVQ
static int pll_configure(int pll_index, uint8_t divr, uint8_t divf,
                         uint8_t divq_op, uint8_t divq_os,
                         uint8_t divq_os2, uint8_t divq_os3) {
    volatile uint32_t *pll_base = sys_clk_base + (PLL0_CTRL_OFFSET / 4)
                                  + (pll_index * 2);

    // 1. Power down and reset PLL
    pll_base[0] = PLL_CTRL_PWRDN | PLL_CTRL_RESET;

    // 2. Set dividers
    pll_base[1] = ((divf & PLL_DIV_M_MASK) << PLL_DIV_M_SHIFT)
                | ((divr & PLL_DIV_N_MASK) << PLL_DIV_N_SHIFT);

    // 3. Set output dividers
    pll_base[2] = (divq_op & PLL_CLKOP_DIV_MASK)
                | ((divq_os & PLL_CLKOS_DIV_MASK) << 8)
                | ((divq_os2 & PLL_CLKOS2_DIV_MASK) << 16)
                | ((divq_os3 & PLL_CLKOS3_DIV_MASK) << 24);

    // 4. Release reset, keep powered down briefly
    pll_base[0] = PLL_CTRL_PWRDN;
    for (volatile int d = 0; d < 100; d++) asm volatile ("nop");

    // 5. Power up PLL
    pll_base[0] = 0;

    // 6. Wait for lock
    return pll_wait_lock(pll_index, PLL_LOCK_TIMEOUT);
}

// Initialize all clock domains
int clock_init(void) {
    int ret;

    // Configure PLL0: 250 MHz ref → 250 MHz ADC + 500 MHz TDC
    // Fvco = 250 * (15+1) / (0+1) = 4000 MHz
    // CLKOP = 4000 / 16 = 250 MHz
    // CLKOS = 4000 / 8 = 500 MHz
    ret = pll_configure(0, 0, 15, 16, 8, 1, 1);
    if (ret != 0) {
        return -1;  // PLL0 lock failed
    }

    // Configure PLL1: 250 MHz ref → 100 MHz system + 100 MHz USB
    // Fvco = 250 * (19+1) / (0+1) = 5000 MHz
    // CLKOP = 5000 / 50 = 100 MHz
    // CLKOS = 5000 / 50 = 100 MHz
    ret = pll_configure(1, 0, 19, 50, 50, 1, 1);
    if (ret != 0) {
        return -2;  // PLL1 lock failed
    }

    // Configure PLL2: 250 MHz ref → 400 MHz DDR3
    // Fvco = 250 * (23+1) / (0+1) = 6000 MHz
    // CLKOP = 6000 / 15 = 400 MHz
    ret = pll_configure(2, 0, 23, 15, 1, 1, 1);
    if (ret != 0) {
        return -3;  // PLL2 lock failed
    }

    // Enable clock domains in SYS_CLK_CTRL
    uint32_t clk_ctrl = SYS_CLK_ENABLE_SYS
                      | SYS_CLK_ENABLE_ADC
                      | SYS_CLK_ENABLE_TDC
                      | SYS_CLK_ENABLE_USB
                      | SYS_CLK_ENABLE_DDR3;
    sys_clk_base[0] = clk_ctrl;

    // Mark all domains as enabled
    for (int i = 0; i < CLOCK_DOMAIN_COUNT; i++) {
        clock_domains[i].enabled = 1;
    }

    return 0;
}

// Get current frequency of a clock domain
uint32_t clock_get_frequency(clock_domain_id_t domain) {
    if (domain >= CLOCK_DOMAIN_COUNT) return 0;
    return clock_domains[domain].frequency_hz;
}

// Disable a clock domain (power saving)
int clock_domain_disable(clock_domain_id_t domain) {
    if (domain >= CLOCK_DOMAIN_COUNT) return -1;

    uint32_t mask;
    switch (domain) {
        case CLOCK_SYS:  mask = SYS_CLK_ENABLE_SYS;  break;
        case CLOCK_ADC:  mask = SYS_CLK_ENABLE_ADC;  break;
        case CLOCK_TDC:  mask = SYS_CLK_ENABLE_TDC;  break;
        case CLOCK_USB:  mask = SYS_CLK_ENABLE_USB;  break;
        case CLOCK_DDR3: mask = SYS_CLK_ENABLE_DDR3; break;
        default: return -1;
    }

    sys_clk_base[0] &= ~mask;
    clock_domains[domain].enabled = 0;
    return 0;
}
```

## 5. GPIO Initialization Code

```c
// gpio_init.c — GPIO initialization for Chronos Pulser
// Maps FPGA pins to their functions and sets initial states

#include "registers.h"
#include "board.h"

// GPIO pin assignments (matching board.h definitions)
typedef struct {
    uint8_t  pin;          // GPIO pin number (0–31)
    uint8_t  direction;    // 0=input, 1=output
    uint8_t  initial_val;  // Initial output value
    uint8_t  pull_enable;  // 0=no pull, 1=pull enabled
    uint8_t  pull_up;      // 0=pull-down, 1=pull-up
    uint8_t  intr_enable;  // 0=no interrupt, 1=interrupt enabled
    uint8_t  intr_type;    // 0=level, 1=edge
    uint8_t  intr_pol;     // 0=low/falling, 1=high/rising
    const char *name;      // Human-readable name
} gpio_pin_config_t;

static volatile uint32_t *gpio_base = (volatile uint32_t *)GPIO_BASE;

// Complete GPIO configuration table
static const gpio_pin_config_t gpio_config[] = {
    // Pulse generator control
    { .pin = GPIO_PULSE_TRIG,    .direction = 1, .initial_val = 0,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "PULSE_TRIG" },

    // SRD bias DAC (PWM output)
    { .pin = GPIO_SRD_BIAS_DAC,  .direction = 1, .initial_val = 0,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "SRD_BIAS_DAC" },

    // WS2812 LED data
    { .pin = GPIO_LED_DATA,      .direction = 1, .initial_val = 0,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "LED_DATA" },

    // Trigger sync output
    { .pin = GPIO_SYNC_OUT,      .direction = 1, .initial_val = 0,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "SYNC_OUT" },

    // External trigger input
    { .pin = GPIO_SYNC_IN,       .direction = 0, .initial_val = 0,
      .pull_enable = 1, .pull_up = 0, .intr_enable = 1,
      .intr_type = 1, .intr_pol = 1, .name = "SYNC_IN" },

    // ADC power-down control
    { .pin = GPIO_ADC_PDWN,      .direction = 1, .initial_val = 0,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "ADC_PDWN" },

    // VGA power-down control
    { .pin = GPIO_VGA_PDWN,      .direction = 1, .initial_val = 0,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "VGA_PDWN" },

    // FT601 reset
    { .pin = GPIO_FT_RESET_N,    .direction = 1, .initial_val = 1,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "FT_RESET_N" },

    // SPI flash chip select (manual control)
    { .pin = GPIO_SPI_FLASH_CS,  .direction = 1, .initial_val = 1,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "SPI_FLASH_CS" },

    // VGA SPI chip select
    { .pin = GPIO_VGA_SPI_CS,    .direction = 1, .initial_val = 1,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "VGA_SPI_CS" },

    // ADC SPI chip select
    { .pin = GPIO_ADC_SPI_CS,    .direction = 1, .initial_val = 1,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "ADC_SPI_CS" },

    // Status LEDs (individual channels for direct control)
    { .pin = GPIO_STATUS_LED_R,  .direction = 1, .initial_val = 0,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "STATUS_LED_R" },
    { .pin = GPIO_STATUS_LED_G,  .direction = 1, .initial_val = 0,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "STATUS_LED_G" },
    { .pin = GPIO_STATUS_LED_B,  .direction = 1, .initial_val = 0,
      .pull_enable = 0, .pull_up = 0, .intr_enable = 0,
      .intr_type = 0, .intr_pol = 0, .name = "STATUS_LED_B" },
};

#define GPIO_CONFIG_COUNT (sizeof(gpio_config) / sizeof(gpio_config[0]))

// Initialize all GPIO pins according to configuration table
int gpio_init(void) {
    uint32_t oe_mask = 0;
    uint32_t out_mask = 0;
    uint32_t pull_en_mask = 0;
    uint32_t pull_sel_mask = 0;
    uint32_t intr_en_mask = 0;
    uint32_t intr_type_mask = 0;
    uint32_t intr_pol_mask = 0;

    // Build register masks from configuration
    for (size_t i = 0; i < GPIO_CONFIG_COUNT; i++) {
        const gpio_pin_config_t *cfg = &gpio_config[i];
        uint32_t pin_mask = (1u << cfg->pin);

        if (cfg->direction == 1) {
            oe_mask |= pin_mask;
            if (cfg->initial_val) {
                out_mask |= pin_mask;
            }
        }
        if (cfg->pull_enable) {
            pull_en_mask |= pin_mask;
            if (cfg->pull_up) {
                pull_sel_mask |= pin_mask;
            }
        }
        if (cfg->intr_enable) {
            intr_en_mask |= pin_mask;
            if (cfg->intr_type) {
                intr_type_mask |= pin_mask;
            }
            if (cfg->intr_pol) {
                intr_pol_mask |= pin_mask;
            }
        }
    }

    // Apply configuration in correct order
    // 1. Set output values before enabling outputs (prevents glitches)
    gpio_base[GPIO_OUT_REG] = out_mask;

    // 2. Configure pull-ups/downs
    gpio_base[GPIO_PULL_SEL_REG] = pull_sel_mask;
    gpio_base[GPIO_PULL_EN_REG]  = pull_en_mask;

    // 3. Configure interrupts
    gpio_base[GPIO_INT_TYPE_REG] = intr_type_mask;
    gpio_base[GPIO_INT_POL_REG]  = intr_pol_mask;
    gpio_base[GPIO_INT_EN_REG]   = intr_en_mask;

    // 4. Enable outputs last
    gpio_base[GPIO_OE_REG] = oe_mask;

    return 0;
}

// Set a GPIO pin value
void gpio_set(uint8_t pin, uint8_t value) {
    uint32_t mask = (1u << pin);
    if (value) {
        gpio_base[GPIO_OUT_REG] |= mask;
    } else {
        gpio_base[GPIO_OUT_REG] &= ~mask;
    }
}

// Read a GPIO pin value
uint8_t gpio_get(uint8_t pin) {
    uint32_t mask = (1u << pin);
    return (gpio_base[GPIO_IN_REG] & mask) ? 1 : 0;
}

// Toggle a GPIO pin
void gpio_toggle(uint8_t pin) {
    uint32_t mask = (1u << pin);
    gpio_base[GPIO_OUT_REG] ^= mask;
}
```

## 6. Full Device Driver: ADC Capture Engine with DMA

### 6.1 ADC Driver Header (adc_driver.h)

```c
// adc_driver.h — AD9230-250 ADC capture engine driver
// Provides DMA-based acquisition with configurable decimation and averaging

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// ADC sample type (12-bit, stored in 16-bit for alignment)
typedef uint16_t adc_sample_t;

// ADC acquisition configuration
typedef struct {
    uint32_t sample_count;       // Number of samples to capture
    uint32_t trigger_delay;      // Delay from trigger in sample clocks
    uint8_t  decimation_factor;  // 0=bypass, 1=/2, 2=/4, ..., 8=/256
    uint8_t  averaging_depth;    // log2 of averaging count (0=1, 8=256)
    bool     continuous;         // Continuous acquisition mode
    bool     triggered;          // Wait for trigger before capturing
    bool     use_dma;            // Use DMA to DDR3
    uint32_t dma_buffer_addr;    // DDR3 physical address for DMA
    uint32_t dma_buffer_size;    // Size in bytes
} adc_acq_config_t;

// ADC acquisition result
typedef struct {
    uint32_t samples_captured;   // Actual samples captured
    uint32_t overrange_events;   // Number of over-range samples
    bool     completed;          // Acquisition completed
    bool     dma_error;          // DMA error occurred
    bool     fifo_overflow;      // FIFO overflow occurred
} adc_acq_result_t;

// ADC SPI register addresses (AD9230)
#define ADC_SPI_REG_CHIP_ID      0x00
#define ADC_SPI_REG_CHIP_GRADE   0x01
#define ADC_SPI_REG_DEV_CFG      0x02
#define ADC_SPI_REG_TEST_MODE    0x0D
#define ADC_SPI_REG_OFFSET       0x10
#define ADC_SPI_REG_GAIN         0x11
#define ADC_SPI_REG_OUTPUT_MODE  0x14
#define ADC_SPI_REG_OUTPUT_ADJ   0x15
#define ADC_SPI_REG_CLOCK_DIV    0x16
#define ADC_SPI_REG_PDWN_MODE    0x18

// ADC device ID expected value
#define ADC_EXPECTED_CHIP_ID     0x64  // AD9230-250

// Function prototypes
int  adc_driver_init(void);
int  adc_spi_read(uint8_t reg_addr, uint8_t *data);
int  adc_spi_write(uint8_t reg_addr, uint8_t data);
int  adc_configure_defaults(void);
int  adc_verify_device(void);
int  adc_start_acquisition(const adc_acq_config_t *config);
int  adc_abort_acquisition(void);
int  adc_wait_completion(uint32_t timeout_ms);
int  adc_get_result(adc_acq_result_t *result);
int  adc_read_samples(adc_sample_t *buffer, uint32_t offset, uint32_t count);
void adc_power_down(void);
void adc_power_up(void);
int  adc_calibrate_offset(uint16_t *offset_out);
int  adc_calibrate_gain(uint16_t *gain_out);

#endif // ADC_DRIVER_H
```

### 6.2 ADC Driver Implementation (adc_driver.c)

```c
// adc_driver.c — AD9230-250 ADC capture engine driver implementation
// Full DMA-based acquisition with SPI configuration and calibration

#include "adc_driver.h"
#include "registers.h"
#include "board.h"
#include <string.h>

// ADC capture engine register base
static volatile uint32_t *adc_base = (volatile uint32_t *)ADC_CAP_BASE;

// Internal state
static adc_acq_config_t current_config;
static bool driver_initialized = false;
static bool adc_powered = false;

// SPI transaction helper for ADC
// The ADC uses a 3-wire SPI (CSB, SCLK, SDIO bidirectional)
static int adc_spi_transaction(uint8_t reg_addr, uint8_t *data, bool is_write) {
    uint32_t tx_val;
    int timeout = 10000;  // 10,000 cycles timeout

    // Build transaction word
    tx_val = (is_write ? (1 << 16) : 0)
           | ((reg_addr & 0xFF) << 8)
           | (*data & 0xFF)
           | (1 << 31);  // Start bit

    // Wait for any previous transaction to complete
    while (adc_base[ADC_SPI_RX_REG] & (1 << 8)) {
        // Transaction done bit is set — previous transaction complete
        break;
    }

    // Start transaction
    adc_base[ADC_SPI_TX_REG] = tx_val;

    // Wait for completion
    while (timeout > 0) {
        uint32_t rx = adc_base[ADC_SPI_RX_REG];
        if (rx & (1 << 8)) {  // Transaction done
            if (!is_write) {
                *data = rx & 0xFF;
            }
            return 0;
        }
        timeout--;
    }

    return -1;  // Timeout
}

int adc_spi_read(uint8_t reg_addr, uint8_t *data) {
    uint8_t dummy = 0;
    return adc_spi_transaction(reg_addr, &dummy, false);
}

int adc_spi_write(uint8_t reg_addr, uint8_t data) {
    return adc_spi_transaction(reg_addr, &data, true);
}

// Initialize ADC driver
int adc_driver_init(void) {
    if (driver_initialized) return 0;

    // Ensure ADC is powered up
    adc_power_up();

    // Reset capture engine
    uint32_t sys_reset = *(volatile uint32_t *)SYS_RESET_REG;
    sys_reset |= SYS_RESET_ADC_CAP;
    *(volatile uint32_t *)SYS_RESET_REG = sys_reset;
    for (volatile int d = 0; d < 100; d++) asm volatile ("nop");
    sys_reset &= ~SYS_RESET_ADC_CAP;
    *(volatile uint32_t *)SYS_RESET_REG = sys_reset;

    // Clear any stale state
    adc_base[ADC_CTRL_REG] = 0;
    adc_base[ADC_SAMPLES_REG] = 0;
    adc_base[ADC_TRIG_DELAY_REG] = 0;
    adc_base[ADC_DMA_BASE_REG] = 0;
    adc_base[ADC_DMA_LEN_REG] = 0;

    driver_initialized = true;
    return 0;
}

// Configure ADC with default settings via SPI
int adc_configure_defaults(void) {
    uint8_t data;
    int ret;

    // 1. Verify chip ID
    ret = adc_spi_read(ADC_SPI_REG_CHIP_ID, &data);
    if (ret != 0) return -1;
    if (data != ADC_EXPECTED_CHIP_ID) return -2;

    // 2. Set output mode: LVDS, DDR, 12-bit, offset binary
    // Register 0x14: [7:6]=00(LVDS), [5]=1(DDR), [4:3]=00(12-bit), [2]=0(offset binary)
    data = (1 << 5);  // DDR mode, LVDS output, 12-bit, offset binary
    ret = adc_spi_write(ADC_SPI_REG_OUTPUT_MODE, data);
    if (ret != 0) return -3;

    // 3. Set output adjust: normal drive strength
    // Register 0x15: [1:0]=00 (3.5 mA LVDS drive)
    data = 0x00;
    ret = adc_spi_write(ADC_SPI_REG_OUTPUT_ADJ, data);
    if (ret != 0) return -4;

    // 4. Disable test mode
    // Register 0x0D: [3:0]=0000 (normal operation)
    data = 0x00;
    ret = adc_spi_write(ADC_SPI_REG_TEST_MODE, data);
    if (ret != 0) return -5;

    // 5. Set clock divider to 1 (no division)
    // Register 0x16: [2:0]=000 (divide by 1)
    data = 0x00;
    ret = adc_spi_write(ADC_SPI_REG_CLOCK_DIV, data);
    if (ret != 0) return -6;

    // 6. Ensure not in power-down modes
    // Register 0x18: [1:0]=00 (normal operation)
    data = 0x00;
    ret = adc_spi_write(ADC_SPI_REG_PDWN_MODE, data);
    if (ret != 0) return -7;

    return 0;
}

// Verify ADC device is present and responding
int adc_verify_device(void) {
    uint8_t chip_id, chip_grade;
    int ret;

    ret = adc_spi_read(ADC_SPI_REG_CHIP_ID, &chip_id);
    if (ret != 0) return -1;

    ret = adc_spi_read(ADC_SPI_REG_CHIP_GRADE, &chip_grade);
    if (ret != 0) return -2;

    if (chip_id != ADC_EXPECTED_CHIP_ID) return -3;

    // Chip grade: 0x00 = 250 MSPS grade
    if ((chip_grade & 0x0F) != 0x00) return -4;

    return 0;
}

// Start an ADC acquisition with DMA
int adc_start_acquisition(const adc_acq_config_t *config) {
    if (!driver_initialized) return -1;
    if (!adc_powered) return -2;

    // Check if acquisition already in progress
    if (adc_base[ADC_STATUS_REG] & ADC_STATUS_BUSY) {
        return -3;  // Busy
    }

    // Save configuration
    memcpy(&current_config, config, sizeof(adc_acq_config_t));

    // Configure capture engine
    adc_base[ADC_SAMPLES_REG] = config->sample_count & 0xFFFFFF;
    adc_base[ADC_TRIG_DELAY_REG] = config->trigger_delay & 0xFFFF;

    // Set decimation and averaging
    uint32_t ctrl = ADC_CTRL_ENABLE;
    if (config->decimation_factor > 0) {
        ctrl |= ADC_CTRL_DECIMATION_EN;
        ctrl |= ((config->decimation_factor & 0xF) << 8);
    }
    if (config->averaging_depth > 0) {
        ctrl |= ADC_CTRL_AVERAGING_EN;
        ctrl |= ((config->averaging_depth & 0xFF) << 16);
    }
    if (config->triggered) {
        ctrl |= ADC_CTRL_TRIGGERED;
    }
    if (config->continuous) {
        ctrl |= ADC_CTRL_CONTINUOUS;
    }

    // Configure DMA if requested
    if (config->use_dma) {
        adc_base[ADC_DMA_BASE_REG] = config->dma_buffer_addr;
        adc_base[ADC_DMA_LEN_REG] = config->dma_buffer_size;
    }

    // Start acquisition
    if (config->continuous) {
        ctrl |= ADC_CTRL_CONTINUOUS;
    } else {
        ctrl |= ADC_CTRL_SINGLE_SHOT;
    }
    adc_base[ADC_CTRL_REG] = ctrl;

    return 0;
}

// Abort an ongoing acquisition
int adc_abort_acquisition(void) {
    if (!driver_initialized) return -1;

    // Clear enable bit to stop capture
    adc_base[ADC_CTRL_REG] &= ~ADC_CTRL_ENABLE;

    // Clear FIFOs
    adc_base[ADC_CTRL_REG] |= ADC_CTRL_CLEAR_FIFO;
    for (volatile int d = 0; d < 10; d++) asm volatile ("nop");
    adc_base[ADC_CTRL_REG] &= ~ADC_CTRL_CLEAR_FIFO;

    return 0;
}

// Wait for acquisition to complete with timeout
int adc_wait_completion(uint32_t timeout_ms) {
    if (!driver_initialized) return -1;

    // Convert ms to approximate loop iterations (at 100 MHz, ~100,000 per ms)
    uint32_t max_loops = timeout_ms * 100000;
    uint32_t loops = 0;

    while (loops < max_loops) {
        uint32_t status = adc_base[ADC_STATUS_REG];

        // Check if not busy (acquisition complete)
        if (!(status & ADC_STATUS_BUSY)) {
            // Check for errors
            if (status & ADC_STATUS_FIFO_OVERFLOW) {
                return -2;  // FIFO overflow
            }
            return 0;  // Success
        }

        loops++;
    }

    return -3;  // Timeout
}

// Get acquisition result
int adc_get_result(adc_acq_result_t *result) {
    if (!driver_initialized) return -1;

    uint32_t status = adc_base[ADC_STATUS_REG];
    uint32_t dma_status = adc_base[ADC_DMA_STATUS_REG];

    result->samples_captured = (status >> 8) & 0xFFFF;
    result->overrange_events = 0;  // Accumulated elsewhere
    result->completed = !(status & ADC_STATUS_BUSY);
    result->dma_error = (dma_status & ADC_DMA_ERROR) ? true : false;
    result->fifo_overflow = (status & ADC_STATUS_FIFO_OVERFLOW) ? true : false;

    return 0;
}

// Read samples from DDR3 buffer after DMA completion
int adc_read_samples(adc_sample_t *buffer, uint32_t offset, uint32_t count) {
    if (!driver_initialized) return -1;

    // DMA must be complete
    uint32_t dma_status = adc_base[ADC_DMA_STATUS_REG];
    if (dma_status & ADC_DMA_BUSY) {
        return -2;  // DMA still running
    }
    if (dma_status & ADC_DMA_ERROR) {
        return -3;  // DMA error
    }

    // Calculate source address in DDR3
    uint32_t src_addr = current_config.dma_buffer_addr + (offset * sizeof(adc_sample_t));
    uint32_t byte_count = count * sizeof(adc_sample_t);

    // Use CPU to copy from DDR3 to local buffer
    // (In production, this would use a DMA engine; here we do memcpy)
    volatile uint32_t *src = (volatile uint32_t *)src_addr;
    uint32_t word_count = (byte_count + 3) / 4;
    uint32_t *dst = (uint32_t *)buffer;

    for (uint32_t i = 0; i < word_count; i++) {
        dst[i] = src[i];
    }

    return 0;
}

// Power down ADC
void adc_power_down(void) {
    // Assert PDWN pin via GPIO
    gpio_set(GPIO_ADC_PDWN, 1);

    // Also set power-down via SPI for full shutdown
    uint8_t data = 0x03;  // Full power-down
    adc_spi_write(ADC_SPI_REG_PDWN_MODE, data);

    adc_powered = false;
}

// Power up ADC
void adc_power_up(void) {
    if (adc_powered) return;

    // De-assert PDWN pin
    gpio_set(GPIO_ADC_PDWN, 0);

    // Set normal operation via SPI
    uint8_t data = 0x00;
    adc_spi_write(ADC_SPI_REG_PDWN_MODE, data);

    // Wait for ADC to wake up (~500 µs)
    for (volatile int d = 0; d < 50000; d++) asm volatile ("nop");

    adc_powered = true;
}

// Calibrate ADC offset
// Short input to ground, measure average value
int adc_calibrate_offset(uint16_t *offset_out) {
    adc_acq_config_t cal_cfg = {
        .sample_count = 10000,
        .trigger_delay = 0,
        .decimation_factor = 0,
        .averaging_depth = 0,
        .continuous = false,
        .triggered = false,
        .use_dma = true,
        .dma_buffer_addr = ADC_CAL_DMA_BUFFER,
        .dma_buffer_size = 20000  // 10,000 samples × 2 bytes
    };

    int ret = adc_start_acquisition(&cal_cfg);
    if (ret != 0) return ret;

    ret = adc_wait_completion(100);  // 100 ms timeout
    if (ret != 0) return ret;

    // Read samples and compute average
    adc_sample_t samples[10000];
    ret = adc_read_samples(samples, 0, 10000);
    if (ret != 0) return ret;

    uint64_t sum = 0;
    for (int i = 0; i < 10000; i++) {
        sum += (samples[i] & 0xFFF);  // Mask to 12 bits
    }

    *offset_out = (uint16_t)(sum / 10000);

    // Write offset to ADC SPI register
    // AD9230 offset register: 8-bit signed, LSB = ~0.5 LSB of ADC
    uint8_t offset_byte = (uint8_t)((*offset_out - 2048) / 2);  // Center at 2048
    adc_spi_write(ADC_SPI_REG_OFFSET, offset_byte);

    return 0;
}

// Calibrate ADC gain (requires known reference signal)
int adc_calibrate_gain(uint16_t *gain_out) {
    // Gain calibration requires a known amplitude reference
    // For now, set unity gain (0x8000 in 16.16 fixed point = 1.0)
    *gain_out = 0x8000;

    // Write to capture engine calibration register
    adc_base[ADC_CAL_GAIN_REG] = *gain_out;

    return 0;
}
```

## 7. Full Device Driver: TDC Core with DMA

### 7.1 TDC Driver Header (tdc_driver.h)

```c
// tdc_driver.h — TDC (Time-to-Digital Converter) core driver
// Sub-nanosecond precision timing measurement engine

#ifndef TDC_DRIVER_H
#define TDC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// TDC timestamp: 48-bit value
// [47:32] Coarse counter (4 ns LSB) — 16 bits
// [31:16] Reserved
// [15:0]  Fine interpolator (~10 ps LSB after calibration) — 16 bits
typedef struct {
    uint16_t coarse;   // 4 ns units
    uint16_t fine;     // ~10 ps units (post-calibration)
} tdc_timestamp_t;

// Convert timestamp to picoseconds
// Requires calibration LUT to be loaded
uint64_t tdc_timestamp_to_ps(const tdc_timestamp_t *ts);

// TDC configuration
typedef struct {
    bool     continuous_mode;     // Continuous timestamping
    bool     single_shot;         // Single measurement
    uint8_t  trigger_source;      // 0=internal, 1=external, 2=ADC_OR
    uint8_t  averaging_depth;     // log2 of averaging count
    bool     rising_edge;         // Detect rising edges
    bool     falling_edge;        // Detect falling edges
    uint8_t  hit_threshold;       // Min pulse width in coarse ticks
    uint8_t  dead_time;           // Dead time after hit in coarse ticks
    bool     calibration_mode;    // Enable calibration mode
} tdc_config_t;

// TDC calibration bin data
typedef struct {
    uint16_t bin_width_fs[256];   // Bin widths in femtoseconds
    uint32_t cal_timestamp;       // When calibration was performed
    float    temperature_c;       // Temperature during calibration
    uint32_t crc;                 // CRC-32 of calibration data
} tdc_calibration_t;

// TDC acquisition result
typedef struct {
    uint32_t timestamps_captured;
    uint32_t triggers_received;
    bool     fifo_overflow;
    bool     completed;
} tdc_result_t;

// Function prototypes
int  tdc_driver_init(void);
int  tdc_load_calibration(const tdc_calibration_t *cal);
int  tdc_save_calibration(tdc_calibration_t *cal);
int  tdc_run_calibration(tdc_calibration_t *cal_out);
int  tdc_configure(const tdc_config_t *config);
int  tdc_start(void);
int  tdc_stop(void);
int  tdc_single_shot(tdc_timestamp_t *ts_out);
int  tdc_read_timestamps(tdc_timestamp_t *buffer, uint32_t count,
                         uint32_t *actual_count);
int  tdc_read_timestamps_dma(tdc_timestamp_t *buffer, uint32_t max_count,
                             uint32_t dma_buffer_addr, uint32_t *actual_count);
int  tdc_get_result(tdc_result_t *result);
void tdc_clear_fifo(void);
int  tdc_self_test(void);

#endif // TDC_DRIVER_H
```

### 7.2 TDC Driver Implementation (tdc_driver.c)

```c
// tdc_driver.c — TDC core driver implementation
// Full calibration, DMA timestamp capture, and precision timing

#include "tdc_driver.h"
#include "registers.h"
#include "board.h"
#include <string.h>

static volatile uint32_t *tdc_base = (volatile uint32_t *)TDC_BASE;
static tdc_calibration_t current_calibration;
static bool driver_initialized = false;
static bool calibration_loaded = false;

// Convert raw timestamp to picoseconds using calibration LUT
uint64_t tdc_timestamp_to_ps(const tdc_timestamp_t *ts) {
    if (!calibration_loaded) {
        // Fallback: use nominal values
        // Coarse: 4 ns = 4000 ps per tick
        // Fine: ~15 ps per bin (uncalibrated)
        return ((uint64_t)ts->coarse * 4000) + ((uint64_t)ts->fine * 15);
    }

    // Use calibration LUT for fine interpolation
    uint64_t coarse_ps = (uint64_t)ts->coarse * 4000;

    // Accumulate bin widths up to the measured fine value
    uint64_t fine_ps = 0;
    uint16_t remaining = ts->fine;
    for (int bin = 0; bin < 256 && remaining > 0; bin++) {
        uint16_t bin_width = current_calibration.bin_width_fs[bin];
        if (remaining >= bin_width) {
            fine_ps += bin_width;
            remaining -= bin_width;
        } else {
            fine_ps += remaining;
            remaining = 0;
        }
    }

    return coarse_ps + fine_ps;
}

// Initialize TDC driver
int tdc_driver_init(void) {
    if (driver_initialized) return 0;

    // Reset TDC core
    uint32_t sys_reset = *(volatile uint32_t *)SYS_RESET_REG;
    sys_reset |= SYS_RESET_TDC;
    *(volatile uint32_t *)SYS_RESET_REG = sys_reset;
    for (volatile int d = 0; d < 100; d++) asm volatile ("nop");
    sys_reset &= ~SYS_RESET_TDC;
    *(volatile uint32_t *)SYS_RESET_REG = sys_reset;

    // Clear any stale state
    tdc_base[TDC_CTRL_REG] = 0;
    tdc_clear_fifo();

    // Verify TDC core version
    uint32_t version = tdc_base[TDC_VERSION_REG];
    uint16_t core_id = (version >> 16) & 0xFFFF;
    if (core_id != 0x5444) {  // "TD"
        return -1;  // Wrong core ID
    }

    driver_initialized = true;
    return 0;
}

// Load calibration data into TDC LUT
int tdc_load_calibration(const tdc_calibration_t *cal) {
    if (!driver_initialized) return -1;

    // Write each bin's width to the calibration LUT
    for (int bin = 0; bin < 256; bin++) {
        // Set address and write enable
        tdc_base[TDC_CAL_ADDR_REG] = (bin & 0xFF) | (1 << 31);
        // Write bin width in femtoseconds
        tdc_base[TDC_CAL_DATA_REG] = cal->bin_width_fs[bin] & 0xFFFF;
    }

    // Store calibration locally
    memcpy(&current_calibration, cal, sizeof(tdc_calibration_t));
    calibration_loaded = true;

    return 0;
}

// Save current calibration data
int tdc_save_calibration(tdc_calibration_t *cal) {
    if (!calibration_loaded) return -1;
    memcpy(cal, &current_calibration, sizeof(tdc_calibration_t));
    return 0;
}

// Run TDC calibration using code density test
// Requires uncorrelated random pulse source
int tdc_run_calibration(tdc_calibration_t *cal_out) {
    if (!driver_initialized) return -1;

    tdc_config_t cal_config = {
        .continuous_mode = false,
        .single_shot = false,
        .trigger_source = 0,       // Internal trigger
        .averaging_depth = 0,      // No averaging
        .rising_edge = true,
        .falling_edge = false,
        .hit_threshold = 1,
        .dead_time = 0,
        .calibration_mode = true   // Enable calibration mode
    };

    int ret = tdc_configure(&cal_config);
    if (ret != 0) return ret;

    // Start calibration acquisition
    tdc_base[TDC_CTRL_REG] |= TDC_CTRL_ENABLE | TDC_CTRL_CAL_MODE;

    // Wait for calibration to complete (collects ~1M hits for statistics)
    // Calibration done flag is set automatically
    uint32_t timeout = 500000000;  // ~5 seconds at 100 MHz
    while (timeout > 0) {
        if (tdc_base[TDC_STATUS_REG] & TDC_STATUS_CAL_DONE) {
            break;
        }
        timeout--;
    }

    if (timeout == 0) {
        tdc_base[TDC_CTRL_REG] &= ~TDC_CTRL_ENABLE;
        return -2;  // Timeout
    }

    // Read calibration data from LUT
    for (int bin = 0; bin < 256; bin++) {
        tdc_base[TDC_CAL_ADDR_REG] = bin & 0xFF;  // Read mode (no write enable)
        cal_out->bin_width_fs[bin] = tdc_base[TDC_CAL_DATA_REG] & 0xFFFF;
    }

    // Fill metadata
    cal_out->cal_timestamp = *(volatile uint32_t *)SYS_UPTIME_LO_REG;
    cal_out->temperature_c = 25.0f;  // Read from TMP117 in production
    cal_out->crc = 0;  // Compute CRC-32 in production

    // Disable calibration mode
    tdc_base[TDC_CTRL_REG] &= ~(TDC_CTRL_ENABLE | TDC_CTRL_CAL_MODE);

    // Load this calibration as active
    tdc_load_calibration(cal_out);

    return 0;
}

// Configure TDC for measurement
int tdc_configure(const tdc_config_t *config) {
    if (!driver_initialized) return -1;

    // Stop any ongoing acquisition
    tdc_stop();

    // Build configuration word
    uint32_t cfg = 0;
    if (config->rising_edge)  cfg |= TDC_CFG_RISING;
    if (config->falling_edge) cfg |= TDC_CFG_FALLING;
    if (config->rising_edge && config->falling_edge) cfg |= TDC_CFG_BOTH;

    cfg |= ((config->hit_threshold & 0xFF) << 8);
    cfg |= ((config->dead_time & 0xFF) << 16);

    tdc_base[TDC_CFG_REG] = cfg;

    // Build control word
    uint32_t ctrl = 0;
    if (config->continuous_mode) ctrl |= TDC_CTRL_CONTINUOUS;
    if (config->single_shot)     ctrl |= TDC_CTRL_SINGLE_SHOT;
    if (config->calibration_mode) ctrl |= TDC_CTRL_CAL_MODE;

    ctrl |= ((config->trigger_source & 0xF) << 8);
    ctrl |= ((config->averaging_depth & 0xFF) << 16);

    tdc_base[TDC_CTRL_REG] = ctrl;

    return 0;
}

// Start TDC acquisition
int tdc_start(void) {
    if (!driver_initialized) return -1;
    tdc_base[TDC_CTRL_REG] |= TDC_CTRL_ENABLE;
    return 0;
}

// Stop TDC acquisition
int tdc_stop(void) {
    if (!driver_initialized) return -1;
    tdc_base[TDC_CTRL_REG] &= ~TDC_CTRL_ENABLE;
    return 0;
}

// Perform a single-shot measurement
int tdc_single_shot(tdc_timestamp_t *ts_out) {
    if (!driver_initialized) return -1;

    // Configure for single shot
    tdc_config_t ss_config = {
        .single_shot = true,
        .trigger_source = 0,
        .rising_edge = true,
        .falling_edge = false,
        .hit_threshold = 1,
        .dead_time = 0,
        .averaging_depth = 0,
        .continuous_mode = false,
        .calibration_mode = false
    };
    tdc_configure(&ss_config);

    // Clear FIFO
    tdc_clear_fifo();

    // Start
    tdc_start();

    // Wait for FIFO to have data
    uint32_t timeout = 1000000;  // ~10 ms
    while (timeout > 0) {
        uint32_t status = tdc_base[TDC_STATUS_REG];
        uint32_t fifo_count = (status >> 8) & 0xFF;
        if (fifo_count > 0) {
            break;
        }
        timeout--;
    }

    if (timeout == 0) {
        tdc_stop();
        return -2;  // Timeout — no hit detected
    }

    // Read timestamp
    uint32_t raw = tdc_base[TDC_FIFO_RD_REG];
    ts_out->coarse = (raw >> 16) & 0xFFFF;
    ts_out->fine = raw & 0xFFFF;

    tdc_stop();
    return 0;
}

// Read timestamps from FIFO (polling mode)
int tdc_read_timestamps(tdc_timestamp_t *buffer, uint32_t count,
                         uint32_t *actual_count) {
    if (!driver_initialized) return -1;

    uint32_t read = 0;
    uint32_t timeout = count * 100;  // Per-entry timeout

    while (read < count && timeout > 0) {
        uint32_t status = tdc_base[TDC_STATUS_REG];
        uint32_t fifo_count = (status >> 8) & 0xFF;

        if (fifo_count > 0) {
            uint32_t raw = tdc_base[TDC_FIFO_RD_REG];
            buffer[read].coarse = (raw >> 16) & 0xFFFF;
            buffer[read].fine = raw & 0xFFFF;
            read++;
        }
        timeout--;
    }

    *actual_count = read;

    if (read < count && timeout == 0) {
        return -2;  // Partial read due to timeout
    }

    return 0;
}

// Read timestamps using DMA to DDR3 buffer
int tdc_read_timestamps_dma(tdc_timestamp_t *buffer, uint32_t max_count,
                             uint32_t dma_buffer_addr, uint32_t *actual_count) {
    if (!driver_initialized) return -1;

    // Configure DMA
    // (TDC DMA is handled by the acquisition sequencer at 0x30010000)
    volatile uint32_t *acq_base = (volatile uint32_t *)ACQ_BASE;

    acq_base[ACQ_TDC_BASE_REG] = dma_buffer_addr;

    // Wait for acquisition to complete
    uint32_t timeout = 5000000;  // ~50 ms
    while (timeout > 0) {
        uint32_t acq_status = acq_base[ACQ_STATUS_REG];
        if (acq_status & ACQ_STATUS_COMPLETE) {
            break;
        }
        timeout--;
    }

    if (timeout == 0) {
        return -2;  // Timeout
    }

    // Copy from DDR3 to local buffer
    uint32_t tdc_count = tdc_base[TDC_STATUS_REG];
    uint32_t fifo_count = (tdc_count >> 8) & 0xFF;
    uint32_t copy_count = (fifo_count < max_count) ? fifo_count : max_count;

    volatile uint32_t *src = (volatile uint32_t *)dma_buffer_addr;
    for (uint32_t i = 0; i < copy_count; i++) {
        uint32_t raw = src[i];
        buffer[i].coarse = (raw >> 16) & 0xFFFF;
        buffer[i].fine = raw & 0xFFFF;
    }

    *actual_count = copy_count;
    return 0;
}

// Get TDC acquisition result
int tdc_get_result(tdc_result_t *result) {
    if (!driver_initialized) return -1;

    uint32_t status = tdc_base[TDC_STATUS_REG];

    result->timestamps_captured = (status >> 8) & 0xFF;
    result->triggers_received = tdc_base[TDC_TRIG_COUNT_REG];
    result->fifo_overflow = (status & TDC_STATUS_FIFO_OVERFLOW) ? true : false;
    result->completed = !(status & TDC_STATUS_BUSY);

    return 0;
}

// Clear TDC FIFO
void tdc_clear_fifo(void) {
    tdc_base[TDC_CTRL_REG] |= TDC_CTRL_CLEAR_FIFO;
    for (volatile int d = 0; d < 10; d++) asm volatile ("nop");
    tdc_base[TDC_CTRL_REG] &= ~TDC_CTRL_CLEAR_FIFO;
}

// TDC self-test using internal loopback
int tdc_self_test(void) {
    if (!driver_initialized) return -1;

    // Test 1: Verify we can read the version register
    uint32_t version = tdc_base[TDC_VERSION_REG];
    if ((version >> 16) != 0x5444) {
        return -10;  // Version check failed
    }

    // Test 2: Clear FIFO and verify it's empty
    tdc_clear_fifo();
    uint32_t status = tdc_base[TDC_STATUS_REG];
    if (status & TDC_STATUS_FIFO_EMPTY) {
        // FIFO is empty — good
    } else {
        return -11;  // FIFO not empty after clear
    }

    // Test 3: Single-shot with internal trigger
    tdc_timestamp_t ts;
    int ret = tdc_single_shot(&ts);
    if (ret != 0) {
        return -12;  // Single-shot failed
    }

    // Test 4: Verify timestamp is reasonable
    // Coarse should be non-zero, fine should be in range
    if (ts.coarse == 0 && ts.fine == 0) {
        return -13;  // Zero timestamp
    }
    if (ts.fine > 65535) {
        return -14;  // Fine value out of range
    }

    // Test 5: Multiple shots for consistency
    tdc_timestamp_t ts2;
    ret = tdc_single_shot(&ts2);
    if (ret != 0) {
        return -15;
    }

    // Timestamps should differ (not stuck at same value)
    if (ts.coarse == ts2.coarse && ts.fine == ts2.fine) {
        return -16;  // Timestamps identical — possible stuck value
    }

    return 0;  // All tests passed
}
```

## 8. Device Tree Overlay (Conceptual)

For a Linux host interacting with Chronos Pulser via USB, a device tree overlay describes the USB device:

```dts
// chronos-pulser.dtso — Device Tree Overlay for Chronos Pulser USB device
/dts-v1/;
/plugin/;

/ {
    compatible = "nous-research,chronos-pulser";

    fragment@0 {
        target = <&usb1>;
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;

            chronos_pulser: device@1 {
                compatible = "nous-research,chronos-pulser";
                reg = <1>;

                // USB 3.0 bulk endpoints
                endpoint@0 {
                    reg = <0>;
                    /* EP1 IN — Bulk data from device */
                    ep_address = <0x81>;
                    ep_type = "bulk";
                    max_packet_size = <1024>;
                };

                endpoint@1 {
                    reg = <1>;
                    /* EP1 OUT — Bulk commands to device */
                    ep_address = <0x01>;
                    ep_type = "bulk";
                    max_packet_size = <1024>;
                };

                // Device capabilities
                capabilities {
                    max_sample_rate = <250000000>;
                    adc_resolution = <12>;
                    tdc_resolution_ps = <10>;
                    max_cable_length_m = <100>;
                    impedance_range_ohm = <10 200>;
                };
            };
        };
    };
};
```

## 9. Build Instructions

### 9.1 FPGA Gateware Build (Yosys + nextpnr-ecp5)

```makefile
# Makefile.ecp5 — FPGA gateware build for Chronos Pulser

# Toolchain
YOSYS      ?= yosys
NEXTPNR    ?= nextpnr-ecp5
ECPPACK    ?= ecppack
ECPPROG    ?= ecpprog

# Target
DEVICE     := LFE5U-45F
PACKAGE    := CABGA381
SPEED      := 6

# Source files
VERILOG_SRCS := \
    rtl/chronos_pulser_top.v \
    rtl/tdc_core.v \
    rtl/tdc_carry_chain.v \
    rtl/tdc_calibration.v \
    rtl/adc_capture.v \
    rtl/adc_lvds_deserializer.v \
    rtl/adc_decimator.v \
    rtl/usb_bridge.v \
    rtl/ft601_fifo.v \
    rtl/pulse_gen.v \
    rtl/pulse_sequencer.v \
    rtl/wishbone_crossbar.v \
    rtl/sys_controller.v \
    rtl/gpio_controller.v \
    rtl/i2c_master.v \
    rtl/spi_master.v \
    rtl/vexriscv_core.v \
    rtl/litedram_core.v

# Constraints
CONSTRAINTS := constraints/chronos_pulser.lpf

# Output files
JSON       := build/chronos_pulser.json
ASC        := build/chronos_pulser.asc
BITSTREAM  := build/chronos_pulser.bit

.PHONY: all synth place route pack prog clean

all: $(BITSTREAM)

synth: $(JSON)

$(JSON): $(VERILOG_SRCS)
    mkdir -p build
    $(YOSYS) -p "synth_ecp5 -top chronos_pulser_top -json $@" $(VERILOG_SRCS)

place: $(ASC)

$(ASC): $(JSON) $(CONSTRAINTS)
    $(NEXTPNR) --$(DEVICE) --package $(PACKAGE) --speed $(SPEED) \
        --json $(JSON) --lpf $(CONSTRAINTS) --textcfg $@

route: $(ASC)
    # nextpnr does placement and routing together

pack: $(BITSTREAM)

$(BITSTREAM): $(ASC)
    $(ECPPACK) --input $(ASC) --bit $@

prog: $(BITSTREAM)
    $(ECPPROG) -s $(BITSTREAM)

clean:
    rm -rf build/
```

### 9.2 Soft CPU Firmware Build (RISC-V GCC)

```makefile
# Makefile.firmware — Soft CPU firmware build for Chronos Pulser

# Toolchain
CROSS_COMPILE ?= riscv64-unknown-elf-
CC            := $(CROSS_COMPILE)gcc
AS            := $(CROSS_COMPILE)as
LD            := $(CROSS_COMPILE)ld
OBJCOPY       := $(CROSS_COMPILE)objcopy
OBJDUMP       := $(CROSS_COMPILE)objdump
SIZE          := $(CROSS_COMPILE)size

# Architecture flags
ARCH_FLAGS    := -march=rv32im -mabi=ilp32 -mcmodel=medany
OPT_FLAGS     := -Os -ffunction-sections -fdata-sections -fno-builtin
WARN_FLAGS    := -Wall -Wextra -Werror -Wno-unused-parameter

CFLAGS        := $(ARCH_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) \
                -I. -Idrivers -Iinclude \
                -DF_CPU=100000000UL \
                -DCHRONOS_PULSER_REV=1

LDFLAGS       := $(ARCH_FLAGS) -Wl,--gc-sections -Wl,-Map=build/firmware.map \
                -T linker/firmware.ld

# Source files
C_SRCS := \
    main.c \
    drivers/clock_config.c \
    drivers/gpio_init.c \
    drivers/adc_driver.c \
    drivers/tdc_driver.c \
    drivers/pulse_driver.c \
    drivers/vga_driver.c \
    drivers/i2c_driver.c \
    drivers/spi_flash.c \
    drivers/usb_protocol.c \
    drivers/temp_monitor.c \
    drivers/error_handler.c \
    drivers/calibration.c \
    drivers/led_controller.c

ASM_SRCS := \
    startup.S \
    vectors.S

# Object files
C_OBJS   := $(C_SRCS:.c=.o)
ASM_OBJS := $(ASM_SRCS:.S=.o)
OBJS     := $(addprefix build/,$(C_OBJS) $(ASM_OBJS))

# Output
ELF       := build/firmware.elf
BIN       := build/firmware.bin
HEX       := build/firmware.hex
LST       := build/firmware.lst

.PHONY: all clean flash size

all: $(BIN) $(HEX) $(LST)

$(ELF): $(OBJS)
    mkdir -p build/drivers
    $(CC) $(LDFLAGS) -o $@ $(OBJS) -lm

$(BIN): $(ELF)
    $(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
    $(OBJCOPY) -O ihex $< $@

$(LST): $(ELF)
    $(OBJDUMP) -d $< > $@

build/%.o: %.c
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS) -c -o $@ $<

build/%.o: %.S
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS) -c -o $@ $<

size: $(ELF)
    $(SIZE) $<

# Flash firmware to SPI flash via FPGA JTAG
flash: $(BIN)
    @echo "Flashing firmware to SPI flash at offset 0x140000..."
    ecpprog -o 0x140000 -w $(BIN)

clean:
    rm -rf build/
```

### 9.3 Full Build Script

```bash
#!/bin/bash
# build_all.sh — Complete Chronos Pulser build

set -e

echo "=== Chronos Pulser Full Build ==="
echo ""

# Step 1: Build FPGA gateware
echo "--- Building FPGA Gateware ---"
make -f Makefile.ecp5 all
echo "FPGA bitstream: build/chronos_pulser.bit"
echo ""

# Step 2: Build soft CPU firmware
echo "--- Building Soft CPU Firmware ---"
make -f Makefile.firmware all
echo "Firmware binary: build/firmware.bin"
echo ""

# Step 3: Generate combined SPI flash image
echo "--- Generating SPI Flash Image ---"
python3 scripts/gen_flash_image.py \
    --bitstream build/chronos_pulser.bit \
    --bootloader build/bootloader.bin \
    --firmware build/firmware.bin \
    --calibration cal/tdc_cal.bin \
    --output build/flash_image.bin
echo "Flash image: build/flash_image.bin"
echo ""

# Step 4: Size report
echo "--- Size Report ---"
echo "FPGA bitstream: $(wc -c < build/chronos_pulser.bit) bytes"
make -f Makefile.firmware size
echo "Flash image: $(wc -c < build/flash_image.bin) bytes"
echo ""

echo "=== Build Complete ==="
```

## 10. USB Protocol Command Set

The host communicates with Chronos Pulser via a binary protocol over USB bulk endpoints.

### 10.1 Command Frame Format

```
Byte 0:     Sync byte (0xAA)
Byte 1:     Command ID
Bytes 2–3:  Payload length (16-bit, little-endian)
Bytes 4–N:  Payload data
Bytes N+1–N+2: CRC-16 (CRC-16-CCITT, over bytes 1 through N)
```

### 10.2 Command IDs

| ID | Name | Direction | Description |
|----|------|-----------|-------------|
| 0x01 | CMD_PING | Host→Dev | Device presence check |
| 0x02 | CMD_GET_INFO | Host→Dev | Get device info (fw ver, serial, capabilities) |
| 0x03 | CMD_RESET | Host→Dev | Soft reset device |
| 0x10 | CMD_ACQ_START | Host→Dev | Start acquisition |
| 0x11 | CMD_ACQ_STOP | Host→Dev | Stop acquisition |
| 0x12 | CMD_ACQ_STATUS | Host→Dev | Get acquisition status |
| 0x13 | CMD_ACQ_DATA | Dev→Host | Acquisition data packet |
| 0x20 | CMD_TDC_SINGLE | Host→Dev | Single TDC measurement |
| 0x21 | CMD_TDC_CALIBRATE | Host→Dev | Run TDC calibration |
| 0x22 | CMD_TDC_GET_CAL | Host→Dev | Get calibration data |
| 0x30 | CMD_PULSE_CONFIG | Host→Dev | Configure pulse generator |
| 0x31 | CMD_PULSE_SINGLE | Host→Dev | Fire single pulse |
| 0x40 | CMD_VGA_SET_GAIN | Host→Dev | Set VGA gain |
| 0x50 | CMD_TEMP_READ | Host→Dev | Read temperature |
| 0x60 | CMD_LED_SET | Host→Dev | Set LED color |
| 0x70 | CMD_FLASH_READ | Host→Dev | Read SPI flash |
| 0x71 | CMD_FLASH_WRITE | Host→Dev | Write SPI flash |
| 0xF0 | CMD_BOOTLOADER | Host→Dev | Enter bootloader mode |
| 0xFF | CMD_NACK | Dev→Host | Negative acknowledge (error) |

### 10.3 Response Frame Format

```
Byte 0:     Sync byte (0x55)
Byte 1:     Response code (0x00 = success, else error code)
Bytes 2–3:  Payload length (16-bit, little-endian)
Bytes 4–N:  Payload data
Bytes N+1–N+2: CRC-16
```

### 10.4 Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | ERR_NONE | Success |
| 0x01 | ERR_BUSY | Device busy |
| 0x02 | ERR_TIMEOUT | Operation timed out |
| 0x03 | ERR_OVERFLOW | FIFO/buffer overflow |
| 0x04 | ERR_PARAM | Invalid parameter |
| 0x05 | ERR_STATE | Invalid state for command |
| 0x06 | ERR_HARDWARE | Hardware fault detected |
| 0x07 | ERR_CRC | CRC check failed |
| 0x08 | ERR_OVERTEMP | Device over temperature |
| 0x09 | ERR_NOT_CALIBRATED | Calibration required |
| 0xFF | ERR_UNKNOWN | Unknown error |

---

*End of Phase 4 — Software Stack*
