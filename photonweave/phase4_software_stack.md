# PhotonWeave — Phase 4: Software Stack

## 1. Boot Strategy

### Overall Boot Sequence

```
Power-On / PCIe Hot-Plug
  │
  ▼
TPS65218 PMIC Power Sequencing (10 ms)
  │  3.3V_STM32 → VCCAUX → VCCINT → MGTAVCC → MGTAVTT → VCCO → VDD_DDR → VTT_DDR
  │  PGOOD asserted
  ▼
STM32H743 Boot (HSE = 25 MHz from Si5345 OUT5)
  │  SystemInit: Configure PLL (HSE / 25 × 480 = 480 MHz SYSCLK)
  │  Initialize I2C1, SPI1, UART4
  │  Read board EEPROM (0x50) for revision, serial
  │  Configure Si5345 via I2C (0x68) — load clock plan
  │  Wait for Si5345 LOL clear (all outputs stable)
  │  Enable Si5345 outputs
  ▼
FPGA Configuration
  │  STM32 asserts FPGA_PROG_B low (≥300 ns pulse)
  │  STM32 releases FPGA_PROG_B, monitors FPGA_INIT_B
  │  FPGA INIT_B goes high → configuration begins
  │  FPGA reads bitstream from MT25QU512 via QSPI (SPI x4 master mode)
  │  Configuration time: ~200 ms (512 Mb @ 50 MHz × 4 bits = 200 Mbps)
  │  FPGA DONE goes high
  │  STM32 detects DONE, sends "FPGA_READY" over SPI
  ▼
FPGA Internal Initialization
  │  MMCM/PLL lock on all clock inputs
  │  MIG (DDR4 Memory Controller) calibration
  │    — Write leveling, read DQS gate training, read data eye training
  │    — ~50 ms calibration time
  │  PCIe Endpoint Block initialization
  │    — Link training: Gen1 → Gen2 → Gen3 negotiation
  │    — ~100 ms (depends on host)
  │  AXI Interconnect bring-up
  │  CGH Pipeline reset release
  ▼
PCIe Enumeration
  │  Host BIOS enumerates PCIe device
  │  BAR0 (64 KB control), BAR1 (16 MB DMA) assigned
  │  MSI-X interrupts configured
  │  Linux driver (photonweave.ko) probes
  ▼
System Ready
  │  STM32 heartbeat LED (D5) blinking at 1 Hz
  │  FPGA DONE LED (D1) solid green
  │  PCIe Link LED (D2) solid blue
  │  /dev/photonweave0 created
  │  Ready for depth map ingest
```

### STM32H743 Firmware Architecture

```
main.c
  ├── SystemClock_Config()     — 480 MHz from HSE bypass
  ├── I2C1_Init()              — 100 kHz, master mode
  ├── SPI1_Init()              — 50 MHz, master, hardware NSS
  ├── UART4_Init()             — 115200 baud, debug console
  ├── Si5345_Configure()       — Load 10-output clock plan
  ├── FPGA_Program()           — Pulse PROG_B, wait for DONE
  ├── Board_EEPROM_Read()      — Validate board identity
  ├── TMP117_Init()            — Configure all 4 sensors (continuous mode)
  ├── Main Loop (1 kHz):
  │   ├── Read all TMP117 temperatures
  │   ├── Check FPGA status over SPI (heartbeat counter, error flags)
  │   ├── Check Si5345 LOL status
  │   ├── Update LED indicators
  │   ├── Handle USB commands from FT601 (config, firmware update)
  │   └── Thermal management: if T > 80°C, alert FPGA to throttle
  └── Interrupt Handlers:
      ├── TMP117 ALERT pin (PE4 EXTI) — thermal warning
      ├── FPGA_INT_N (PE0 EXTI) — FPGA attention
      └── Si5345 INTR (PE5 EXTI) — clock fault
```

## 2. MMIO Register Definitions

### FPGA Control Register Map (BAR0, 64 KB)

All registers are 32-bit, little-endian, accessed via PCIe Memory-Mapped I/O.

```
Base Address: BAR0 (assigned by BIOS)

Offset  | Name                    | Access | Description
--------|-------------------------|--------|----------------------------------
0x0000  | PW_REG_MAGIC            | RO     | Magic number: 0x50484F54 ("PHOT")
0x0004  | PW_REG_VERSION          | RO     | Hardware version: [31:16]=major, [15:0]=minor
0x0008  | PW_REG_STATUS           | RO     | Status register
0x000C  | PW_REG_CONTROL          | RW     | Control register
0x0010  | PW_REG_INTERRUPT_MASK   | RW     | Interrupt mask
0x0014  | PW_REG_INTERRUPT_STATUS | RW1C   | Interrupt status (write 1 to clear)
0x0018  | PW_REG_DMA_CTRL         | RW     | DMA engine control
0x001C  | PW_REG_DMA_STATUS       | RO     | DMA engine status
0x0020  | PW_REG_DMA_SRC_ADDR_LO  | RW     | DMA source address (lower 32 bits)
0x0024  | PW_REG_DMA_SRC_ADDR_HI  | RW     | DMA source address (upper 32 bits)
0x0028  | PW_REG_DMA_DST_ADDR_LO  | RW     | DMA destination address (lower 32 bits)
0x002C  | PW_REG_DMA_DST_ADDR_HI  | RW     | DMA destination address (upper 32 bits)
0x0030  | PW_REG_DMA_LENGTH       | RW     | DMA transfer length (bytes)
0x0034  | PW_REG_DMA_DESC_ADDR_LO | RW     | DMA descriptor ring base (lower 32 bits)
0x0038  | PW_REG_DMA_DESC_ADDR_HI | RW     | DMA descriptor ring base (upper 32 bits)
0x003C  | PW_REG_DMA_DESC_COUNT   | RW     | Number of descriptors in ring
0x0040  | PW_REG_CGH_CTRL         | RW     | CGH pipeline control
0x0044  | PW_REG_CGH_STATUS       | RO     | CGH pipeline status
0x0048  | PW_REG_CGH_PARAMS       | RW     | CGH parameter set selector
0x004C  | PW_REG_CGH_WAVELENGTH   | RW     | Reference wavelength (nm × 1000, e.g., 532000 for 532nm)
0x0050  | PW_REG_CGH_DEPTH_MIN    | RW     | Minimum depth plane (µm)
0x0054  | PW_REG_CGH_DEPTH_MAX    | RW     | Maximum depth plane (µm)
0x0058  | PW_REG_CGH_DEPTH_PLANES | RW     | Number of depth planes (1-256)
0x005C  | PW_REG_CGH_INPUT_W      | RW     | Input depth map width (pixels)
0x0060  | PW_REG_CGH_INPUT_H      | RW     | Input depth map height (pixels)
0x0064  | PW_REG_CGH_OUTPUT_W     | RW     | Output hologram width (pixels)
0x0068  | PW_REG_CGH_OUTPUT_H     | RW     | Output hologram height (pixels)
0x006C  | PW_REG_CGH_FRAME_COUNT  | RO     | Frames processed since reset
0x0070  | PW_REG_CGH_FRAME_TIME   | RO     | Last frame processing time (µs)
0x0074  | PW_REG_CGH_FPS          | RO     | Current frames per second (×1000)
0x0078  | PW_REG_CGH_ERROR_FLAGS  | RO     | Pipeline error flags
0x007C  | PW_REG_CGH_FFT_SCRATCH  | RW     | FFT scratch buffer base offset in DDR4
0x0080  | PW_REG_HDMI_CTRL        | RW     | HDMI output control
0x0084  | PW_REG_HDMI_STATUS      | RO     | HDMI output status
0x0088  | PW_REG_HDMI_FRAME_ADDR  | RW     | HDMI frame buffer address in DDR4
0x008C  | PW_REG_HDMI_TIMING_H    | RW     | HDMI horizontal timing (HActive, HFP, HSync, HBP)
0x0090  | PW_REG_HDMI_TIMING_V    | RW     | HDMI vertical timing (VActive, VFP, VSync, VBP)
0x0094  | PW_REG_QSFP_CTRL        | RW     | QSFP+ control
0x0098  | PW_REG_QSFP_STATUS      | RO     | QSFP+ status
0x009C  | PW_REG_QSFP_RX_FRAME    | RW     | QSFP+ receive frame buffer address
0x00A0  | PW_REG_TEMP_FPGA_1      | RO     | FPGA temperature sensor 1 (millidegrees C)
0x00A4  | PW_REG_TEMP_FPGA_2      | RO     | FPGA temperature sensor 2 (millidegrees C)
0x00A8  | PW_REG_TEMP_DDR4        | RO     | DDR4 temperature (millidegrees C)
0x00AC  | PW_REG_TEMP_PMIC        | RO     | PMIC temperature (millidegrees C)
0x00B0  | PW_REG_POWER_VCCINT     | RO     | VCCINT voltage (mV)
0x00B4  | PW_REG_POWER_VCCAUX     | RO     | VCCAUX voltage (mV)
0x00B8  | PW_REG_POWER_VDD_DDR    | RO     | VDD_DDR voltage (mV)
0x00BC  | PW_REG_POWER_MGTAVCC    | RO     | MGTAVCC voltage (mV)
0x00C0  | PW_REG_POWER_MGTAVTT    | RO     | MGTAVTT voltage (mV)
0x00C4  | PW_REG_POWER_CURRENT    | RO     | Total board current (mA)
0x00C8  | PW_REG_POWER_CONSUMED   | RO     | Total energy consumed (mWh)
0x00CC  | PW_REG_CLOCK_STATUS     | RO     | Clock monitor status
0x00D0  | PW_REG_DEBUG_UART       | RW     | Debug UART data (to STM32)
0x00D4  | PW_REG_SCRATCH_0        | RW     | General-purpose scratch register
0x00D8  | PW_REG_SCRATCH_1        | RW     | General-purpose scratch register
0x00DC  | PW_REG_SCRATCH_2        | RW     | General-purpose scratch register
0x00E0  | PW_REG_SCRATCH_3        | RW     | General-purpose scratch register
0x00E4  | PW_REG_FPGA_DIE_ID_LO   | RO     | Xilinx Device DNA (lower 32 bits)
0x00E8  | PW_REG_FPGA_DIE_ID_HI   | RO     | Xilinx Device DNA (upper 32 bits)
0x00EC  | PW_REG_EFUSE_USER       | RO     | User eFuse value
0x00F0  | PW_REG_RESET_CONTROL    | RW     | Soft reset control
0x00F4  | PW_REG_CLOCK_THROTTLE   | RW     | Dynamic clock scaling control
0x00F8  | PW_REG_WATCHDOG_KICK    | WO     | Watchdog timer kick (write any value)
0x00FC  | PW_REG_WATCHDOG_TIMEOUT | RW     | Watchdog timeout (ms, 0=disabled)
...
0x1000  | PW_REG_DMA_APERTURE     | —      | Start of 16 MB DMA aperture (BAR1)
```

### Status Register (0x0008) Bit Fields

```
Bit 0:   FPGA_READY          — FPGA configuration complete, all PLLs locked
Bit 1:   PCIE_LINK_UP        — PCIe link in L0 state
Bit 2:   DDR4_CAL_DONE       — All 4 DDR4 interfaces calibrated
Bit 3:   CGH_IDLE            — CGH pipeline idle (ready for new frame)
Bit 4:   CGH_BUSY            — CGH pipeline processing frame
Bit 5:   CGH_FRAME_DONE      — Frame complete, hologram ready
Bit 6:   HDMI_ACTIVE         — HDMI output streaming
Bit 7:   QSFP_LINK_UP        — QSFP+ link established
Bit 8:   THERMAL_WARNING     — Temperature >80°C
Bit 9:   THERMAL_CRITICAL    — Temperature >95°C (throttling active)
Bit 10:  CLOCK_LOL           — Si5345 loss of lock detected
Bit 11:  DDR4_ECC_ERROR      — DDR4 ECC error detected
Bit 12:  DMA_ERROR           — DMA transfer error
Bit 13:  PCIE_ERROR          — PCIe link error
Bit 14:  WATCHDOG_EXPIRED    — Watchdog timer expired
Bit 15:  POWER_FAULT         — Power rail out of spec
Bit 16-23: FFT_ENGINE_FAULT  — Per-engine fault flags (8 engines)
Bit 24-31: Reserved
```

### Control Register (0x000C) Bit Fields

```
Bit 0:   CGH_START           — Write 1 to start CGH pipeline processing
Bit 1:   CGH_ABORT           — Write 1 to abort current frame
Bit 2:   HDMI_ENABLE         — Enable HDMI output
Bit 3:   QSFP_ENABLE         — Enable QSFP+ receiver
Bit 4:   DMA_ENABLE          — Enable DMA engine
Bit 5:   INTERRUPT_ENABLE    — Global interrupt enable
Bit 6:   THERMAL_THROTTLE    — Enable automatic thermal throttling
Bit 7:   ECC_ENABLE          — Enable DDR4 ECC monitoring
Bit 8:   WATCHDOG_ENABLE     — Enable watchdog timer
Bit 9:   SOFT_RESET_CGH      — Reset CGH pipeline only
Bit 10:  SOFT_RESET_DMA      — Reset DMA engine only
Bit 11:  SOFT_RESET_HDMI     — Reset HDMI output only
Bit 12:  CLOCK_SCALE_DOWN    — Scale clocks down for low power
Bit 13-15: Reserved
Bit 16-19: CGH_PARAM_SET     — Select active parameter set (0-15)
Bit 20-23: FFT_ENGINE_MASK   — Bitmask of active FFT engines (0=all 8)
Bit 24-31: Reserved
```

### CGH Parameter Set Structure (16 sets, each at offset 0x100 + N×0x40)

```
Offset within set:
0x00: wavelength_nm_x1000    — Reference wavelength (e.g., 532000 for 532nm green)
0x04: depth_min_um           — Minimum depth in micrometers
0x08: depth_max_um           — Maximum depth in micrometers
0x0C: depth_planes           — Number of depth planes (1-256)
0x10: input_width            — Input depth map width
0x14: input_height           — Input depth map height
0x18: output_width           — Output hologram width
0x1C: output_height          — Output hologram height
0x20: fft_size               — FFT size (must be power of 2, ≥ output dimensions)
0x24: propagation_model      — 0=Fresnel, 1=Angular Spectrum, 2=Fraunhofer
0x28: phase_quantize_bits    — Output quantization bits (8 or 10)
0x2C: color_channel          — 0=Red(638nm), 1=Green(532nm), 2=Blue(450nm)
0x30: pixel_pitch_um_x100    — SLM pixel pitch in µm × 100 (e.g., 360 for 3.6µm)
0x34: fill_factor_percent    — SLM fill factor (0-100)
0x38: reserved_0
0x3C: reserved_1
```

## 3. Clock Configuration Code

### STM32H743 System Clock Configuration

