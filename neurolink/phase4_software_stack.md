# NeuroLink — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Boot Sequence

```
Power-On Reset
    │
    ├─ STM32H743: Boot from Flash (0x08000000)
    │   ├─ Option bytes: BOOT_ADD0 = 0x08000000
    │   └─ Dual-bank flash: Bank1 = application, Bank2 = recovery
    │
    ├─ Stage 1: SPL (Secondary Program Loader) — 0x08000000
    │   ├─ Initialize clocks: HSE 27MHz → PLL1 (480 MHz SYSCLK)
    │   ├─ Configure MPU (Memory Protection Unit)
    │   ├─ Enable FMC (DDR3L controller)
    │   ├─ Configure AXI SRAM, DTCM, ITCM
    │   ├─ Initialize I2C2 → BQ25895 (check power status)
    │   ├─ Initialize SPI3 → Load iCE40 bitstream from W25Q128
    │   ├─ Initialize QSPI → Memory-mapped mode for firmware extensions
    │   ├─ Jump to Stage 2
    │   └─ (If boot fails 3×, enter DFU mode on USB)
    │
    ├─ Stage 2: Main Application — 0x08004000
    │   ├─ Initialize all peripherals (SPI1, SPI2, I2C1, USART1, USB HS)
    │   ├─ Configure DMA channels (MDMA for ADS1299, BDMA for USB)
    │   ├─ Boot ADS1299 chain (reset → config registers → start)
    │   ├─ Boot nRF52840 (UART AT command: "AT+START")
    │   ├─ Start RTOS scheduler (FreeRTOS)
    │   └─ Enter normal operation
    │
    └─ Stage 3: OTA Update (optional)
        ├─ Receive firmware image via BLE or USB
        ├─ Write to Flash Bank2
        ├─ Verify CRC32
        └─ Swap banks, reboot
```

### 1.2 FPGA Bitstream Loading

```
STM32H743:
  1. Assert FPGA_RESET_N low (PB0)
  2. Wait 10 ms
  3. Configure SPI3 in QSPI mode for W25Q128
  4. Read bitstream from 0x00400000 (16 MB flash, bitstream at offset 0x400000)
  5. Switch to SPI2 master mode
  6. De-assert FPGA_RESET_N
  7. Stream bitstream via SPI2 to iCE40 (MOSI only, 30 MHz)
  8. Poll FPGA_CDONE (PB1) — should go high within 100 ms
  9. Send 49 dummy clocks (SPI2) to complete config
  10. Verify: read FPGA ID register via SPI2 command
```

### 1.3 ADS1299 Initialization

```
For each ADS1299 in daisy chain (U4 through U11):
  1. Assert ADS_RESET_N low for 18 µs, then high
  2. Wait 2 ms for internal reference to settle
  3. Write CONFIG1: 0x96 (daisy-chain mode, 250 SPS, internal clock)
  4. Write CONFIG2: 0xC0 (internal reference, 2.4V bias)
  5. Write CONFIG3: 0xEC (bias measurement enabled, BIASREF=internal)
  6. For each channel (1–8):
     - Write CHnSET: 0x60 (gain=6, normal electrode input)
  7. Send SDATAC command (stop continuous read)
  8. Send RDATAC command (enable continuous read mode)
  9. Assert ADS_START high to begin conversions
```

---

## 2. Clock Configuration Code

### 2.1 System Clock Tree

```
HSE (27 MHz crystal)
    │
    ├─ PLL1:
    │   ├─ M = 27 (div → 1 MHz)
    │   ├─ N = 480 (mul → 480 MHz)
    │   ├─ P = 1 → SYSCLK = 480 MHz
    │   ├─ Q = 20 → PLL1_Q = 24 MHz (USB PHY)
    │   └─ R = 2  → PLL1_R = 240 MHz (FMC / DDR3L)
    │
    ├─ PLL2:
    │   ├─ M = 27
    │   ├─ N = 400
    │   ├─ P = 8 → PLL2_P = 50 MHz (SDMMC)
    │   └─ Q = 2  → PLL2_Q = 200 MHz (SPI1/SPI2 kernel)
    │
    ├─ PLL3:
    │   ├─ M = 27
    │   ├─ N = 344
    │   └─ P = 4 → PLL3_P = 86 MHz (I2S, not used here)
    │
    ├─ LSE (32.768 kHz) → RTC
    │
    └─ MCO1 (PA8) = HSE/1 = 27 MHz → ADS1299 CLK (via divider: /13.18 ≈ 2.048 MHz)
        (Actually: use PLL2_Q/97 ≈ 2.062 MHz, or configure a TIM PWM output at 2.048 MHz)
```

