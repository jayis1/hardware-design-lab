# Aether-Link: Phase 4 — Software Stack

## 1. Boot Strategy

### 1.1 Overall Boot Flow

```
Power-On / PCIe PERST# Deassertion
│
├─[1] FPGA Configuration (SPI x4 Master Mode)
│   ├── FPGA samples M[2:0] = 001 → SPI x4 master
│   ├── FPGA drives CCLK @ 50 MHz (initial), reads bitstream header
│   ├── Bitstream sync word 0xAA995566 detected
│   ├── Full bitstream loaded from MT25QU512 (512 Mb QSPI)
│   ├── Configuration time: ~200ms (32 MB bitstream @ 50 MHz x4 = 200 Mbps)
│   └── CONFIG_DONE asserts high
│
├─[2] Internal Initialization
│   ├── MMCMs lock to reference clocks
│   │   ├── PCIe refclk (100 MHz) → 250 MHz system clock
│   │   ├── DDR4 refclk (200 MHz) → MIG PHY clock
│   │   └── CMAC refclk (161.1328125 MHz) → CMAC SerDes clock
│   ├── DDR4 MIG calibration (~100ms)
│   │   ├── Write leveling
│   │   ├── Read DQS gate training
│   │   ├── Read data eye training
│   │   └── VREF training
│   ├── PCIe Hard IP reset deassertion
│   │   ├── PLL locks to refclk
│   │   ├── Receiver detection on all 8 lanes
│   │   └── Link training begins (Gen1 → Gen2 → Gen3 → Gen4)
│   └── CMAC SerDes PLL lock
│
├─[3] MicroBlaze Boot
│   ├── MicroBlaze reset deasserted
│   ├── Executes from BRAM (64 KB boot ROM)
│   ├── Boot ROM code:
│   │   ├── Initialize GPIO, I2C, UART
│   │   ├── Read TMP117 temperature sensors
│   │   ├── Read INA226 power monitor
│   │   ├── Initialize EMC2301 fan controller (30% PWM default)
│   │   ├── Check QSFP28 module presence via ModPrsL
│   │   ├── Read QSFP28 module EEPROM (I2C) for capabilities
│   │   ├── Wait for PCIe link training completion
│   │   ├── Wait for DDR4 calibration completion
│   │   ├── Release NVMe Command Processor reset
│   │   ├── Release NVMe-oF PDU Engine reset
│   │   ├── Release TCP Offload Engine reset
│   │   ├── Release CMAC reset
│   │   └── Enter main management loop
│   └── Boot time: ~50ms
│
├─[4] PCIe Enumeration
│   ├── Host BIOS enumerates PCIe bus
│   ├── Device reports:
│   │   ├── Vendor ID: 0x10EE (Xilinx)
│   │   ├── Device ID: 0x9038 (custom)
│   │   ├── Class Code: 0x010802 (NVMe controller)
│   │   ├── Subsystem Vendor ID: 0x1A4B (Nous Research)
│   │   ├── Subsystem ID: 0x0001 (Aether-Link)
│   │   └── BAR0: 64KB MMIO (NVMe registers + doorbells)
│   ├── MSI-X capability: 2048 vectors
│   ├── PCIe ACS capability enabled
│   └── AER (Advanced Error Reporting) enabled
│
├─[5] Host Driver Load
│   ├── Linux NVMe driver (nvme.ko) binds to PCI device
│   ├── nvme_reset_work() called
│   │   ├── CC.EN = 0 (disable controller)
│   │   ├── Wait for CSTS.RDY = 0
│   │   ├── Configure Admin Queue (ASQ/ACQ in BAR0)
│   │   ├── CC.EN = 1 (enable controller)
│   │   └── Wait for CSTS.RDY = 1
│   ├── Admin Identify commands issued
│   │   ├── Identify Controller
│   │   ├── Identify Namespace (x128)
│   │   └── Set Features (number of queues, arbitration, etc.)
│   ├── I/O Submission/Completion Queues created
│   └── Block device /dev/nvme0n1 created
│
└─[6] Management Driver Load
    ├── aether_mgmt.ko loaded
    ├── sysfs entries created:
    │   ├── /sys/class/aether/aether0/temp_fpga
    │   ├── /sys/class/aether/aether0/temp_qsfp0
    │   ├── /sys/class/aether/aether0/power_mw
    │   ├── /sys/class/aether/aether0/fan_rpm
    │   ├── /sys/class/aether/aether0/link_speed
    │   ├── /sys/class/aether/aether0/connections
    │   ├── /sys/class/aether/aether0/firmware_version
    │   └── /sys/class/aether/aether0/fw_update
    └── Device fully operational
```

### 1.2 Fallback Boot (Golden Image)

If the primary bitstream in SPI flash is corrupted:
1. FPGA fails configuration (INIT_B stays low after timeout).
2. FPGA asserts fallback trigger (internal watchdog).
3. FPGA re-attempts configuration from SPI address 0x02000000 (golden image at 32MB offset).
4. Golden image is a minimal "safe mode" bitstream with PCIe endpoint only.
5. Host can re-flash primary image via vendor-specific NVMe command.

## 2. MMIO Register Definitions

### 2.1 NVMe Controller Registers (BAR0, offset 0x0000-0x0FFF)

Standard NVMe 1.4c register set. Key registers:

| Offset | Register | Size | Description                              |
|--------|----------|------|------------------------------------------|
| 0x00   | CAP      | 8B   | Controller Capabilities                  |
| 0x08   | VS       | 4B   | Version (1.4.0 = 0x00010400)             |
| 0x0C   | INTMS    | 4B   | Interrupt Mask Set                       |
| 0x10   | INTMC    | 4B   | Interrupt Mask Clear                     |
| 0x14   | CC       | 4B   | Controller Configuration                 |
| 0x1C   | CSTS     | 4B   | Controller Status                        |
| 0x20   | NSSR     | 4B   | NVM Subsystem Reset                      |
| 0x24   | AQA      | 4B   | Admin Queue Attributes                   |
| 0x28   | ASQ      | 8B   | Admin Submission Queue Base Address      |
| 0x30   | ACQ      | 8B   | Admin Completion Queue Base Address      |
| 0x38   | CMBLOC   | 4B   | Controller Memory Buffer Location        |
| 0x3C   | CMBSZ    | 4B   | Controller Memory Buffer Size            |
| 0x1000 | SQ0TDBL  | 4B   | Submission Queue 0 Tail Doorbell          |
| 0x1004 | CQ0HDBL  | 4B   | Completion Queue 0 Head Doorbell         |
| ...    | ...      | ...  | (up to 256 SQ + 256 CQ doorbells)        |

### 2.2 Aether-Link Vendor-Specific Registers (BAR0, offset 0x2000-0x2FFF)

These registers are accessed via the management driver (aether_mgmt.ko) through
a vendor-specific NVMe Admin command (opcode 0xC1 — AETHER_MGMT_CMD).

| Offset | Register              | Size | Access | Description                              |
|--------|-----------------------|------|--------|------------------------------------------|
| 0x2000 | AETHER_MAGIC          | 4B   | RO     | Magic number 0xA37A4C10 ("AETHLINK")     |
| 0x2004 | AETHER_VERSION        | 4B   | RO     | Firmware version (major.minor.patch.build)|
| 0x2008 | AETHER_FEATURES       | 4B   | RO     | Feature bitmap                           |
| 0x200C | AETHER_STATUS         | 4B   | RO     | Global status register                   |
| 0x2010 | AETHER_CONTROL        | 4B   | RW     | Global control register                  |
| 0x2014 | AETHER_TEMP_FPGA      | 2B   | RO     | FPGA junction temp (0.01°C LSB)          |
| 0x2016 | AETHER_TEMP_QSFP0     | 2B   | RO     | QSFP0 module temp (0.01°C LSB)           |
| 0x2018 | AETHER_TEMP_QSFP1     | 2B   | RO     | QSFP1 module temp (0.01°C LSB)           |
| 0x201A | AETHER_POWER_MW       | 4B   | RO     | Total board power in milliwatts          |
| 0x201E | AETHER_FAN0_RPM       | 2B   | RO     | Fan 0 RPM                               |
| 0x2020 | AETHER_FAN1_RPM       | 2B   | RO     | Fan 1 RPM                               |
| 0x2022 | AETHER_FAN_PWM        | 1B   | RW     | Fan PWM duty cycle (0-255)               |
| 0x2024 | AETHER_CONN_COUNT     | 2B   | RO     | Active NVMe-oF connection count          |
| 0x2028 | AETHER_CONN_TABLE_BASE| 8B   | RO     | DDR4 base address of connection table    |
| 0x2030 | AETHER_PORT0_STATUS   | 4B   | RO     | QSFP0 link status                        |
| 0x2034 | AETHER_PORT0_SPEED    | 4B   | RO     | QSFP0 negotiated speed (Mbps)            |
| 0x2038 | AETHER_PORT0_MAC_ADDR_LO| 4B | RO     | Port 0 MAC address [31:0]               |
| 0x203C | AETHER_PORT0_MAC_ADDR_HI| 2B | RO     | Port 0 MAC address [47:32]              |
| 0x2040 | AETHER_PORT1_STATUS   | 4B   | RO     | QSFP1 link status                        |
| 0x2044 | AETHER_PORT1_SPEED    | 4B   | RO     | QSFP1 negotiated speed (Mbps)            |
| 0x2048 | AETHER_PORT1_MAC_ADDR_LO| 4B | RO     | Port 1 MAC address [31:0]               |
| 0x204C | AETHER_PORT1_MAC_ADDR_HI| 2B | RO     | Port 1 MAC address [47:32]              |
| 0x2050 | AETHER_STATS_TX_PKTS  | 8B   | RO     | Total TX packets (both ports)            |
| 0x2058 | AETHER_STATS_RX_PKTS  | 8B   | RO     | Total RX packets (both ports)            |
| 0x2060 | AETHER_STATS_TX_BYTES | 8B   | RO     | Total TX bytes (both ports)             |
| 0x2068 | AETHER_STATS_RX_BYTES | 8B   | RO     | Total RX bytes (both ports)             |
| 0x2070 | AETHER_STATS_TX_DROP  | 8B   | RO     | Total TX drops                          |
| 0x2078 | AETHER_STATS_RX_DROP  | 8B   | RO     | Total RX drops                          |
| 0x2080 | AETHER_STATS_TCP_RETX | 8B   | RO     | TCP retransmissions                     |
| 0x2088 | AETHER_STATS_NVME_IOPS| 8B   | RO     | NVMe I/O commands completed             |
| 0x2090 | AETHER_FW_UPDATE_ADDR | 8B   | WO     | SPI flash address for FW update         |
| 0x2098 | AETHER_FW_UPDATE_LEN  | 4B   | WO     | FW update data length                   |
| 0x209C | AETHER_FW_UPDATE_GO   | 1B   | WO     | Trigger FW update (write 0x01)          |
| 0x20A0 | AETHER_CONN_ADD_TARGET| 32B  | WO     | Add connection: IP[4], port[2], etc.    |
| 0x20C0 | AETHER_CONN_DEL       | 2B   | WO     | Delete connection by ID                 |
| 0x20D0 | AETHER_ERROR_LOG_BASE | 8B   | RO     | DDR4 base of error log ring buffer      |
| 0x20D8 | AETHER_ERROR_LOG_IDX  | 4B   | RO     | Current error log write index           |
| 0x20E0 | AETHER_DEBUG_UART_TX  | 1B   | WO     | Write byte to debug UART                |
| 0x20E1 | AETHER_DEBUG_UART_RX  | 1B   | RO     | Read byte from debug UART               |
| 0x20F0 | AETHER_SCRATCH0       | 4B   | RW     | Scratch register 0 (debug)              |
| 0x20F4 | AETHER_SCRATCH1       | 4B   | RW     | Scratch register 1 (debug)              |
| 0x20F8 | AETHER_SCRATCH2       | 4B   | RW     | Scratch register 2 (debug)              |
| 0x20FC | AETHER_SCRATCH3       | 4B   | RW     | Scratch register 3 (debug)              |

