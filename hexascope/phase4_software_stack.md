# HexaScope — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Boot Sequence

```
Power-On → FUSB302 PD Negotiation (15V) → DA9063 Power Rails Sequenced
    → STM32G474 Reset Deasserted → STM32 Boot from Internal Flash
    → STM32 Initializes: Clocks, GPIO, I²C, SPI, UART
    → STM32 Configures DA9063 (I²C) → Power Rails Stable
    → STM32 Loads FPGA Bitstream from W25Q256 via SPI
    → STM32 Deasserts FPGA PROG_N → FPGA Configures
    → STM32 Waits for FPGA DONE → FPGA Active
    → STM32 Configures FPGA Registers via SPI (trigger, decimation, channels)
    → STM32 Initializes ESP32-C6 via UART (Wi-Fi AP/STA mode)
    → STM32 Initializes USB3340 via ULPI → USB Device Ready
    → HexaScope Operational — Awaiting Host Commands
```

### 1.2 Clock Configuration (STM32G474)

```c
// Clock tree: HSE 25 MHz → PLL (×8 /2) = 100 MHz SYSCLK, /1 = 100 MHz AHB
//              PLLQ (/4) = 50 MHz USB clock (via internal FS)
//              MCO1 = 48 MHz for USB3340

#define HSE_VALUE        25000000U   // 25 MHz crystal
#define SYSCLK_FREQ      170000000U  // 170 MHz (max for STM32G4)
#define AHB_PRESCALER    1           // 170 MHz AHB
#define APB1_PRESCALER   1           // 170 MHz APB1
#define APB2_PRESCALER   1           // 170 MHz APB2

// PLL configuration for 170 MHz from 25 MHz HSE:
// PLLM = 5, PLLN = 68, PLLP = 2, PLLQ = 8
// VCO = 25/5 * 68 = 340 MHz
// PLL_P = 340/2 = 170 MHz (SYSCLK)
// PLL_Q = 340/8 = 42.5 MHz (USB FS) — use HSI48 for USB instead
```

### 1.3 STM32 Startup Code (SystemInit)

```c
void SystemInit(void) {
    // Enable power clock and configure voltage
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    PWR->CR1 |= PWR_CR1_VOS_0;  // Voltage scale 1 (1.2V) for 170 MHz

    // Enable HSE
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // Configure PLL: HSE 25 MHz -> 170 MHz
    // PLLM=5, PLLN=68, PLLP=2, PLLQ=8, PLLR=2
    RCC->PLLCFGR = (5 << RCC_PLLCFGR_PLLM_Pos)     |
                   (68 << RCC_PLLCFGR_PLLN_Pos)    |
                   (0 << RCC_PLLCFGR_PLLP_Pos)     |  // PLLP=2
                   (RCC_PLLCFGR_PLLQ_0)            |  // PLLQ=8... actually:
                   (RCC_PLLCFGR_PLLSRC_HSE)        |
                   (0 << RCC_PLLCFGR_PLLR_Pos);      // PLLR=2

    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // Enable PLLR output (SYSCLK source)
    RCC->CR |= RCC_CR_PLLRDY;

    // Flash latency for 170 MHz
    FLASH->ACR = FLASH_ACR_LATENCY_5WS | FLASH_ACR_PRFTBE;

    // Switch SYSCLK to PLL
    RCC->CFGR = (RCC_CFGR_SW_PLL | RCC_CFGR_HPRE_DIV1 |
                 RCC_CFGR_PPRE1_DIV1 | RCC_CFGR_PPRE2_DIV1);
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}
```

---

## 2. MMIO Register Definitions

### 2.1 STM32G474 Peripheral Register Map