### 2.2 Clock Configuration (C code)

```c
// In main.c — clock_init()

#define RCC_BASE_ADDR   0x40023800
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE_ADDR + 0x00))
#define RCC_PLLCKSELR   (*(volatile uint32_t *)(RCC_BASE_ADDR + 0x28))
#define RCC_PLLCFGR     (*(volatile uint32_t *)(RCC_BASE_ADDR + 0x2C))
#define RCC_PLL1DIVR    (*(volatile uint32_t *)(RCC_BASE_ADDR + 0x30))
#define RCC_PLL2DIVR    (*(volatile uint32_t *)(RCC_BASE_ADDR + 0x34))
#define RCC_PLL3DIVR    (*(volatile uint32_t *)(RCC_BASE_ADDR + 0x38))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE_ADDR + 0x10))
#define RCC_D1CFGR      (*(volatile uint32_t *)(RCC_BASE_ADDR + 0x18))
#define RCC_D2CFGR      (*(volatile uint32_t *)(RCC_BASE_ADDR + 0x1C))
#define RCC_D3CFGR      (*(volatile uint32_t *)(RCC_BASE_ADDR + 0x20))

void clock_init(void) {
    // Enable HSE (27 MHz)
    RCC_CR |= (1 << 16);  // HSEON
    while (!(RCC_CR & (1 << 17)));  // Wait for HSERDY

    // Configure PLL1: 27 MHz → 480 MHz SYSCLK
    // M=27, N=480, P=1, Q=20, R=2
    RCC_PLLCKSELR = (27 << 0) | (1 << 4) | (0 << 6); // PLL1 src=HSE, M=27
    RCC_PLLCFGR |= (1 << 24);  // PLL1 VCO range 128-560 MHz
    RCC_PLL1DIVR = (479 << 0) | (0 << 9) | (19 << 16) | (1 << 24);
    // N=480 (reg value 479), P=1 (reg 0), Q=20 (reg 19), R=2 (reg 1)

    // Configure PLL2: 27 MHz → 200 MHz for SPI
    // M=27, N=400, P=8, Q=2
    RCC_PLLCKSELR |= (27 << 8) | (1 << 12);  // PLL2 M=27, src=HSE
    RCC_PLLCFGR |= (1 << 26);  // PLL2 VCO range
    RCC_PLL2DIVR = (399 << 0) | (7 << 9) | (1 << 16);

    // Enable PLL1 and PLL2
    RCC_CR |= (1 << 24) | (1 << 26);  // PLL1ON, PLL2ON
    while (!(RCC_CR & (1 << 25)));  // Wait PLL1RDY
    while (!(RCC_CR & (1 << 27)));  // Wait PLL2RDY

    // Configure bus dividers
    // SYSCLK = 480 MHz, AHB = 240 MHz, APB1 = 120 MHz, APB2 = 120 MHz
    RCC_D1CFGR = (0 << 0) | (1 << 4) | (1 << 8);  // HPRE=/2, D1PPRE=/2
    RCC_D2CFGR = (1 << 0) | (1 << 8);  // D1PPRE=/2, D2PPRE=/2
    RCC_D3CFGR = (1 << 0);  // D3PPRE=/2

    // Switch system clock to PLL1
    RCC_CFGR = (3 << 0);  // SW=PLL1
    while (((RCC_CFGR >> 3) & 3) != 3);  // Wait for SWS=PLL1

    // Configure MCO1 output (PA8) = 2.048 MHz for ADS1299
    // TIM1 CH1 PWM output configured separately in timer_init()
}
```

---

## 3. MMIO Register Definitions