### 2.3 Feature Bitmap (AETHER_FEATURES)

| Bit | Feature                        |
|-----|--------------------------------|
| 0   | NVMe-oF/TCP offload supported  |
| 1   | Dual-port supported            |
| 2   | RDMA zero-copy supported       |
| 3   | TSO (TCP Segmentation Offload) |
| 4   | LRO (Large Receive Offload)    |
| 5   | DCTCP congestion control       |
| 6   | ECN marking support            |
| 7   | DH-HMAC-CHAP authentication    |
| 8   | CRC32C offload                 |
| 9   | Firmware update supported      |
| 10  | Secure boot (AES-GCM)          |
| 11  | DDR4 ECC enabled               |
| 12  | Thermal throttling              |
| 13  | Telemetry (temp/power/stats)   |
| 14  | Debug UART available            |
| 15  | Reserved                       |

### 2.4 Status Register (AETHER_STATUS)

| Bit | Status                         |
|-----|--------------------------------|
| 0   | FPGA configured and ready       |
| 1   | DDR4 calibrated                 |
| 2   | PCIe link up (Gen4)             |
| 3   | Port 0 link up                  |
| 4   | Port 1 link up                  |
| 5   | NVMe controller enabled         |
| 6   | Thermal throttling active       |
| 7   | Firmware update in progress     |
| 8   | Error condition active          |
| 9   | Secure boot passed              |
| 10  | QSFP0 module present            |
| 11  | QSFP1 module present            |
| 12-15| Reserved                       |

## 3. Clock Configuration Code

### 3.1 MMCM Configuration (Vivado TCL / C equivalent)

The FPGA uses MMCME2_ADV primitives to generate all internal clocks from the
100 MHz PCIe reference clock.

```c
// clock_config.c — MMCM and PLL configuration for Aether-Link
// This code runs on MicroBlaze during boot ROM execution.

#include "registers.h"

// MMCM register definitions (dynamic reconfiguration port)
#define MMCM_BASE_ADDR      0x80000000  // AXI4-Lite base for clocking
#define MMCM_CLKOUT0_DIV    0x08        // 250 MHz system clock
#define MMCM_CLKOUT1_DIV    0x0A        // 200 MHz user clock
#define MMCM_CLKFBOUT_DIV   0x10        // Feedback divider

typedef struct {
    volatile uint32_t CR;        // 0x00: Control
    volatile uint32_t SR;        // 0x04: Status
    volatile uint32_t CLKOUT0_1; // 0x08: CLKOUT0 divide (low) + CLKOUT1 divide (high)
    volatile uint32_t CLKOUT2_3; // 0x0C: CLKOUT2/3
    volatile uint32_t CLKOUT4_5; // 0x10: CLKOUT4/5
    volatile uint32_t CLKOUT6;   // 0x14: CLKOUT6
    volatile uint32_t CLKFBOUT;  // 0x18: Feedback divide + fraction
    volatile uint32_t DIVCLK;    // 0x1C: DIVCLK divide
    volatile uint32_t LOCK;      // 0x20: Lock registers
    volatile uint32_t FILTER;   // 0x24: Digital filter
    volatile uint32_t POWER;    // 0x28: Power register
} mmcm_regs_t;

static mmcm_regs_t * const mmcm = (mmcm_regs_t *)MMCM_BASE_ADDR;

// Input: 100 MHz PCIe refclk
// VCO target: 1000 MHz (100 MHz × CLKFBOUT_MULT = 100 × 10 = 1000 MHz)
// CLKOUT0: 1000 / 4 = 250 MHz (system clock for NVMe/PDU/TOE)
// CLKOUT1: 1000 / 5 = 200 MHz (user clock for MicroBlaze, peripherals)

#define MMCM_CLKFBOUT_MULT  10
#define MMCM_DIVCLK_DIVIDE  1
#define MMCM_CLKOUT0_DIVIDE 4
#define MMCM_CLKOUT1_DIVIDE 5

void clock_mmcm_init(void) {
    uint32_t timeout;

    // Step 1: Power up MMCM
    mmcm->POWER = 0x0000;  // Clear power-down bits

    // Step 2: Configure dividers
    // CLKOUT0_1 register: [15:8] = CLKOUT1 divide, [7:0] = CLKOUT0 divide
    mmcm->CLKOUT0_1 = (MMCM_CLKOUT1_DIVIDE << 8) | MMCM_CLKOUT0_DIVIDE;

    // CLKFBOUT register: [7:0] = integer divide, [15:8] = multiply
    mmcm->CLKFBOUT = (MMCM_CLKFBOUT_MULT << 8) | MMCM_CLKFBOUT_MULT;

    // DIVCLK register: [7:0] = divide
    mmcm->DIVCLK = MMCM_DIVCLK_DIVIDE;

    // Step 3: Set digital filter for 100 MHz input
    // LOCK register: set LOCKREG1 based on input frequency
    // For 100 MHz: LOCKREG1 = 0x3E8 (1000 decimal)
    mmcm->LOCK = 0x03E8;

    // Step 4: Start MMCM
    mmcm->CR = 0x0001;  // Set START bit

    // Step 5: Wait for lock (timeout = 100ms at 200MHz = 20M cycles)
    timeout = 20000000;
    while (!(mmcm->SR & 0x01)) {  // Check LOCKED bit
        if (--timeout == 0) {
            // MMCM lock failed — enter error state
            error_handler(ERR_MMCM_LOCK_FAILED);
            return;
        }
    }

    // Step 6: Verify output frequencies via status
    // (In production, read CLKOUT0/1 actual dividers from status registers)
}

// DDR4 MIG reference clock: 200 MHz from SiT9121 (external oscillator)
// This is fed directly to the MIG PHY — no MMCM needed for refclk.
// The MIG internally generates the 800 MHz DDR4 PHY clock from this 200 MHz ref.

// CMAC reference clock: 161.1328125 MHz from SiT9365
// This is fed directly to CMAC SerDes quad PLL.
// CMAC internally multiplies to 25.78125 Gbps line rate.

// QSFP28 reference clocks: 156.25 MHz from SiT9365 (one per port)
// Fed directly to GTX quad PLLs for 25.78125 Gbps per lane.
```

## 4. GPIO Initialization Code