```c
// ============================================================
// registers.h — HexaScope MMIO Register Definitions
// ============================================================

#ifndef HEXASCOPE_REGISTERS_H
#define HEXASCOPE_REGISTERS_H

#include <stdint.h>

// ------------------------------------------------------------
// STM32G474 Peripheral Base Addresses (from STM32 RM0440)
// ------------------------------------------------------------
#define PERIPH_BASE          0x40000000U
#define APB1_PERIPH_BASE     PERIPH_BASE
#define APB2_PERIPH_BASE     (PERIPH_BASE + 0x00010000U)
#define AHB_PERIPH_BASE      (PERIPH_BASE + 0x00020000U)

// GPIO
#define GPIOA_BASE           (AHB_PERIPH_BASE + 0x0000U)
#define GPIOB_BASE           (AHB_PERIPH_BASE + 0x0400U)
#define GPIOC_BASE           (AHB_PERIPH_BASE + 0x0800U)
#define GPIOD_BASE           (AHB_PERIPH_BASE + 0x0C00U)

// RCC
#define RCC_BASE             (AHB_PERIPH_BASE + 0x2400U)

// SPI
#define SPI1_BASE            (APB2_PERIPH_BASE + 0x3000U)
#define SPI2_BASE            (APB1_PERIPH_BASE + 0x3800U)
#define SPI4_BASE            (APB2_PERIPH_BASE + 0x5000U)

// I2C
#define I2C1_BASE            (APB1_PERIPH_BASE + 0x5400U)
#define I2C2_BASE            (APB1_PERIPH_BASE + 0x5800U)

// USART
#define USART1_BASE          (APB2_PERIPH_BASE + 0x3800U)
#define USART3_BASE          (APB1_PERIPH_BASE + 0x4800U)

// USB (internal FS)
#define USB_BASE             (APB1_PERIPH_BASE + 0x6000U)

// DMA
#define DMA1_BASE            (AHB_PERIPH_BASE + 0x6000U)

// ADC (internal)
#define ADC12_BASE           (AHB_PERIPH_BASE + 0x4200U)

// TIM
#define TIM2_BASE            (APB1_PERIPH_BASE + 0x0000U)
#define TIM3_BASE            (APB1_PERIPH_BASE + 0x0400U)

// ------------------------------------------------------------
// RCC Register Offsets
// ------------------------------------------------------------
#define RCC_CR               (*(volatile uint32_t *)(RCC_BASE + 0x00U))
#define RCC_CFGR             (*(volatile uint32_t *)(RCC_BASE + 0x04U))
#define RCC_PLLCFGR          (*(volatile uint32_t *)(RCC_BASE + 0x08U))
#define RCC_AHBENR           (*(volatile uint32_t *)(RCC_BASE + 0x34U))
#define RCC_APB1ENR1         (*(volatile uint32_t *)(RCC_BASE + 0x40U))
#define RCC_APB1ENR2         (*(volatile uint32_t *)(RCC_BASE + 0x44U))
#define RCC_APB2ENR          (*(volatile uint32_t *)(RCC_BASE + 0x48U))

// ------------------------------------------------------------
// GPIO Register Offsets (each GPIO port)
// ------------------------------------------------------------
#define GPIOx_MODER(b)       (*(volatile uint32_t *)((b) + 0x00U))
#define GPIOx_OTYPER(b)      (*(volatile uint32_t *)((b) + 0x04U))
#define GPIOx_OSPEEDR(b)     (*(volatile uint32_t *)((b) + 0x08U))
#define GPIOx_PUPDR(b)       (*(volatile uint32_t *)((b) + 0x0CU))
#define GPIOx_IDR(b)         (*(volatile uint32_t *)((b) + 0x10U))
#define GPIOx_ODR(b)         (*(volatile uint32_t *)((b) + 0x14U))
#define GPIOx_BSRR(b)        (*(volatile uint32_t *)((b) + 0x18U))
#define GPIOx_AFRL(b)        (*(volatile uint32_t *)((b) + 0x20U))
#define GPIOx_AFRH(b)        (*(volatile uint32_t *)((b) + 0x24U))

// ------------------------------------------------------------
// SPI Register Offsets
// ------------------------------------------------------------
#define SPIx_CR1(b)          (*(volatile uint32_t *)((b) + 0x00U))
#define SPIx_CR2(b)          (*(volatile uint32_t *)((b) + 0x04U))
#define SPIx_SR(b)           (*(volatile uint32_t *)((b) + 0x08U))
#define SPIx_DR(b)           (*(volatile uint32_t *)((b) + 0x0CU))

// ------------------------------------------------------------
// I2C Register Offsets
// ------------------------------------------------------------
#define I2Cx_CR1(b)          (*(volatile uint32_t *)((b) + 0x00U))
#define I2Cx_CR2(b)          (*(volatile uint32_t *)((b) + 0x04U))
#define I2Cx_ISR(b)          (*(volatile uint32_t *)((b) + 0x18U))
#define I2Cx_TXDR(b)         (*(volatile uint32_t *)((b) + 0x28U))
#define I2Cx_RXDR(b)         (*(volatile uint32_t *)((b) + 0x24U))

// ------------------------------------------------------------
// USART Register Offsets
// ------------------------------------------------------------
#define USARTx_CR1(b)        (*(volatile uint32_t *)((b) + 0x00U))
#define USARTx_CR2(b)        (*(volatile uint32_t *)((b) + 0x04U))
#define USARTx_CR3(b)        (*(volatile uint32_t *)((b) + 0x08U))
#define USARTx_BRR(b)        (*(volatile uint32_t *)((b) + 0x0CU))
#define USARTx_ISR(b)        (*(volatile uint32_t *)((b) + 0x1CU))
#define USARTx_TDR(b)        (*(volatile uint32_t *)((b) + 0x20U))
#define USARTx_RDR(b)        (*(volatile uint32_t *)((b) + 0x24U))

// ------------------------------------------------------------
// FPGA Register Space (accessed via SPI4 from STM32)
// Address map matches Phase 1 block diagram
// ------------------------------------------------------------
#define FPGA_REG_BASE        0x00000000U

// Trigger Engine registers (offset 0x0000)
#define FPGA_TRIG_TYPE       (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0000U))
#define FPGA_TRIG_THRESH_CH1 (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0004U))
#define FPGA_TRIG_THRESH_CH2 (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0008U))
#define FPGA_TRIG_THRESH_CH3 (*(volatile uint32_t *)(FPGA_REG_BASE + 0x000CU))
#define FPGA_TRIG_THRESH_CH4 (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0010U))
#define FPGA_TRIG_HYST       (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0014U))
#define FPGA_TRIG_PULSE_MIN  (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0018U))
#define FPGA_TRIG_PULSE_MAX  (*(volatile uint32_t *)(FPGA_REG_BASE + 0x001CU))
#define FPGA_TRIG_TIMEOUT    (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0020U))
#define FPGA_TRIG_LOGIC_MASK (*(volatile uint32_t *)(FPGA_REG_BASE + 0x0024U))

// Decimator registers (offset 0x10000)
#define FPGA_DEC_RATE        (*(volatile uint32_t *)(FPGA_REG_BASE + 0x10000U))
#define FPGA_DEC_MODE        (*(volatile uint32_t *)(FPGA_REG_BASE + 0x10004U))
#define FPGA_DEC_AVG_EN      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x10008U))

// Protocol Decoder registers (offset 0x20000)
#define FPGA_PROTO_SEL       (*(volatile uint32_t *)(FPGA_REG_BASE + 0x20000U))
#define FPGA_PROTO_BAUD      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x20004U))
#define FPGA_PROTO_CONFIG    (*(volatile uint32_t *)(FPGA_REG_BASE + 0x20008U))

// Channel MUX registers (offset 0x30000)
#define FPGA_MUX_CONFIG      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x30000U))
#define FPGA_MUX_ANA_EN      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x30004U))
#define FPGA_MUX_DIG_EN      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x30008U))

// DAC Control registers (offset 0x40000)
#define FPGA_DAC_THRESH_D1_P (*(volatile uint32_t *)(FPGA_REG_BASE + 0x40000U))
#define FPGA_DAC_THRESH_D1_N (*(volatile uint32_t *)(FPGA_REG_BASE + 0x40004U))
#define FPGA_DAC_THRESH_D2_P (*(volatile uint32_t *)(FPGA_REG_BASE + 0x40008U))
#define FPGA_DAC_THRESH_D2_N (*(volatile uint32_t *)(FPGA_REG_BASE + 0x4000CU))

// Calibration registers (offset 0x50000)
#define FPGA_CAL_GAIN_CH1    (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50000U))
#define FPGA_CAL_OFFSET_CH1  (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50004U))
#define FPGA_CAL_GAIN_CH2    (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50008U))
#define FPGA_CAL_OFFSET_CH2  (*(volatile uint32_t *)(FPGA_REG_BASE + 0x5000CU))
#define FPGA_CAL_GAIN_CH3    (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50010U))
#define FPGA_CAL_OFFSET_CH3  (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50014U))
#define FPGA_CAL_GAIN_CH4    (*(volatile uint32_t *)(FPGA_REG_BASE + 0x50018U))
#define FPGA_CAL_OFFSET_CH4  (*(volatile uint32_t *)(FPGA_REG_BASE + 0x5001CU))

// Status registers (offset 0x60000)
#define FPGA_STATUS          (*(volatile uint32_t *)(FPGA_REG_BASE + 0x60000U))
#define FPGA_FIFO_LEVEL      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x60004U))
#define FPGA_SAMPLE_COUNT    (*(volatile uint32_t *)(FPGA_REG_BASE + 0x60008U))

// Status register bit definitions
#define FPGA_STATUS_ARMED     (1 << 0)
#define FPGA_STATUS_TRIGGERED (1 << 1)
#define FPGA_STATUS_DONE      (1 << 2)
#define FPGA_STATUS_OVERFLOW  (1 << 3)
#define FPGA_STATUS_USB_RDY  (1 << 4)
#define FPGA_STATUS_WIFI_RDY (1 << 5)

// USB DMA registers (offset 0x70000)
#define FPGA_USB_DMA_LEN     (*(volatile uint32_t *)(FPGA_REG_BASE + 0x70000U))
#define FPGA_USB_DMA_EP      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x70004U))
#define FPGA_USB_DMA_START   (*(volatile uint32_t *)(FPGA_REG_BASE + 0x70008U))

// Wi-Fi DMA registers (offset 0x80000)
#define FPGA_WIFI_DMA_LEN    (*(volatile uint32_t *)(FPGA_REG_BASE + 0x80000U))
#define FPGA_WIFI_BAUD      (*(volatile uint32_t *)(FPGA_REG_BASE + 0x80004U))
#define FPGA_WIFI_DMA_START (*(volatile uint32_t *)(FPGA_REG_BASE + 0x80008U))

// ------------------------------------------------------------
// DA9063 PMIC Register Map (I2C address 0x58)
// ------------------------------------------------------------
#define DA9063_I2C_ADDR      0x58U

#define DA9063_REG_PAGE_CON   0x00U
#define DA9063_REG_STATUS_A   0x01U
#define DA9063_REG_STATUS_B   0x02U
#define DA9063_REG_STATUS_C   0x03U
#define DA9063_REG_STATUS_D   0x04U
#define DA9063_REG_FAULT_LOG  0x05U
#define DA9063_REG_EVENT_A    0x06U
#define DA9063_REG_EVENT_B    0x07U
#define DA9063_REG_EVENT_C    0x08U
#define DA9063_REG_EVENT_D    0x09U
#define DA9063_REG_IRQ_MASK_A 0x0AU
#define DA9063_REG_IRQ_MASK_B 0x0BU
#define DA9063_REG_IRQ_MASK_C 0x0CU
#define DA9063_REG_IRQ_MASK_D 0x0DU
#define DA9063_REG_CONTROL_A   0x0EU
#define DA9063_REG_CONTROL_B   0x0FU
#define DA9063_REG_CONTROL_C   0x10U
#define DA9063_REG_CONTROL_D   0x11U
#define DA9063_REG_CONTROL_E   0x12U
#define DA9063_REG_CONTROL_F   0x13U
#define DA9063_REG_BUCK1_CONT  0x20U  // VDD_CORE (1.1V)
#define DA9063_REG_BUCK2_CONT  0x21U  // VDD_DDR (1.35V)
#define DA9063_REG_BUCK3_CONT  0x22U  // VDD_IO18 (1.8V)
#define DA9063_REG_LDO1_CONT   0x30U  // VDD_IO33 (3.3V)
#define DA9063_REG_LDO2_CONT   0x31U  // VDD_ANA (5.0V)
#define DA9063_REG_LDO3_CONT   0x32U  // VDD_RTC (3.3V)

// ------------------------------------------------------------
// DAC60508 Register Map (SPI, address 0x0C for DAC)
// ------------------------------------------------------------
#define DAC60508_REG_NOOP      0x00U
#define DAC60508_REG_DEVICE_ID 0x01U
#define DAC60508_REG_SYNC      0x02U
#define DAC60508_REG_CONFIG    0x03U
#define DAC60508_REG_GAIN      0x04U
#define DAC60508_REG_TRIGGER   0x05U
#define DAC60508_REG_STATUS    0x06U
#define DAC60508_REG_DAC_CH0   0x08U
#define DAC60508_REG_DAC_CH1   0x09U
#define DAC60508_REG_DAC_CH2   0x0AU
#define DAC60508_REG_DAC_CH3   0x0BU
#define DAC60508_REG_DAC_CH4   0x0CU
#define DAC60508_REG_DAC_CH5   0x0DU
#define DAC60508_REG_DAC_CH6   0x0EU
#define DAC60508_REG_DAC_CH7   0x0FU

// ------------------------------------------------------------
// ADS61B49 ADC Register Map (accessed via FPGA, not directly)
// ADC control is done through FPGA LVDS interface.
// ADS61B49 uses CMOS serial interface (SDATA, SCLK, SLOAD)
// These are driven by FPGA, not STM32 directly.
// ------------------------------------------------------------
// ADC registers are written by FPGA bitstream configuration.
// See FPGA gateware documentation for ADC register access.

// ------------------------------------------------------------
// ESP32-C6 AT Command Set (via UART)
// ------------------------------------------------------------
#define ESP_AT_CMD_RESET      "AT+RST\r\n"
#define ESP_AT_CMD_WIFI_MODE  "AT+CWMODE=3\r\n"       // AP+STA mode
#define ESP_AT_CMD_AP_CONFIG  "AT+CWSAP=\"HexaScope-XXXX\",\"scope123\",6,3\r\n"
#define ESP_AT_CMD_CONN_START "AT+CIPSTART=\"UDP\",0,0,0\r\n"  // UDP server
#define ESP_AT_CMD_SEND       "AT+CIPSEND=%d\r\n"

#endif // HEXASCOPE_REGISTERS_H