```c
// system_clock.c — STM32H743 Clock Configuration
// HSE bypass: 25 MHz from Si5345 OUT5 → PH0 (OSC_IN)
// SYSCLK = 480 MHz, HCLK = 240 MHz, APB1 = 120 MHz, APB2 = 120 MHz

#include "stm32h743xx.h"

void SystemClock_Config(void)
{
    // 1. Enable HSE bypass mode (external clock from Si5345)
    RCC->CR |= RCC_CR_HSEBYP;          // HSE bypass
    RCC->CR |= RCC_CR_HSEON;           // HSE enable
    while (!(RCC->CR & RCC_CR_HSERDY)); // Wait for HSE ready

    // 2. Configure power supply for 480 MHz operation
    // VOS1 (Voltage Scale 1) required for >400 MHz
    PWR->CR3 |= PWR_CR3_SCUEN;         // Enable supply configuration
    PWR->D3CR |= (PWR_D3CR_VOS_0 | PWR_D3CR_VOS_1); // VOS scale 1
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY)); // Wait for VOS ready

    // 3. Configure Flash latency for 480 MHz
    // 480 MHz @ VOS1 requires 4 wait states
    FLASH->ACR = FLASH_ACR_LATENCY_4WS;

    // 4. PLL1 configuration: 480 MHz SYSCLK
    // PLL1 source = HSE (25 MHz)
    // DIVM1 = 5  → VCO input = 5 MHz
    // DIVN1 = 96 → VCO = 5 × 96 = 480 MHz
    // DIVP1 = 2  → PLL1P = 480 / 2 = 240 MHz (SYSCLK source)
    // DIVQ1 = 4  → PLL1Q = 480 / 4 = 120 MHz
    // DIVR1 = 2  → PLL1R = 480 / 2 = 240 MHz

    RCC->PLLCKSELR = (5 << RCC_PLLCKSELR_DIVM1_Pos) |  // DIVM1 = 5
                     (RCC_PLLCKSELR_PLLSRC_HSE);        // Source = HSE

    RCC->PLL1DIVR = ((2-1) << RCC_PLL1DIVR_DIVN1_Pos) | // DIVN1 = 96 (register = N-1)
                     ((2-1) << RCC_PLL1DIVR_DIVP1_Pos) | // DIVP1 = 2
                     ((4-1) << RCC_PLL1DIVR_DIVQ1_Pos) | // DIVQ1 = 4
                     ((2-1) << RCC_PLL1DIVR_DIVR1_Pos);  // DIVR1 = 2

    // Enable PLL1
    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY));

    // 5. Configure system clock dividers
    RCC->D1CFGR = (0 << RCC_D1CFGR_HPRE_Pos)  |  // HCLK = SYSCLK / 1 = 480 MHz
                  (2 << RCC_D1CFGR_D1PPRE_Pos) |  // APB3 = HCLK / 4 = 120 MHz
                  (2 << RCC_D1CFGR_D1CPRE_Pos);   // SYSCLK = PLL1P / 1 = 240 MHz

    RCC->D2CFGR = (2 << RCC_D2CFGR_D2PPRE1_Pos) | // APB1 = HCLK / 4 = 120 MHz
                  (2 << RCC_D2CFGR_D2PPRE2_Pos);  // APB2 = HCLK / 4 = 120 MHz

    RCC->D3CFGR = (2 << RCC_D3CFGR_D3PPRE_Pos);   // APB4 = HCLK / 4 = 120 MHz

    // 6. Switch system clock to PLL1
    RCC->CFGR |= RCC_CFGR_SW_PLL1;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1);

    // 7. Update SystemCoreClock variable
    SystemCoreClock = 480000000;
}
```

### Si5345 Clock Generator Configuration (via STM32 I2C)

```c
// si5345_config.c — Si5345A Clock Generator Configuration
// Reference: 25 MHz TCXO (SiT5356)
// Outputs: 10 clocks for FPGA, STM32, FT601

#include "si5345_config.h"

// Si5345 register map (partial, relevant registers)
#define SI5345_PAGE_REG          0x01
#define SI5345_DEVICE_READY      0xFE
#define SI5345_SOFT_RESET        0x001C
#define SI5345_LOL_STATUS        0x000E
#define SI5345_INTR_STATUS       0x0011
#define SI5345_OE_CONTROL        0x0002
#define SI5345_NVM_READ          0x052B

// Frequency plan for each output
// OUT0: 200.000000 MHz (FFT core)
// OUT1: 125.000000 MHz (PCIe user)
// OUT2: 300.000000 MHz (DDR4 ref)
// OUT3: 148.500000 MHz (HDMI pixel)
// OUT4: 100.000000 MHz (AXI interconnect)
// OUT5:  25.000000 MHz (STM32 HSE)
// OUT6:  50.000000 MHz (FPGA config)
// OUT7:  60.000000 MHz (FT601 ref)
// OUT8: 156.250000 MHz (QSFP+ ref)
// OUT9:  26.000000 MHz (HDMI TMDS ref)

// Si5345 frequency plan calculation:
// Fvco = Fref × (N_num / N_den) where N = Nn / Nden
// Fout = Fvco / R

// For 25 MHz reference, target VCO = 15000 MHz (15 GHz)
// N = 15000 / 25 = 600 → Nn=600, Nden=1
// OUT0: R = 15000 / 200 = 75
// OUT1: R = 15000 / 125 = 120
// OUT2: R = 15000 / 300 = 50
// OUT3: R = 15000 / 148.5 = 101.0101... → use fractional: Rn=101, Rden=99
// OUT4: R = 15000 / 100 = 150
// OUT5: R = 15000 / 25 = 600
// OUT6: R = 15000 / 50 = 300
// OUT7: R = 15000 / 60 = 250
// OUT8: R = 15000 / 156.25 = 96
// OUT9: R = 15000 / 26 = 576.923... → use fractional: Rn=577, Rden=1000

typedef struct {
    uint16_t addr;
    uint8_t  value;
} si5345_reg_t;

// Full register configuration for the clock plan
static const si5345_reg_t si5345_config[] = {
    // Page 0: Device configuration
    {0x0000, 0x00}, // Page select
    {0x0001, 0x00}, // Device ID (read-only)
    {0x0002, 0x00}, // OE control (all outputs enabled after config)
    {0x000B, 0x00}, // Input divider: Nden[7:0] = 0 (Nden=1)
    {0x000C, 0x00}, // Nden[15:8]
    {0x000D, 0x00}, // Nden[23:16]
    {0x000E, 0x00}, // LOL status (read-only)
    {0x0011, 0x00}, // Interrupt status (read-only)
    {0x0013, 0x00}, // Nn[7:0] = 0x58 (600 decimal)
    {0x0014, 0x02}, // Nn[15:8] = 0x02 → Nn = 0x0258 = 600
    {0x0015, 0x00}, // Nn[23:16]
    {0x0016, 0x00}, // Nn[31:24]
    {0x0017, 0x00}, // Nnum[7:0] = 0 (integer mode)
    {0x0018, 0x00}, // Nnum[15:8]
    {0x0019, 0x00}, // Nnum[23:16]
    {0x001A, 0x00}, // Nnum[31:24]
    {0x001B, 0x00}, // Nden[7:0] = 1
    {0x001C, 0x00}, // Nden[15:8]
    {0x001D, 0x00}, // Nden[23:16]
    {0x001E, 0x00}, // Nden[31:24]

    // OUT0: 200 MHz, R = 75
    {0x0100, 0x00}, // OUT0 R[7:0] = 75
    {0x0101, 0x4B}, // OUT0 R[15:8] = 0x4B → R = 75
    {0x0102, 0x00}, // OUT0 R[23:16]
    {0x0103, 0x00}, // OUT0 format: LVCMOS 3.3V
    {0x0104, 0x00}, // OUT0 enable
    {0x0105, 0x00}, // OUT0 driver settings

    // OUT1: 125 MHz, R = 120
    {0x0108, 0x00}, // OUT1 R[7:0] = 120
    {0x0109, 0x78}, // OUT1 R[15:8] = 0x78 → R = 120
    {0x010A, 0x00},
    {0x010B, 0x00}, // LVCMOS 3.3V
    {0x010C, 0x00},
    {0x010D, 0x00},

    // OUT2: 300 MHz, R = 50
    {0x0110, 0x00}, // OUT2 R[7:0] = 50
    {0x0111, 0x32}, // OUT2 R[15:8] = 0x32 → R = 50
    {0x0112, 0x00},
    {0x0113, 0x00}, // LVCMOS 3.3V
    {0x0114, 0x00},
    {0x0115, 0x00},

    // OUT3: 148.5 MHz, R = 101 + 1/99 (fractional)
    {0x0118, 0x00}, // OUT3 R[7:0] = 101
    {0x0119, 0x65}, // OUT3 R[15:8] = 0x65 → R = 101
    {0x011A, 0x00},
    {0x011B, 0x00}, // LVCMOS 3.3V
    {0x011C, 0x00},
    {0x011D, 0x00},
    // Fractional: Rn=101, Rden=99, Rnum=1
    {0x0230, 0x00}, // OUT3 Rnum[7:0] = 1
    {0x0231, 0x01},
    {0x0232, 0x00},
    {0x0233, 0x00},
    {0x0234, 0x00}, // OUT3 Rden[7:0] = 99
    {0x0235, 0x63},
    {0x0236, 0x00},
    {0x0237, 0x00},

    // OUT4: 100 MHz, R = 150
    {0x0120, 0x00}, // OUT4 R[7:0] = 150
    {0x0121, 0x96}, // OUT4 R[15:8] = 0x96 → R = 150
    {0x0122, 0x00},
    {0x0123, 0x00},
    {0x0124, 0x00},
    {0x0125, 0x00},

    // OUT5: 25 MHz, R = 600
    {0x0128, 0x00}, // OUT5 R[7:0] = 600
    {0x0129, 0x58}, // OUT5 R[15:8] = 0x58
    {0x012A, 0x02}, // OUT5 R[23:16] = 0x02 → R = 0x0258 = 600
    {0x012B, 0x00}, // LVCMOS 3.3V
    {0x012C, 0x00},
    {0x012D, 0x00},

    // OUT6: 50 MHz, R = 300
    {0x0130, 0x00}, // OUT6 R[7:0] = 300
    {0x0131, 0x2C}, // OUT6 R[15:8] = 0x2C
    {0x0132, 0x01}, // OUT6 R[23:16] = 0x01 → R = 0x012C = 300
    {0x0133, 0x00},
    {0x0134, 0x00},
    {0x0135, 0x00},

    // OUT7: 60 MHz, R = 250
    {0x0138, 0x00}, // OUT7 R[7:0] = 250
    {0x0139, 0xFA}, // OUT7 R[15:8] = 0xFA → R = 250
    {0x013A, 0x00},
    {0x013B, 0x00},
    {0x013C, 0x00},
    {0x013D, 0x00},

    // OUT8: 156.25 MHz, R = 96
    {0x0140, 0x00}, // OUT8 R[7:0] = 96
    {0x0141, 0x60}, // OUT8 R[15:8] = 0x60 → R = 96
    {0x0142, 0x00},
    {0x0143, 0x00}, // LVCMOS 3.3V
    {0x0144, 0x00},
    {0x0145, 0x00},

    // OUT9: 26 MHz, R = 577 - 923/1000 (fractional)
    {0x0148, 0x00}, // OUT9 R[7:0] = 577
    {0x0149, 0x41}, // OUT9 R[15:8] = 0x41
    {0x014A, 0x02}, // OUT9 R[23:16] = 0x02 → R = 0x0241 = 577
    {0x014B, 0x00},
    {0x014C, 0x00},
    {0x014D, 0x00},
    // Fractional: Rn=577, Rden=1000, Rnum=923
    {0x0248, 0x00}, // OUT9 Rnum[7:0] = 923
    {0x0249, 0x9B},
    {0x024A, 0x03},
    {0x024B, 0x00},
    {0x024C, 0x00}, // OUT9 Rden[7:0] = 1000
    {0x024D, 0xE8},
    {0x024E, 0x03},
    {0x024F, 0x00},

    // End of configuration
    {0xFFFF, 0x00} // Sentinel
};

int si5345_configure(I2C_HandleTypeDef *hi2c)
{
    uint8_t page = 0xFF; // Force page write on first register
    uint8_t data[2];

    // 1. Wait for device ready
    // Read register 0xFE (DEVICE_READY)
    uint8_t dev_ready;
    for (int retry = 0; retry < 100; retry++) {
        HAL_I2C_Mem_Read(hi2c, SI5345_ADDR, SI5345_DEVICE_READY,
                         I2C_MEMADD_SIZE_8BIT, &dev_ready, 1, 100);
        if (dev_ready & 0x01) break;
        HAL_Delay(10);
    }
    if (!(dev_ready & 0x01)) return -1; // Device not ready

    // 2. Soft reset
    data[0] = 0x01; // Reset bit
    HAL_I2C_Mem_Write(hi2c, SI5345_ADDR, SI5345_SOFT_RESET,
                      I2C_MEMADD_SIZE_8BIT, data, 1, 100);
    HAL_Delay(50); // Wait for reset to complete

    // 3. Write configuration registers
    for (int i = 0; si5345_config[i].addr != 0xFFFF; i++) {
        uint16_t addr = si5345_config[i].addr;
        uint8_t new_page = (addr >> 8) & 0xFF;
        uint8_t reg = addr & 0xFF;

        // Write page register if page changed
        if (new_page != page) {
            data[0] = new_page;
            HAL_I2C_Mem_Write(hi2c, SI5345_ADDR, SI5345_PAGE_REG,
                             I2C_MEMADD_SIZE_8BIT, data, 1, 100);
            page = new_page;
        }

        // Write register value
        data[0] = si5345_config[i].value;
        HAL_I2C_Mem_Write(hi2c, SI5345_ADDR, reg,
                         I2C_MEMADD_SIZE_8BIT, data, 1, 100);
    }

    // 4. Wait for PLL lock
    HAL_Delay(100); // Allow time for PLL to lock

    // Check LOL status
    uint8_t lol_status;
    HAL_I2C_Mem_Read(hi2c, SI5345_ADDR, SI5345_LOL_STATUS,
                    I2C_MEMADD_SIZE_8BIT, &lol_status, 1, 100);
    if (lol_status & 0x01) return -2; // PLL not locked

    // 5. Enable all outputs
    data[0] = 0x00; // All outputs enabled
    HAL_I2C_Mem_Write(hi2c, SI5345_ADDR, SI5345_OE_CONTROL,
                      I2C_MEMADD_SIZE_8BIT, data, 1, 100);

    return 0; // Success
}
```

## 4. GPIO Initialization Code