```c
// gpio_init.c — GPIO and peripheral initialization for Aether-Link
// Runs on MicroBlaze during boot ROM execution.

#include "registers.h"
#include "board.h"

// GPIO controller base addresses (AXI4-Lite)
#define GPIO_LEDS_BASE      0x80010000
#define GPIO_QSFP0_BASE     0x80020000
#define GPIO_QSFP1_BASE     0x80030000
#define GPIO_SYSTEM_BASE    0x80040000
#define I2C_MASTER_BASE     0x80050000
#define UART_BASE           0x80060000
#define SPI_MASTER_BASE     0x80070000
#define FAN_PWM_BASE        0x80080000

// GPIO register structure (Xilinx AXI GPIO)
typedef struct {
    volatile uint32_t GPIO_DATA;     // 0x00: Channel 1 data
    volatile uint32_t GPIO_TRI;      // 0x04: Channel 1 tristate
    volatile uint32_t GPIO2_DATA;    // 0x08: Channel 2 data
    volatile uint32_t GPIO2_TRI;     // 0x0C: Channel 2 tristate
    volatile uint32_t GIER;          // 0x11C: Global interrupt enable
    volatile uint32_t IPISR;         // 0x120: IP interrupt status
    volatile uint32_t IPIER;         // 0x128: IP interrupt enable
} gpio_regs_t;

// I2C register structure (Xilinx AXI IIC)
typedef struct {
    volatile uint32_t GIE;           // 0x1C: Global interrupt enable
    volatile uint32_t ISR;           // 0x20: Interrupt status
    volatile uint32_t IER;           // 0x28: Interrupt enable
    volatile uint32_t SOFTR;         // 0x40: Soft reset
    volatile uint32_t CR;            // 0x100: Control register
    volatile uint32_t SR;            // 0x104: Status register
    volatile uint32_t TX_FIFO;       // 0x108: Transmit FIFO
    volatile uint32_t RX_FIFO;       // 0x10C: Receive FIFO
    volatile uint32_t ADR;           // 0x110: Slave address
    volatile uint32_t TX_FIFO_OCY;   // 0x114: TX FIFO occupancy
    volatile uint32_t RX_FIFO_OCY;   // 0x118: RX FIFO occupancy
    volatile uint32_t TEN_ADR;       // 0x11C: 10-bit address
    volatile uint32_t RX_FIFO_PIRQ;  // 0x120: RX FIFO programmable depth
    volatile uint32_t GPO;           // 0x124: General purpose output
    volatile uint32_t TSUSTA;        // 0x128: Timing setup
    volatile uint32_t TSUSTO;        // 0x12C: Timing hold
    volatile uint32_t THDSTA;        // 0x130: Timing hold start
    volatile uint32_t THD_STO;       // 0x134: Timing hold stop
    volatile uint32_t TBUF;          // 0x138: Bus free time
    volatile uint32_t THIGH;         // 0x13C: High period
    volatile uint32_t TLOW;          // 0x140: Low period
    volatile uint32_t THD_DAT;       // 0x144: Data hold time
} i2c_regs_t;

// UART register structure (Xilinx AXI UART Lite)
typedef struct {
    volatile uint32_t RX_FIFO;       // 0x00: Receive data FIFO
    volatile uint32_t TX_FIFO;       // 0x04: Transmit data FIFO
    volatile uint32_t STAT;          // 0x08: Status register
    volatile uint32_t CTRL;          // 0x0C: Control register
} uart_regs_t;

static gpio_regs_t * const gpio_leds   = (gpio_regs_t *)GPIO_LEDS_BASE;
static gpio_regs_t * const gpio_qsfp0  = (gpio_regs_t *)GPIO_QSFP0_BASE;
static gpio_regs_t * const gpio_qsfp1  = (gpio_regs_t *)GPIO_QSFP1_BASE;
static gpio_regs_t * const gpio_sys    = (gpio_regs_t *)GPIO_SYSTEM_BASE;
static i2c_regs_t  * const i2c         = (i2c_regs_t *)I2C_MASTER_BASE;
static uart_regs_t * const uart        = (uart_regs_t *)UART_BASE;

void gpio_init_all(void) {
    // --- LED GPIO ---
    // 4 bi-color LEDs: 8 output bits (GREEN0, RED0, GREEN1, RED1, ...)
    gpio_leds->GPIO_TRI = 0x00;   // All outputs
    gpio_leds->GPIO_DATA = 0x00;  // All LEDs off initially

    // --- QSFP0 Control GPIO ---
    // Bits: [0]=ModPrsL(in), [1]=ResetL(out), [2]=LPMode(out), [3]=IntL(in)
    gpio_qsfp0->GPIO_TRI = 0x05;  // Bits 0 and 2 are inputs (ModPrsL, IntL)
    gpio_qsfp0->GPIO_DATA = 0x02; // ResetL=0 (hold in reset), LPMode=0 (normal)

    // --- QSFP1 Control GPIO ---
    gpio_qsfp1->GPIO_TRI = 0x05;
    gpio_qsfp1->GPIO_DATA = 0x02;

    // --- System GPIO ---
    // Bits: [0]=CONFIG_DONE(in), [1]=PCIe_PERST#(in), [2]=DDR4_CAL_DONE(in)
    gpio_sys->GPIO_TRI = 0x07;   // All inputs
}

void led_set(uint8_t led_idx, uint8_t color) {
    // led_idx: 0-3, color: 0=off, 1=green, 2=red, 3=both(amber)
    uint32_t mask = 0x03 << (led_idx * 2);
    uint32_t val  = color << (led_idx * 2);
    uint32_t data = gpio_leds->GPIO_DATA;
    data = (data & ~mask) | val;
    gpio_leds->GPIO_DATA = data;
}

void qsfp_reset_control(uint8_t port, uint8_t assert_reset) {
    gpio_regs_t *gpio = (port == 0) ? gpio_qsfp0 : gpio_qsfp1;
    uint32_t data = gpio->GPIO_DATA;
    if (assert_reset) {
        data &= ~0x02;  // ResetL = 0
    } else {
        data |= 0x02;   // ResetL = 1
    }
    gpio->GPIO_DATA = data;
}

void qsfp_lpmode_control(uint8_t port, uint8_t enter_lpmode) {
    gpio_regs_t *gpio = (port == 0) ? gpio_qsfp0 : gpio_qsfp1;
    uint32_t data = gpio->GPIO_DATA;
    if (enter_lpmode) {
        data |= 0x04;   // LPMode = 1
    } else {
        data &= ~0x04;  // LPMode = 0
    }
    gpio->GPIO_DATA = data;
}

uint8_t qsfp_module_present(uint8_t port) {
    gpio_regs_t *gpio = (port == 0) ? gpio_qsfp0 : gpio_qsfp1;
    // ModPrsL is active low: 0 = present, 1 = absent
    return ((gpio->GPIO_DATA & 0x01) == 0) ? 1 : 0;
}

// --- I2C Initialization ---
void i2c_init(void) {
    // Soft reset the I2C controller
    i2c->SOFTR = 0x0000000A;  // Write 0xA to reset
    for (volatile int i = 0; i < 1000; i++);  // Wait for reset

    // Configure for 400 kHz Fast Mode
    // Input clock: 200 MHz (MicroBlaze system clock)
    // SCL period = 2.5 µs → 400 kHz
    // 200 MHz → 5 ns per cycle → 500 cycles per SCL period
    // THIGH = 250 cycles (1.25 µs), TLOW = 250 cycles (1.25 µs)
    i2c->THIGH   = 250;
    i2c->TLOW    = 250;
    i2c->TBUF    = 250;  // Bus free time between stop/start
    i2c->THD_STA = 100;  // Hold time after (repeated) start
    i2c->TSU_STA = 100;  // Setup time for repeated start
    i2c->TSU_STO = 100;  // Setup time for stop
    i2c->THD_DAT = 50;   // Data hold time

    // Enable the I2C controller (clear TX/RX FIFO, enable)
    i2c->CR = 0x00000001;  // Enable, 7-bit addressing
}

// --- UART Initialization ---
void uart_init(void) {
    // Configure for 115200 baud, 8N1
    // Input clock: 200 MHz
    // Baud rate divisor = 200,000,000 / (16 × 115200) = 108.5 → 109
    // Actual baud = 200,000,000 / (16 × 109) = 114,679 (0.45% error, acceptable)
    uart->CTRL = 0x0000006D;  // Divisor = 109, enable RX/TX, no interrupts
}

void uart_putc(char c) {
    while (uart->STAT & 0x08);  // Wait while TX FIFO full
    uart->TX_FIFO = (uint32_t)c;
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

char uart_getc(void) {
    while (!(uart->STAT & 0x01));  // Wait while RX FIFO empty
    return (char)(uart->RX_FIFO & 0xFF);
}
```

## 5. Full Device Driver — NVMe-oF TCP Offload Engine (TOE) Driver

### 5.1 TOE Driver Header (toe_driver.h)