```c
// registers.h — Memory-Mapped I/O Register Map

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

// Base addresses
#define PERIPH_BASE         0x40000000U
#define APB1PERIPH_BASE      PERIPH_BASE
#define APB2PERIPH_BASE      (PERIPH_BASE + 0x00010000U)
#define AHB1PERIPH_BASE      (PERIPH_BASE + 0x00020000U)
#define AHB4PERIPH_BASE      (PERIPH_BASE + 0x58000000U)

// ---- GPIO ----
#define GPIOA_BASE           (AHB4PERIPH_BASE + 0x0000U)
#define GPIOB_BASE           (AHB4PERIPH_BASE + 0x0400U)
#define GPIOC_BASE           (AHB4PERIPH_BASE + 0x0800U)
#define GPIOD_BASE           (AHB4PERIPH_BASE + 0x0C00U)
#define GPIOE_BASE           (AHB4PERIPH_BASE + 0x1000U)
#define GPIOF_BASE           (AHB4PERIPH_BASE + 0x1400U)
#define GPIOG_BASE           (AHB4PERIPH_BASE + 0x1800U)
#define GPIOH_BASE           (AHB4PERIPH_BASE + 0x1C00U)

#define GPIO_MODER(gpio)     (*(volatile uint32_t *)((gpio) + 0x00U))
#define GPIO_OTYPER(gpio)    (*(volatile uint32_t *)((gpio) + 0x04U))
#define GPIO_OSPEEDR(gpio)   (*(volatile uint32_t *)((gpio) + 0x08U))
#define GPIO_PUPDR(gpio)     (*(volatile uint32_t *)((gpio) + 0x0CU))
#define GPIO_IDR(gpio)       (*(volatile uint32_t *)((gpio) + 0x10U))
#define GPIO_ODR(gpio)       (*(volatile uint32_t *)((gpio) + 0x14U))
#define GPIO_BSRR(gpio)      (*(volatile uint32_t *)((gpio) + 0x18U))
#define GPIO_AFRL(gpio)      (*(volatile uint32_t *)((gpio) + 0x20U))
#define GPIO_AFRH(gpio)      (*(volatile uint32_t *)((gpio) + 0x24U))

// ---- SPI1 (ADS1299) ----
#define SPI1_BASE            (APB2PERIPH_BASE + 0x3000U)
#define SPI1_CR1             (*(volatile uint32_t *)(SPI1_BASE + 0x00U))
#define SPI1_CR2             (*(volatile uint32_t *)(SPI1_BASE + 0x04U))
#define SPI1_SR              (*(volatile uint32_t *)(SPI1_BASE + 0x08U))
#define SPI1_DR              (*(volatile uint32_t *)(SPI1_BASE + 0x0CU))
#define SPI1_CRCPR           (*(volatile uint32_t *)(SPI1_BASE + 0x10U))
#define SPI1_RXCRCR          (*(volatile uint32_t *)(SPI1_BASE + 0x14U))
#define SPI1_TXCRCR          (*(volatile uint32_t *)(SPI1_BASE + 0x18U))

// ---- SPI2 (iCE40 FPGA) ----
#define SPI2_BASE            (APB1PERIPH_BASE + 0x3800U)
#define SPI2_CR1             (*(volatile uint32_t *)(SPI2_BASE + 0x00U))
#define SPI2_CR2             (*(volatile uint32_t *)(SPI2_BASE + 0x04U))
#define SPI2_SR              (*(volatile uint32_t *)(SPI2_BASE + 0x08U))
#define SPI2_DR              (*(volatile uint32_t *)(SPI2_BASE + 0x0CU))

// ---- SPI3 / QSPI (W25Q128) ----
#define OCTOSPI1_BASE        0x52000000U
#define OCTOSPI1_CR          (*(volatile uint32_t *)(OCTOSPI1_BASE + 0x00U))
#define OCTOSPI1_DCR1        (*(volatile uint32_t *)(OCTOSPI1_BASE + 0x04U))
#define OCTOSPI1_DCR2        (*(volatile uint32_t *)(OCTOSPI1_BASE + 0x08U))
#define OCTOSPI1_SR          (*(volatile uint32_t *)(OCTOSPI1_BASE + 0x20U))
#define OCTOSPI1_FCR         (*(volatile uint32_t *)(OCTOSPI1_BASE + 0x24U))
#define OCTOSPI1_TCR         (*(volatile uint32_t *)(OCTOSPI1_BASE + 0x28U))
#define OCTOSPI1_IR          (*(volatile uint32_t *)(OCTOSPI1_BASE + 0x30U))
#define OCTOSPI1_AR          (*(volatile uint32_t *)(OCTOSPI1_BASE + 0x34U))
#define OCTOSPI1_DR          (*(volatile uint32_t *)(OCTOSPI1_BASE + 0x38U))

// ---- I2C1 (IMU) ----
#define I2C1_BASE            (APB1PERIPH_BASE + 0x5400U)
#define I2C1_CR1             (*(volatile uint32_t *)(I2C1_BASE + 0x00U))
#define I2C1_CR2             (*(volatile uint32_t *)(I2C1_BASE + 0x04U))
#define I2C1_SR1             (*(volatile uint32_t *)(I2C1_BASE + 0x14U))
#define I2C1_SR2             (*(volatile uint32_t *)(I2C1_BASE + 0x18U))
#define I2C1_TXDR            (*(volatile uint32_t *)(I2C1_BASE + 0x28U))
#define I2C1_RXDR            (*(volatile uint32_t *)(I2C1_BASE + 0x2CU))
#define I2C1_ISR             (*(volatile uint32_t *)(I2C1_BASE + 0x04U))

// ---- I2C2 (PMIC + Temp) ----
#define I2C2_BASE            (APB1PERIPH_BASE + 0x5800U)
#define I2C2_CR1             (*(volatile uint32_t *)(I2C2_BASE + 0x00U))
#define I2C2_CR2             (*(volatile uint32_t *)(I2C2_BASE + 0x04U))
#define I2C2_ISR             (*(volatile uint32_t *)(I2C2_BASE + 0x04U))
#define I2C2_TXDR            (*(volatile uint32_t *)(I2C2_BASE + 0x28U))
#define I2C2_RXDR            (*(volatile uint32_t *)(I2C2_BASE + 0x2CU))

// ---- USART1 (BLE) ----
#define USART1_BASE          (APB2PERIPH_BASE + 0x1000U)
#define USART1_CR1           (*(volatile uint32_t *)(USART1_BASE + 0x00U))
#define USART1_CR2           (*(volatile uint32_t *)(USART1_BASE + 0x04U))
#define USART1_CR3           (*(volatile uint32_t *)(USART1_BASE + 0x08U))
#define USART1_BRR           (*(volatile uint32_t *)(USART1_BASE + 0x0CU))
#define USART1_ISR           (*(volatile uint32_t *)(USART1_BASE + 0x1CU))
#define USART1_TDR           (*(volatile uint32_t *)(USART1_BASE + 0x28U))
#define USART1_RDR           (*(volatile uint32_t *)(USART1_BASE + 0x24U))

// ---- USB OTG HS ----
#define USB_OTG_HS_BASE      0x40040000U
#define USB_OTG_HS_GUSBCFG   (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x00CU))
#define USB_OTG_HS_GINTSTS   (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x018U))
#define USB_OTG_HS_GINTMSK   (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x01CU))
#define USB_OTG_HS_DCTL      (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x804U))
#define USB_OTG_HS_DIEPCTL0  (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0x900U))
#define USB_OTG_HS_DOEPCTL0  (*(volatile uint32_t *)(USB_OTG_HS_BASE + 0xB00U))

// ---- DMA / MDMA ----
#define MDMA_BASE            0x50080000U
#define MDMA_CISR(x)         (*(volatile uint32_t *)(MDMA_BASE + 0x0000U + (x)*0x40U))
#define MDMA_CCR(x)          (*(volatile uint32_t *)(MDMA_BASE + 0x0010U + (x)*0x40U))
#define MDMA_CNDTR(x)        (*(volatile uint32_t *)(MDMA_BASE + 0x0014U + (x)*0x40U))
#define MDMA_CSAR(x)         (*(volatile uint32_t *)(MDMA_BASE + 0x0018U + (x)*0x40U))
#define MDMA_CDAR(x)         (*(volatile uint32_t *)(MDMA_BASE + 0x001CU + (x)*0x40U))
#define MDMA_CTCR(x)         (*(volatile uint32_t *)(MDMA_BASE + 0x0008U + (x)*0x40U))
#define MDMA_CBRUR(x)        (*(volatile uint32_t *)(MDMA_BASE + 0x0020U + (x)*0x40U))

// ---- FMC (DDR3L Controller) ----
#define FMC_BASE             0xA0000000U
#define FMC_BCR1             (*(volatile uint32_t *)(FMC_BASE + 0x0000U))
#define FMC_BTR1             (*(volatile uint32_t *)(FMC_BASE + 0x0004U))
#define FMC_BWTR1            (*(volatile uint32_t *)(FMC_BASE + 0x0104U))
#define FMC_SDCR1            (*(volatile uint32_t *)(FMC_BASE + 0x0140U))
#define FMC_SDCR2            (*(volatile uint32_t *)(FMC_BASE + 0x0144U))
#define FMC_SDRTR            (*(volatile uint32_t *)(FMC_BASE + 0x0150U))

// ---- Memory-mapped regions ----
#define AXI_SRAM_BASE        0x24000000U
#define SRAM1_BASE           0x30000000U
#define SRAM2_BASE           0x30020000U
#define SRAM4_BASE           0x38000000U
#define DDR3L_BASE           0xC0000000U
#define QSPI_FLASH_BASE      0x90000000U
#define FLASH_BASE           0x08000000U

// ---- External device register maps ----

// ADS1299 Register Map (accessed via SPI command)
#define ADS1299_REG_ID         0x00
#define ADS1299_REG_CONFIG1    0x01
#define ADS1299_REG_CONFIG2    0x02
#define ADS1299_REG_CONFIG3    0x03
#define ADS1299_REG_LOFF       0x04
#define ADS1299_REG_CH1SET     0x05
#define ADS1299_REG_CH2SET     0x06
#define ADS1299_REG_CH3SET     0x07
#define ADS1299_REG_CH4SET     0x08
#define ADS1299_REG_CH5SET     0x09
#define ADS1299_REG_CH6SET     0x0A
#define ADS1299_REG_CH7SET     0x0B
#define ADS1299_REG_CH8SET     0x0C
#define ADS1299_REG_BIAS_SENSP 0x0D
#define ADS1299_REG_BIAS_SENSN 0x0E
#define ADS1299_REG_LOFF_SENSP 0x0F
#define ADS1299_REG_LOFF_SENSN 0x10
#define ADS1299_REG_LOFF_FLIP  0x11
#define ADS1299_REG_LOFF_STATP 0x12
#define ADS1299_REG_LOFF_STATN 0x13
#define ADS1299_REG_GPIO       0x14
#define ADS1299_REG_MISC1      0x15
#define ADS1299_REG_MISC2      0x16
#define ADS1299_REG_CONFIG4    0x17

// ADS1299 SPI Commands
#define ADS1299_CMD_WAKEUP     0x02
#define ADS1299_CMD_STANDBY    0x04
#define ADS1299_CMD_RESET      0x06
#define ADS1299_CMD_START      0x08
#define ADS1299_CMD_STOP       0x0A
#define ADS1299_CMD_RDATAC     0x10
#define ADS1299_CMD_SDATAC     0x11
#define ADS1299_CMD_RDATA      0x12
#define ADS1299_CMD_RREG       0x20
#define ADS1299_CMD_WREG       0x40

// LSM6DSOX Register Map
#define LSM6DSOX_WHO_AM_I      0x0F
#define LSM6DSOX_CTRL1_XL      0x10
#define LSM6DSOX_CTRL2_G       0x11
#define LSM6DSOX_CTRL3_C       0x12
#define LSM6DSOX_CTRL4_C       0x13
#define LSM6DSOX_STATUS_REG    0x1E
#define LSM6DSOX_OUTX_L_G      0x22
#define LSM6DSOX_OUTX_L_XL     0x28

// BQ25895 Register Map
#define BQ25895_REG_00         0x00  // ILLIM_VINDPM
#define BQ25895_REG_03         0x03  // WD_RST / CHG_CONFIG
#define BQ25895_REG_04         0x04  // ICHG
#define BQ25895_REG_05         0x05  // IPRECHG / ITERM
#define BQ25895_REG_0A         0x0A  // VREG
#define BQ25895_REG_0B         0x0B  // VINDPM
#define BQ25895_REG_0C         0x0C  // TERM_ENABLE / ICOEN
#define BQ25895_REG_0D         0x0D  // FORCE_DPDM / TMR2X_EN
#define BQ25895_REG_0E         0x0E  // CHG_STATUS
#define BQ25895_REG_0F         0x0F  // FAULT_STATUS

// nRF52840 UART Command Protocol
#define BLE_CMD_START           0x01
#define BLE_CMD_STOP            0x02
#define BLE_CMD_SET_ADV         0x03
#define BLE_CMD_CONN_PARAM      0x04
#define BLE_CMD_SEND_DATA       0x05
#define BLE_CMD_GET_STATUS      0x06
#define BLE_CMD_SET_TX_POWER    0x07
#define BLE_CMD_DISCONNECT      0x08

#endif // REGISTERS_H