```c
// gpio_init.c — STM32H743 GPIO Initialization
// Configures all I/O pins for FPGA control, I2C, SPI, UART, LEDs, sensors

#include "stm32h743xx.h"
#include "board.h"

void GPIO_Init(void)
{
    // Enable all required GPIO clocks
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOBEN |
                    RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN |
                    RCC_AHB4ENR_GPIOEEN | RCC_AHB4ENR_GPIOHEN;

    // ── I2C1: PB6 (SCL), PB7 (SDA) ──
    // Alternate function 4, open-drain, pull-up (external 2.2k)
    GPIOB->MODER   &= ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
    GPIOB->MODER   |= (GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1); // AF mode
    GPIOB->OTYPER  |= (GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);       // Open-drain
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED6 | GPIO_OSPEEDR_OSPEED7); // High speed
    GPIOB->PUPDR   &= ~(GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7);   // No pull (external)
    GPIOB->AFR[0]  &= ~(0xFF << 24); // Clear AF for pins 6,7
    GPIOB->AFR[0]  |= (4 << 24) | (4 << 28); // AF4 = I2C1

    // ── SPI1: PA5 (SCK), PA6 (MISO), PA7 (MOSI), PA4 (NSS) ──
    // Alternate function 5, push-pull, high speed
    GPIOA->MODER   &= ~(GPIO_MODER_MODE4 | GPIO_MODER_MODE5 |
                        GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
    GPIOA->MODER   |= (GPIO_MODER_MODE4_1 | GPIO_MODER_MODE5_1 |
                        GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1);
    GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED4 | GPIO_OSPEEDR_OSPEED5 |
                        GPIO_OSPEEDR_OSPEED6 | GPIO_OSPEEDR_OSPEED7); // Very high
    GPIOA->AFR[0]  &= ~(0xFFFF << 16); // Clear AF for pins 4-7
    GPIOA->AFR[0]  |= (5 << 16) | (5 << 20) | (5 << 24) | (5 << 28); // AF5 = SPI1

    // ── UART4: PD1 (TX), PD0 (RX) ──
    // Alternate function 8, push-pull
    GPIOD->MODER   &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE1);
    GPIOD->MODER   |= (GPIO_MODER_MODE0_1 | GPIO_MODER_MODE1_1);
    GPIOD->AFR[0]  &= ~(0xFF);
    GPIOD->AFR[0]  |= (8 << 0) | (8 << 4); // AF8 = UART4

    // ── FPGA Control Signals (PE0-PE3) ──
    // PE0: FPGA_INT_N (input, pull-up)
    GPIOE->MODER   &= ~GPIO_MODER_MODE0; // Input
    GPIOE->PUPDR   &= ~GPIO_PUPDR_PUPD0;
    GPIOE->PUPDR   |= GPIO_PUPDR_PUPD0_0; // Pull-up

    // PE1: FPGA_PROG_B (output, push-pull)
    GPIOE->MODER   &= ~GPIO_MODER_MODE1;
    GPIOE->MODER   |= GPIO_MODER_MODE1_0; // Output
    GPIOE->BSRR    = GPIO_BSRR_BS1; // Set high (inactive)

    // PE2: FPGA_INIT_B (input, pull-up)
    GPIOE->MODER   &= ~GPIO_MODER_MODE2; // Input
    GPIOE->PUPDR   &= ~GPIO_PUPDR_PUPD2;
    GPIOE->PUPDR   |= GPIO_PUPDR_PUPD2_0; // Pull-up

    // PE3: FPGA_DONE (input, pull-up)
    GPIOE->MODER   &= ~GPIO_MODER_MODE3; // Input
    GPIOE->PUPDR   &= ~GPIO_PUPDR_PUPD3;
    GPIOE->PUPDR   |= GPIO_PUPDR_PUPD3_0; // Pull-up

    // PE4: TMP117_ALERT (input, pull-up, EXTI)
    GPIOE->MODER   &= ~GPIO_MODER_MODE4; // Input
    GPIOE->PUPDR   &= ~GPIO_PUPDR_PUPD4;
    GPIOE->PUPDR   |= GPIO_PUPDR_PUPD4_0; // Pull-up

    // PE5: Si5345_INTR (input, pull-up, EXTI)
    GPIOE->MODER   &= ~GPIO_MODER_MODE5; // Input
    GPIOE->PUPDR   &= ~GPIO_PUPDR_PUPD5;
    GPIOE->PUPDR   |= GPIO_PUPDR_PUPD5_0; // Pull-up

    // ── LED Outputs ──
    // PB0: STM32 Heartbeat LED (D5)
    GPIOB->MODER   &= ~GPIO_MODER_MODE0;
    GPIOB->MODER   |= GPIO_MODER_MODE0_0; // Output
    GPIOB->BSRR    = GPIO_BSRR_BR0; // Start low (off)

    // PB1: USB Enumeration LED (D6)
    GPIOB->MODER   &= ~GPIO_MODER_MODE1;
    GPIOB->MODER   |= GPIO_MODER_MODE1_0; // Output
    GPIOB->BSRR    = GPIO_BSRR_BR1; // Start low (off)

    // ── EXTI Configuration for interrupts ──
    // Map PE0 (FPGA_INT_N) to EXTI0
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PE; // Port E

    // Map PE4 (TMP117_ALERT) to EXTI4
    SYSCFG->EXTICR[1] &= ~SYSCFG_EXTICR2_EXTI4;
    SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI4_PE;

    // Map PE5 (Si5345_INTR) to EXTI5
    SYSCFG->EXTICR[1] &= ~SYSCFG_EXTICR2_EXTI5;
    SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI5_PE;

    // Configure EXTI triggers: falling edge for all
    EXTI->FTSR1 |= (EXTI_FTSR1_FT0 | EXTI_FTSR1_FT4 | EXTI_FTSR1_FT5);
    EXTI->RTSR1 &= ~(EXTI_RTSR1_RT0 | EXTI_RTSR1_RT4 | EXTI_RTSR1_RT5);

    // Enable EXTI interrupts in NVIC
    NVIC_SetPriority(EXTI0_IRQn, 2);
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_SetPriority(EXTI4_IRQn, 3);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_SetPriority(EXTI9_5_IRQn, 3);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    // Unmask EXTI lines
    EXTI->IMR1 |= (EXTI_IMR1_IM0 | EXTI_IMR1_IM4 | EXTI_IMR1_IM5);
}
```

## 5. Full Device Driver: DDR4 Memory Controller (MIG) with DMA

### DDR4 MIG Driver Header

```c
// ddr4_mig_driver.h — Xilinx 7-Series MIG DDR4 Memory Controller Driver
// Manages 4 independent 16-bit DDR4-2400 interfaces
// Provides AXI4 interface to CGH pipeline and DMA engine

#ifndef DDR4_MIG_DRIVER_H
#define DDR4_MIG_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// DDR4 Physical Configuration
#define DDR4_NUM_INTERFACES     4
#define DDR4_DATA_WIDTH         16      // Per interface
#define DDR4_TOTAL_DATA_WIDTH   64      // Combined
#define DDR4_CLK_FREQ_MHZ       300     // DDR4 reference clock
#define DDR4_DATA_RATE_MBPS     2400    // DDR4-2400
#define DDR4_CAPACITY_MB        1024    // Per chip (8Gb)
#define DDR4_TOTAL_CAPACITY_MB  4096    // 4 GB total

// DDR4 Timing Parameters (MT40A512M16JY-075E, DDR4-2400)
// tCK = 0.833 ns (1200 MHz clock)
#define DDR4_tCK_PS             833
#define DDR4_tRCD_NS            14      // RAS-to-CAS delay
#define DDR4_tRP_NS             14      // Row precharge time
#define DDR4_tRAS_NS            32      // Row active time
#define DDR4_tRC_NS             46      // Row cycle time
#define DDR4_tRRD_NS            5       // Row-to-row delay
#define DDR4_tFAW_NS            21      // Four-activate window
#define DDR4_tWTR_NS            4       // Write-to-read delay
#define DDR4_tRTP_NS            7       // Read-to-precharge delay
#define DDR4_tWR_NS             15      // Write recovery time
#define DDR4_tCL_CK             15      // CAS latency (CK cycles)
#define DDR4_tCWL_CK            11      // CAS write latency
#define DDR4_tAL_CK             0       // Additive latency
#define DDR4_tCCD_CK            4       // CAS-to-CAS delay

// MIG Register Map (AXI4-Lite Slave, base address in FPGA fabric)
#define MIG0_BASE_ADDR          0x44000000  // Interface 0
#define MIG1_BASE_ADDR          0x44010000  // Interface 1
#define MIG2_BASE_ADDR          0x44020000  // Interface 2
#define MIG3_BASE_ADDR          0x44030000  // Interface 3

// MIG Control Registers (offset from base)
#define MIG_REG_CTRL            0x00    // Control register
#define MIG_REG_STATUS          0x04    // Status register
#define MIG_REG_CAL_STATUS      0x08    // Calibration status
#define MIG_REG_ECC_STATUS      0x0C    // ECC error status
#define MIG_REG_ECC_ERR_ADDR    0x10    // ECC error address
#define MIG_REG_ECC_ERR_COUNT  0x14    // ECC error counter
#define MIG_REG_TEMP            0x18    // DRAM temperature (from MR4)
#define MIG_REG_REFRESH_COUNT   0x1C    // Refresh cycle counter
#define MIG_REG_PERF_RD_COUNT   0x20    // Read transaction counter
#define MIG_REG_PERF_WR_COUNT   0x24    // Write transaction counter
#define MIG_REG_PERF_RD_BYTES   0x28    // Read bytes counter
#define MIG_REG_PERF_WR_BYTES   0x2C    // Write bytes counter
#define MIG_REG_PERF_LATENCY    0x30    // Average read latency (ns)
#define MIG_REG_PERF_BANDWIDTH  0x34    // Current bandwidth (MB/s × 100)

// MIG Status Register Bits
#define MIG_STATUS_CAL_DONE     0x01    // Calibration complete
#define MIG_STATUS_PLL_LOCKED   0x02    // MMCM locked
#define MIG_STATUS_IDLE         0x04    // Controller idle
#define MIG_STATUS_ECC_ENABLED  0x08    // ECC enabled
#define MIG_STATUS_ECC_ERROR    0x10    // ECC error detected
#define MIG_STATUS_ECC_UNCORR   0x20    // Uncorrectable error
#define MIG_STATUS_TEMP_ALERT   0x40    // Temperature alert (>85°C)
#define MIG_STATUS_REFRESH_OVF  0x80    // Refresh overflow

// DDR4 Memory Partition Map
#define DDR4_PART_FRAME_A       0x00000000  // Frame Buffer A (256 MB)
#define DDR4_PART_FRAME_B       0x10000000  // Frame Buffer B (256 MB)
#define DDR4_PART_WORKING       0x20000000  // Working Buffer (512 MB)
#define DDR4_PART_FFT_SCRATCH   0x40000000  // FFT Scratch (1024 MB)
#define DDR4_PART_HOLOGRAM_0    0x80000000  // Hologram Buffer 0 (256 MB)
#define DDR4_PART_HOLOGRAM_1    0x90000000  // Hologram Buffer 1 (256 MB)
#define DDR4_PART_OUTPUT        0xA0000000  // Output Buffer (64 MB)
#define DDR4_PART_DESC          0xA4000000  // Descriptor Tables (16 MB)
#define DDR4_PART_RESERVED      0xA5000000  // Reserved (1.5 GB)

// Function prototypes
int  ddr4_mig_init_all(void);
int  ddr4_mig_init_interface(uint8_t iface);
bool ddr4_mig_is_calibrated(uint8_t iface);
int  ddr4_mig_wait_calibration(uint8_t iface, uint32_t timeout_ms);
void ddr4_mig_get_status(uint8_t iface, uint32_t *status);
int  ddr4_mig_check_ecc(uint8_t iface, uint32_t *error_count,
                         uint64_t *error_addr, bool *uncorrectable);
void ddr4_mig_get_performance(uint8_t iface, uint32_t *rd_bw,
                               uint32_t *wr_bw, uint32_t *avg_latency);
void ddr4_mig_get_temperature(uint8_t iface, int32_t *temp_millic);
int  ddr4_mig_axi_write(uint8_t iface, uint64_t addr, const void *data,
                         uint32_t len_bytes);
int  ddr4_mig_axi_read(uint8_t iface, uint64_t addr, void *data,
                        uint32_t len_bytes);
int  ddr4_mig_axi_burst_write(uint8_t iface, uint64_t addr,
                               const void *data, uint32_t burst_len);
int  ddr4_mig_axi_burst_read(uint8_t iface, uint64_t addr,
                              void *data, uint32_t burst_len);
void ddr4_mig_soft_reset(uint8_t iface);

#endif // DDR4_MIG_DRIVER_H
```

### DDR4 MIG Driver Implementation