```c
// toe_driver.h — TCP Offload Engine Driver for Aether-Link
// Manages 256 hardware TCP connections with TSO/LRO/DCTCP support.

#ifndef TOE_DRIVER_H
#define TOE_DRIVER_H

#include <stdint.h>
#include <stddef.h>

// --- Connection State Structure ---
// Each connection occupies 4096 bytes in DDR4 for alignment.
// 256 connections × 4096 = 1 MB total connection table.

typedef struct {
    uint32_t src_ip;           // Source IP address (host side)
    uint32_t dst_ip;           // Destination IP address (target side)
    uint16_t src_port;         // Source TCP port
    uint16_t dst_port;         // Destination TCP port
    uint32_t snd_una;          // Oldest unacknowledged sequence number
    uint32_t snd_nxt;          // Next sequence number to send
    uint32_t snd_wnd;          // Send window (from target)
    uint32_t snd_wl1;          // Sequence number of last window update
    uint32_t snd_wl2;          // Ack number of last window update
    uint32_t snd_cwnd;         // Congestion window (DCTCP)
    uint32_t snd_ssthresh;     // Slow start threshold
    uint32_t rcv_nxt;          // Next expected receive sequence number
    uint32_t rcv_wnd;          // Receive window (advertised to target)
    uint32_t rcv_adv;          // Last advertised receive window
    uint16_t mss;              // Maximum segment size
    uint16_t mss_clamp;        // Clamped MSS from path MTU discovery
    uint8_t  tcp_state;        // TCP FSM state (CLOSED, LISTEN, SYN_SENT, etc.)
    uint8_t  retransmit_count; // Number of retransmissions for current segment
    uint8_t  dctcp_ecn_flags;  // DCTCP ECN state
    uint8_t  ttl;              // IP TTL value
    uint32_t rto;              // Retransmission timeout (in microseconds)
    uint32_t srtt;             // Smoothed round-trip time
    uint32_t rttvar;           // Round-trip time variation
    uint32_t rtx_timer;        // Retransmit timer (hardware countdown)
    uint32_t keepalive_timer;  // Keepalive timer
    uint32_t ts_recent;        // Most recent timestamp from peer
    uint32_t ts_val;           // Current timestamp value
    uint32_t last_ack_sent;    // Last ACK sent to peer
    uint32_t total_tx_bytes;   // Total bytes transmitted
    uint32_t total_rx_bytes;   // Total bytes received
    uint32_t total_retx;       // Total retransmissions
    uint32_t tx_buffer_base;   // DDR4 base address of TX buffer (256KB)
    uint32_t rx_buffer_base;   // DDR4 base address of RX buffer (256KB)
    uint16_t tx_buffer_head;   // TX ring buffer head offset
    uint16_t tx_buffer_tail;   // TX ring buffer tail offset
    uint16_t rx_buffer_head;   // RX ring buffer head offset
    uint16_t rx_buffer_tail;   // RX ring buffer tail offset
    uint8_t  port_id;          // QSFP28 port (0 or 1)
    uint8_t  conn_id;          // Connection ID (0-255)
    uint8_t  nvme_qid;         // Associated NVMe queue ID
    uint8_t  padding[3];       // Align to 4-byte boundary
    uint32_t checksum;         // CRC32 of connection state (for integrity)
} toe_conn_state_t;

// --- TCP States ---
#define TCP_STATE_CLOSED       0
#define TCP_STATE_LISTEN       1
#define TCP_STATE_SYN_SENT     2
#define TCP_STATE_SYN_RCVD     3
#define TCP_STATE_ESTABLISHED  4
#define TCP_STATE_FIN_WAIT1    5
#define TCP_STATE_FIN_WAIT2    6
#define TCP_STATE_CLOSE_WAIT   7
#define TCP_STATE_CLOSING      8
#define TCP_STATE_LAST_ACK     9
#define TCP_STATE_TIME_WAIT    10

// --- TOE Hardware Register Interface ---
// These registers are memory-mapped in the FPGA fabric, accessed via AXI4-Lite
// from the MicroBlaze management processor.

#define TOE_REG_BASE            0x80100000
#define TOE_REG_CTRL            0x00   // Control register
#define TOE_REG_STATUS          0x04   // Status register
#define TOE_REG_CONN_TABLE_BASE 0x08   // DDR4 base of connection table
#define TOE_REG_CONN_COUNT      0x0C   // Active connection count
#define TOE_REG_PORT0_MAC_LO    0x10   // Port 0 MAC address low
#define TOE_REG_PORT0_MAC_HI    0x14   // Port 0 MAC address high
#define TOE_REG_PORT1_MAC_LO    0x18   // Port 1 MAC address low
#define TOE_REG_PORT1_MAC_HI    0x1C   // Port 1 MAC address high
#define TOE_REG_PORT0_IP        0x20   // Port 0 IP address
#define TOE_REG_PORT1_IP        0x24   // Port 1 IP address
#define TOE_REG_PORT0_NETMASK   0x28   // Port 0 netmask
#define TOE_REG_PORT1_NETMASK   0x2C   // Port 1 netmask
#define TOE_REG_PORT0_GATEWAY   0x30   // Port 0 default gateway
#define TOE_REG_PORT1_GATEWAY   0x34   // Port 1 default gateway
#define TOE_REG_ARP_TABLE_BASE  0x38   // DDR4 base of ARP table
#define TOE_REG_TX_DESC_BASE    0x3C   // TX descriptor ring base
#define TOE_REG_RX_DESC_BASE    0x40   // RX descriptor ring base
#define TOE_REG_INTERRUPT_MASK  0x44   // Interrupt mask
#define TOE_REG_INTERRUPT_STATUS 0x48  // Interrupt status
#define TOE_REG_STATS_BASE      0x4C   // Statistics counters base

// TOE Control Register bits
#define TOE_CTRL_ENABLE         0x01
#define TOE_CTRL_RESET          0x02
#define TOE_CTRL_PORT0_EN       0x04
#define TOE_CTRL_PORT1_EN       0x08
#define TOE_CTRL_TSO_EN         0x10
#define TOE_CTRL_LRO_EN         0x20
#define TOE_CTRL_DCTCP_EN       0x40
#define TOE_CTRL_CRC32C_EN      0x80

// TOE Status Register bits
#define TOE_STATUS_READY        0x01
#define TOE_STATUS_PORT0_LINK   0x02
#define TOE_STATUS_PORT1_LINK   0x04
#define TOE_STATUS_ERROR        0x08
#define TOE_STATUS_BUSY         0x10

// --- Function Prototypes ---

// Initialization
int  toe_init(uint32_t conn_table_ddr4_base, uint8_t *port0_mac,
              uint8_t *port1_mac, uint32_t port0_ip, uint32_t port1_ip);
void toe_enable(void);
void toe_disable(void);
int  toe_get_status(void);

// Connection management
int  toe_connect(uint8_t conn_id, uint32_t dst_ip, uint16_t dst_port,
                 uint16_t src_port, uint8_t port_id);
int  toe_disconnect(uint8_t conn_id);
int  toe_get_conn_state(uint8_t conn_id, toe_conn_state_t *state);

// Data transfer
int  toe_send_data(uint8_t conn_id, const uint8_t *data, uint32_t len);
int  toe_recv_data(uint8_t conn_id, uint8_t *buffer, uint32_t max_len,
                   uint32_t *actual_len);

// Statistics
void toe_get_stats(uint64_t *tx_pkts, uint64_t *rx_pkts,
                   uint64_t *tx_bytes, uint64_t *rx_bytes,
                   uint64_t *tx_drop, uint64_t *rx_drop,
                   uint64_t *retx_count);

// Interrupt handling
void toe_interrupt_handler(void);
void toe_set_interrupt_mask(uint32_t mask);

#endif // TOE_DRIVER_H
```

### 5.2 TOE Driver Implementation (toe_driver.c)

```c
// toe_driver.c — TCP Offload Engine Driver Implementation
// Full hardware-accelerated TCP/IP stack management for Aether-Link.
// Target: MicroBlaze soft processor in XC7K325T FPGA fabric.

#include "toe_driver.h"
#include "registers.h"
#include "board.h"
#include <string.h>

// --- Hardware Register Access ---
// The TOE hardware block is accessed via AXI4-Lite at TOE_REG_BASE.
// All registers are 32-bit, little-endian.

static volatile uint32_t * const toe_regs = (volatile uint32_t *)TOE_REG_BASE;

static inline uint32_t toe_reg_read(uint32_t offset) {
    return toe_regs[offset / 4];
}

static inline void toe_reg_write(uint32_t offset, uint32_t value) {
    toe_regs[offset / 4] = value;
}

// --- Global State ---
static toe_conn_state_t *conn_table;  // Pointer to DDR4 connection table
static uint32_t conn_table_ddr4_base;
static uint8_t toe_initialized = 0;
static uint8_t port0_mac[6];
static uint8_t port1_mac[6];

// --- ARP Table ---
// Simple ARP table: 256 entries, each 16 bytes (IP + MAC + TTL)
#define ARP_TABLE_SIZE      256
#define ARP_ENTRY_SIZE      16
#define ARP_ENTRY_VALID     0x01
#define ARP_ENTRY_STATIC    0x02

typedef struct {
    uint32_t ip_addr;
    uint8_t  mac_addr[6];
    uint16_t ttl_seconds;
    uint8_t  flags;
    uint8_t  padding;
} arp_entry_t;

static arp_entry_t *arp_table;

// --- Initialization ---
int toe_init(uint32_t conn_table_base, uint8_t *p0_mac,
             uint8_t *p1_mac, uint32_t p0_ip, uint32_t p1_ip) {

    if (toe_initialized) {
        return -1;  // Already initialized
    }

    // Store parameters
    conn_table_ddr4_base = conn_table_base;
    conn_table = (toe_conn_state_t *)conn_table_base;
    memcpy(port0_mac, p0_mac, 6);
    memcpy(port1_mac, p1_mac, 6);

    // Initialize ARP table in DDR4 (after connection table)
    arp_table = (arp_entry_t *)(conn_table_base +
                 (256 * sizeof(toe_conn_state_t)));
    memset(arp_table, 0, ARP_TABLE_SIZE * ARP_ENTRY_SIZE);

    // Reset TOE hardware
    toe_reg_write(TOE_REG_CTRL, TOE_CTRL_RESET);
    for (volatile int i = 0; i < 10000; i++);  // Wait for reset
    toe_reg_write(TOE_REG_CTRL, 0x00);

    // Configure MAC addresses
    uint32_t mac0_lo = port0_mac[0] | (port0_mac[1] << 8) |
                       (port0_mac[2] << 16) | (port0_mac[3] << 24);
    uint32_t mac0_hi = port0_mac[4] | (port0_mac[5] << 8);
    uint32_t mac1_lo = port1_mac[0] | (port1_mac[1] << 8) |
                       (port1_mac[2] << 16) | (port1_mac[3] << 24);
    uint32_t mac1_hi = port1_mac[4] | (port1_mac[5] << 8);

    toe_reg_write(TOE_REG_PORT0_MAC_LO, mac0_lo);
    toe_reg_write(TOE_REG_PORT0_MAC_HI, mac0_hi);
    toe_reg_write(TOE_REG_PORT1_MAC_LO, mac1_lo);
    toe_reg_write(TOE_REG_PORT1_MAC_HI, mac1_hi);

    // Configure IP addresses
    toe_reg_write(TOE_REG_PORT0_IP, p0_ip);
    toe_reg_write(TOE_REG_PORT1_IP, p1_ip);

    // Default netmask: /24 (255.255.255.0)
    toe_reg_write(TOE_REG_PORT0_NETMASK, 0x00FFFFFF);
    toe_reg_write(TOE_REG_PORT1_NETMASK, 0x00FFFFFF);

    // Gateway: .1 of subnet (derived from IP)
    toe_reg_write(TOE_REG_PORT0_GATEWAY, (p0_ip & 0x00FFFFFF) | 0x01000000);
    toe_reg_write(TOE_REG_PORT1_GATEWAY, (p1_ip & 0x00FFFFFF) | 0x01000000);

    // Set connection table base in hardware
    toe_reg_write(TOE_REG_CONN_TABLE_BASE, conn_table_base);
    toe_reg_write(TOE_REG_ARP_TABLE_BASE,
                  conn_table_base + (256 * sizeof(toe_conn_state_t)));

    // Clear all connection states
    memset(conn_table, 0, 256 * sizeof(toe_conn_state_t));

    // Initialize each connection slot
    for (int i = 0; i < 256; i++) {
        conn_table[i].conn_id = i;
        conn_table[i].tcp_state = TCP_STATE_CLOSED;
        conn_table[i].mss = 1460;  // Default: 1500 MTU - 40 bytes headers
        conn_table[i].mss_clamp = 1460;
        conn_table[i].ttl = 64;
        conn_table[i].rto = 200000;  // 200ms initial RTO
        conn_table[i].tx_buffer_base = conn_table_base +
            (256 * sizeof(toe_conn_state_t)) +
            (ARP_TABLE_SIZE * ARP_ENTRY_SIZE) +
            (i * 512 * 1024);  // 256KB TX + 256KB RX per connection
        conn_table[i].rx_buffer_base = conn_table[i].tx_buffer_base + 262144;
        conn_table[i].checksum = 0;  // Will be computed on state changes
    }

    // Configure TX/RX descriptor rings in DDR4
    uint32_t desc_base = conn_table_base +
        (256 * sizeof(toe_conn_state_t)) +
        (ARP_TABLE_SIZE * ARP_ENTRY_SIZE) +
        (256 * 512 * 1024);  // After all connection buffers
    toe_reg_write(TOE_REG_TX_DESC_BASE, desc_base);
    toe_reg_write(TOE_REG_RX_DESC_BASE, desc_base + (4096 * 256));

    // Enable interrupts for connection events
    toe_reg_write(TOE_REG_INTERRUPT_MASK,
        0x0000000F);  // Enable connect, disconnect, error, data ready

    toe_initialized = 1;
    return 0;
}

void toe_enable(void) {
    uint32_t ctrl = TOE_CTRL_ENABLE | TOE_CTRL_PORT0_EN | TOE_CTRL_PORT1_EN |
                    TOE_CTRL_TSO_EN | TOE_CTRL_LRO_EN | TOE_CTRL_DCTCP_EN |
                    TOE_CTRL_CRC32C_EN;
    toe_reg_write(TOE_REG_CTRL, ctrl);
}

void toe_disable(void) {
    toe_reg_write(TOE_REG_CTRL, 0x00);
}

int toe_get_status(void) {
    return toe_reg_read(TOE_REG_STATUS);
}

// --- Connection Management ---
int toe_connect(uint8_t conn_id, uint32_t dst_ip, uint16_t dst_port,
                uint16_t src_port, uint8_t port_id) {

    if (!toe_initialized || conn_id >= 256) {
        return -1;
    }

    toe_conn_state_t *conn = &conn_table[conn_id];

    if (conn->tcp_state != TCP_STATE_CLOSED) {
        return -2;  // Connection slot already in use
    }

    // Initialize connection parameters
    conn->dst_ip = dst_ip;
    conn->src_ip = (port_id == 0) ?
        toe_reg_read(TOE_REG_PORT0_IP) : toe_reg_read(TOE_REG_PORT1_IP);
    conn->dst_port = dst_port;
    conn->src_port = src_port;
    conn->port_id = port_id;
    conn->tcp_state = TCP_STATE_SYN_SENT;
    conn->snd_nxt = 0xA0000000 + (conn_id * 0x10000);  // Pseudo-random ISN
    conn->snd_una = conn->snd_nxt;
    conn->snd_wnd = 65535;  // Initial window
    conn->snd_cwnd = conn->mss * 10;  // Initial congestion window (10 segments)
    conn->snd_ssthresh = 65535;
    conn->rcv_nxt = 0;  // Will be set on SYN-ACK
    conn->rcv_wnd = 262144;  // 256KB receive window
    conn->rto = 200000;  // 200ms initial RTO
    conn->srtt = 0;
    conn->rttvar = 0;
    conn->retransmit_count = 0;
    conn->dctcp_ecn_flags = 0;
    conn->tx_buffer_head = 0;
    conn->tx_buffer_tail = 0;
    conn->rx_buffer_head = 0;
    conn->rx_buffer_tail = 0;

    // Trigger hardware TCP handshake
    // Write connection request to hardware command queue
    uint32_t cmd = (0x01 << 24) |  // Command: CONNECT
                   (conn_id << 16) |
                   (port_id << 8);
    toe_reg_write(TOE_REG_CTRL + 0x100, cmd);  // Command register

    return 0;
}

int toe_disconnect(uint8_t conn_id) {
    if (!toe_initialized || conn_id >= 256) {
        return -1;
    }

    toe_conn_state_t *conn = &conn_table[conn_id];

    if (conn->tcp_state == TCP_STATE_CLOSED ||
        conn->tcp_state == TCP_STATE_TIME_WAIT) {
        return -2;  // Already closed
    }

    // Trigger hardware TCP graceful close (FIN sequence)
    uint32_t cmd = (0x02 << 24) |  // Command: DISCONNECT
                   (conn_id << 16);
    toe_reg_write(TOE_REG_CTRL + 0x100, cmd);

    return 0;
}

int toe_get_conn_state(uint8_t conn_id, toe_conn_state_t *state) {
    if (!toe_initialized || conn_id >= 256 || state == NULL) {
        return -1;
    }

    // Copy from DDR4 connection table
    memcpy(state, &conn_table[conn_id], sizeof(toe_conn_state_t));

    // Verify integrity
    uint32_t computed_crc = crc32c((uint8_t *)state,
                                    sizeof(toe_conn_state_t) - 4);
    if (computed_crc != state->checksum) {
        return -3;  // Data corruption detected
    }

    return 0;
}

// --- Data Transfer ---
int toe_send_data(uint8_t conn_id, const uint8_t *data, uint32_t len) {
    if (!toe_initialized || conn_id >= 256 || data == NULL || len == 0) {
        return -1;
    }

    toe_conn_state_t *conn = &conn_table[conn_id];

    if (conn->tcp_state != TCP_STATE_ESTABLISHED) {
        return -2;  // Connection not established
    }

    // Check TX buffer space
    uint32_t tx_buf_size = 262144;  // 256KB
    uint16_t head = conn->tx_buffer_head;
    uint16_t tail = conn->tx_buffer_tail;
    uint32_t used;

    if (head >= tail) {
        used = head - tail;
    } else {
        used = tx_buf_size - tail + head;
    }

    uint32_t available = tx_buf_size - used - 1;  // Leave 1 byte gap
    if (len > available) {
        return -3;  // TX buffer full
    }

    // Copy data to TX buffer in DDR4
    uint8_t *tx_buf = (uint8_t *)conn->tx_buffer_base;
    uint32_t first_chunk = tx_buf_size - tail;

    if (len <= first_chunk) {
        memcpy(tx_buf + tail, data, len);
    } else {
        memcpy(tx_buf + tail, data, first_chunk);
        memcpy(tx_buf, data + first_chunk, len - first_chunk);
    }

    // Update tail pointer
    conn->tx_buffer_tail = (tail + len) % tx_buf_size;

    // Signal hardware to transmit
    uint32_t cmd = (0x03 << 24) |  // Command: SEND
                   (conn_id << 16) |
                   (len & 0xFFFF);
    toe_reg_write(TOE_REG_CTRL + 0x100, cmd);

    return (int)len;
}

int toe_recv_data(uint8_t conn_id, uint8_t *buffer, uint32_t max_len,
                  uint32_t *actual_len) {
    if (!toe_initialized || conn_id >= 256 || buffer == NULL ||
        max_len == 0 || actual_len == NULL) {
        return -1;
    }

    toe_conn_state_t *conn = &conn_table[conn_id];

    if (conn->tcp_state != TCP_STATE_ESTABLISHED) {
        return -2;
    }

    // Check RX buffer for data
    uint32_t rx_buf_size = 262144;
    uint16_t head = conn->rx_buffer_head;
    uint16_t tail = conn->rx_buffer_tail;
    uint32_t available;

    if (tail >= head) {
        available = tail - head;
    } else {
        available = rx_buf_size - head + tail;
    }

    if (available == 0) {
        *actual_len = 0;
        return 0;  // No data available
    }

    // Read data from RX buffer
    uint32_t read_len = (available < max_len) ? available : max_len;
    uint8_t *rx_buf = (uint8_t *)conn->rx_buffer_base;
    uint32_t first_chunk = rx_buf_size - head;

    if (read_len <= first_chunk) {
        memcpy(buffer, rx_buf + head, read_len);
    } else {
        memcpy(buffer, rx_buf + head, first_chunk);
        memcpy(buffer + first_chunk, rx_buf, read_len - first_chunk);
    }

    // Update head pointer
    conn->rx_buffer_head = (head + read_len) % rx_buf_size;
    *actual_len = read_len;

    return 0;
}

// --- Statistics ---
void toe_get_stats(uint64_t *tx_pkts, uint64_t *rx_pkts,
                   uint64_t *tx_bytes, uint64_t *rx_bytes,
                   uint64_t *tx_drop, uint64_t *rx_drop,
                   uint64_t *retx_count) {

    uint32_t stats_base = toe_reg_read(TOE_REG_STATS_BASE);
    volatile uint64_t *stats = (volatile uint64_t *)stats_base;

    // Statistics are 64-bit counters in DDR4, incremented by hardware
    // Order: TX_PKTS, RX_PKTS, TX_BYTES, RX_BYTES, TX_DROP, RX_DROP, RETX
    if (tx_pkts)   *tx_pkts   = stats[0];
    if (rx_pkts)   *rx_pkts   = stats[1];
    if (tx_bytes)  *tx_bytes  = stats[2];
    if (rx_bytes)  *rx_bytes  = stats[3];
    if (tx_drop)   *tx_drop   = stats[4];
    if (rx_drop)   *rx_drop   = stats[5];
    if (retx_count) *retx_count = stats[6];
}

// --- Interrupt Handler ---
void toe_interrupt_handler(void) {
    uint32_t status = toe_reg_read(TOE_REG_INTERRUPT_STATUS);

    if (status & 0x01) {  // Connection established
        uint8_t conn_id = (status >> 8) & 0xFF;
        conn_table[conn_id].tcp_state = TCP_STATE_ESTABLISHED;
        // Notify NVMe-oF PDU engine that connection is ready
        nvmeof_conn_ready_callback(conn_id);
    }

    if (status & 0x02) {  // Connection closed
        uint8_t conn_id = (status >> 16) & 0xFF;
        conn_table[conn_id].tcp_state = TCP_STATE_CLOSED;
        nvmeof_conn_closed_callback(conn_id);
    }

    if (status & 0x04) {  // Data received
        uint8_t conn_id = (status >> 24) & 0xFF;
        // NVMe-oF PDU engine will process received data
        nvmeof_data_ready_callback(conn_id);
    }

    if (status & 0x08) {  // Error condition
        uint8_t conn_id = (status >> 8) & 0xFF;
        uint8_t error_code = status & 0xFF;
        toe_error_handler(conn_id, error_code);
    }

    // Clear interrupt status
    toe_reg_write(TOE_REG_INTERRUPT_STATUS, status);
}

void toe_set_interrupt_mask(uint32_t mask) {
    toe_reg_write(TOE_REG_INTERRUPT_MASK, mask);
}

// --- CRC32C Implementation (Castagnoli polynomial) ---
// Used for connection state integrity checks and NVMe-oF PDU validation.
// Polynomial: 0x1EDC6F41 (Castagnoli)

static const uint32_t crc32c_table[256] = {
    0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4,
    0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
    0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B,
    0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24,
    0x105EC76F, 0xE235446C, 0xF165B798, 0x030E349B,
    0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
    0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54,
    0x5D1D08BF, 0xAF768BBC, 0xBC267848, 0x4E4DFB4B,
    0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A,
    0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35,
    0xAA64D611, 0x580F5512, 0x4B5FA6E6, 0xB93425E5,
    0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
    0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45,
    0xF779DEAE, 0x05125DAD, 0x1642AE59, 0xE4292D5A,
    0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A,
    0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595,
    0x417B1DBC, 0xB3109EBF, 0xA0406D4B, 0x522BEE48,
    0x86E18AA3, 0x748A09A0, 0x67DAFA54, 0x95B17957,
    0xCBA24573, 0x39C9C670, 0x2A993584, 0xD8F2B687,
    0x0C38D26C, 0xFE53516F, 0xED03A29B, 0x1F682198,
    0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927,
    0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38,
    0xDBFC821C, 0x2997011F, 0x3AC7F2EB, 0xC8AC71E8,
    0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
    0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096,
    0xA65C047D, 0x5437877E, 0x4767748A, 0xB50CF789,
    0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859,
    0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46,
    0x7198540D, 0x83F3D70E, 0x90A324FA, 0x62C8A7F9,
    0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
    0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36,
    0x3CDB9BDD, 0xCEB018DE, 0xDDE0EB2A, 0x2F8B6829,
    0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C,
    0x456CAC67, 0xB7072F64, 0xA457DC90, 0x563C5F93,
    0x082F63B7, 0xFA44E0B4, 0xE9141340, 0x1B7F9043,
    0xCFB5F4A8, 0x3DDE77AB, 0x2E8E845F, 0xDCE5075C,
    0x92A8FC17, 0x60C37F14, 0x73938CE0, 0x81F80FE3,
    0x55326B08, 0xA759E80B, 0xB4091BFF, 0x466298FC,
    0x1871A4D8, 0xEA1A27DB, 0xF94AD42F, 0x0B21572C,
    0xDFEB33C7, 0x2D80B0C4, 0x3ED04330, 0xCCBBC033,
    0xA24BB5A6, 0x502036A5, 0x4370C551, 0xB11B4652,
    0x65D122B9, 0x97BAA1BA, 0x84EA524E, 0x7681D14D,
    0x2892ED69, 0xDAF96E6A, 0xC9A99D9E, 0x3BC21E9D,
    0xEF087A76, 0x1D63F975, 0x0E330A81, 0xFC588982,
    0xB21572C9, 0x407EF1CA, 0x532E023E, 0xA145813D,
    0x758FE5D6, 0x87E466D5, 0x94B49521, 0x66DF1622,
    0x38CC2A06, 0xCAA7A905, 0xD9F75AF1, 0x2B9CD9F2,
    0xFF56BD19, 0x0D3D3E1A, 0x1E6DCDEE, 0xEC064EED,
    0xC38D26C4, 0x31E6A5C7, 0x22B65633, 0xD0DDD530,
    0x0417B1DB, 0xF67C32D8, 0xE52CC12C, 0x1747422F,
    0x49547E0B, 0xBB3FFD08, 0xA86F0EFC, 0x5A048DFF,
    0x8ECEE914, 0x7CA56A17, 0x6FF599E3, 0x9D9E1AE0,
    0xD3D3E1AB, 0x21B862A8, 0x32E8915C, 0xC083125F,
    0x144976B4, 0xE622F5B7, 0xF5720643, 0x07198540,
    0x590AB964, 0xAB613A67, 0xB831C993, 0x4A5A4A90,
    0x9E902E7B, 0x6CFBAD78, 0x7FAB5E8C, 0x8DC0DD8F,
    0xE330A81A, 0x115B2B19, 0x020BD8ED, 0xF0605BEE,
    0x24AA3F05, 0xD6C1BC06, 0xC5914FF2, 0x37FACCF1,
    0x69E9F0D5, 0x9B8273D6, 0x88D28022, 0x7AB90321,
    0xAE7367CA, 0x5C18E4C9, 0x4F48173D, 0xBD23943E,
    0xF36E6F75, 0x0105EC76, 0x12551F82, 0xE03E9C81,
    0x34F4F86A, 0xC69F7B69, 0xD5CF889D, 0x27A40B9E,
    0x79B737BA, 0x8BDCB4B9, 0x988C474D, 0x6AE7C44E,
    0xBE2DA0A5, 0x4C4623A6, 0x5F16D052, 0xAD7D5351
};

uint32_t crc32c(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    while (len--) {
        crc = crc32c_table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

// --- Error Handler ---
static void toe_error_handler(uint8_t conn_id, uint8_t error_code) {
    toe_conn_state_t *conn = &conn_table[conn_id];

    switch (error_code) {
        case 0x01:  // Retransmission timeout exceeded
            conn->tcp_state = TCP_STATE_CLOSED;
            conn->retransmit_count = 0;
            break;
        case 0x02:  // Invalid sequence number
            // Force reset by sending RST
            toe_reg_write(TOE_REG_CTRL + 0x104,
                          (0x04 << 24) | (conn_id << 16));  // RST command
            conn->tcp_state = TCP_STATE_CLOSED;
            break;
        case 0x03:  // Checksum error on received segment
            conn->total_rx_bytes++;  // Count as dropped
            break;
        case 0x04:  // Connection timeout (keepalive)
            conn->tcp_state = TCP_STATE_CLOSED;
            break;
        default:
            break;
    }
}

// --- Callback stubs (implemented in NVMe-oF PDU engine driver) ---
__attribute__((weak)) void nvmeof_conn_ready_callback(uint8_t conn_id) {
    (void)conn_id;
}

__attribute__((weak)) void nvmeof_conn_closed_callback(uint8_t conn_id) {
    (void)conn_id;
}

__attribute__((weak)) void nvmeof_data_ready_callback(uint8_t conn_id) {
    (void)conn_id;
}
```