```c
// ddr4_mig_driver.c — DDR4 MIG Driver Implementation
// Full driver for Xilinx 7-Series MIG DDR4 controller
// Handles calibration, ECC, performance monitoring, AXI transactions

#include "ddr4_mig_driver.h"
#include "registers.h"
#include <string.h>

// Internal MIG register base addresses
static const uint32_t mig_base_addrs[DDR4_NUM_INTERFACES] = {
    MIG0_BASE_ADDR, MIG1_BASE_ADDR, MIG2_BASE_ADDR, MIG3_BASE_ADDR
};

// AXI4-Lite register read helper
static inline uint32_t mig_reg_read(uint8_t iface, uint32_t offset)
{
    volatile uint32_t *reg = (volatile uint32_t *)(mig_base_addrs[iface] + offset);
    return *reg;
}

// AXI4-Lite register write helper
static inline void mig_reg_write(uint8_t iface, uint32_t offset, uint32_t value)
{
    volatile uint32_t *reg = (volatile uint32_t *)(mig_base_addrs[iface] + offset);
    *reg = value;
}

// Initialize all 4 DDR4 interfaces
int ddr4_mig_init_all(void)
{
    int result;
    uint32_t status;

    // Initialize each interface sequentially
    for (uint8_t iface = 0; iface < DDR4_NUM_INTERFACES; iface++) {
        result = ddr4_mig_init_interface(iface);
        if (result != 0) {
            return result; // Return error code from failing interface
        }
    }

    // Verify all interfaces are calibrated
    for (uint8_t iface = 0; iface < DDR4_NUM_INTERFACES; iface++) {
        if (!ddr4_mig_is_calibrated(iface)) {
            return -10 - iface; // Calibration failed
        }
    }

    // Enable ECC on all interfaces
    for (uint8_t iface = 0; iface < DDR4_NUM_INTERFACES; iface++) {
        uint32_t ctrl = mig_reg_read(iface, MIG_REG_CTRL);
        ctrl |= 0x08; // ECC enable bit
        mig_reg_write(iface, MIG_REG_CTRL, ctrl);
    }

    return 0; // All interfaces initialized successfully
}

// Initialize a single DDR4 interface
int ddr4_mig_init_interface(uint8_t iface)
{
    if (iface >= DDR4_NUM_INTERFACES) return -1;

    uint32_t ctrl, status;

    // 1. Check if already initialized
    status = mig_reg_read(iface, MIG_REG_STATUS);
    if (status & MIG_STATUS_CAL_DONE) {
        return 0; // Already calibrated
    }

    // 2. Assert soft reset to MIG
    ctrl = mig_reg_read(iface, MIG_REG_CTRL);
    ctrl |= 0x01; // Soft reset bit
    mig_reg_write(iface, MIG_REG_CTRL, ctrl);

    // Wait 10 µs for reset to propagate
    for (volatile int i = 0; i < 10000; i++) { __asm__("nop"); }

    // 3. Release reset
    ctrl &= ~0x01;
    mig_reg_write(iface, MIG_REG_CTRL, ctrl);

    // 4. Wait for PLL lock
    for (int retry = 0; retry < 1000; retry++) {
        status = mig_reg_read(iface, MIG_REG_STATUS);
        if (status & MIG_STATUS_PLL_LOCKED) break;
        for (volatile int i = 0; i < 1000; i++) { __asm__("nop"); } // ~1 µs delay
    }

    if (!(status & MIG_STATUS_PLL_LOCKED)) {
        return -2; // PLL failed to lock
    }

    // 5. Trigger calibration
    ctrl = mig_reg_read(iface, MIG_REG_CTRL);
    ctrl |= 0x02; // Start calibration bit
    mig_reg_write(iface, MIG_REG_CTRL, ctrl);

    // 6. Wait for calibration to complete (up to 100 ms)
    int result = ddr4_mig_wait_calibration(iface, 100);
    if (result != 0) {
        return result;
    }

    return 0;
}

// Check if interface is calibrated
bool ddr4_mig_is_calibrated(uint8_t iface)
{
    if (iface >= DDR4_NUM_INTERFACES) return false;
    uint32_t status = mig_reg_read(iface, MIG_REG_STATUS);
    return (status & MIG_STATUS_CAL_DONE) != 0;
}

// Wait for calibration with timeout
int ddr4_mig_wait_calibration(uint8_t iface, uint32_t timeout_ms)
{
    if (iface >= DDR4_NUM_INTERFACES) return -1;

    uint32_t cal_status;
    uint32_t elapsed_us = 0;

    while (elapsed_us < (timeout_ms * 1000)) {
        cal_status = mig_reg_read(iface, MIG_REG_CAL_STATUS);

        // Check calibration stage completion bits
        // Stage 0: Write leveling
        // Stage 1: Read DQS gate training
        // Stage 2: Read data eye training
        // Stage 3: Write data eye training (optional)
        // All stages done → bit 7 set
        if (cal_status & 0x80) {
            // Verify CAL_DONE in main status register
            uint32_t status = mig_reg_read(iface, MIG_REG_STATUS);
            if (status & MIG_STATUS_CAL_DONE) {
                return 0; // Calibration successful
            }
        }

        // Check for calibration failure
        if (cal_status & 0x40) { // Error bit
            // Read error details
            uint32_t error_stage = (cal_status >> 8) & 0x07;
            uint32_t error_byte = (cal_status >> 16) & 0x0F;
            return -100 - (error_stage * 16) - error_byte;
        }

        // ~10 µs polling interval
        for (volatile int i = 0; i < 10000; i++) { __asm__("nop"); }
        elapsed_us += 10;
    }

    return -3; // Timeout
}

// Get MIG status word
void ddr4_mig_get_status(uint8_t iface, uint32_t *status)
{
    if (iface >= DDR4_NUM_INTERFACES || status == NULL) return;
    *status = mig_reg_read(iface, MIG_REG_STATUS);
}

// Check ECC status
int ddr4_mig_check_ecc(uint8_t iface, uint32_t *error_count,
                        uint64_t *error_addr, bool *uncorrectable)
{
    if (iface >= DDR4_NUM_INTERFACES) return -1;

    uint32_t ecc_status = mig_reg_read(iface, MIG_REG_ECC_STATUS);

    if (error_count) {
        *error_count = mig_reg_read(iface, MIG_REG_ECC_ERR_COUNT);
    }

    if (error_addr) {
        uint32_t addr_lo = mig_reg_read(iface, MIG_REG_ECC_ERR_ADDR);
        uint32_t addr_hi = mig_reg_read(iface, MIG_REG_ECC_ERR_ADDR + 4);
        *error_addr = ((uint64_t)addr_hi << 32) | addr_lo;
    }

    if (uncorrectable) {
        *uncorrectable = (ecc_status & 0x02) != 0; // Uncorrectable error bit
    }

    // Clear ECC status by writing 1 to clear bits
    mig_reg_write(iface, MIG_REG_ECC_STATUS, ecc_status);

    return (ecc_status & 0x01) ? 1 : 0; // Return 1 if error detected
}

// Get performance metrics
void ddr4_mig_get_performance(uint8_t iface, uint32_t *rd_bw,
                               uint32_t *wr_bw, uint32_t *avg_latency)
{
    if (iface >= DDR4_NUM_INTERFACES) return;

    if (rd_bw) {
        *rd_bw = mig_reg_read(iface, MIG_REG_PERF_BANDWIDTH);
    }

    // Write bandwidth = total bandwidth - read bandwidth (approximate)
    if (wr_bw) {
        uint32_t total_bytes = mig_reg_read(iface, MIG_REG_PERF_WR_BYTES);
        uint32_t total_cycles = mig_reg_read(iface, MIG_REG_REFRESH_COUNT);
        // Simplified: wr_bw = wr_bytes / elapsed_time
        *wr_bw = 0; // Would need time reference
    }

    if (avg_latency) {
        *avg_latency = mig_reg_read(iface, MIG_REG_PERF_LATENCY);
    }
}

// Get DRAM temperature from MR4
void ddr4_mig_get_temperature(uint8_t iface, int32_t *temp_millic)
{
    if (iface >= DDR4_NUM_INTERFACES || temp_millic == NULL) return;

    uint32_t temp_raw = mig_reg_read(iface, MIG_REG_TEMP);
    // MR4 temperature: 7-bit signed value, 0.5°C resolution
    // Convert to millidegrees C
    int32_t temp_c_x2 = (int32_t)((temp_raw & 0x7F) << 25) >> 25; // Sign-extend 7-bit
    *temp_millic = temp_c_x2 * 500;
}

// AXI4 write transaction (single beat, up to 64 bytes)
int ddr4_mig_axi_write(uint8_t iface, uint64_t addr, const void *data,
                        uint32_t len_bytes)
{
    if (iface >= DDR4_NUM_INTERFACES || data == NULL) return -1;
    if (len_bytes == 0 || len_bytes > 64) return -2;
    if (addr & 0x3) return -3; // Must be 4-byte aligned

    // AXI4 write: write address channel, then write data channel
    volatile uint32_t *axi_base = (volatile uint32_t *)(mig_base_addrs[iface] + 0x1000);

    // Write address
    axi_base[0] = (uint32_t)(addr & 0xFFFFFFFF);       // AWADDR low
    axi_base[1] = (uint32_t)(addr >> 32);               // AWADDR high
    axi_base[2] = len_bytes;                            // AWLEN (burst length in bytes)
    axi_base[3] = 0x01;                                 // AWVALID

    // Wait for AWREADY
    while (!(axi_base[4] & 0x02)); // AWREADY bit

    // Write data (copy up to 64 bytes in 32-bit words)
    const uint32_t *src = (const uint32_t *)data;
    uint32_t words = (len_bytes + 3) >> 2;
    for (uint32_t i = 0; i < words; i++) {
        axi_base[8 + i] = src[i]; // WDATA
    }
    axi_base[7] = 0x01; // WVALID
    axi_base[7] |= (words - 1) << 8; // WSTRB (all bytes valid)

    // Wait for WREADY
    while (!(axi_base[4] & 0x08)); // WREADY bit

    // Write response
    while (!(axi_base[4] & 0x10)); // BVALID
    uint32_t bresp = axi_base[5];
    axi_base[4] = 0x10; // BREADY

    return (bresp == 0) ? 0 : -4; // OKAY=0, EXOKAY=0, SLVERR=2, DECERR=3
}

// AXI4 read transaction (single beat, up to 64 bytes)
int ddr4_mig_axi_read(uint8_t iface, uint64_t addr, void *data,
                       uint32_t len_bytes)
{
    if (iface >= DDR4_NUM_INTERFACES || data == NULL) return -1;
    if (len_bytes == 0 || len_bytes > 64) return -2;
    if (addr & 0x3) return -3;

    volatile uint32_t *axi_base = (volatile uint32_t *)(mig_base_addrs[iface] + 0x1000);

    // Read address
    axi_base[6] = (uint32_t)(addr & 0xFFFFFFFF);       // ARADDR low
    axi_base[6 + 1] = (uint32_t)(addr >> 32);          // ARADDR high
    axi_base[6 + 2] = len_bytes;                        // ARLEN
    axi_base[6 + 3] = 0x01;                            // ARVALID

    // Wait for ARREADY
    while (!(axi_base[4] & 0x01)); // ARREADY bit

    // Wait for RVALID
    while (!(axi_base[4] & 0x20)); // RVALID bit

    // Read data
    uint32_t *dst = (uint32_t *)data;
    uint32_t words = (len_bytes + 3) >> 2;
    for (uint32_t i = 0; i < words; i++) {
        dst[i] = axi_base[16 + i]; // RDATA
    }
    axi_base[4] = 0x20; // RREADY

    uint32_t rresp = axi_base[17];
    return (rresp == 0) ? 0 : -4;
}

// AXI4 burst write (up to 256 beats × 64 bytes = 16 KB)
int ddr4_mig_axi_burst_write(uint8_t iface, uint64_t addr,
                              const void *data, uint32_t burst_len)
{
    if (iface >= DDR4_NUM_INTERFACES || data == NULL) return -1;
    if (burst_len == 0 || burst_len > 256) return -2;
    if (addr & 0x3F) return -3; // Burst-aligned to 64-byte boundary

    volatile uint32_t *axi_base = (volatile uint32_t *)(mig_base_addrs[iface] + 0x1000);

    // Write address with burst length
    axi_base[0] = (uint32_t)(addr & 0xFFFFFFFF);
    axi_base[1] = (uint32_t)(addr >> 32);
    axi_base[2] = burst_len - 1; // AXI LEN = burst_len - 1
    axi_base[3] = 0x01; // AWVALID

    while (!(axi_base[4] & 0x02));

    // Write data beats
    const uint32_t *src = (const uint32_t *)data;
    for (uint32_t beat = 0; beat < burst_len; beat++) {
        for (uint32_t w = 0; w < 16; w++) { // 16 words = 64 bytes per beat
            axi_base[8 + w] = src[beat * 16 + w];
        }
        axi_base[7] = 0x01 | (0xFFFF << 8); // WVALID + all strobes
        if (beat == burst_len - 1) {
            axi_base[7] |= 0x02; // WLAST on final beat
        }
        while (!(axi_base[4] & 0x08)); // Wait for WREADY per beat
    }

    // Write response
    while (!(axi_base[4] & 0x10));
    uint32_t bresp = axi_base[5];
    axi_base[4] = 0x10;

    return (bresp == 0) ? 0 : -4;
}

// AXI4 burst read (up to 256 beats × 64 bytes = 16 KB)
int ddr4_mig_axi_burst_read(uint8_t iface, uint64_t addr,
                             void *data, uint32_t burst_len)
{
    if (iface >= DDR4_NUM_INTERFACES || data == NULL) return -1;
    if (burst_len == 0 || burst_len > 256) return -2;
    if (addr & 0x3F) return -3;

    volatile uint32_t *axi_base = (volatile uint32_t *)(mig_base_addrs[iface] + 0x1000);

    // Read address
    axi_base[6] = (uint32_t)(addr & 0xFFFFFFFF);
    axi_base[7] = (uint32_t)(addr >> 32);
    axi_base[8] = burst_len - 1;
    axi_base[9] = 0x01; // ARVALID

    while (!(axi_base[4] & 0x01));

    // Read data beats
    uint32_t *dst = (uint32_t *)data;
    for (uint32_t beat = 0; beat < burst_len; beat++) {
        while (!(axi_base[4] & 0x20)); // Wait for RVALID
        for (uint32_t w = 0; w < 16; w++) {
            dst[beat * 16 + w] = axi_base[16 + w];
        }
        axi_base[4] = 0x20; // RREADY
    }

    return 0;
}

// Soft reset a MIG interface
void ddr4_mig_soft_reset(uint8_t iface)
{
    if (iface >= DDR4_NUM_INTERFACES) return;

    uint32_t ctrl = mig_reg_read(iface, MIG_REG_CTRL);
    ctrl |= 0x01; // Assert reset
    mig_reg_write(iface, MIG_REG_CTRL, ctrl);

    for (volatile int i = 0; i < 10000; i++) { __asm__("nop"); }

    ctrl &= ~0x01; // Release reset
    mig_reg_write(iface, MIG_REG_CTRL, ctrl);
}
```

## 6. Device Tree Overlay (Linux)

```dts
// photonweave.dtso — Device Tree Overlay for PhotonWeave PCIe Card
// Applied at boot or via dtoverlay on Raspberry Pi / embedded Linux host

/dts-v1/;
/plugin/;

/ {
    compatible = "nous,photonweave-cgh";
    part-number = "PW-001";
    version = "1.0";

    fragment@0 {
        target = <&pcie>;
        __overlay__ {
            photonweave@0 {
                compatible = "nous,photonweave";
                reg = <0x0000 0 0 0 0>; // Bus 0, Device 0, Function 0

                // BAR0: 64 KB control registers
                assigned-addresses = <0x82000000 0 0 0 0 0x10000>;

                // MSI-X interrupt configuration
                interrupt-parent = <&pcie>;
                interrupts = <0 1 2 3>; // 4 MSI-X vectors
                interrupt-names = "frame_done", "error",
                                  "thermal", "dma_complete";

                // DMA coherent memory pool
                memory-region = <&photonweave_dma_pool>;

                // Clock configuration
                clocks = <&si5345 0>, <&si5345 1>, <&si5345 2>,
                         <&si5345 3>, <&si5345 4>;
                clock-names = "fft_core", "pcie_user", "ddr4_ref",
                              "hdmi_pixel", "axi_interconnect";

                // Thermal zone reference
                thermal-zone = "photonweave-thermal";

                // HDMI output
                hdmi-output {
                    compatible = "nous,photonweave-hdmi";
                    port {
                        photonweave_hdmi_out: endpoint {
                            remote-endpoint = <&slm_input>;
                        };
                    };
                };

                // QSFP+ input
                qsfp-input {
                    compatible = "nous,photonweave-qsfp";
                    port {
                        photonweave_qsfp_in: endpoint {
                            remote-endpoint = <&camera_array_out>;
                        };
                    };
                };
            };
        };
    };

    fragment@1 {
        target = <&reserved_memory>;
        __overlay__ {
            photonweave_dma_pool: photonweave-dma@0x40000000 {
                compatible = "shared-dma-pool";
                reg = <0x0 0x40000000 0x0 0x10000000>; // 256 MB
                no-map;
                linux,cma-default;
            };
        };
    };

    fragment@2 {
        target = <&thermal_zones>;
        __overlay__ {
            photonweave_thermal: photonweave-thermal {
                polling-delay-passive = <1000>; // 1 second
                polling-delay = <5000>;         // 5 seconds
                thermal-sensors = <&photonweave_temp 0>;

                trips {
                    photonweave_alert: trip-alert {
                        temperature = <80000>; // 80°C
                        hysteresis = <2000>;
                        type = "passive";
                    };
                    photonweave_crit: trip-crit {
                        temperature = <95000>; // 95°C
                        hysteresis = <5000>;
                        type = "critical";
                    };
                };

                cooling-maps {
                    map0 {
                        trip = <&photonweave_alert>;
                        cooling-device = <&photonweave_fan 0 1>;
                        contribution = <100>;
                    };
                };
            };
        };
    };
};
```

## 7. Build Instructions

### FPGA Bitstream Build (Vivado)

```bash
# Build PhotonWeave FPGA bitstream
# Requires: Xilinx Vivado 2023.2+, Kintex-7 device support

# 1. Create project
vivado -mode batch -source create_project.tcl

# create_project.tcl:
#   create_project photonweave ./photonweave_vivado -part xc7k325tffg900-2
#   set_property target_language Verilog [current_project]
#   add_files -norecurse {
#     rtl/photonweave_top.v
#     rtl/pcie_gen3_ep.v
#     rtl/axi_interconnect.v
#     rtl/mig_ddr4_4x16.v
#     rtl/cgh_pipeline.v
#     rtl/fft_engine.v
#     rtl/fft_butterfly.v
#     rtl/fresnel_propagator.v
#     rtl/hologram_accumulator.v
#     rtl/phase_quantizer.v
#     rtl/hdmi_tx_gtx.v
#     rtl/hdmi_timing_gen.v
#     rtl/qsfp_interface.v
#     rtl/ft601_fifo_bridge.v
#     rtl/stm32_spi_slave.v
#     rtl/i2c_slave.v
#     rtl/register_file.v
#     rtl/dma_engine.v
#     rtl/thermal_monitor.v
#     rtl/clock_manager.v
#     rtl/watchdog.v
#   }
#   add_files -fileset constrs_1 {
#     constraints/photonweave.xdc
#     constraints/pcie_timing.xdc
#     constraints/ddr4_timing.xdc
#     constraints/hdmi_timing.xdc
#   }
#   launch_runs impl_1 -to_step write_bitstream
#   wait_on_run impl_1

# 2. Generate MIG DDR4 IP
# The MIG is generated as an IP core within Vivado:
#   - 4 controllers, each 16-bit, DDR4-2400
#   - 300 MHz reference clock
#   - AXI4 interface, 64-byte burst
#   - ECC enabled

# 3. Generate PCIe Gen3 x8 Endpoint
#   - Integrated Block for PCI Express (7 Series)
#   - Gen3, x8 lanes, 8 GT/s
#   - AXI4-Stream interface
#   - MSI-X capable (4 vectors)

# 4. Synthesis and Implementation
vivado -mode batch -source build_bitstream.tcl

# build_bitstream.tcl:
#   open_project photonweave.xpr
#   reset_run synth_1
#   launch_runs synth_1 -jobs 8
#   wait_on_run synth_1
#   launch_runs impl_1 -to_step write_bitstream -jobs 8
#   wait_on_run impl_1
#   write_cfgmem -format mcs -size 512 -interface SPIx4 \
#     -loadbit "up 0x0 impl_1/photonweave.bit" \
#     -file photonweave.mcs

# 5. Program flash via STM32
# The .mcs file is loaded onto the MT25QU512 via STM32 USB
```

### STM32 Firmware Build (ARM GCC)

```bash
# Build STM32H743 firmware
# Requires: arm-none-eabi-gcc (12.3+), STM32CubeH7 HAL

cd firmware/

# Build
make -j4

# Output: build/photonweave_stm32.elf, .bin, .hex

# Flash via ST-Link or USB DFU
st-flash write build/photonweave_stm32.bin 0x08000000
# or
dfu-util -a 0 -s 0x08000000:leave -D build/photonweave_stm32.bin
```