## 6. NVMe-oF PDU Engine Driver

### 6.1 PDU Engine Header (nvmeof_pdu.h)

```c
// nvmeof_pdu.h — NVMe-oF PDU Engine Driver for Aether-Link
// Handles NVMe-oF/TCP capsule construction, parsing, and CRC32C validation.

#ifndef NVMEOF_PDU_H
#define NVMEOF_PDU_H

#include <stdint.h>

// --- NVMe-oF/TCP PDU Types ---
#define NVMEOF_PDU_ICREQ     0x00  // Initialize Connection Request
#define NVMEOF_PDU_ICRESP    0x01  // Initialize Connection Response
#define NVMEOF_PDU_H2D_DATA  0x02  // Host-to-Data PDU (write data)
#define NVMEOF_PDU_D2H_DATA  0x03  // Data-to-Host PDU (read data)
#define NVMEOF_PDU_CAP_CMD   0x04  // Command Capsule
#define NVMEOF_PDU_CAP_RESP  0x05  // Response Capsule
#define NVMEOF_PDU_TERM_REQ  0x06  // Terminate Request
#define NVMEOF_PDU_TERM_RESP 0x07  // Terminate Response

// --- PDU Header Structure (common to all PDU types) ---
typedef struct __attribute__((packed)) {
    uint8_t  pdu_type;         // PDU type (0-7)
    uint8_t  flags;            // PDU-specific flags
    uint8_t  version;          // NVMe-oF version (1.1 = 0x01)
    uint8_t  reserved0;
    uint16_t pdu_data_offset;  // Offset to PDU data (in bytes from header start)
    uint16_t pdu_data_length;  // Length of PDU data
    uint32_t pdu_specific0;    // PDU-specific field 0
    uint32_t pdu_specific1;    // PDU-specific field 1
    uint32_t pdu_specific2;    // PDU-specific field 2
    uint32_t pdu_specific3;    // PDU-specific field 3
    uint32_t pdu_specific4;    // PDU-specific field 4
    uint32_t pdu_specific5;    // PDU-specific field 5
    uint32_t pdu_specific6;    // PDU-specific field 6
    uint32_t pdu_specific7;    // PDU-specific field 7
    uint32_t pdu_specific8;    // PDU-specific field 8
    uint32_t pdu_specific9;    // PDU-specific field 9
    uint32_t header_digest;    // CRC32C of PDU header (optional)
} nvmeof_pdu_header_t;

// --- Command Capsule (ICReq / CAP_CMD) ---
typedef struct __attribute__((packed)) {
    nvmeof_pdu_header_t hdr;
    // NVMe command (64 bytes SQE)
    uint8_t  sqe_opcode;
    uint8_t  sqe_flags;
    uint16_t sqe_command_id;
    uint32_t sqe_nsid;
    uint64_t sqe_reserved;
    uint64_t sqe_metadata_ptr;
    uint64_t sqe_data_ptr[2];  // PRP1, PRP2
    uint32_t sqe_data_len[2];  // Data length, metadata length
    uint32_t sqe_cdw10;
    uint32_t sqe_cdw11;
    uint32_t sqe_cdw12;
    uint32_t sqe_cdw13;
    uint32_t sqe_cdw14;
    uint32_t sqe_cdw15;
    // Optional data digest
    uint32_t data_digest;      // CRC32C of PDU data (optional)
} nvmeof_cmd_capsule_t;

// --- Response Capsule (ICResp / CAP_RESP) ---
typedef struct __attribute__((packed)) {
    nvmeof_pdu_header_t hdr;
    // NVMe completion (16 bytes CQE)
    uint32_t cqe_dw0;          // Command specific
    uint32_t cqe_dw1;          // Command specific
    uint16_t cqe_sq_head;      // SQ head pointer
    uint16_t cqe_sq_id;        // SQ identifier
    uint16_t cqe_command_id;   // Command identifier
    uint16_t cqe_status;       // Status field (phase bit + status)
    uint32_t data_digest;      // CRC32C of PDU data (optional)
} nvmeof_resp_capsule_t;

// --- PDU Engine Hardware Registers ---
#define NVMEOF_REG_BASE         0x80200000
#define NVMEOF_REG_CTRL         0x00
#define NVMEOF_REG_STATUS       0x04
#define NVMEOF_REG_CMD_QUEUE    0x08
#define NVMEOF_REG_RESP_QUEUE   0x0C
#define NVMEOF_REG_DATA_BUF_BASE 0x10
#define NVMEOF_REG_CONN_MAP     0x14
#define NVMEOF_REG_MAX_CONNS    0x18
#define NVMEOF_REG_IOPS_CNT     0x1C
#define NVMEOF_REG_ERROR_CNT    0x20

// --- Function Prototypes ---
int  nvmeof_pdu_init(uint32_t data_buf_ddr4_base);
int  nvmeof_pdu_send_cmd(uint8_t conn_id, const uint8_t *sqe, uint32_t data_len,
                         const uint8_t *data);
int  nvmeof_pdu_recv_resp(uint8_t conn_id, uint8_t *cqe, uint32_t *data_len,
                          uint8_t *data_buf, uint32_t max_data);
void nvmeof_pdu_process_incoming(uint8_t conn_id);
uint64_t nvmeof_pdu_get_iops(void);
uint32_t nvmeof_pdu_get_errors(void);

#endif // NVMEOF_PDU_H
```

### 6.2 PDU Engine Implementation (nvmeof_pdu.c)

```c
// nvmeof_pdu.c — NVMe-oF PDU Engine Implementation
// Full hardware-accelerated NVMe-oF/TCP PDU processing.

#include "nvmeof_pdu.h"
#include "toe_driver.h"
#include "registers.h"
#include <string.h>

static volatile uint32_t * const nvmeof_regs = (volatile uint32_t *)NVMEOF_REG_BASE;
static uint8_t *data_buffer;  // DDR4 data buffer for PDU assembly
static uint32_t data_buf_base;

static inline uint32_t nvmeof_reg_read(uint32_t offset) {
    return nvmeof_regs[offset / 4];
}

static inline void nvmeof_reg_write(uint32_t offset, uint32_t value) {
    nvmeof_regs[offset / 4] = value;
}

// CRC32C table (same as in toe_driver.c)
extern const uint32_t crc32c_table[256];
extern uint32_t crc32c(const uint8_t *data, size_t len);

int nvmeof_pdu_init(uint32_t ddr4_base) {
    data_buf_base = ddr4_base;
    data_buffer = (uint8_t *)ddr4_base;

    // Reset PDU engine
    nvmeof_reg_write(NVMEOF_REG_CTRL, 0x02);  // Reset
    for (volatile int i = 0; i < 1000; i++);
    nvmeof_reg_write(NVMEOF_REG_CTRL, 0x00);

    // Configure data buffer base
    nvmeof_reg_write(NVMEOF_REG_DATA_BUF_BASE, ddr4_base);

    // Set max connections (256)
    nvmeof_reg_write(NVMEOF_REG_MAX_CONNS, 256);

    // Enable PDU engine
    nvmeof_reg_write(NVMEOF_REG_CTRL, 0x01);  // Enable

    return 0;
}

int nvmeof_pdu_send_cmd(uint8_t conn_id, const uint8_t *sqe,
                        uint32_t data_len, const uint8_t *data) {

    if (conn_id >= 256 || sqe == NULL) {
        return -1;
    }

    // Build Command Capsule in DDR4 buffer
    nvmeof_cmd_capsule_t *capsule = (nvmeof_cmd_capsule_t *)data_buffer;

    // Initialize PDU header
    memset(&capsule->hdr, 0, sizeof(nvmeof_pdu_header_t));
    capsule->hdr.pdu_type = NVMEOF_PDU_CAP_CMD;
    capsule->hdr.version = 0x01;  // NVMe-oF 1.1
    capsule->hdr.pdu_data_offset = sizeof(nvmeof_pdu_header_t) + 64;
    capsule->hdr.pdu_data_length = (uint16_t)data_len;

    // Copy SQE (64 bytes)
    memcpy(&capsule->sqe_opcode, sqe, 64);

    // Compute header digest (CRC32C of header, excluding digest field itself)
    capsule->hdr.header_digest = crc32c((uint8_t *)capsule,
                                         sizeof(nvmeof_pdu_header_t) - 4);

    // Copy data after capsule if present
    uint32_t total_len = sizeof(nvmeof_cmd_capsule_t);
    if (data != NULL && data_len > 0) {
        memcpy(data_buffer + sizeof(nvmeof_cmd_capsule_t), data, data_len);
        capsule->data_digest = crc32c(data, data_len);
        total_len += data_len;
    } else {
        capsule->data_digest = 0;
    }

    // Send via TOE
    int ret = toe_send_data(conn_id, data_buffer, total_len);
    return ret;
}

int nvmeof_pdu_recv_resp(uint8_t conn_id, uint8_t *cqe, uint32_t *data_len,
                         uint8_t *data_buf, uint32_t max_data) {

    if (conn_id >= 256 || cqe == NULL) {
        return -1;
    }

    // Receive data from TOE
    uint32_t actual_len = 0;
    int ret = toe_recv_data(conn_id, data_buffer, 65536, &actual_len);
    if (ret != 0 || actual_len < sizeof(nvmeof_resp_capsule_t)) {
        return -2;
    }

    // Parse Response Capsule
    nvmeof_resp_capsule_t *capsule = (nvmeof_resp_capsule_t *)data_buffer;

    // Validate PDU type
    if (capsule->hdr.pdu_type != NVMEOF_PDU_CAP_RESP &&
        capsule->hdr.pdu_type != NVMEOF_PDU_D2H_DATA) {
        return -3;
    }

    // Validate header digest
    uint32_t computed_digest = crc32c((uint8_t *)capsule,
                                       sizeof(nvmeof_pdu_header_t) - 4);
    if (computed_digest != capsule->hdr.header_digest) {
        return -4;  // Header corruption
    }

    // Copy CQE (16 bytes)
    memcpy(cqe, &capsule->cqe_dw0, 16);

    // Extract data if present
    if (capsule->hdr.pdu_data_length > 0 && data_buf != NULL) {
        uint32_t copy_len = capsule->hdr.pdu_data_length;
        if (copy_len > max_data) copy_len = max_data;

        uint8_t *pdu_data = data_buffer + capsule->hdr.pdu_data_offset;
        memcpy(data_buf, pdu_data, copy_len);

        // Validate data digest
        if (capsule->data_digest != 0) {
            uint32_t computed_data_digest = crc32c(pdu_data,
                                                    capsule->hdr.pdu_data_length);
            if (computed_data_digest != capsule->data_digest) {
                return -5;  // Data corruption
            }
        }

        if (data_len) *data_len = copy_len;
    } else {
        if (data_len) *data_len = 0;
    }

    return 0;
}

void nvmeof_pdu_process_incoming(uint8_t conn_id) {
    // Called by TOE interrupt handler when data arrives on a connection.
    // The hardware PDU engine automatically processes incoming PDUs:
    // 1. Validates CRC32C header digest
    // 2. Validates CRC32C data digest (if present)
    // 3. Extracts NVMe CQE from response capsule
    // 4. Posts CQE to host Completion Queue via PCIe
    // 5. If D2H data PDU, DMAs data to host memory via PRP list
    // 6. Generates MSI-X interrupt to host

    // This function handles any software-side processing needed,
    // such as updating statistics or handling error conditions.

    uint32_t status = nvmeof_reg_read(NVMEOF_REG_STATUS);
    if (status & 0x01) {  // PDU processed successfully
        // Update IOPS counter
        uint64_t iops = nvmeof_reg_read(NVMEOF_REG_IOPS_CNT);
        iops |= ((uint64_t)nvmeof_reg_read(NVMEOF_REG_IOPS_CNT + 4) << 32);
        // IOPS counter is incremented by hardware automatically
    }
    if (status & 0x02) {  // PDU error
        uint32_t errors = nvmeof_reg_read(NVMEOF_REG_ERROR_CNT);
        // Log error for diagnostics
    }
}

uint64_t nvmeof_pdu_get_iops(void) {
    uint32_t lo = nvmeof_reg_read(NVMEOF_REG_IOPS_CNT);
    uint32_t hi = nvmeof_reg_read(NVMEOF_REG_IOPS_CNT + 4);
    return ((uint64_t)hi << 32) | lo;
}

uint32_t nvmeof_pdu_get_errors(void) {
    return nvmeof_reg_read(NVMEOF_REG_ERROR_CNT);
}
```

## 7. Device Tree Overlay