### Makefile

```makefile
# firmware/Makefile — PhotonWeave STM32H743 Firmware
# Cross-compile for ARM Cortex-M7

# Toolchain
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

# MCU flags
MCU     = cortex-m7
FPU     = fpv5-sp-d16
FLOAT   = hard
CPUFLAGS = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT)

# Part number
PART    = STM32H743VITx
DEFINES = -D$(PART) -DUSE_HAL_DRIVER

# Directories
BUILD_DIR = build
SRC_DIR   = .
DRV_DIR   = drivers
HAL_DIR   = STM32CubeH7/Drivers

# Sources
C_SOURCES = \
    main.c \
    system_clock.c \
    gpio_init.c \
    si5345_config.c \
    fpga_program.c \
    board_eeprom.c \
    tmp117_sensor.c \
    thermal_manager.c \
    usb_command.c \
    $(DRV_DIR)/ddr4_mig_driver.c \
    $(DRV_DIR)/cgh_pipeline.c \
    $(DRV_DIR)/hdmi_output.c \
    $(DRV_DIR)/qsfp_ingest.c \
    $(DRV_DIR)/dma_engine.c \
    $(DRV_DIR)/error_handler.c \
    $(DRV_DIR)/watchdog.c

# HAL sources (minimal set)
HAL_SOURCES = \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c \
    $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c

# Startup
STARTUP = startup_stm32h743xx.s
SYSTEM  = system_stm32h7xx.c

# Includes
INCLUDES = \
    -I. \
    -Idrivers \
    -I$(HAL_DIR)/CMSIS/Include \
    -I$(HAL_DIR)/CMSIS/Device/ST/STM32H7xx/Include \
    -I$(HAL_DIR)/STM32H7xx_HAL_Driver/Inc

# Flags
CFLAGS  = $(CPUFLAGS) $(DEFINES) $(INCLUDES)
CFLAGS += -Os -Wall -Wextra -Werror
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fno-common -fno-strict-aliasing
CFLAGS += -std=gnu11 -g3 -gdwarf-2

LDFLAGS = $(CPUFLAGS) -T STM32H743VITx_FLASH.ld
LDFLAGS += -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/photonweave_stm32.map
LDFLAGS += -specs=nano.specs -specs=nosys.specs
LDFLAGS += -Wl,--print-memory-usage

# Objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(HAL_SOURCES:.c=.o)))
OBJECTS += $(BUILD_DIR)/startup.o
OBJECTS += $(BUILD_DIR)/system.o

# Targets
.PHONY: all clean flash

all: $(BUILD_DIR)/photonweave_stm32.bin

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(DRV_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(HAL_DIR)/STM32H7xx_HAL_Driver/Src/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/startup.o: $(STARTUP)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPUFLAGS) -c $< -o $@

$(BUILD_DIR)/system.o: $(SYSTEM)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/photonweave_stm32.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@
	$(SIZE) $@

$(BUILD_DIR)/photonweave_stm32.bin: $(BUILD_DIR)/photonweave_stm32.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/photonweave_stm32.hex: $(BUILD_DIR)/photonweave_stm32.elf
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -rf $(BUILD_DIR)

flash: $(BUILD_DIR)/photonweave_stm32.bin
	st-flash write $< 0x08000000
```

### Linux Kernel Driver Build

```bash
# Build PhotonWeave Linux PCIe driver
# Requires: Linux kernel headers, gcc, make

cd driver/
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

# Load driver
sudo insmod photonweave.ko

# Verify
dmesg | tail -20
ls -la /dev/photonweave0
lspci -vvv -s 01:00.0
```

## 8. CGH Pipeline Driver

```c
// cgh_pipeline.h — CGH Compute Pipeline Driver Header

#ifndef CGH_PIPELINE_H
#define CGH_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>

// CGH parameter set
typedef struct {
    uint32_t wavelength_nm_x1000;   // e.g., 532000 for 532nm
    uint32_t depth_min_um;          // Minimum depth in µm
    uint32_t depth_max_um;          // Maximum depth in µm
    uint32_t depth_planes;          // Number of planes (1-256)
    uint32_t input_width;           // Depth map width
    uint32_t input_height;          // Depth map height
    uint32_t output_width;          // Hologram width
    uint32_t output_height;         // Hologram height
    uint32_t fft_size;              // FFT size (power of 2)
    uint32_t propagation_model;     // 0=Fresnel, 1=ASM, 2=Fraunhofer
    uint32_t phase_quantize_bits;   // 8 or 10
    uint32_t color_channel;         // 0=R, 1=G, 2=B
    uint32_t pixel_pitch_um_x100;   // SLM pixel pitch × 100
    uint32_t fill_factor_percent;   // SLM fill factor
} cgh_params_t;

// CGH pipeline status
typedef struct {
    bool     idle;
    bool     busy;
    bool     frame_done;
    bool     error;
    uint32_t error_flags;
    uint32_t frame_count;
    uint32_t frame_time_us;
    uint32_t fps_x1000;
    uint32_t active_engines;
} cgh_status_t;

// Function prototypes
int  cgh_pipeline_init(void);
int  cgh_pipeline_configure(const cgh_params_t *params, uint8_t param_set);
int  cgh_pipeline_start(void);
int  cgh_pipeline_abort(void);
void cgh_pipeline_get_status(cgh_status_t *status);
int  cgh_pipeline_wait_frame_done(uint32_t timeout_ms);
int  cgh_pipeline_submit_depth_map(uint64_t ddr4_addr, uint32_t width,
                                    uint32_t height, uint32_t color_channel);
int  cgh_pipeline_retrieve_hologram(uint64_t *ddr4_addr);
void cgh_pipeline_set_fft_engine_mask(uint8_t mask);
int  cgh_pipeline_soft_reset(void);

#endif // CGH_PIPELINE_H
```

```c
// cgh_pipeline.c — CGH Compute Pipeline Driver Implementation

#include "cgh_pipeline.h"
#include "registers.h"
#include "ddr4_mig_driver.h"

// CGH pipeline register base (in FPGA BAR0)
#define CGH_REG_BASE            0x0040

// Register offsets from CGH_REG_BASE
#define CGH_REG_CTRL            0x00
#define CGH_REG_STATUS          0x04
#define CGH_REG_PARAMS          0x08
#define CGH_REG_WAVELENGTH      0x0C
#define CGH_REG_DEPTH_MIN       0x10
#define CGH_REG_DEPTH_MAX       0x14
#define CGH_REG_DEPTH_PLANES    0x18
#define CGH_REG_INPUT_W         0x1C
#define CGH_REG_INPUT_H         0x20
#define CGH_REG_OUTPUT_W        0x24
#define CGH_REG_OUTPUT_H        0x28
#define CGH_REG_FRAME_COUNT     0x2C
#define CGH_REG_FRAME_TIME      0x30
#define CGH_REG_FPS             0x34
#define CGH_REG_ERROR_FLAGS     0x38
#define CGH_REG_FFT_SCRATCH     0x3C

// Parameter set base (16 sets, 0x40 bytes each)
#define CGH_PARAM_SET_BASE      0x0100

// Control register bits
#define CGH_CTRL_START          0x01
#define CGH_CTRL_ABORT          0x02
#define CGH_CTRL_FFT_MASK_SHIFT 20
#define CGH_CTRL_PARAM_SET_SHIFT 16

// Status register bits
#define CGH_STATUS_IDLE         0x01
#define CGH_STATUS_BUSY         0x02
#define CGH_STATUS_FRAME_DONE   0x04
#define CGH_STATUS_ERROR        0x08
#define CGH_STATUS_FFT_FAULT_SHIFT 16

// Error flags
#define CGH_ERR_DDR4_READ       0x0001
#define CGH_ERR_DDR4_WRITE      0x0002
#define CGH_ERR_FFT_OVERFLOW    0x0004
#define CGH_ERR_FFT_TIMEOUT     0x0008
#define CGH_ERR_PARAM_INVALID   0x0010
#define CGH_ERR_DMA_FAULT       0x0020
#define CGH_ERR_BUFFER_OVERRUN  0x0040
#define CGH_ERR_WATCHDOG        0x0080

// Read a CGH register
static inline uint32_t cgh_reg_read(uint32_t offset)
{
    volatile uint32_t *reg = (volatile uint32_t *)(BAR0_BASE + CGH_REG_BASE + offset);
    return *reg;
}

// Write a CGH register
static inline void cgh_reg_write(uint32_t offset, uint32_t value)
{
    volatile uint32_t *reg = (volatile uint32_t *)(BAR0_BASE + CGH_REG_BASE + offset);
    *reg = value;
}

// Initialize CGH pipeline
int cgh_pipeline_init(void)
{
    // 1. Verify magic number
    if (cgh_reg_read(0x0000 - CGH_REG_BASE) != 0x50484F54) { // PW_REG_MAGIC
        return -1; // Not a PhotonWeave device
    }

    // 2. Check FPGA ready
    uint32_t status = cgh_reg_read(0x0008 - CGH_REG_BASE); // PW_REG_STATUS
    if (!(status & 0x01)) { // FPGA_READY
        return -2;
    }

    // 3. Check DDR4 calibration
    if (!(status & 0x04)) { // DDR4_CAL_DONE
        return -3;
    }

    // 4. Reset CGH pipeline
    cgh_pipeline_soft_reset();

    // 5. Set default parameters
    cgh_params_t default_params = {
        .wavelength_nm_x1000 = 532000,     // 532 nm green
        .depth_min_um = 0,                  // 0 µm
        .depth_max_um = 100000,            // 100 mm
        .depth_planes = 256,               // 256 planes
        .input_width = 2048,
        .input_height = 2048,
        .output_width = 3840,
        .output_height = 2160,
        .fft_size = 4096,
        .propagation_model = 1,            // Angular Spectrum Method
        .phase_quantize_bits = 8,
        .color_channel = 1,                // Green
        .pixel_pitch_um_x100 = 360,        // 3.6 µm
        .fill_factor_percent = 92
    };
    cgh_pipeline_configure(&default_params, 0);

    // 6. Enable all 8 FFT engines
    cgh_pipeline_set_fft_engine_mask(0xFF);

    return 0;
}

// Configure a parameter set
int cgh_pipeline_configure(const cgh_params_t *params, uint8_t param_set)
{
    if (params == NULL || param_set > 15) return -1;

    // Validate parameters
    if (params->depth_planes < 1 || params->depth_planes > 256) return -2;
    if (params->fft_size & (params->fft_size - 1)) return -3; // Must be power of 2
    if (params->fft_size < params->output_width ||
        params->fft_size < params->output_height) return -4;
    if (params->color_channel > 2) return -5;

    // Calculate parameter set base address
    uint32_t set_base = CGH_PARAM_SET_BASE + (param_set * 0x40);

    // Write parameters to register file
    volatile uint32_t *regs = (volatile uint32_t *)(BAR0_BASE + set_base);

    regs[0]  = params->wavelength_nm_x1000;
    regs[1]  = params->depth_min_um;
    regs[2]  = params->depth_max_um;
    regs[3]  = params->depth_planes;
    regs[4]  = params->input_width;
    regs[5]  = params->input_height;
    regs[6]  = params->output_width;
    regs[7]  = params->output_height;
    regs[8]  = params->fft_size;
    regs[9]  = params->propagation_model;
    regs[10] = params->phase_quantize_bits;
    regs[11] = params->color_channel;
    regs[12] = params->pixel_pitch_um_x100;
    regs[13] = params->fill_factor_percent;

    return 0;
}

// Start CGH pipeline processing
int cgh_pipeline_start(void)
{
    // Check if pipeline is idle
    uint32_t status = cgh_reg_read(CGH_REG_STATUS);
    if (status & CGH_STATUS_BUSY) {
        return -1; // Already processing
    }

    // Set start bit
    uint32_t ctrl = cgh_reg_read(CGH_REG_CTRL);
    ctrl |= CGH_CTRL_START;
    cgh_reg_write(CGH_REG_CTRL, ctrl);

    return 0;
}

// Abort current frame processing
int cgh_pipeline_abort(void)
{
    uint32_t ctrl = cgh_reg_read(CGH_REG_CTRL);
    ctrl |= CGH_CTRL_ABORT;
    cgh_reg_write(CGH_REG_CTRL, ctrl);

    // Wait for pipeline to return to idle
    for (int retry = 0; retry < 1000; retry++) {
        uint32_t status = cgh_reg_read(CGH_REG_STATUS);
        if (status & CGH_STATUS_IDLE) return 0;
        for (volatile int i = 0; i < 1000; i++) { __asm__("nop"); }
    }

    return -1; // Timeout waiting for abort
}

// Get pipeline status
void cgh_pipeline_get_status(cgh_status_t *status)
{
    if (status == NULL) return;

    uint32_t reg_status = cgh_reg_read(CGH_REG_STATUS);

    status->idle         = (reg_status & CGH_STATUS_IDLE) != 0;
    status->busy         = (reg_status & CGH_STATUS_BUSY) != 0;
    status->frame_done   = (reg_status & CGH_STATUS_FRAME_DONE) != 0;
    status->error        = (reg_status & CGH_STATUS_ERROR) != 0;
    status->error_flags  = cgh_reg_read(CGH_REG_ERROR_FLAGS);
    status->frame_count  = cgh_reg_read(CGH_REG_FRAME_COUNT);
    status->frame_time_us = cgh_reg_read(CGH_REG_FRAME_TIME);
    status->fps_x1000    = cgh_reg_read(CGH_REG_FPS);
    status->active_engines = (reg_status >> CGH_STATUS_FFT_FAULT_SHIFT) & 0xFF;
}

// Wait for frame completion
int cgh_pipeline_wait_frame_done(uint32_t timeout_ms)
{
    uint32_t elapsed_us = 0;

    while (elapsed_us < (timeout_ms * 1000)) {
        uint32_t status = cgh_reg_read(CGH_REG_STATUS);

        if (status & CGH_STATUS_FRAME_DONE) {
            // Clear frame done bit
            cgh_reg_write(CGH_REG_STATUS, status & ~CGH_STATUS_FRAME_DONE);
            return 0;
        }

        if (status & CGH_STATUS_ERROR) {
            return -1; // Error occurred
        }

        // ~10 µs polling interval
        for (volatile int i = 0; i < 10000; i++) { __asm__("nop"); }
        elapsed_us += 10;
    }

    return -2; // Timeout
}

// Submit depth map for processing
int cgh_pipeline_submit_depth_map(uint64_t ddr4_addr, uint32_t width,
                                   uint32_t height, uint32_t color_channel)
{
    // Validate alignment
    if (ddr4_addr & 0x3F) return -1; // Must be 64-byte aligned

    // Write input parameters
    cgh_reg_write(CGH_REG_INPUT_W, width);
    cgh_reg_write(CGH_REG_INPUT_H, height);

    // Select color channel parameter set
    uint32_t ctrl = cgh_reg_read(CGH_REG_CTRL);
    ctrl &= ~(0x0F << CGH_CTRL_PARAM_SET_SHIFT);
    ctrl |= (color_channel & 0x0F) << CGH_CTRL_PARAM_SET_SHIFT;
    cgh_reg_write(CGH_REG_CTRL, ctrl);

    // Set DMA source to depth map address
    // (DMA engine registers at BAR0 offset 0x0018-0x003C)
    volatile uint32_t *dma_regs = (volatile uint32_t *)(BAR0_BASE + 0x0018);
    dma_regs[2] = (uint32_t)(ddr4_addr & 0xFFFFFFFF);  // DMA_SRC_ADDR_LO
    dma_regs[3] = (uint32_t)(ddr4_addr >> 32);          // DMA_SRC_ADDR_HI
    dma_regs[4] = width * height * 2;                    // DMA_LENGTH (16-bit depth)

    // Start pipeline
    return cgh_pipeline_start();
}

// Retrieve computed hologram address
int cgh_pipeline_retrieve_hologram(uint64_t *ddr4_addr)
{
    if (ddr4_addr == NULL) return -1;

    // Check frame done
    uint32_t status = cgh_reg_read(CGH_REG_STATUS);
    if (!(status & CGH_STATUS_FRAME_DONE)) {
        return -2; // No frame ready
    }

    // Hologram is in DDR4_PART_HOLOGRAM_0 or _1 (ping-pong)
    // Read which buffer is active from scratch register
    uint32_t buf_sel = cgh_reg_read(0x00D4); // PW_REG_SCRATCH_0
    if (buf_sel == 0) {
        *ddr4_addr = DDR4_PART_HOLOGRAM_0;
    } else {
        *ddr4_addr = DDR4_PART_HOLOGRAM_1;
    }

    // Clear frame done
    cgh_reg_write(CGH_REG_STATUS, status & ~CGH_STATUS_FRAME_DONE);

    return 0;
}

// Set active FFT engine mask
void cgh_pipeline_set_fft_engine_mask(uint8_t mask)
{
    uint32_t ctrl = cgh_reg_read(CGH_REG_CTRL);
    ctrl &= ~(0xFF << CGH_CTRL_FFT_MASK_SHIFT);
    ctrl |= (mask & 0xFF) << CGH_CTRL_FFT_MASK_SHIFT;
    cgh_reg_write(CGH_REG_CTRL, ctrl);
}

// Soft reset CGH pipeline
int cgh_pipeline_soft_reset(void)
{
    // Write soft reset via main control register
    volatile uint32_t *ctrl_reg = (volatile uint32_t *)(BAR0_BASE + 0x000C);
    uint32_t ctrl = *ctrl_reg;
    ctrl |= 0x0200; // SOFT_RESET_CGH bit
    *ctrl_reg = ctrl;

    // Wait for reset to complete
    for (volatile int i = 0; i < 100000; i++) { __asm__("nop"); }

    ctrl &= ~0x0200;
    *ctrl_reg = ctrl;

    // Verify pipeline is idle
    uint32_t status = cgh_reg_read(CGH_REG_STATUS);
    return (status & CGH_STATUS_IDLE) ? 0 : -1;
}
```