```dts
// aether-link.dtso — Device Tree Overlay for Aether-Link PCIe Card
// Applied at boot to describe the NVMe controller and management interface.

/dts-v1/;
/plugin/;

/ {
    compatible = "nous-research,aether-link";

    fragment@0 {
        target = <&pcie_root>;
        __overlay__ {
            #address-cells = <3>;
            #size-cells = <2>;

            aether_link: nvme@0 {
                compatible = "nous-research,aether-link-nvme";
                reg = <0x0000 0x00 0x00 0x00 0x00>;  // BDF assigned by BIOS

                // NVMe controller properties
                nvme,num_namespaces = <128>;
                nvme,max_queue_pairs = <256>;
                nvme,admin_queue_size = <4096>;
                nvme,io_queue_size = <65536>;
                nvme,msix_vectors = <2048>;

                // Aether-Link specific properties
                aether,firmware-version = <0x00010000>;
                aether,port-count = <2>;
                aether,max-connections = <256>;
                aether,ddr4-size-mb = <8192>;
                aether,features = <0x00007FFF>;

                // Default network configuration (overridden by management driver)
                aether,port0-mac-address = [00 1A 4B 00 00 01];
                aether,port1-mac-address = [00 1A 4B 00 00 02];
                aether,port0-ip-address = <0xC0A8010A>;  // 192.168.1.10
                aether,port1-ip-address = <0xC0A8020A>;  // 192.168.2.10
                aether,port0-netmask = <0xFFFFFF00>;     // /24
                aether,port1-netmask = <0xFFFFFF00>;     // /24

                // Thermal management
                aether,fan-min-pwm = <30>;
                aether,fan-max-pwm = <255>;
                aether,temp-warn-celsius = <85>;
                aether,temp-crit-celsius = <95>;
                aether,temp-shutdown-celsius = <100>;

                // Interrupt mapping
                interrupt-parent = <&pcie_intc>;
                interrupts = <0 1 2 3 4 5 6 7>;
                interrupt-names = "nvme-admin", "nvme-io0", "nvme-io1",
                                  "nvme-io2", "nvme-io3", "toe-event",
                                  "thermal", "error";
            };
        };
    };

    fragment@1 {
        target = <&pcie_root>;
        __overlay__ {
            aether_mgmt: aether-mgmt@0,1 {
                compatible = "nous-research,aether-link-mgmt";
                reg = <0x0000 0x00 0x00 0x01 0x00>;

                // Management driver properties
                aether,parent-nvme = <&aether_link>;
                aether,sysfs-class = "aether";
            };
        };
    };
};
```

## 8. Build Instructions

### 8.1 FPGA Bitstream Build (Vivado 2023.2)

```bash
# 1. Create Vivado project
vivado -mode batch -source create_project.tcl

# create_project.tcl:
create_project aether_link ./vivado_project -part xc7k325tffg900-2
set_property target_language VHDL [current_project]

# Add IP cores
create_ip -name clk_wiz -vendor xilinx.com -library ip -module_name mmcm_system
create_ip -name mig_7series -vendor xilinx.com -library ip -module_name ddr4_mig
create_ip -name pcie_7x -vendor xilinx.com -library ip -module_name pcie_gen4x8
create_ip -name cmac_usplus -vendor xilinx.com -library ip -module_name cmac_100gbe
create_ip -name microblaze -vendor xilinx.com -library ip -module_name mb_mgmt

# Configure MIG for DDR4-3200
set_property CONFIG.DDR4_CustomParts CSV [get_ips ddr4_mig]
set_property CONFIG.DDR4_DataWidth 32 [get_ips ddr4_mig]
set_property CONFIG.DDR4_DataMask 1 [get_ips ddr4_mig]
set_property CONFIG.DDR4_ECC 1 [get_ips ddr4_mig]
set_property CONFIG.DDR4_CLKINPeriod 5000 [get_ips ddr4_mig]
set_property CONFIG.DDR4_tCK 625 [get_ips ddr4_mig]

# Configure PCIe for Gen4 x8
set_property CONFIG.PCIE_LINK_CAP_MAX_LINK_SPEED 4 [get_ips pcie_gen4x8]
set_property CONFIG.PCIE_LINK_CAP_MAX_LINK_WIDTH 8 [get_ips pcie_gen4x8]
set_property CONFIG.axi_data_width 256 [get_ips pcie_gen4x8]
set_property CONFIG.axisten_freq 250 [get_ips pcie_gen4x8]
set_property CONFIG.pf0_class_code 0x010802 [get_ips pcie_gen4x8]
set_property CONFIG.pf0_msix_cap_pba_bir 0 [get_ips pcie_gen4x8]
set_property CONFIG.pf0_msix_cap_table_size 2048 [get_ips pcie_gen4x8]

# Add custom RTL
add_files -norecurse ./rtl/
add_files -norecurse ./rtl/nvme_cmd_processor.v
add_files -norecurse ./rtl/nvmeof_pdu_engine.v
add_files -norecurse ./rtl/toe_engine.v
add_files -norecurse ./rtl/axi_interconnect.v
add_files -norecurse ./rtl/aether_top.v

# Run synthesis and implementation
launch_runs synth_1 -jobs 8
wait_on_run synth_1
launch_runs impl_1 -to_step write_bitstream -jobs 8
wait_on_run impl_1

# Generate bitstream with encryption
write_bitstream -force -encrypt -key AES256 aether_link.bit
```

### 8.2 MicroBlaze Firmware Build

```bash
# Build MicroBlaze firmware (bare-metal, no OS)
# Toolchain: Xilinx Vitis 2023.2 or riscv64-unknown-elf-gcc (if using RISC-V MicroBlaze)

cd firmware/

# Compile
mb-gcc -c -O2 -mcpu=v11.0 -mno-xl-soft-mul -Wall -Wextra \
    main.c -o main.o
mb-gcc -c -O2 -mcpu=v11.0 -mno-xl-soft-mul -Wall -Wextra \
    drivers/toe_driver.c -o toe_driver.o
mb-gcc -c -O2 -mcpu=v11.0 -mno-xl-soft-mul -Wall -Wextra \
    drivers/nvmeof_pdu.c -o nvmeof_pdu.o
mb-gcc -c -O2 -mcpu=v11.0 -mno-xl-soft-mul -Wall -Wextra \
    drivers/gpio_init.c -o gpio_init.o
mb-gcc -c -O2 -mcpu=v11.0 -mno-xl-soft-mul -Wall -Wextra \
    drivers/clock_config.c -o clock_config.o

# Link
mb-gcc -T linker_script.ld -o aether_fw.elf \
    main.o toe_driver.o nvmeof_pdu.o gpio_init.o clock_config.o

# Convert to binary for BRAM initialization
mb-objcopy -O binary aether_fw.elf aether_fw.bin

# Generate BRAM initialization COE file
python3 bin_to_coe.py aether_fw.bin aether_fw.coe
```

### 8.3 Linux Management Driver Build

```bash
# Build aether_mgmt.ko (Linux kernel module)
cd driver/

make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

# Load driver
insmod aether_mgmt.ko

# Verify
dmesg | tail -20
ls /sys/class/aether/aether0/
```

### 8.4 React Native App Build

```bash
cd app/

# Install dependencies
npm install

# Start Metro bundler
npx react-native start

# Build for Android
npx react-native run-android

# Build for iOS
cd ios && pod install && cd ..
npx react-native run-ios
```

## 9. Memory Map Summary

### 9.1 DDR4 Memory Layout (8 GB total, 4 GB per channel)

```
Channel A (4 GB):
  0x0000_0000 - 0x0010_0000: Connection state table (256 × 4KB)
  0x0010_0000 - 0x0010_4000: ARP table (256 × 16B)
  0x0010_4000 - 0x0810_4000: Connection TX/RX buffers (256 × 512KB)
  0x0810_4000 - 0x0910_4000: TX descriptor rings
  0x0910_4000 - 0x0A10_4000: RX descriptor rings
  0x0A10_4000 - 0x0A10_8000: Statistics counters
  0x0A10_8000 - 0x0A20_8000: PDU assembly buffer (1 MB)
  0x0A20_8000 - 0x0A20_C000: Error log ring buffer (16 KB)
  0x0A20_C000 - 0x1000_0000: Reserved for future use

Channel B (4 GB):
  0x0000_0000 - 0x8000_0000: NVMe data buffer pool (2 GB)
  0x8000_0000 - 0xFFFF_FFFF: NVMe PRP/SGL page pool (2 GB)
```

### 9.2 AXI4-Lite Peripheral Map (MicroBlaze View)

```
0x8000_0000 - 0x8000_FFFF: MMCM clock configuration
0x8001_0000 - 0x8001_FFFF: GPIO (LEDs)
0x8002_0000 - 0x8002_FFFF: GPIO (QSFP0 control)
0x8003_0000 - 0x8003_FFFF: GPIO (QSFP1 control)
0x8004_0000 - 0x8004_FFFF: GPIO (System status)
0x8005_0000 - 0x8005_FFFF: I2C master
0x8006_0000 - 0x8006_FFFF: UART Lite
0x8007_0000 - 0x8007_FFFF: SPI master (flash access)
0x8008_0000 - 0x8008_FFFF: Fan PWM controller
0x8010_0000 - 0x8010_FFFF: TOE registers
0x8020_0000 - 0x8020_FFFF: NVMe-oF PDU engine registers
0x8030_0000 - 0x8030_FFFF: NVMe command processor registers
0x8040_0000 - 0x8040_FFFF: PCIe configuration and status
0x9000_0000 - 0x9000_FFFF: MicroBlaze local BRAM (64 KB)
```

## 10. Error Handling and Recovery

### 10.1 Error Categories

| Category       | Examples                              | Recovery Strategy                      |
|----------------|---------------------------------------|----------------------------------------|
| PCIe errors    | Link down, TLP poison, completion timeout | AER reporting, link retraining       |
| DDR4 errors    | ECC correctable/uncorrectable         | SEC: log and continue; DED: reset channel|
| Network errors | Link down, CRC errors, retransmit exhaustion | Port failover, connection reset    |
| NVMe-oF errors | PDU CRC mismatch, capsule format error| Discard PDU, increment error counter  |
| Thermal errors | Over-temperature                     | Throttle, then shutdown if critical   |
| Power errors   | Over-current, under-voltage           | Shutdown affected rail, report to host|

### 10.2 Watchdog Timer

- MicroBlaze has a 1-second hardware watchdog timer.
- Main loop must pet the watchdog every 500ms.
- If watchdog expires, FPGA reconfigures from golden image.
- Host driver detects device disappearance and logs critical error.