## 9. Error Handler Driver

```c
// error_handler.h — System Error Handler Header

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

// Error severity levels
typedef enum {
    ERROR_SEV_INFO     = 0,  // Informational only
    ERROR_SEV_WARNING  = 1,  // Recoverable, system continues
    ERROR_SEV_ERROR    = 2,  // Recoverable with intervention
    ERROR_SEV_CRITICAL = 3,  // Unrecoverable, system halt
} error_severity_t;

// Error source modules
typedef enum {
    ERROR_SRC_FPGA     = 0,
    ERROR_SRC_DDR4     = 1,
    ERROR_SRC_CGH      = 2,
    ERROR_SRC_HDMI     = 3,
    ERROR_SRC_QSFP     = 4,
    ERROR_SRC_PCIE     = 5,
    ERROR_SRC_DMA      = 6,
    ERROR_SRC_THERMAL  = 7,
    ERROR_SRC_POWER    = 8,
    ERROR_SRC_CLOCK    = 9,
    ERROR_SRC_STM32    = 10,
    ERROR_SRC_USB      = 11,
} error_source_t;

// Error record
typedef struct {
    uint32_t        timestamp_ms;   // System uptime when error occurred
    error_source_t  source;
    error_severity_t severity;
    uint32_t        code;           // Module-specific error code
    uint32_t        data;           // Additional error data
    bool            active;         // Error still active
} error_record_t;

#define MAX_ERROR_RECORDS 64

// Function prototypes
void error_handler_init(void);
int  error_handler_report(error_source_t source, error_severity_t severity,
                           uint32_t code, uint32_t data);
void error_handler_clear(error_source_t source, uint32_t code);
void error_handler_clear_all(void);
int  error_handler_get_count(void);
int  error_handler_get_record(uint32_t index, error_record_t *record);
bool error_handler_has_critical(void);
void error_handler_dump_all(void);
void error_handler_handle_irq(void);

// Error handler ISR
void Error_Handler_ISR(void);

#endif // ERROR_HANDLER_H
```

```c
// error_handler.c — System Error Handler Implementation

#include "error_handler.h"
#include "registers.h"
#include <string.h>

// Error record ring buffer
static error_record_t error_records[MAX_ERROR_RECORDS];
static uint32_t error_count = 0;
static uint32_t error_write_idx = 0;
static uint32_t system_uptime_ms = 0;

// Error source name strings
static const char *error_source_names[] = {
    "FPGA", "DDR4", "CGH", "HDMI", "QSFP",
    "PCIe", "DMA", "THERMAL", "POWER", "CLOCK",
    "STM32", "USB"
};

// Error severity strings
static const char *error_severity_names[] = {
    "INFO", "WARNING", "ERROR", "CRITICAL"
};

// Initialize error handler
void error_handler_init(void)
{
    memset(error_records, 0, sizeof(error_records));
    error_count = 0;
    error_write_idx = 0;
    system_uptime_ms = 0;

    // Enable error interrupt in FPGA
    volatile uint32_t *int_mask = (volatile uint32_t *)(BAR0_BASE + 0x0010);
    *int_mask |= 0x0002; // Enable error interrupt (bit 1)
}

// Report an error
int error_handler_report(error_source_t source, error_severity_t severity,
                          uint32_t code, uint32_t data)
{
    if (source > ERROR_SRC_USB || severity > ERROR_SEV_CRITICAL) {
        return -1;
    }

    // Check for duplicate (same source + code, still active)
    for (uint32_t i = 0; i < MAX_ERROR_RECORDS; i++) {
        if (error_records[i].active &&
            error_records[i].source == source &&
            error_records[i].code == code) {
            // Update existing record
            error_records[i].timestamp_ms = system_uptime_ms;
            error_records[i].data = data;
            return 0; // Duplicate, updated
        }
    }

    // Write new record
    error_record_t *rec = &error_records[error_write_idx];
    rec->timestamp_ms = system_uptime_ms;
    rec->source = source;
    rec->severity = severity;
    rec->code = code;
    rec->data = data;
    rec->active = true;

    // Advance ring buffer
    error_write_idx = (error_write_idx + 1) % MAX_ERROR_RECORDS;
    if (error_count < MAX_ERROR_RECORDS) {
        error_count++;
    }

    // Critical errors: halt system
    if (severity == ERROR_SEV_CRITICAL) {
        // Set error LED (D4) solid red
        volatile uint32_t *gpio_reg = (volatile uint32_t *)(BAR0_BASE + 0x0200);
        *gpio_reg |= 0x08; // D4 on

        // Disable CGH pipeline
        volatile uint32_t *ctrl_reg = (volatile uint32_t *)(BAR0_BASE + 0x000C);
        *ctrl_reg &= ~0x01; // Clear CGH_START

        // Log to debug UART
        volatile uint32_t *uart_reg = (volatile uint32_t *)(BAR0_BASE + 0x00D0);
        *uart_reg = 0x43524954; // "CRIT" in ASCII
    }

    return 0;
}

// Clear a specific error
void error_handler_clear(error_source_t source, uint32_t code)
{
    for (uint32_t i = 0; i < MAX_ERROR_RECORDS; i++) {
        if (error_records[i].active &&
            error_records[i].source == source &&
            error_records[i].code == code) {
            error_records[i].active = false;
            return;
        }
    }
}

// Clear all errors
void error_handler_clear_all(void)
{
    for (uint32_t i = 0; i < MAX_ERROR_RECORDS; i++) {
        error_records[i].active = false;
    }
    error_count = 0;
    error_write_idx = 0;
}

// Get active error count
int error_handler_get_count(void)
{
    int count = 0;
    for (uint32_t i = 0; i < MAX_ERROR_RECORDS; i++) {
        if (error_records[i].active) count++;
    }
    return count;
}

// Get error record by index
int error_handler_get_record(uint32_t index, error_record_t *record)
{
    if (index >= MAX_ERROR_RECORDS || record == NULL) return -1;

    // Find the Nth active record
    uint32_t active_idx = 0;
    for (uint32_t i = 0; i < MAX_ERROR_RECORDS; i++) {
        if (error_records[i].active) {
            if (active_idx == index) {
                memcpy(record, &error_records[i], sizeof(error_record_t));
                return 0;
            }
            active_idx++;
        }
    }
    return -2; // Index out of range
}

// Check for critical errors
bool error_handler_has_critical(void)
{
    for (uint32_t i = 0; i < MAX_ERROR_RECORDS; i++) {
        if (error_records[i].active &&
            error_records[i].severity == ERROR_SEV_CRITICAL) {
            return true;
        }
    }
    return false;
}

// Dump all errors to debug console
void error_handler_dump_all(void)
{
    // Write header to debug UART
    volatile uint32_t *uart_reg = (volatile uint32_t *)(BAR0_BASE + 0x00D0);

    for (uint32_t i = 0; i < MAX_ERROR_RECORDS; i++) {
        if (error_records[i].active) {
            error_record_t *rec = &error_records[i];

            // Format: [SRC:SEV:CODE] @TIME data=DATA
            // Send via debug UART in chunks
            *uart_reg = (uint32_t)rec->source;
            *uart_reg = (uint32_t)rec->severity;
            *uart_reg = rec->code;
            *uart_reg = rec->timestamp_ms;
            *uart_reg = rec->data;
        }
    }
}

// Handle error interrupt from FPGA
void error_handler_handle_irq(void)
{
    // Read interrupt status register
    volatile uint32_t *int_status = (volatile uint32_t *)(BAR0_BASE + 0x0014);

    if (*int_status & 0x0002) { // Error interrupt bit
        // Read error flags from CGH pipeline
        volatile uint32_t *cgh_error = (volatile uint32_t *)(BAR0_BASE + 0x0078);
        uint32_t flags = *cgh_error;

        if (flags & 0x0001) error_handler_report(ERROR_SRC_DDR4, ERROR_SEV_ERROR, 1, 0);
        if (flags & 0x0002) error_handler_report(ERROR_SRC_DDR4, ERROR_SEV_ERROR, 2, 0);
        if (flags & 0x0004) error_handler_report(ERROR_SRC_CGH, ERROR_SEV_ERROR, 1, 0);
        if (flags & 0x0008) error_handler_report(ERROR_SRC_CGH, ERROR_SEV_ERROR, 2, 0);
        if (flags & 0x0010) error_handler_report(ERROR_SRC_CGH, ERROR_SEV_WARNING, 3, 0);
        if (flags & 0x0020) error_handler_report(ERROR_SRC_DMA, ERROR_SEV_ERROR, 1, 0);
        if (flags & 0x0040) error_handler_report(ERROR_SRC_DDR4, ERROR_SEV_ERROR, 3, 0);
        if (flags & 0x0080) error_handler_report(ERROR_SRC_FPGA, ERROR_SEV_CRITICAL, 1, 0);

        // Clear interrupt
        *int_status = 0x0002;
    }
}

// Error handler ISR (called from interrupt vector)
void Error_Handler_ISR(void)
{
    error_handler_handle_irq();
}
```

## 10. USB Descriptors (for STM32 USB via FT601)

```c
// usb_descriptors.h — USB 3.0 Descriptors for PhotonWeave
// Used by FT601Q FIFO bridge for USB enumeration
// The FT601 presents as a vendor-specific USB 3.0 device

#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <stdint.h>

// USB Vendor ID (Nous Research — would be assigned by USB-IF)
#define USB_VID_NOUS            0x4E4F  // "NO" in ASCII hex
#define USB_PID_PHOTONWEAVE     0x5057  // "PW"

// USB 3.0 Device Descriptor
static const uint8_t usb_device_descriptor[] = {
    0x12,                       // bLength (18 bytes)
    0x01,                       // bDescriptorType (DEVICE)
    0x00, 0x03,                 // bcdUSB (3.0)
    0x00,                       // bDeviceClass (vendor-specific)
    0x00,                       // bDeviceSubClass
    0x00,                       // bDeviceProtocol
    0x09,                       // bMaxPacketSize0 (512 bytes for SuperSpeed)
    USB_VID_NOUS & 0xFF,        // idVendor low
    (USB_VID_NOUS >> 8) & 0xFF, // idVendor high
    USB_PID_PHOTONWEAVE & 0xFF, // idProduct low
    (USB_PID_PHOTONWEAVE >> 8) & 0xFF, // idProduct high
    0x00, 0x01,                 // bcdDevice (1.0)
    0x01,                       // iManufacturer (string index 1)
    0x02,                       // iProduct (string index 2)
    0x03,                       // iSerialNumber (string index 3)
    0x01                        // bNumConfigurations
};

// USB 3.0 Configuration Descriptor
static const uint8_t usb_config_descriptor[] = {
    // Configuration descriptor
    0x09,                       // bLength
    0x02,                       // bDescriptorType (CONFIGURATION)
    0x2C, 0x00,                 // wTotalLength (44 bytes)
    0x01,                       // bNumInterfaces
    0x01,                       // bConfigurationValue
    0x00,                       // iConfiguration
    0x80,                       // bmAttributes (bus-powered)
    0xFA,                       // bMaxPower (500 mA × 2 = 1000 mA)

    // Interface descriptor (Interface 0, Alt 0)
    0x09,                       // bLength
    0x04,                       // bDescriptorType (INTERFACE)
    0x00,                       // bInterfaceNumber
    0x00,                       // bAlternateSetting
    0x02,                       // bNumEndpoints (Bulk IN + Bulk OUT)
    0xFF,                       // bInterfaceClass (vendor-specific)
    0xFF,                       // bInterfaceSubClass
    0xFF,                       // bInterfaceProtocol
    0x00,                       // iInterface

    // Bulk OUT Endpoint Descriptor (Host → Device)
    0x07,                       // bLength
    0x05,                       // bDescriptorType (ENDPOINT)
    0x01,                       // bEndpointAddress (EP1 OUT)
    0x02,                       // bmAttributes (Bulk)
    0x00, 0x04,                 // wMaxPacketSize (1024 bytes)
    0x00,                       // bInterval (ignored for bulk)
    0x08,                       // bMaxBurst (8 bursts × 1024 = 8KB per service interval)

    // Bulk IN Endpoint Descriptor (Device → Host)
    0x07,                       // bLength
    0x05,                       // bDescriptorType (ENDPOINT)
    0x82,                       // bEndpointAddress (EP2 IN)
    0x02,                       // bmAttributes (Bulk)
    0x00, 0x04,                 // wMaxPacketSize (1024 bytes)
    0x00,                       // bInterval
    0x08,                       // bMaxBurst (8 bursts)

    // SuperSpeed Endpoint Companion Descriptor (Bulk OUT)
    0x06,                       // bLength
    0x30,                       // bDescriptorType (SS_ENDPOINT_COMPANION)
    0x00,                       // bMaxBurst (up to 16 packets)
    0x00,                       // bmAttributes (bulk, no streams)
    0x00, 0x00,                 // wBytesPerInterval (0 = no periodic)

    // SuperSpeed Endpoint Companion Descriptor (Bulk IN)
    0x06,                       // bLength
    0x30,                       // bDescriptorType
    0x00,                       // bMaxBurst
    0x00,                       // bmAttributes
    0x00, 0x00                  // wBytesPerInterval
};

// USB 3.0 BOS (Binary Object Store) Descriptor
static const uint8_t usb_bos_descriptor[] = {
    0x05,                       // bLength
    0x0F,                       // bDescriptorType (BOS)
    0x16, 0x00,                 // wTotalLength (22 bytes)
    0x02,                       // bNumDeviceCaps

    // USB 2.0 Extension Device Capability
    0x07,                       // bLength
    0x10,                       // bDescriptorType (DEVICE CAPABILITY)
    0x02,                       // bDevCapabilityType (USB 2.0 EXTENSION)
    0x02, 0x00, 0x00, 0x00,    // bmAttributes (LPM supported)

    // SuperSpeed USB Device Capability
    0x0A,                       // bLength
    0x10,                       // bDescriptorType
    0x03,                       // bDevCapabilityType (SUPERSPEED USB)
    0x00,                       // bmAttributes
    0x00, 0x0E,                 // wSpeedsSupported (Low, Full, High, SuperSpeed)
    0x01,                       // bFunctionalitySupport (SuperSpeed)
    0x0A,                       // bU1DevExitLat (10 µs)
    0x00, 0x00                  // wU2DevExitLat (0 — not supported)
};

// String Descriptors
// String 0: Language ID
static const uint8_t usb_string_langid[] = {
    0x04,                       // bLength
    0x03,                       // bDescriptorType (STRING)
    0x09, 0x04                  // wLANGID (0x0409 = English US)
};

// String 1: Manufacturer
static const uint8_t usb_string_manufacturer[] = {
    0x1C,                       // bLength (28 bytes)
    0x03,                       // bDescriptorType
    'N', 0x00, 'o', 0x00, 'u', 0x00, 's', 0x00,
    ' ', 0x00, 'R', 0x00, 'e', 0x00, 's', 0x00,
    'e', 0x00, 'a', 0x00, 'r', 0x00, 'c', 0x00,
    'h', 0x00
};

// String 2: Product
static const uint8_t usb_string_product[] = {
    0x2A,                       // bLength (42 bytes)
    0x03,                       // bDescriptorType
    'P', 0x00, 'h', 0x00, 'o', 0x00, 't', 0x00,
    'o', 0x00, 'n', 0x00, 'W', 0x00, 'e', 0x00,
    'a', 0x00, 'v', 0x00, 'e', 0x00, ' ', 0x00,
    'C', 0x00, 'G', 0x00, 'H', 0x00, ' ', 0x00,
    'E', 0x00, 'n', 0x00, 'g', 0x00, 'i', 0x00,
    'n', 0x00, 'e', 0x00
};

// String 3: Serial Number (unique per device, from EEPROM)
// This is generated at runtime from the board EEPROM serial
static uint8_t usb_string_serial[26]; // 12 chars + header

#endif // USB_DESCRIPTORS_H
```

## 11. DMA Engine Driver

```c
// dma_engine.h — PCIe DMA Engine Driver Header

#ifndef DMA_ENGINE_H
#define DMA_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

// DMA descriptor structure (8 × 32-bit words = 32 bytes)
typedef struct {
    uint32_t src_addr_lo;       // Source address low 32 bits
    uint32_t src_addr_hi;       // Source address high 32 bits
    uint32_t dst_addr_lo;       // Destination address low 32 bits
    uint32_t dst_addr_hi;       // Destination address high 32 bits
    uint32_t length;            // Transfer length in bytes
    uint32_t control;           // Control flags
    uint32_t next_desc_lo;      // Next descriptor pointer low
    uint32_t next_desc_hi;      // Next descriptor pointer high
} dma_desc_t;

// DMA control flags
#define DMA_CTRL_IRQ_ON_COMPLETE  0x01
#define DMA_CTRL_CHAIN            0x02
#define DMA_CTRL_CRC_ENABLE       0x04
#define DMA_CTRL_DIR_HOST_TO_FPGA 0x08  // 1=Host→FPGA, 0=FPGA→Host
#define DMA_CTRL_LAST_DESC        0x10

// DMA status
typedef struct {
    bool     active;
    bool     error;
    uint32_t bytes_transferred;
    uint32_t descriptors_processed;
    uint32_t current_desc_index;
    uint32_t crc_errors;
} dma_status_t;

// Function prototypes
int  dma_engine_init(void);
int  dma_engine_submit_chain(dma_desc_t *descriptors, uint32_t count);
int  dma_engine_submit_single(uint64_t src, uint64_t dst, uint32_t len,
                               bool host_to_fpga, bool irq_on_complete);
int  dma_engine_wait_completion(uint32_t timeout_ms);
void dma_engine_get_status(dma_status_t *status);
int  dma_engine_abort(void);
void dma_engine_soft_reset(void);

#endif // DMA_ENGINE_H
```

```c
// dma_engine.c — PCIe DMA Engine Driver Implementation

#include "dma_engine.h"
#include "registers.h"
#include "ddr4_mig_driver.h"
#include <string.h>

// DMA register offsets (from BAR0)
#define DMA_REG_CTRL            0x0018
#define DMA_REG_STATUS          0x001C
#define DMA_REG_SRC_ADDR_LO     0x0020
#define DMA_REG_SRC_ADDR_HI     0x0024
#define DMA_REG_DST_ADDR_LO     0x0028
#define DMA_REG_DST_ADDR_HI     0x002C
#define DMA_REG_LENGTH          0x0030
#define DMA_REG_DESC_ADDR_LO    0x0034
#define DMA_REG_DESC_ADDR_HI    0x0038
#define DMA_REG_DESC_COUNT      0x003C
#define DMA_REG_BYTES_DONE      0x0040
#define DMA_REG_DESC_DONE       0x0044
#define DMA_REG_CRC_ERRORS      0x0048

// DMA control bits
#define DMA_CTRL_START           0x01
#define DMA_CTRL_ABORT           0x02
#define DMA_CTRL_CHAIN_MODE      0x04
#define DMA_CTRL_IRQ_ENABLE     0x08
#define DMA_CTRL_CRC_ENABLE      0x10
#define DMA_CTRL_DIRECTION       0x20  // 1=Host→FPGA

// DMA status bits
#define DMA_STATUS_ACTIVE        0x01
#define DMA_STATUS_COMPLETE      0x02
#define DMA_STATUS_ERROR         0x04
#define DMA_STATUS_CHAIN_ACTIVE  0x08

// Read DMA register
static inline uint32_t dma_reg_read(uint32_t offset)
{
    volatile uint32_t *reg = (volatile uint32_t *)(BAR0_BASE + offset);
    return *reg;
}

// Write DMA register
static inline void dma_reg_write(uint32_t offset, uint32_t value)
{
    volatile uint32_t *reg = (volatile uint32_t *)(BAR0_BASE + offset);
    *reg = value;
}

// Initialize DMA engine
int dma_engine_init(void)
{
    // Reset DMA engine
    dma_engine_soft_reset();

    // Verify reset complete
    uint32_t status = dma_reg_read(DMA_REG_STATUS);
    if (status & DMA_STATUS_ACTIVE) {
        return -1; // Reset failed
    }

    // Enable CRC checking by default
    uint32_t ctrl = dma_reg_read(DMA_REG_CTRL);
    ctrl |= DMA_CTRL_CRC_ENABLE;
    dma_reg_write(DMA_REG_CTRL, ctrl);

    return 0;
}

// Submit a descriptor chain
int dma_engine_submit_chain(dma_desc_t *descriptors, uint32_t count)
{
    if (descriptors == NULL || count == 0 || count > 256) return -1;

    // Check DMA idle
    uint32_t status = dma_reg_read(DMA_REG_STATUS);
    if (status & DMA_STATUS_ACTIVE) return -2;

    // Descriptors must be in FPGA-accessible memory (DDR4 or BAR aperture)
    // For this implementation, descriptors are in DDR4 descriptor region
    uint64_t desc_phys_addr = DDR4_PART_DESC;

    // Copy descriptors to DDR4
    int result = ddr4_mig_axi_burst_write(0, desc_phys_addr, descriptors,
                                           count * sizeof(dma_desc_t) / 64);
    if (result != 0) return -3;

    // Configure DMA for chain mode
    dma_reg_write(DMA_REG_DESC_ADDR_LO, (uint32_t)(desc_phys_addr & 0xFFFFFFFF));
    dma_reg_write(DMA_REG_DESC_ADDR_HI, (uint32_t)(desc_phys_addr >> 32));
    dma_reg_write(DMA_REG_DESC_COUNT, count);

    // Set chain mode and start
    uint32_t ctrl = dma_reg_read(DMA_REG_CTRL);
    ctrl |= DMA_CTRL_CHAIN_MODE | DMA_CTRL_START;
    dma_reg_write(DMA_REG_CTRL, ctrl);

    return 0;
}

// Submit a single DMA transfer
int dma_engine_submit_single(uint64_t src, uint64_t dst, uint32_t len,
                               bool host_to_fpga, bool irq_on_complete)
{
    if (len == 0 || len > (16 * 1024 * 1024)) return -1; // Max 16 MB single transfer
    if (src & 0x3 || dst & 0x3) return -2; // 4-byte aligned

    // Check DMA idle
    uint32_t status = dma_reg_read(DMA_REG_STATUS);
    if (status & DMA_STATUS_ACTIVE) return -3;

    // Configure transfer
    dma_reg_write(DMA_REG_SRC_ADDR_LO, (uint32_t)(src & 0xFFFFFFFF));
    dma_reg_write(DMA_REG_SRC_ADDR_HI, (uint32_t)(src >> 32));
    dma_reg_write(DMA_REG_DST_ADDR_LO, (uint32_t)(dst & 0xFFFFFFFF));
    dma_reg_write(DMA_REG_DST_ADDR_HI, (uint32_t)(dst >> 32));
    dma_reg_write(DMA_REG_LENGTH, len);

    // Set control: direction, IRQ, start
    uint32_t ctrl = DMA_CTRL_START;
    if (host_to_fpga) ctrl |= DMA_CTRL_DIRECTION;
    if (irq_on_complete) ctrl |= DMA_CTRL_IRQ_ENABLE;
    ctrl |= DMA_CTRL_CRC_ENABLE; // Always CRC-check

    // Clear chain mode
    ctrl &= ~DMA_CTRL_CHAIN_MODE;

    dma_reg_write(DMA_REG_CTRL, ctrl);

    return 0;
}

// Wait for DMA completion
int dma_engine_wait_completion(uint32_t timeout_ms)
{
    uint32_t elapsed_us = 0;

    while (elapsed_us < (timeout_ms * 1000)) {
        uint32_t status = dma_reg_read(DMA_REG_STATUS);

        if (status & DMA_STATUS_COMPLETE) {
            return 0; // Done
        }

        if (status & DMA_STATUS_ERROR) {
            return -1; // Error
        }

        for (volatile int i = 0; i < 10000; i++) { __asm__("nop"); }
        elapsed_us += 10;
    }

    return -2; // Timeout
}

// Get DMA status
void dma_engine_get_status(dma_status_t *status)
{
    if (status == NULL) return;

    uint32_t reg = dma_reg_read(DMA_REG_STATUS);

    status->active               = (reg & DMA_STATUS_ACTIVE) != 0;
    status->error                = (reg & DMA_STATUS_ERROR) != 0;
    status->bytes_transferred    = dma_reg_read(DMA_REG_BYTES_DONE);
    status->descriptors_processed = dma_reg_read(DMA_REG_DESC_DONE);
    status->current_desc_index   = dma_reg_read(DMA_REG_DESC_DONE);
    status->crc_errors           = dma_reg_read(DMA_REG_CRC_ERRORS);
}

// Abort DMA transfer
int dma_engine_abort(void)
{
    uint32_t ctrl = dma_reg_read(DMA_REG_CTRL);
    ctrl |= DMA_CTRL_ABORT;
    dma_reg_write(DMA_REG_CTRL, ctrl);

    // Wait for abort to complete
    for (int retry = 0; retry < 1000; retry++) {
        uint32_t status = dma_reg_read(DMA_REG_STATUS);
        if (!(status & DMA_STATUS_ACTIVE)) return 0;
        for (volatile int i = 0; i < 1000; i++) { __asm__("nop"); }
    }

    return -1;
}

// Soft reset DMA engine
void dma_engine_soft_reset(void)
{
    // Use main control register soft reset bit
    volatile uint32_t *main_ctrl = (volatile uint32_t *)(BAR0_BASE + 0x000C);
    uint32_t ctrl = *main_ctrl;
    ctrl |= 0x0400; // SOFT_RESET_DMA bit
    *main_ctrl = ctrl;

    for (volatile int i = 0; i < 100000; i++) { __asm__("nop"); }

    ctrl &= ~0x0400;
    *main_ctrl = ctrl;
}
```

## 12. Watchdog Driver

```c
// watchdog.h — FPGA Watchdog Timer Driver Header

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdint.h>
#include <stdbool.h>

// Function prototypes
int  watchdog_init(uint32_t timeout_ms);
void watchdog_kick(void);
void watchdog_disable(void);
bool watchdog_has_expired(void);
uint32_t watchdog_get_remaining_ms(void);

#endif // WATCHDOG_H
```

```c
// watchdog.c — FPGA Watchdog Timer Driver Implementation

#include "watchdog.h"
#include "registers.h"

// Watchdog register offsets (from BAR0)
#define WDT_REG_KICK        0x00F8
#define WDT_REG_TIMEOUT     0x00FC
#define WDT_REG_REMAINING   0x0100

// Initialize watchdog with timeout
int watchdog_init(uint32_t timeout_ms)
{
    if (timeout_ms == 0) {
        watchdog_disable();
        return 0;
    }

    if (timeout_ms < 10 || timeout_ms > 60000) {
        return -1; // Range: 10ms to 60s
    }

    // Set timeout
    volatile uint32_t *timeout_reg = (volatile uint32_t *)(BAR0_BASE + WDT_REG_TIMEOUT);
    *timeout_reg = timeout_ms;

    // Enable watchdog in main control register
    volatile uint32_t *ctrl_reg = (volatile uint32_t *)(BAR0_BASE + 0x000C);
    *ctrl_reg |= 0x0100; // WATCHDOG_ENABLE bit

    // Initial kick
    watchdog_kick();

    return 0;
}

// Kick the watchdog (reset timer)
void watchdog_kick(void)
{
    volatile uint32_t *kick_reg = (volatile uint32_t *)(BAR0_BASE + WDT_REG_KICK);
    *kick_reg = 0xDEADBEEF; // Any write resets the timer
}

// Disable watchdog
void watchdog_disable(void)
{
    volatile uint32_t *ctrl_reg = (volatile uint32_t *)(BAR0_BASE + 0x000C);
    *ctrl_reg &= ~0x0100; // Clear WATCHDOG_ENABLE
}

// Check if watchdog has expired
bool watchdog_has_expired(void)
{
    volatile uint32_t *status_reg = (volatile uint32_t *)(BAR0_BASE + 0x0008);
    return (*status_reg & 0x4000) != 0; // WATCHDOG_EXPIRED bit
}

// Get remaining time before watchdog fires
uint32_t watchdog_get_remaining_ms(void)
{
    volatile uint32_t *remaining_reg = (volatile uint32_t *)(BAR0_BASE + WDT_REG_REMAINING);
    return *remaining_reg;
}
```

## 13. HDMI Output Driver

```c
// hdmi_output.h — HDMI 2.0 Output Driver Header

#ifndef HDMI_OUTPUT_H
#define HDMI_OUTPUT_H

#include <stdint.h>
#include <stdbool.h>

// HDMI timing parameters for 4K60 (3840×2160 @ 60 Hz)
// CTA-861-G VIC 97: 3840×2160, 60 Hz, 16:9
#define HDMI_4K60_H_ACTIVE      3840
#define HDMI_4K60_H_FRONT_PORCH 176
#define HDMI_4K60_H_SYNC        88
#define HDMI_4K60_H_BACK_PORCH  296
#define HDMI_4K60_H_TOTAL       4400

#define HDMI_4K60_V_ACTIVE      2160
#define HDMI_4K60_V_FRONT_PORCH 8
#define HDMI_4K60_V_SYNC        10
#define HDMI_4K60_V_BACK_PORCH  72
#define HDMI_4K60_V_TOTAL       2250

#define HDMI_4K60_PIXEL_CLK_HZ  594000000  // 594 MHz (4400×2250×60)

// HDMI status
typedef struct {
    bool     active;
    bool     hpd_asserted;       // Hot plug detect
    bool     tmds_locked;        // TMDS PLL locked
    uint32_t frame_count;
    uint32_t underrun_count;     // Buffer underrun errors
    uint32_t current_line;
} hdmi_status_t;

// Function prototypes
int  hdmi_output_init(void);
int  hdmi_output_start(uint64_t frame_buffer_addr);
int  hdmi_output_stop(void);
void hdmi_output_get_status(hdmi_status_t *status);
int  hdmi_output_set_timing(uint32_t h_active, uint32_t h_fp,
                              uint32_t h_sync, uint32_t h_bp,
                              uint32_t v_active, uint32_t v_fp,
                              uint32_t v_sync, uint32_t v_bp);
bool hdmi_output_is_connected(void);
void hdmi_output_soft_reset(void);

#endif // HDMI_OUTPUT_H
```

```c
// hdmi_output.c — HDMI 2.0 Output Driver Implementation

#include "hdmi_output.h"
#include "registers.h"

// HDMI register offsets (from BAR0)
#define HDMI_REG_CTRL           0x0080
#define HDMI_REG_STATUS         0x0084
#define HDMI_REG_FRAME_ADDR_LO  0x0088
#define HDMI_REG_FRAME_ADDR_HI  0x008C
#define HDMI_REG_TIMING_H       0x0090
#define HDMI_REG_TIMING_V       0x0094
#define HDMI_REG_FRAME_COUNT    0x0098
#define HDMI_REG_UNDERRUN       0x009C
#define HDMI_REG_CURRENT_LINE   0x00A0

// HDMI control bits
#define HDMI_CTRL_ENABLE        0x01
#define HDMI_CTRL_GRAYSCALE     0x02  // 1=8-bit grayscale, 0=24-bit RGB
#define HDMI_CTRL_DITHER        0x04  // Enable temporal dithering
#define HDMI_CTRL_INVERT        0x08  // Invert output (for some SLMs)

// HDMI status bits
#define HDMI_STATUS_ACTIVE      0x01
#define HDMI_STATUS_HPD         0x02
#define HDMI_STATUS_TMDS_LOCK   0x04
#define HDMI_STATUS_UNDERRUN    0x08

// Read HDMI register
static inline uint32_t hdmi_reg_read(uint32_t offset)
{
    volatile uint32_t *reg = (volatile uint32_t *)(BAR0_BASE + offset);
    return *reg;
}

// Write HDMI register
static inline void hdmi_reg_write(uint32_t offset, uint32_t value)
{
    volatile uint32_t *reg = (volatile uint32_t *)(BAR0_BASE + offset);
    *reg = value;
}

// Initialize HDMI output
int hdmi_output_init(void)
{
    // Reset HDMI output
    hdmi_output_soft_reset();

    // Set default 4K60 timing
    hdmi_output_set_timing(
        HDMI_4K60_H_ACTIVE, HDMI_4K60_H_FRONT_PORCH,
        HDMI_4K60_H_SYNC, HDMI_4K60_H_BACK_PORCH,
        HDMI_4K60_V_ACTIVE, HDMI_4K60_V_FRONT_PORCH,
        HDMI_4K60_V_SYNC, HDMI_4K60_V_BACK_PORCH
    );

    // Configure for grayscale output (SLM is phase-only, 8-bit)
    uint32_t ctrl = HDMI_CTRL_GRAYSCALE | HDMI_CTRL_DITHER;
    hdmi_reg_write(HDMI_REG_CTRL, ctrl);

    return 0;
}

// Start HDMI output streaming
int hdmi_output_start(uint64_t frame_buffer_addr)
{
    if (frame_buffer_addr & 0x3F) return -1; // 64-byte aligned

    // Set frame buffer address
    hdmi_reg_write(HDMI_REG_FRAME_ADDR_LO, (uint32_t)(frame_buffer_addr & 0xFFFFFFFF));
    hdmi_reg_write(HDMI_REG_FRAME_ADDR_HI, (uint32_t)(frame_buffer_addr >> 32));

    // Enable output
    uint32_t ctrl = hdmi_reg_read(HDMI_REG_CTRL);
    ctrl |= HDMI_CTRL_ENABLE;
    hdmi_reg_write(HDMI_REG_CTRL, ctrl);

    // Also enable in main control register
    volatile uint32_t *main_ctrl = (volatile uint32_t *)(BAR0_BASE + 0x000C);
    *main_ctrl |= 0x04; // HDMI_ENABLE

    return 0;
}

// Stop HDMI output
int hdmi_output_stop(void)
{
    uint32_t ctrl = hdmi_reg_read(HDMI_REG_CTRL);
    ctrl &= ~HDMI_CTRL_ENABLE;
    hdmi_reg_write(HDMI_REG_CTRL, ctrl);

    volatile uint32_t *main_ctrl = (volatile uint32_t *)(BAR0_BASE + 0x000C);
    *main_ctrl &= ~0x04;

    return 0;
}

// Get HDMI status
void hdmi_output_get_status(hdmi_status_t *status)
{
    if (status == NULL) return;

    uint32_t reg = hdmi_reg_read(HDMI_REG_STATUS);

    status->active         = (reg & HDMI_STATUS_ACTIVE) != 0;
    status->hpd_asserted   = (reg & HDMI_STATUS_HPD) != 0;
    status->tmds_locked    = (reg & HDMI_STATUS_TMDS_LOCK) != 0;
    status->frame_count    = hdmi_reg_read(HDMI_REG_FRAME_COUNT);
    status->underrun_count = hdmi_reg_read(HDMI_REG_UNDERRUN);
    status->current_line   = hdmi_reg_read(HDMI_REG_CURRENT_LINE);
}

// Set custom timing parameters
int hdmi_output_set_timing(uint32_t h_active, uint32_t h_fp,
                            uint32_t h_sync, uint32_t h_bp,
                            uint32_t v_active, uint32_t v_fp,
                            uint32_t v_sync, uint32_t v_bp)
{
    // Validate parameters
    if (h_active == 0 || h_active > 4096) return -1;
    if (v_active == 0 || v_active > 4096) return -2;
    if (h_sync < 8 || h_sync > 256) return -3;
    if (v_sync < 2 || v_sync > 64) return -4;

    // Pack horizontal timing: [11:0]=HActive, [23:12]=HFP, [31:24]=HSync
    // HBP is computed as HTotal - HActive - HFP - HSync
    uint32_t h_total = h_active + h_fp + h_sync + h_bp;
    uint32_t h_timing = (h_active & 0xFFF) |
                        ((h_fp & 0xFFF) << 12) |
                        ((h_sync & 0xFF) << 24);
    hdmi_reg_write(HDMI_REG_TIMING_H, h_timing);

    // Pack vertical timing: [11:0]=VActive, [23:12]=VFP, [31:24]=VSync
    uint32_t v_timing = (v_active & 0xFFF) |
                        ((v_fp & 0xFFF) << 12) |
                        ((v_sync & 0xFF) << 24);
    hdmi_reg_write(HDMI_REG_TIMING_V, v_timing);

    return 0;
}

// Check if HDMI sink is connected
bool hdmi_output_is_connected(void)
{
    uint32_t status = hdmi_reg_read(HDMI_REG_STATUS);
    return (status & HDMI_STATUS_HPD) != 0;
}

// Soft reset HDMI output
void hdmi_output_soft_reset(void)
{
    volatile uint32_t *main_ctrl = (volatile uint32_t *)(BAR0_BASE + 0x000C);
    uint32_t ctrl = *main_ctrl;
    ctrl |= 0x0800; // SOFT_RESET_HDMI bit
    *main_ctrl = ctrl;

    for (volatile int i = 0; i < 100000; i++) { __asm__("nop"); }

    ctrl &= ~0x0800;
    *main_ctrl = ctrl;
}
```

## 14. QSFP+ Ingest Driver

```c
// qsfp_ingest.h — QSFP+ Direct Camera Ingest Driver Header

#ifndef QSFP_INGEST_H
#define QSFP_INGEST_H

#include <stdint.h>
#include <stdbool.h>

// QSFP+ status
typedef struct {
    bool     module_present;
    bool     link_up;
    bool     rx_active;
    uint32_t rx_frame_count;
    uint32_t rx_byte_count;
    uint32_t rx_error_count;
    uint32_t link_speed_mbps;   // Negotiated link speed
} qsfp_status_t;

// Function prototypes
int  qsfp_ingest_init(void);
int  qsfp_ingest_enable(void);
int  qsfp_ingest_disable(void);
void qsfp_ingest_get_status(qsfp_status_t *status);
int  qsfp_ingest_set_rx_buffer(uint64_t ddr4_addr);
bool qsfp_ingest_is_module_present(void);
int  qsfp_ingest_read_module_eeprom(uint8_t page, uint8_t *data, uint8_t len);
void qsfp_ingest_soft_reset(void);

#endif // QSFP_INGEST_H
```

```c
// qsfp_ingest.c — QSFP+ Direct Camera Ingest Driver Implementation

#include "qsfp_ingest.h"
#include "registers.h"

// QSFP+ register offsets (from BAR0)
#define QSFP_REG_CTRL           0x0094
#define QSFP_REG_STATUS         0x0098
#define QSFP_REG_RX_FRAME_LO    0x009C
#define QSFP_REG_RX_FRAME_HI    0x00A0
#define QSFP_REG_RX_COUNT       0x00A4
#define QSFP_REG_RX_BYTES_LO    0x00A8
#define QSFP_REG_RX_BYTES_HI    0x00AC
#define QSFP_REG_RX_ERRORS      0x00B0
#define QSFP_REG_LINK_SPEED     0x00B4

// QSFP+ control bits
#define QSFP_CTRL_ENABLE        0x01
#define QSFP_CTRL_LPMODE        0x02  // Low power mode
#define QSFP_CTRL_RESET         0x04  // Module reset

// QSFP+ status bits
#define QSFP_STATUS_MOD_PRES    0x01
#define QSFP_STATUS_LINK_UP     0x02
#define QSFP_STATUS_RX_ACTIVE   0x04
#define QSFP_STATUS_RX_ERROR    0x08
#define QSFP_STATUS_INTL        0x10  // Module interrupt

// Read QSFP register
static inline uint32_t qsfp_reg_read(uint32_t offset)
{
    volatile uint32_t *reg = (volatile uint32_t *)(BAR0_BASE + offset);
    return *reg;
}

// Write QSFP register
static inline void qsfp_reg_write(uint32_t offset, uint32_t value)
{
    volatile uint32_t *reg = (volatile uint32_t *)(BAR0_BASE + offset);
    *reg = value;
}

// Initialize QSFP+ interface
int qsfp_ingest_init(void)
{
    // Reset QSFP+ interface
    qsfp_ingest_soft_reset();

    // Check for module presence
    if (!qsfp_ingest_is_module_present()) {
        return -1; // No module installed
    }

    // Take module out of low-power mode
    uint32_t ctrl = qsfp_reg_read(QSFP_REG_CTRL);
    ctrl &= ~QSFP_CTRL_LPMODE;
    qsfp_reg_write(QSFP_REG_CTRL, ctrl);

    // Wait for module to initialize (300 ms per SFF-8679)
    for (volatile int i = 0; i < 300000; i++) { __asm__("nop"); }

    return 0;
}

// Enable QSFP+ receive
int qsfp_ingest_enable(void)
{
    if (!qsfp_ingest_is_module_present()) return -1;

    uint32_t ctrl = qsfp_reg_read(QSFP_REG_CTRL);
    ctrl |= QSFP_CTRL_ENABLE;
    qsfp_reg_write(QSFP_REG_CTRL, ctrl);

    // Enable in main control register
    volatile uint32_t *main_ctrl = (volatile uint32_t *)(BAR0_BASE + 0x000C);
    *main_ctrl |= 0x08; // QSFP_ENABLE

    return 0;
}

// Disable QSFP+ receive
int qsfp_ingest_disable(void)
{
    uint32_t ctrl = qsfp_reg_read(QSFP_REG_CTRL);
    ctrl &= ~QSFP_CTRL_ENABLE;
    qsfp_reg_write(QSFP_REG_CTRL, ctrl);

    volatile uint32_t *main_ctrl = (volatile uint32_t *)(BAR0_BASE + 0x000C);
    *main_ctrl &= ~0x08;

    return 0;
}

// Get QSFP+ status
void qsfp_ingest_get_status(qsfp_status_t *status)
{
    if (status == NULL) return;

    uint32_t reg = qsfp_reg_read(QSFP_REG_STATUS);

    status->module_present  = (reg & QSFP_STATUS_MOD_PRES) != 0;
    status->link_up         = (reg & QSFP_STATUS_LINK_UP) != 0;
    status->rx_active       = (reg & QSFP_STATUS_RX_ACTIVE) != 0;
    status->rx_frame_count  = qsfp_reg_read(QSFP_REG_RX_COUNT);
    status->rx_byte_count   = qsfp_reg_read(QSFP_REG_RX_BYTES_LO) |
                             ((uint64_t)qsfp_reg_read(QSFP_REG_RX_BYTES_HI) << 32);
    status->rx_error_count  = qsfp_reg_read(QSFP_REG_RX_ERRORS);
    status->link_speed_mbps = qsfp_reg_read(QSFP_REG_LINK_SPEED);
}

// Set receive buffer address in DDR4
int qsfp_ingest_set_rx_buffer(uint64_t ddr4_addr)
{
    if (ddr4_addr & 0x3F) return -1; // 64-byte aligned

    qsfp_reg_write(QSFP_REG_RX_FRAME_LO, (uint32_t)(ddr4_addr & 0xFFFFFFFF));
    qsfp_reg_write(QSFP_REG_RX_FRAME_HI, (uint32_t)(ddr4_addr >> 32));

    return 0;
}

// Check if QSFP+ module is present
bool qsfp_ingest_is_module_present(void)
{
    uint32_t status = qsfp_reg_read(QSFP_REG_STATUS);
    return (status & QSFP_STATUS_MOD_PRES) != 0;
}

// Read QSFP+ module EEPROM (via I2C in FPGA fabric)
int qsfp_ingest_read_module_eeprom(uint8_t page, uint8_t *data, uint8_t len)
{
    if (data == NULL || len == 0 || len > 128) return -1;
    if (page > 3) return -2;

    // QSFP+ EEPROM is accessed via I2C at address 0x50
    // Page select at byte 127
    // This is handled by the FPGA I2C master to the QSFP+ module

    // For this driver, we use a simplified register-based access
    volatile uint32_t *eeprom_reg = (volatile uint32_t *)(BAR0_BASE + 0x0200);

    // Write page select
    *eeprom_reg = (page << 24) | (len << 16) | 0x01; // Command: read page

    // Wait for completion
    for (int retry = 0; retry < 100; retry++) {
        if ((*eeprom_reg & 0x80000000) == 0) break; // Done bit cleared
        for (volatile int i = 0; i < 1000; i++) { __asm__("nop"); }
    }

    // Read data (4 bytes at a time from 32-bit register)
    for (uint8_t i = 0; i < len; i += 4) {
        uint32_t word = *eeprom_reg;
        uint8_t remaining = (len - i) < 4 ? (len - i) : 4;
        for (uint8_t j = 0; j < remaining; j++) {
            data[i + j] = (word >> (j * 8)) & 0xFF;
        }
    }

    return 0;
}

// Soft reset QSFP+ interface
void qsfp_ingest_soft_reset(void)
{
    // Reset via module reset bit
    uint32_t ctrl = qsfp_reg_read(QSFP_REG_CTRL);
    ctrl |= QSFP_CTRL_RESET;
    qsfp_reg_write(QSFP_REG_CTRL, ctrl);

    for (volatile int i = 0; i < 100000; i++) { __asm__("nop"); }

    ctrl &= ~QSFP_CTRL_RESET;
    qsfp_reg_write(QSFP_REG_CTRL, ctrl);
}
```
