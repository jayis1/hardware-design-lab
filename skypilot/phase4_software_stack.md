# SkyPilot — Phase 4: Software Stack

## 1. Boot Strategy

The SkyPilot boot sequence proceeds in three phases:

### Phase 0: ROM Bootloader (STM32H743 internal)
- Checks BOOT0 pin: LOW = Flash boot, HIGH = System memory (DFU over USB-C)
- If BOOT0 = LOW: Jump to flash at 0x08000000

### Phase 1: SPL (Secondary Program Loader)
- Vector table at 0x08000000, reset handler at 0x08000004
- Initialize .data section (copy from Flash to DTCM at 0x20000000)
- Zero .bss section in DTCM
- Configure HSE 24MHz crystal → PLL → SYSCLK 480MHz
- Enable I-Cache, D-Cache, ART accelerator
- Enable MPU: mark ITCM as Strongly Ordered, DTCM as Non-Cacheable
- Jump to main()

### Phase 2: main() — Board Initialization
```
main():
  ├── board_init_clocks()        // 480 MHz SYSCLK, 240 MHz AHB, 120 MHz APB1/2
  ├── board_init_gpio()          // All pin mux, pull-ups/downs
  ├── board_init_dma()           // DMA1/DMA2 channels for SPI/UART
  ├── board_init_systick()       // 1ms tick for FreeRTOS
  ├── imu1_init()                // ICM-42688-P via SPI1 @ 30MHz
  ├── imu2_init()                // BMI270 via SPI2 @ 30MHz
  ├── baro_init()                // BMP390 via I2C1 @ 400kHz
  ├── gnss_init()                // SAM-M10Q via UART1 @ 460800
  ├── lte_modem_init()           // LARA-R6 via UART8 @ 921600
  ├── companion_init()           // ESP32-H2 via UART4 @ 921600
  ├── motor_init()               // TIM1/2/3/8 DShot1200 output
  ├── adc_init()                 // Battery voltage/current sensing
  ├── watchdog_init()            // IWDG, 2-second timeout
  └── flight_loop_start()       // Enter 8kHz sensor read loop
```

## 2. MMIO Register Definitions

### 2.1 STM32H743 Key Register Map

```c
// registers.h — STM32H743 MMIO Register Definitions for SkyPilot

#ifndef SKYPILOT_REGISTERS_H
#define SKYPILOT_REGISTERS_H

#include <stdint.h>

// Base addresses
#define RCC_BASE            0x58024400UL
#define GPIOA_BASE          0x58020000UL
#define GPIOB_BASE          0x58020400UL
#define GPIOC_BASE          0x58020800UL
#define GPIOD_BASE          0x58020C00UL
#define GPIOE_BASE          0x58021000UL
#define SPI1_BASE           0x40013000UL
#define SPI2_BASE           0x40003800UL
#define SPI4_BASE           0x40013400UL
#define I2C1_BASE           0x40005400UL
#define USART1_BASE         0x40011000UL
#define USART4_BASE         0x40004400UL
#define USART8_BASE         0x40007C00UL
#define TIM1_BASE           0x40012C00UL
#define TIM2_BASE           0x40000000UL
#define TIM3_BASE           0x40000400UL
#define TIM8_BASE           0x40013400UL
#define DMA1_BASE           0x40020000UL
#define DMA2_BASE           0x40020400UL
#define ADC1_BASE           0x40022000UL
#define FDCAN1_BASE         0x40006000UL
#define PWR_BASE            0x58004000UL
#define FLASH_BASE          0x52002000UL
#define SYSCFG_BASE         0x58001400UL
#define DBGMCU_BASE         0x58005400UL
#define NVIC_BASE           0xE000E100UL
#define SCB_BASE            0xE000ED00UL
#define IWDG_BASE           0x58004C00UL

// Register bit definitions
typedef volatile uint32_t reg32_t;
typedef volatile uint16_t reg16_t;
typedef volatile uint8_t  reg8_t;

// RCC registers
typedef struct {
    reg32_t CR;         // 0x00: Clock control register
    reg32_t HSICFGR;   // 0x04: HSI configuration
    reg32_t CRRCR;     // 0x08: Clock recovery RC
    reg32_t CSICFGR;   // 0x0C: CSI configuration
    reg32_t CFGR;      // 0x10: Clock configuration
    reg32_t D1CFGR;    // 0x14: Domain 1 clock configuration
    reg32_t D2CFGR;    // 0x18: Domain 2 clock configuration
    reg32_t D3CFGR;    // 0x1C: Domain 3 clock configuration
    reg32_t reserved0[2];
    reg32_t PLLCKSELR; // 0x28: PLL clock selection
    reg32_t PLLCFGR;   // 0x2C: PLL configuration
    reg32_t PLL1DIVR;  // 0x30: PLL1 divider
    reg32_t PLL1FRACR; // 0x34: PLL1 fractional
    reg32_t PLL2DIVR;  // 0x38: PLL2 divider
    reg32_t PLL2FRACR; // 0x3C: PLL2 fractional
    reg32_t PLL3DIVR;  // 0x40: PLL3 divider
    reg32_t PLL3FRACR; // 0x44: PLL3 fractional
    reg32_t reserved1;
    reg32_t D1CCIPR;   // 0x4C: Domain 1 peripheral clock
    reg32_t D2CCIP1R;  // 0x50: Domain 2 clock 1
    reg32_t D2CCIP2R;  // 0x54: Domain 2 clock 2
    reg32_t D3CCIPR;   // 0x58: Domain 3 peripheral clock
    reg32_t reserved2;
    reg32_t CIER;      // 0x60: Clock interrupt enable
    reg32_t CIFR;      // 0x64: Clock interrupt flag
    reg32_t CICR;      // 0x68: Clock interrupt clear
    reg32_t reserved3;
    reg32_t BDCR;      // 0x70: Backup domain control
    reg32_t CSR;       // 0x74: Clock status register
    reg32_t reserved4[2];
    reg32_t AHB3ENR;   // 0x80: AHB3 peripheral enable
    reg32_t AHB1ENR;   // 0x84: AHB1 peripheral enable
    reg32_t AHB2ENR;   // 0x88: AHB2 peripheral enable
    reg32_t AHB4ENR;   // 0x8C: AHB4 peripheral enable
    reg32_t APB1LENR;  // 0x90: APB1L peripheral enable
    reg32_t APB1HENR;  // 0x94: APB1H peripheral enable
    reg32_t APB2ENR;   // 0x98: APB2 peripheral enable
    reg32_t APB3ENR;   // 0x9C: APB3 peripheral enable
    reg32_t APB4ENR;   // 0xA0: APB4 peripheral enable
} RCC_Type;

#define RCC ((RCC_Type *)RCC_BASE)

// GPIO registers
typedef struct {
    reg32_t MODER;     // 0x00: Port mode (2 bits per pin)
    reg32_t OTYPER;    // 0x04: Output type
    reg32_t OSPEEDR;   // 0x08: Output speed (2 bits per pin)
    reg32_t PUPDR;     // 0x0C: Pull-up/pull-down (2 bits per pin)
    reg32_t IDR;       // 0x10: Input data
    reg32_t ODR;       // 0x14: Output data
    reg32_t BSRR;      // 0x18: Bit set/reset
    reg32_t LCKR;      // 0x1C: Lock
    reg32_t AFR[2];    // 0x20,0x24: Alternate function (4 bits per pin)
} GPIO_Type;

#define GPIOA ((GPIO_Type *)GPIOA_BASE)
#define GPIOB ((GPIO_Type *)GPIOB_BASE)
#define GPIOC ((GPIO_Type *)GPIOC_BASE)
#define GPIOD ((GPIO_Type *)GPIOD_BASE)
#define GPIOE ((GPIO_Type *)GPIOE_BASE)

// SPI registers
typedef struct {
    reg32_t CR1;       // 0x00: Control 1
    reg32_t CR2;       // 0x04: Control 2
    reg32_t SR;        // 0x08: Status
    reg32_t DR;        // 0x0C: Data
    reg32_t CRCPR;     // 0x10: CRC polynomial
    reg32_t RXCRCR;    // 0x14: RX CRC
    reg32_t TXCRCR;    // 0x18: TX CRC
    reg32_t I2SCFGR;   // 0x1C: I2S config
    reg32_t I2SPR;     // 0x20: I2S prescaler
    reg32_t CFG1;      // 0x28: SPI config (H7 specific)
    reg32_t CFG2;      // 0x2C: SPI config 2 (H7 specific)
    reg32_t IER;       // 0x30: Interrupt enable
    reg32_t SR;        // 0x34: Status (H7)
    reg32_t IFCR;      // 0x38: Interrupt flag clear
    reg32_t TXDR;      // 0x3C: TX data (H7)
    reg32_t RXDR;      // 0x40: RX data (H7)
} SPI_Type;

#define SPI1 ((SPI_Type *)SPI1_BASE)
#define SPI2 ((SPI_Type *)SPI2_BASE)
#define SPI4 ((SPI_Type *)SPI4_BASE)

// USART registers (simplified)
typedef struct {
    reg32_t CR1;       // 0x00: Control 1
    reg32_t CR2;       // 0x04: Control 2
    reg32_t CR3;       // 0x08: Control 3
    reg32_t BRR;       // 0x0C: Baud rate
    reg32_t GTPR;      // 0x10: Guard time/prescaler
    reg32_t RTOR;      // 0x14: Receiver timeout
    reg32_t RQR;       // 0x18: Request
    reg32_t ISR;       // 0x1C: Interrupt status
    reg32_t ICR;       // 0x20: Interrupt flag clear
    reg32_t RDR;       // 0x24: Receive data
    reg32_t TDR;       // 0x28: Transmit data
} USART_Type;

#define USART1 ((USART_Type *)USART1_BASE)
#define USART4 ((USART_Type *)USART4_BASE)
#define USART8 ((USART_Type *)USART8_BASE)

// Timer registers
typedef struct {
    reg32_t CR1;       // 0x00: Control 1
    reg32_t CR2;       // 0x04: Control 2
    reg32_t SMCR;      // 0x08: Slave mode control
    reg32_t DIER;      // 0x0C: DMA/interrupt enable
    reg32_t SR;        // 0x10: Status
    reg32_t EGR;       // 0x14: Event generation
    reg32_t CCMR1;     // 0x18: Capture/compare mode 1
    reg32_t CCMR2;     // 0x1C: Capture/compare mode 2
    reg32_t CCER;      // 0x20: Capture/compare enable
    reg32_t CNT;       // 0x24: Counter
    reg32_t PSC;       // 0x28: Prescaler
    reg32_t ARR;       // 0x2C: Auto-reload
    reg32_t RCR;       // 0x30: Repetition counter
    reg32_t CCR[4];    // 0x34-0x40: Capture/compare 1-4
    reg32_t BDTR;      // 0x44: Break and dead-time
    reg32_t DCR;       // 0x48: DMA control
    reg32_t DMAR;      // 0x4C: DMA address
} TIM_Type;

#define TIM1 ((TIM_Type *)TIM1_BASE)
#define TIM2 ((TIM_Type *)TIM2_BASE)

// I2C registers (simplified)
typedef struct {
    reg32_t CR1;       // 0x00: Control 1
    reg32_t CR2;       // 0x04: Control 2
    reg32_t OAR1;      // 0x08: Own address 1
    reg32_t OAR2;      // 0x0C: Own address 2
    reg32_t TIMINGR;   // 0x10: Timing
    reg32_t TIMEOUTR;  // 0x14: Timeout
    reg32_t ISR;       // 0x18: Interrupt status
    reg32_t ICR;       // 0x1C: Interrupt flag clear
    reg32_t PECR;      // 0x20: PEC
    reg32_t RXDR;      // 0x24: Receive data
    reg32_t TXDR;      // 0x28: Transmit data
} I2C_Type;

#define I2C1 ((I2C_Type *)I2C1_BASE)

// DMA registers (stream-level, H7)
typedef struct {
    reg32_t CR;        // 0x00: Control
    reg32_t NDTR;      // 0x04: Number of data
    reg32_t PAR;       // 0x08: Peripheral address
    reg32_t M0AR;      // 0x0C: Memory 0 address
    reg32_t M1AR;      // 0x10: Memory 1 address (double buffer)
    reg32_t FCR;       // 0x14: FIFO control
} DMA_Stream_Type;

// IWDG registers
typedef struct {
    reg32_t KR;        // 0x00: Key register
    reg32_t PR;        // 0x04: Prescaler
    reg32_t RLR;       // 0x08: Reload
    reg32_t SR;        // 0x0C: Status
    reg32_t WINR;      // 0x10: Window
} IWDG_Type;

#define IWDG ((IWDG_Type *)IWDG_BASE)

#endif // SKYPILOT_REGISTERS_H
```

## 3. Clock Configuration Code

```c
// Clock configuration: HSE 24MHz → PLL → SYSCLK 480MHz
// AHB = 240MHz, APB1 = 120MHz, APB2 = 120MHz, APB4 = 60MHz

#define HSE_FREQ        24000000UL   // 24 MHz external crystal
#define SYSCLK_FREQ     480000000UL  // 480 MHz system clock
#define AHB_FREQ        240000000UL  // 240 MHz AHB
#define APB1_FREQ       120000000UL  // 120 MHz APB1 (timers 2-7, 12-14)
#define APB2_FREQ       120000000UL  // 120 MHz APB2 (timers 1, 8-11)
#define APB4_FREQ        60000000UL  //  60 MHz APB4

static void board_init_clocks(void) {
    // Step 1: Enable HSE and wait for ready
    RCC->CR |= (1UL << 16);  // HSEON
    while (!(RCC->CR & (1UL << 17)));  // Wait for HSERDY

    // Step 2: Configure voltage scale (VOS0 for 480MHz)
    // PWR D3CR: VOS = 0 (scale 0 for > 400MHz)
    *(volatile uint32_t *)0x5800400C = (3UL << 14);  // VOS0
    while (!((*(volatile uint32_t *)0x58004010) & (1UL << 13)));  // VOSRDY

    // Step 3: Configure PLL1 for 480MHz
    // PLL1 source = HSE (24MHz)
    // PLL1 M = 6 → 4MHz PLL1 input
    // PLL1 N = 240 → 960MHz VCO
    // PLL1 P = 2 → 480MHz PLL1 output
    // PLL1 Q = 4 → 240MHz (for USB etc.)
    RCC->PLLCKSELR = (6UL << 4) | (2UL << 0);  // PLL1SRC=HSE, DIVM1=6

    RCC->PLLCFGR = (0UL << 0) |    // PLL1 VCO range: 128-560MHz (wide)
                    (1UL << 3) |    // PLL1 FRAC enable
                    (0UL << 6) |    // PLL1 input range: 1-2MHz
                    (0UL << 16);    // PLL2/3 defaults

    RCC->PLL1DIVR = (240UL - 1) |   // DIVN1 = 240
                     ((2UL - 1) << 9) |    // DIVP1 = 2
                     ((4UL - 1) << 16) |   // DIVQ1 = 4
                     ((2UL - 1) << 24);    // DIVR1 = 2

    // Step 4: Enable PLL1
    RCC->CR |= (1UL << 24);  // PLL1ON
    while (!(RCC->CR & (1UL << 25)));  // Wait for PLL1RDY

    // Step 5: Configure system clock mux
    // D1CFGR: CPRE = 0 (sysclk not divided)
    RCC->D1CFGR = 0;  // AHB = SYSCLK/1 = 480MHz
    // Actually need AHB = 240MHz, so divide by 2:
    // CPRE = 0b1000 = /2
    RCC->D1CFGR = (8UL << 0);  // CPRE = /2 → AHB = 240MHz

    // APB1 prescaler = /2 → 120MHz
    RCC->D2CFGR = (4UL << 8);   // APB1 = AHB/2 = 120MHz
    // APB2 prescaler = /2 → 120MHz
    RCC->D2CFGR |= (4UL << 0);  // APB2 = AHB/2 = 120MHz

    // APB4 prescaler = /4 → 60MHz
    RCC->D3CFGR = (5UL << 0);   // APB4 = AHB/4 = 60MHz

    // Step 6: Switch system clock to PLL1
    RCC->CFGR = (3UL << 0);  // SW = PLL1
    while (((RCC->CFGR >> 3) & 7) != 3);  // Wait for SWS = PLL1

    // Step 7: Enable caches
    // Enable I-Cache
    *(volatile uint32_t *)0x58001400 = 1;  // ICIEN
    // Enable D-Cache
    *(volatile uint32_t *)0x58001408 = 1;  // DCIEN
}
```

## 4. GPIO Initialization Code

```c
// board.h — Pin definitions for SkyPilot

#ifndef SKYPILOT_BOARD_H
#define SKYPILOT_BOARD_H

#include "registers.h"

// ==================== Pin Definitions ====================

// SPI1 — ICM-42688-P (Primary IMU)
#define IMU1_CS_PIN        4    // PA4
#define IMU1_CS_PORT        GPIOA
#define IMU1_SCK_PIN        5    // PA5
#define IMU1_MISO_PIN       6    // PA6
#define IMU1_MOSI_PIN       7    // PA7
#define IMU1_INT1_PIN       3    // PB3
#define IMU1_INT1_PORT      GPIOB
#define IMU1_INT2_PIN       11   // PD11
#define IMU1_INT2_PORT      GPIOD

// SPI2 — BMI270 (Secondary IMU)
#define IMU2_CS_PIN         4    // PC4
#define IMU2_CS_PORT        GPIOC
#define IMU2_SCK_PIN        5    // PC5
#define IMU2_SCK_PORT       GPIOC
#define IMU2_MISO_PIN       13   // PB13
#define IMU2_MISO_PORT      GPIOB
#define IMU2_MOSI_PIN       14   // PB14
#define IMU2_MOSI_PORT      GPIOB
#define IMU2_INT1_PIN       4    // PB4
#define IMU2_INT1_PORT      GPIOB
#define IMU2_INT2_PIN       12   // PD12
#define IMU2_INT2_PORT      GPIOD

// I2C1 — BMP390 + RV-3032
#define I2C1_SCL_PIN        6    // PB6
#define I2C1_SCL_PORT       GPIOB
#define I2C1_SDA_PIN        5    // PB5
#define I2C1_SDA_PORT       GPIOB

// UART1 — SAM-M10Q GNSS
#define GNSS_TX_PIN         8    // PB8
#define GNSS_TX_PORT        GPIOB
#define GNSS_RX_PIN         7    // PB7
#define GNSS_RX_PORT        GPIOB
#define GNSS_RST_PIN        7    // PD7
#define GNSS_RST_PORT       GPIOD

// UART4 — ESP32-H2 Companion
#define COMP_TX_PIN         13   // PB13 → reuses SPI2_MISO (alternate config)
#define COMP_RX_PIN         8    // PD8
#define COMP_RX_PORT        GPIOD

// UART8 — LARA-R6 LTE Modem
#define LTE_TX_PIN          10   // PC10
#define LTE_TX_PORT         GPIOC
#define LTE_RX_PIN          9    // PC9
#define LTE_RX_PORT         GPIOC
#define LTE_RTS_PIN         14   // PD14
#define LTE_RTS_PORT        GPIOD
#define LTE_CTS_PIN         15   // PD15
#define LTE_CTS_PORT        GPIOD
#define LTE_PWR_ON_PIN      0    // PA0 (alternate: ADC, but used as GPIO here)
#define LTE_PWR_ON_PORT     GPIOA
#define LTE_STATUS_PIN      9    // PD9
#define LTE_STATUS_PORT     GPIOD

// USB — Configuration port
#define USB_DM_PIN          11   // PA11
#define USB_DP_PIN          12   // PA12
#define USB_DM_PORT         GPIOA
#define USB_DP_PORT         GPIOA

// DShot Motor Outputs (TIM1/2/3/8)
#define MOTOR1_PIN          8    // PE8  TIM1_CH2
#define MOTOR2_PIN          9    // PE9  TIM1_CH3
#define MOTOR3_PIN          10   // PE10 TIM1_CH3
#define MOTOR4_PIN          11   // PE11 TIM1_CH4
#define MOTOR5_PIN          12   // PE12 TIM1_CH1N
#define MOTOR6_PIN          13   // PE13 TIM1_CH2N
#define MOTOR7_PIN          14   // PE14 TIM1_CH3N
#define MOTOR8_PIN          0    // PB0  TIM3_CH3
#define MOTOR9_PIN          1    // PB1  TIM3_CH4
#define MOTOR10_PIN         10   // PA10 TIM8_CH1
#define MOTOR11_PIN         11   // PB11 TIM2_CH4
#define MOTOR12_PIN         10   // PB10 TIM2_CH3

// ADC — Battery/Current sensing
#define ADC_BATT_PIN        0    // PA0 → ADC1_INP16
#define ADC_CURR_PIN        1    // PA1 → ADC1_INP17

// LEDs
#define LED_POWER_PIN       2    // PD2
#define LED_POWER_PORT      GPIOD
#define LED_STATUS_PIN      7    // PE7
#define LED_STATUS_PORT     GPIOE
#define LED_LTE_PIN         1    // PD1
#define LED_LTE_PORT        GPIOD

// Buttons
#define BTN_RESET_PIN       3    // PD3
#define BTN_RESET_PORT      GPIOD
#define BTN_BOOT_PIN        4    // PD4
#define BTN_BOOT_PORT       GPIOD

// SPI4 — W25Q128 Flash
#define FLASH_CS_PIN        1    // PE1
#define FLASH_CS_PORT       GPIOE
#define FLASH_SCK_PIN       2    // PE2
#define FLASH_SCK_PORT      GPIOE
#define FLASH_MISO_PIN      3    // PE3
#define FLASH_MISO_PORT     GPIOE
#define FLASH_MOSI_PIN      4    // PE4
#define FLASH_MOSI_PORT     GPIOE

// ==================== Alternate Function Table ====================
// STM32H743 AF assignments:
// PA4  = AF5 (SPI1_NSS)
// PA5  = AF5 (SPI1_SCK)
// PA6  = AF5 (SPI1_MISO)
// PA7  = AF5 (SPI1_MOSI)
// PB5  = AF4 (I2C1_SDA)
// PB6  = AF4 (I2C1_SCL)
// PB7  = AF4 (USART1_RX)
// PB8  = AF4 (USART1_TX)
// PC4  = AF5 (SPI2_NSS)
// PC5  = AF5 (SPI2_SCK)
// PB13 = AF5 (SPI2_MISO)
// PB14 = AF5 (SPI2_MOSI)
// PA11 = AF10 (USB_OTG_FS_DM)
// PA12 = AF10 (USB_OTG_FS_DP)
// PC9  = AF8 (UART8_RX)
// PC10 = AF8 (UART8_TX)
// PD14 = AF8 (UART8_RTS) — note: alternate mapping
// PD15 = AF8 (UART8_CTS) — note: alternate mapping
// PE2  = AF5 (SPI4_SCK)
// PE3  = AF5 (SPI4_MISO)
// PE4  = AF5 (SPI4_MOSI)

// ==================== GPIO Init Function ====================

static inline void board_init_gpio(void) {
    // Enable GPIO clocks (AHB4)
    RCC->AHB4ENR |= (1UL << 0) |   // GPIOA
                     (1UL << 1) |   // GPIOB
                     (1UL << 2) |   // GPIOC
                     (1UL << 3) |   // GPIOD
                     (1UL << 4);    // GPIOE

    // --- SPI1 Pins (PA4-PA7): AF5 ---
    // PA4 = SPI1_NSS (AF5)
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (4*2))) | (2UL << (4*2));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFUL << (4*4))) | (5UL << (4*4));
    GPIOA->OSPEEDR |= (3UL << (4*2));  // Very high speed
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3UL << (4*2))) | (1UL << (4*2));  // Pull-up on CS

    // PA5 = SPI1_SCK (AF5)
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (5*2))) | (2UL << (5*2));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFUL << (5*4))) | (5UL << (5*4));
    GPIOA->OSPEEDR |= (3UL << (5*2));

    // PA6 = SPI1_MISO (AF5)
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (6*2))) | (2UL << (6*2));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFUL << (6*4))) | (5UL << (6*4));
    GPIOA->OSPEEDR |= (3UL << (6*2));

    // PA7 = SPI1_MOSI (AF5)
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (7*2))) | (2UL << (7*2));
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFUL << (7*4))) | (5UL << (7*4));
    GPIOA->OSPEEDR |= (3UL << (7*2));

    // --- SPI2 Pins (PC4, PC5, PB13, PB14): AF5 ---
    // PC4 = SPI2_NSS (AF5)
    GPIOC->MODER = (GPIOC->MODER & ~(3UL << (4*2))) | (2UL << (4*2));
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~(0xFUL << (4*4))) | (5UL << (4*4));
    GPIOC->OSPEEDR |= (3UL << (4*2));
    GPIOC->PUPDR = (GPIOC->PUPDR & ~(3UL << (4*2))) | (1UL << (4*2));

    // PC5 = SPI2_SCK (AF5)
    GPIOC->MODER = (GPIOC->MODER & ~(3UL << (5*2))) | (2UL << (5*2));
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~(0xFUL << (5*4))) | (5UL << (5*4));
    GPIOC->OSPEEDR |= (3UL << (5*2));

    // PB13 = SPI2_MISO (AF5)
    GPIOB->MODER = (GPIOB->MODER & ~(3UL << (13*2))) | (2UL << (13*2));
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(0xFUL << ((13-8)*4))) | (5UL << ((13-8)*4));
    GPIOB->OSPEEDR |= (3UL << (13*2));

    // PB14 = SPI2_MOSI (AF5)
    GPIOB->MODER = (GPIOB->MODER & ~(3UL << (14*2))) | (2UL << (14*2));
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(0xFUL << ((14-8)*4))) | (5UL << ((14-8)*4));
    GPIOB->OSPEEDR |= (3UL << (14*2));

    // --- I2C1 Pins (PB5, PB6): AF4 ---
    GPIOB->MODER = (GPIOB->MODER & ~(3UL << (5*2))) | (2UL << (5*2));
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xFUL << (5*4))) | (4UL << (5*4));
    GPIOB->OSPEEDR |= (3UL << (5*2));
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(3UL << (5*2))) | (1UL << (5*2));

    GPIOB->MODER = (GPIOB->MODER & ~(3UL << (6*2))) | (2UL << (6*2));
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xFUL << (6*4))) | (4UL << (6*4));
    GPIOB->OSPEEDR |= (3UL << (6*2));
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(3UL << (6*2))) | (1UL << (6*2));

    // --- UART1 Pins (PB7 RX, PB8 TX): AF4 ---
    GPIOB->MODER = (GPIOB->MODER & ~(3UL << (7*2))) | (2UL << (7*2));
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xFUL << (7*4))) | (4UL << (7*4));
    GPIOB->OSPEEDR |= (3UL << (7*2));

    GPIOB->MODER = (GPIOB->MODER & ~(3UL << (8*2))) | (2UL << (8*2));
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~(0xFUL << (0*4))) | (4UL << (0*4));
    GPIOB->OSPEEDR |= (3UL << (8*2));

    // --- UART8 Pins (PC9 RX, PC10 TX): AF8 ---
    GPIOC->MODER = (GPIOC->MODER & ~(3UL << (9*2))) | (2UL << (9*2));
    GPIOC->AFR[1] = (GPIOC->AFR[1] & ~(0xFUL << (1*4))) | (8UL << (1*4));
    GPIOC->OSPEEDR |= (3UL << (9*2));

    GPIOC->MODER = (GPIOC->MODER & ~(3UL << (10*2))) | (2UL << (10*2));
    GPIOC->AFR[1] = (GPIOC->AFR[1] & ~(0xFUL << (2*4))) | (8UL << (2*4));
    GPIOC->OSPEEDR |= (3UL << (10*2));

    // --- LED outputs ---
    GPIOD->MODER = (GPIOD->MODER & ~(3UL << (2*2))) | (1UL << (2*2));  // PD2 = output (Power LED)
    GPIOD->MODER = (GPIOD->MODER & ~(3UL << (1*2))) | (1UL << (1*2));  // PD1 = output (LTE LED)
    GPIOE->MODER = (GPIOE->MODER & ~(3UL << (7*2))) | (1UL << (7*2));  // PE7 = output (Status LED)

    // --- Button inputs ---
    GPIOD->MODER = (GPIOD->MODER & ~(3UL << (3*2)));  // PD3 = input (Reset)
    GPIOD->PUPDR = (GPIOD->PUPDR & ~(3UL << (3*2))) | (1UL << (3*2));  // Pull-up
    GPIOD->MODER = (GPIOD->MODER & ~(3UL << (4*2)));  // PD4 = input (Boot)
    GPIOD->PUPDR = (GPIOD->PUPDR & ~(3UL << (4*2)));  // No pull

    // --- LTE control pins ---
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << (0*2))) | (1UL << (0*2));  // PA0 = output (LTE_PWR_ON)
    GPIOD->MODER = (GPIOD->MODER & ~(3UL << (9*2))) | (1UL << (9*2));  // PD9 = output (LTE_STATUS)
    GPIOD->MODER = (GPIOD->MODER & ~(3UL << (14*2))) | (1UL << (14*2));  // PD14 = output (LTE_RTS)
    GPIOD->MODER = (GPIOD->MODER & ~(3UL << (15*2)));  // PD15 = input (LTE_CTS)

    // --- GNSS reset ---
    GPIOD->MODER = (GPIOD->MODER & ~(3UL << (7*2))) | (1UL << (7*2));  // PD7 = output (GNSS_RST)
}

#endif // SKYPILOT_BOARD_H
```

## 5. Device Driver: ICM-42688-P IMU (SPI1 + DMA)

```c
// drivers/imu_icm42688.h

#ifndef SKYPILOT_IMU_ICM42688_H
#define SKYPILOT_IMU_ICM42688_H

#include <stdint.h>
#include <stdbool.h>

// ICM-42688-P Register Map
#define ICM42688_WHO_AM_I          0x75  // Expected: 0x47
#define ICM42688_DEVICE_CONFIG     0x11
#define ICM42688_DRIVE_CONFIG      0x13
#define ICM42688_INT_CONFIG        0x14
#define ICM42688_FIFO_CONFIG       0x16
#define ICM42688_TEMP_DATA1        0x1D
#define ICM42688_TEMP_DATA0        0x1E
#define ICM42688_ACCEL_DATA_X1    0x1F
#define ICM42688_ACCEL_DATA_X0    0x20
#define ICM42688_ACCEL_DATA_Y1    0x21
#define ICM42688_ACCEL_DATA_Y0    0x22
#define ICM42688_ACCEL_DATA_Z1    0x23
#define ICM42688_ACCEL_DATA_Z0    0x24
#define ICM42688_GYRO_DATA_X1     0x25
#define ICM42688_GYRO_DATA_X0     0x26
#define ICM42688_GYRO_DATA_Y1     0x27
#define ICM42688_GYRO_DATA_Y0     0x28
#define ICM42688_GYRO_DATA_Z1     0x29
#define ICM42688_GYRO_DATA_Z0     0x2A
#define ICM42688_TMST_FSYNCH      0x2B
#define ICM42688_TMST_FSYNCL      0x2C
#define ICM42688_INT_STATUS        0x2D
#define ICM42688_FIFO_COUNTH       0x2E
#define ICM42688_FIFO_COUNTL       0x2F
#define ICM42688_FIFO_DATA         0x30
#define ICM42688_APEX_DATA0        0x31
#define ICM42688_APEX_DATA1        0x32
#define ICM42688_APEX_DATA2        0x33
#define ICM42688_APEX_DATA3        0x34
#define ICM42688_APEX_DATA4        0x35
#define ICM42688_APEX_DATA5        0x36
#define ICM42688_INT_STATUS2       0x37
#define ICM42688_INT_STATUS3       0x38
#define ICM42688_SIGNAL_PATH_RESET 0x4B
#define ICM42688_INTF_CONFIG0      0x4C
#define ICM42688_INTF_CONFIG1      0x4D
#define ICM42688_PWR_MGMT0         0x4E
#define ICM42688_GYRO_SMPLRT_DIV  0x4F
#define ICM42688_GYRO_CONFIG0      0x50
#define ICM42688_ACCEL_SMPLRT_DIV  0x52
#define ICM42688_ACCEL_CONFIG0     0x53
#define ICM42688_GYRO_CONFIG1      0x54
#define ICM42688_ACCEL_CONFIG1      0x55
#define ICM42688_GYRO_ACCEL_CONFIG0 0x56
#define ICM42688_ACCEL_CONFIG2     0x57
#define ICM42688_FSYNC_CONFIG      0x58
#define ICM42688_APEX_CONFIG0      0x59
#define ICM42688_APEX_CONFIG1      0x5A
#define ICM42688_INT_SOURCE0       0x5C
#define ICM42688_INT_SOURCE1       0x5D
#define ICM42688_INT_SOURCE3       0x5F
#define ICM42688_INT_SOURCE4       0x60
#define ICM42688_FIFO_LOST_PKT0    0x6A
#define ICM42688_FIFO_LOST_PKT1    0x6B
#define ICM42688_USER_BANK         0x76

// Bank 1 registers
#define ICM42688_BANK1_GYRO_CONFIG_STATIC2  0x0B
#define ICM42688_BANK1_GYRO_CONFIG_STATIC3  0x0C
#define ICM42688_BANK1_GYRO_CONFIG_STATIC4  0x0D
#define ICM42688_BANK1_GYRO_CONFIG_STATIC5  0x0E
#define ICM42688_BANK1_GYRO_CONFIG_STATIC6  0x0F
#define ICM42688_BANK1_GYRO_CONFIG_STATIC7  0x10
#define ICM42688_BANK1_GYRO_CONFIG_STATIC8  0x11
#define ICM42688_BANK1_GYRO_CONFIG_STATIC9  0x12
#define ICM42688_BANK1_GYRO_CONFIG_STATIC10 0x13

// Bank 2 registers
#define ICM42688_BANK2_ACCEL_CONFIG_STATIC2 0x0B
#define ICM42688_BANK2_ACCEL_CONFIG_STATIC3 0x0C
#define ICM42688_BANK2_ACCEL_CONFIG_STATIC4 0x0D
#define ICM42688_BANK2_ACCEL_CONFIG_STATIC5 0x0E
#define ICM42688_BANK2_ACCEL_CONFIG_STATIC6 0x0F

// Bank 4 registers
#define ICM42688_BANK4_APEX_CONFIG2  0x02
#define ICM42688_BANK4_APEX_CONFIG3  0x03
#define ICM42688_BANK4_APEX_CONFIG4  0x04
#define ICM42688_BANK4_APEX_CONFIG5  0x05
#define ICM42688_BANK4_APEX_CONFIG6  0x06
#define ICM42688_BANK4_APEX_CONFIG7  0x07
#define ICM42688_BANK4_APEX_CONFIG8  0x08
#define ICM42688_BANK4_APEX_CONFIG9  0x09

// Gyroscope FS selections (dps)
#define ICM42688_GYRO_FS_2000DPS   0x00
#define ICM42688_GYRO_FS_1000DPS   0x01
#define ICM42688_GYRO_FS_500DPS    0x02
#define ICM42688_GYRO_FS_250DPS    0x03
#define ICM42688_GYRO_FS_125DPS    0x04
#define ICM42688_GYRO_FS_62_5DPS   0x05
#define ICM42688_GYRO_FS_31_25DPS  0x06
#define ICM42688_GYRO_FS_15_625DPS 0x07

// Accelerometer FS selections (g)
#define ICM42688_ACCEL_FS_16G   0x00
#define ICM42688_ACCEL_FS_8G    0x01
#define ICM42688_ACCEL_FS_4G    0x02
#define ICM42688_ACCEL_FS_2G    0x03

// ODR selections
#define ICM42688_ODR_32KHZ   0x01
#define ICM42688_ODR_16KHZ   0x02
#define ICM42688_ODR_8KHZ    0x03
#define ICM42688_ODR_4KHZ    0x04
#define ICM42688_ODR_2KHZ    0x05
#define ICM42688_ODR_1KHZ    0x06

// Low noise mode
#define ICM42688_GYRO_MODE_LN   0x03
#define ICM42688_ACCEL_MODE_LN  0x03

// Data structures
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temperature;
    uint32_t timestamp;
} imu_data_t;

typedef struct {
    float accel_x;  // in g
    float accel_y;
    float accel_z;
    float gyro_x;   // in dps
    float gyro_y;
    float gyro_z;
    float temperature; // in °C
} imu_scaled_t;

// Function prototypes
int  icm42688_init(void);
int  icm42688_who_am_i(void);
int  icm42688_reset(void);
int  icm42688_config_gyro(uint8_t fs, uint8_t odr);
int  icm42688_config_accel(uint8_t fs, uint8_t odr);
int  icm42688_read_raw(imu_data_t *data);
int  icm42688_read_scaled(imu_scaled_t *data);
void icm42688_scale_data(const imu_data_t *raw, imu_scaled_t *scaled);
int  icm42688_set_bank(uint8_t bank);
int  icm42688_read_reg(uint8_t reg, uint8_t *buf, uint16_t len);
int  icm42688_write_reg(uint8_t reg, const uint8_t *buf, uint16_t len);
int  icm42688_start_dma_read(uint8_t *rx_buf, uint16_t len);
int  icm42688_dma_complete(void);

// SPI read/write macros (with CS toggle)
#define ICM42688_CS_LOW()   (GPIOA->BSRR = (1UL << (IMU1_CS_PIN + 16)))
#define ICM42688_CS_HIGH()  (GPIOA->BSRR = (1UL << IMU1_CS_PIN))

#endif // SKYPILOT_IMU_ICM42688_H
```

```c
// drivers/imu_icm42688.c — ICM-42688-P IMU driver with DMA

#include "board.h"
#include "registers.h"
#include "imu_icm42688.h"

// Sensitivity values
#define ICM42688_ACCEL_SENS_16G   2048.0f   // LSB/g for ±16g
#define ICM42688_ACCEL_SENS_8G    4096.0f    // LSB/g for ±8g
#define ICM42688_ACCEL_SENS_4G    8192.0f    // LSB/g for ±4g
#define ICM42688_ACCEL_SENS_2G    16384.0f   // LSB/g for ±2g
#define ICM42688_GYRO_SENS_2000   16.4f      // LSB/dps for ±2000dps
#define ICM42688_GYRO_SENS_1000  32.8f       // LSB/dps for ±1000dps
#define ICM42688_GYRO_SENS_500   65.5f       // LSB/dps for ±500dps
#define ICM42688_GYRO_SENS_250   131.0f      // LSB/dps for ±250dps
#define ICM42688_TEMP_SENS        132.48f    // LSB/°C
#define ICM42688_TEMP_OFFSET       25.0f     // °C at 0 LSB

static float accel_sens = ICM42688_ACCEL_SENS_16G;
static float gyro_sens = ICM42688_GYRO_SENS_2000;

static volatile int dma_complete_flag = 0;

// ---- SPI1 Low-Level Transfer ----

static uint8_t spi1_transfer(uint8_t tx_byte) {
    // Wait for TX empty
    while (!(SPI1->SR & (1UL << 1)));   // TXP flag
    *(volatile uint8_t *)&SPI1->TXDR = tx_byte;
    // Wait for RX not empty
    while (!(SPI1->SR & (1UL << 0)));   // RXP flag
    return *(volatile uint8_t *)&SPI1->RXDR;
}

static void spi1_init(void) {
    // Enable SPI1 clock
    RCC->APB2ENR |= (1UL << 12);  // SPI1EN

    // Disable SPI1 first
    SPI1->CR1 &= ~(1UL << 0);  // SPE = 0

    // Configure SPI1 as master, CPOL=0, CPHA=0, 8-bit, MSB first
    SPI1->CFG1 = (7UL << 0) |    // MBR: baud rate = 120MHz/256 = 468kHz (init)
                  (0UL << 4) |    // CRCSIZE: 8-bit CRC
                  (0UL << 8) |    // CRCEN: no CRC (for now)
                  (0UL << 12) |   // RXDMA: disabled
                  (0UL << 14) |   // TXDMA: disabled
                  (0UL << 16);   // FTHLV: 1 data

    SPI1->CFG2 = (0UL << 0) |    // MSSI: 1 bit
                  (0UL << 4) |    // SSI: 1 bit
                  (0UL << 8) |    // CPOL: low idle
                  (0UL << 9) |    // CPHA: first edge
                  (0UL << 10) |   // LSBFIRST: MSB first
                  (0UL << 11) |   // MASTER: master mode
                  (0UL << 12) |   // SSOE: managed by CS pin
                  (0UL << 19) |   // SP: Motorola mode
                  (0UL << 22) |   // COMM: full duplex
                  (0UL << 23) |   // IOSWP: no swap
                  (7UL << 24) |   // DATASIZE: 8 bits
                  (0UL << 28);   // FIFO threshold

    // Set transfer size to 1 byte initially
    SPI1->CR2 = 1;  // TSIZE = 1

    // Enable SPI1
    SPI1->CR1 |= (1UL << 0);  // SPE = 1
}

// ---- ICM-42688 Register Access ----

int icm42688_set_bank(uint8_t bank) {
    uint8_t reg = (ICM42688_USER_BANK & 0xFC) | (bank & 0x03);
    uint8_t val = bank & 0x03;
    ICM42688_CS_LOW();
    spi1_transfer(reg);     // Write register address
    spi1_transfer(val);     // Write value
    ICM42688_CS_HIGH();
    return 0;
}

int icm42688_read_reg(uint8_t reg, uint8_t *buf, uint16_t len) {
    uint8_t addr = reg | 0x80;  // Read bit = bit7 set
    ICM42688_CS_LOW();
    spi1_transfer(addr);
    // For ICM-42688, first byte after address is dummy
    spi1_transfer(0x00);
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = spi1_transfer(0x00);
    }
    ICM42688_CS_HIGH();
    return 0;
}

int icm42688_write_reg(uint8_t reg, const uint8_t *buf, uint16_t len) {
    uint8_t addr = reg & 0x7F;  // Write bit = bit7 clear
    ICM42688_CS_LOW();
    spi1_transfer(addr);
    for (uint16_t i = 0; i < len; i++) {
        spi1_transfer(buf[i]);
    }
    ICM42688_CS_HIGH();
    return 0;
}

int icm42688_who_am_i(void) {
    uint8_t who_am_i = 0;
    icm42688_set_bank(0);
    icm42688_read_reg(ICM42688_WHO_AM_I, &who_am_i, 1);
    return who_am_i;  // Should return 0x47
}

int icm42688_reset(void) {
    uint8_t val = 0x01;  // SOFT_RESET
    icm42688_set_bank(0);
    icm42688_write_reg(ICM42688_SIGNAL_PATH_RESET, &val, 1);
    // Wait 1ms for reset
    for (volatile int i = 0; i < 48000; i++);  // ~1ms at 480MHz
    return 0;
}

int icm42688_init(void) {
    spi1_init();

    // Reset device
    icm42688_reset();
    for (volatile int i = 0; i < 480000; i++);  // ~10ms

    // Verify WHO_AM_I
    uint8_t who_am_i = icm42688_who_am_i();
    if (who_am_i != 0x47) {
        return -1;  // Device not found
    }

    // Bank 0: Configure interface
    icm42688_set_bank(0);

    // INTF_CONFIG0: SPI mode, 16-bit FIFO, no burst
    uint8_t intf_cfg0 = 0x02;  // SPI_SAPD_DIS=1, no accel LP
    icm42688_write_reg(ICM42688_INTF_CONFIG0, &intf_cfg0, 1);

    // INTF_CONFIG1: Clock PLL
    uint8_t intf_cfg1 = 0x01;  // AACLPCK_SEL = PLL
    icm42688_write_reg(ICM42688_INTF_CONFIG1, &intf_cfg1, 1);

    // PWR_MGMT0: Enable gyro and accel in low-noise mode
    // Gyro: LN mode, Accel: LN mode
    uint8_t pwr_mgmt = (ICM42688_GYRO_MODE_LN << 2) | ICM42688_ACCEL_MODE_LN;
    icm42688_write_reg(ICM42688_PWR_MGMT0, &pwr_mgmt, 1);

    // Wait for sensors to stabilize
    for (volatile int i = 0; i < 240000; i++);  // ~5ms

    // Gyro CONFIG0: ±2000dps, 8kHz ODR
    uint8_t gyro_cfg0 = (ICM42688_GYRO_FS_2000DPS << 5) | ICM42688_ODR_8KHZ;
    icm42688_write_reg(ICM42688_GYRO_CONFIG0, &gyro_cfg0, 1);

    // Accel CONFIG0: ±16g, 8kHz ODR
    uint8_t accel_cfg0 = (ICM42688_ACCEL_FS_16G << 5) | ICM42688_ODR_8KHZ;
    icm42688_write_reg(ICM42688_ACCEL_CONFIG0, &accel_cfg0, 1);

    // Gyro ACCEL_CONFIG0: Filter BW = ODR/4
    uint8_t ga_cfg0 = 0x00;  // UI_FILT_BW = ODR/4 for both
    icm42688_write_reg(ICM42688_GYRO_ACCEL_CONFIG0, &ga_cfg0, 1);

    // Update sensitivity
    accel_sens = ICM42688_ACCEL_SENS_16G;
    gyro_sens = ICM42688_GYRO_SENS_2000;

    // Configure INT1 pin for data ready
    uint8_t int_source0 = 0x01;  // UI_DRDY_INT1_EN
    icm42688_write_reg(ICM42688_INT_SOURCE0, &int_source0, 1);

    // Now increase SPI speed to 30MHz
    SPI1->CR1 &= ~(1UL << 0);  // Disable SPI
    SPI1->CFG1 = (3UL << 0) |   // MBR = 120MHz/8 = 15MHz initially
                  (0UL << 4) |
                  (0UL << 8) |
                  (0UL << 12) |
                  (0UL << 14) |
                  (0UL << 16);
    SPI1->CR2 = 1;
    SPI1->CR1 |= (1UL << 0);   // Enable SPI

    return 0;
}

int icm42688_config_gyro(uint8_t fs, uint8_t odr) {
    icm42688_set_bank(0);
    uint8_t gyro_cfg0 = (fs << 5) | odr;
    icm42688_write_reg(ICM42688_GYRO_CONFIG0, &gyro_cfg0, 1);

    switch (fs) {
        case ICM42688_GYRO_FS_2000DPS: gyro_sens = ICM42688_GYRO_SENS_2000; break;
        case ICM42688_GYRO_FS_1000DPS:  gyro_sens = ICM42688_GYRO_SENS_1000; break;
        case ICM42688_GYRO_FS_500DPS:   gyro_sens = ICM42688_GYRO_SENS_500;  break;
        case ICM42688_GYRO_FS_250DPS:   gyro_sens = ICM42688_GYRO_SENS_250;  break;
        default: gyro_sens = ICM42688_GYRO_SENS_2000; break;
    }
    return 0;
}

int icm42688_config_accel(uint8_t fs, uint8_t odr) {
    icm42688_set_bank(0);
    uint8_t accel_cfg0 = (fs << 5) | odr;
    icm42688_write_reg(ICM42688_ACCEL_CONFIG0, &accel_cfg0, 1);

    switch (fs) {
        case ICM42688_ACCEL_FS_16G: accel_sens = ICM42688_ACCEL_SENS_16G; break;
        case ICM42688_ACCEL_FS_8G:  accel_sens = ICM42688_ACCEL_SENS_8G;  break;
        case ICM42688_ACCEL_FS_4G:  accel_sens = ICM42688_ACCEL_SENS_4G;  break;
        case ICM42688_ACCEL_FS_2G:  accel_sens = ICM42688_ACCEL_SENS_2G;  break;
        default: accel_sens = ICM42688_ACCEL_SENS_16G; break;
    }
    return 0;
}

int icm42688_read_raw(imu_data_t *data) {
    uint8_t buf[14];  // 14 bytes: temp(2) + accel(6) + gyro(6)
    icm42688_set_bank(0);
    int rc = icm42688_read_reg(ICM42688_TEMP_DATA1, buf, 14);
    if (rc != 0) return rc;

    data->temperature = (int16_t)((buf[0] << 8) | buf[1]);
    data->accel_x     = (int16_t)((buf[2] << 8) | buf[3]);
    data->accel_y     = (int16_t)((buf[4] << 8) | buf[5]);
    data->accel_z     = (int16_t)((buf[6] << 8) | buf[7]);
    data->gyro_x      = (int16_t)((buf[8] << 8) | buf[9]);
    data->gyro_y      = (int16_t)((buf[10] << 8) | buf[11]);
    data->gyro_z      = (int16_t)((buf[12] << 8) | buf[13]);

    return 0;
}

void icm42688_scale_data(const imu_data_t *raw, imu_scaled_t *scaled) {
    scaled->temperature = ((float)raw->temperature / ICM42688_TEMP_SENS) + ICM42688_TEMP_OFFSET;
    scaled->accel_x = (float)raw->accel_x / accel_sens;
    scaled->accel_y = (float)raw->accel_y / accel_sens;
    scaled->accel_z = (float)raw->accel_z / accel_sens;
    scaled->gyro_x  = (float)raw->gyro_x / gyro_sens;
    scaled->gyro_y  = (float)raw->gyro_y / gyro_sens;
    scaled->gyro_z  = (float)raw->gyro_z / gyro_sens;
}

int icm42688_read_scaled(imu_scaled_t *data) {
    imu_data_t raw;
    int rc = icm42688_read_raw(&raw);
    if (rc != 0) return rc;
    icm42688_scale_data(&raw, data);
    return 0;
}

// ---- DMA Transfer ----

int icm42688_start_dma_read(uint8_t *rx_buf, uint16_t len) {
    // Configure DMA1 Stream 0 for SPI1 RX
    // This is a simplified DMA setup for the STM32H743
    dma_complete_flag = 0;

    // Enable DMA1 clock
    RCC->AHB1ENR |= (1UL << 0);  // DMA1EN

    // Disable DMA stream
    DMA_Stream_Type *stream = (DMA_Stream_Type *)(DMA1_BASE + 0x10 * 0);  // Stream 0
    stream->CR &= ~(1UL << 0);  // EN = 0
    while (stream->CR & (1UL << 0));  // Wait for disable

    // Configure DMA stream
    stream->PAR = (uint32_t)&SPI1->RXDR;  // Peripheral address
    stream->M0AR = (uint32_t)rx_buf;       // Memory address
    stream->NDTR = len;

    // CR: CHSEL=3 (SPI1_RX), MSIZE=8bit, PSIZE=8bit, MINC, DIR=P2M, TCIE
    stream->CR = (3UL << 0) |    // CHSEL: Channel 3 (SPI1_RX on H7)
                 (0UL << 4) |    // MBURST: single
                 (0UL << 7) |    // MINC: memory increment
                 (0UL << 9) |    // PINC: no peripheral increment
                 (0UL << 11) |   // DIR: Peripheral-to-memory
                 (1UL << 12) |   // CIRC: not circular
                 (1UL << 4) |    // TCIE: transfer complete interrupt
                 (0UL << 6) |    // HTIE: no half-transfer interrupt
                 (0UL << 8) |    // TEIE: no transfer error interrupt
                 (0UL << 14) |   // MSIZE: 8-bit
                 (0UL << 16) |   // PSIZE: 8-bit
                 (0UL << 18) |   // PINCOS: no
                 (3UL << 23);    // PL: very high priority

    // Enable DMA stream
    stream->CR |= (1UL << 0);  // EN = 1

    // Enable SPI1 DMA request
    SPI1->CFG1 |= (1UL << 12) | (1UL << 14);  // RXDMAEN + TXDMAEN

    return 0;
}

int icm42688_dma_complete(void) {
    return dma_complete_flag;
}

// DMA interrupt handler (called from DMA1_Stream0_IRQHandler)
void dma1_stream0_irqhandler(void) {
    DMA_Stream_Type *stream = (DMA_Stream_Type *)(DMA1_BASE + 0x10 * 0);
    if (stream->CR & (1UL << 0)) {  // Check if enabled
        // Check transfer complete
        if (*(volatile uint32_t *)(DMA1_BASE + 0x0C) & (1UL << 0)) {  // LISR TCIF0
            dma_complete_flag = 1;
            // Clear interrupt
            *(volatile uint32_t *)(DMA1_BASE + 0x08) = (1UL << 0);  // LIFCR
        }
    }
}
```

## 6. Device Driver: LARA-R6 LTE Modem (UART8 + DMA)

```c
// drivers/lte_lara_r6.h

#ifndef SKYPILOT_LTE_LARA_R6_H
#define SKYPILOT_LTE_LARA_R6_H

#include <stdint.h>
#include <stdbool.h>

// LARA-R6 AT command interface
// UART8 @ 921600 baud, 8N1, hardware flow control (RTS/CTS)

// LARA-R6 GPIO control pins
#define LTE_PWR_ON_LOW()    (GPIOA->BSRR = (1UL << (LTE_PWR_ON_PIN + 16)))  // PA0 LOW
#define LTE_PWR_ON_HIGH()   (GPIOA->BSRR = (1UL << LTE_PWR_ON_PIN))        // PA0 HIGH
#define LTE_RTS_LOW()       (GPIOD->BSRR = (1UL << (LTE_RTS_PIN + 16)))     // PD14 LOW
#define LTE_RTS_HIGH()      (GPIOD->BSRR = (1UL << LTE_RTS_PIN))           // PD14 HIGH
#define LTE_CTS_READ()      ((GPIOD->IDR >> LTE_CTS_PIN) & 1)              // PD15 read

// UART8 RX DMA buffer size
#define LTE_RX_BUF_SIZE     4096

// LARA-R6 states
typedef enum {
    LTE_STATE_OFF,
    LTE_STATE_BOOTING,
    LTE_STATE_READY,
    LTE_STATE_REGISTERED,
    LTE_STATE_CONNECTED,
    LTE_STATE_ERROR
} lte_state_t;

// MAVLink-over-LTE session
typedef struct {
    lte_state_t state;
    uint8_t  rx_buffer[LTE_RX_BUF_SIZE];
    uint16_t rx_head;
    uint16_t rx_tail;
    uint16_t uart_baud;
    int      socket_id;
    char     imei[16];
    char     iccid[21];
    int8_t   signal_rssi;
    int8_t   signal_rsrp;
    int8_t   signal_rsrq;
} lte_modem_t;

// Function prototypes
int  lte_init(void);
int  lte_power_on(void);
int  lte_power_off(void);
int  lte_send_at(const char *cmd, char *response, uint16_t resp_len, uint32_t timeout_ms);
int  lte_wait_response(const char *expected, char *response, uint16_t resp_len, uint32_t timeout_ms);
int  lte_configure_baud(uint32_t baud);
int  lte_check_sim(void);
int  lte_check_registration(void);
int  lte_get_signal_quality(int8_t *rssi, int8_t *rsrp, int8_t *rsrq);
int  lte_open_socket(const char *host, uint16_t port, uint8_t protocol);
int  lte_send_data(int socket, const uint8_t *data, uint16_t len);
int  lte_receive_data(int socket, uint8_t *data, uint16_t max_len, uint32_t timeout_ms);
int  lte_close_socket(int socket);
void lte_uart8_irqhandler(void);
void lte_dma_irqhandler(void);

// AT command helpers
int  lte_send_at_raw(const uint8_t *data, uint16_t len);
int  lte_setup_tcp_connection(const char *host, uint16_t port);
int  lte_send_mavlink(const uint8_t *mavlink_msg, uint16_t len);
int  lte_receive_mavlink(uint8_t *mavlink_msg, uint16_t max_len, uint32_t timeout_ms);

#endif // SKYPILOT_LTE_LARA_R6_H
```

```c
// drivers/lte_lara_r6.c — u-blox LARA-R6401 LTE modem driver with DMA

#include "board.h"
#include "registers.h"
#include "lte_lara_r6.h"
#include <string.h>

static lte_modem_t modem;

// ---- UART8 Initialization ----

static void uart8_init(uint32_t baud) {
    // Enable UART8 clock (APB1L)
    RCC->APB1LENR |= (1UL << 1);  // UART8EN

    // Disable UART8
    USART8->CR1 &= ~(1UL << 0);  // UE = 0

    // Set baud rate: BRR = APB1_FREQ / baud
    // APB1 = 120MHz
    USART8->BRR = APB1_FREQ / baud;

    // 8 data bits, 1 stop bit, no parity, hardware flow control
    USART8->CR1 = (0UL << 12) |  // M0: 8 bits
                  (0UL << 28) |  // M1: 8 bits
                  (0UL << 10) |  // PCE: no parity
                  (1UL << 5)  |  // RXNEIE: enable RX interrupt
                  (1UL << 2)  |  // RE: receiver enable
                  (1UL << 3);   // TE: transmitter enable

    USART8->CR2 = (0UL << 12);  // 1 stop bit

    USART8->CR3 = (1UL << 8) |   // DMAT: DMA enable transmitter
                   (1UL << 6) |   // DMAR: DMA enable receiver
                   (0UL << 13);   // NO DMAT in half-duplex

    // Enable UART8
    USART8->CR1 |= (1UL << 0);  // UE = 1

    // Enable NVIC interrupt for UART8
    *(volatile uint32_t *)0xE000E100 |= (1UL << 71);  // UART8 global interrupt

    modem.uart_baud = baud;
}

// ---- DMA Setup for UART8 RX ----

static void uart8_dma_rx_init(void) {
    // Enable DMA1 clock
    RCC->AHB1ENR |= (1UL << 0);  // DMA1EN

    // Configure DMA1 Stream 3 for UART8_RX (Channel 6 on H7)
    DMA_Stream_Type *stream = (DMA_Stream_Type *)(DMA1_BASE + 0x10 * 3);  // Stream 3

    // Disable stream
    stream->CR &= ~(1UL << 0);  // EN = 0
    while (stream->CR & (1UL << 0));

    // Circular mode DMA for continuous RX
    stream->PAR = (uint32_t)&USART8->RDR;     // Peripheral: UART8 RX data register
    stream->M0AR = (uint32_t)modem.rx_buffer;  // Memory: ring buffer
    stream->NDTR = LTE_RX_BUF_SIZE;

    // CR: CHSEL=6 (UART8_RX), MSIZE=8bit, PSIZE=8bit, MINC, CIRC, DIR=P2M
    stream->CR = (6UL << 0) |    // CHSEL: Channel 6
                 (0UL << 4) |    // MBURST: single
                 (1UL << 10) |   // MINC: memory increment
                 (0UL << 9) |    // PINC: no peripheral increment
                 (0UL << 11) |   // DIR: P2M (peripheral to memory)
                 (1UL << 8) |    // CIRC: circular mode
                 (0UL << 14) |   // MSIZE: 8-bit
                 (0UL << 16) |   // PSIZE: 8-bit
                 (3UL << 23);    // PL: very high priority

    // Enable stream
    stream->CR |= (1UL << 0);  // EN = 1

    modem.rx_head = 0;
    modem.rx_tail = 0;
}

// ---- LTE Modem Control ----

int lte_init(void) {
    memset(&modem, 0, sizeof(modem));
    modem.state = LTE_STATE_OFF;
    modem.socket_id = -1;

    // Initialize UART8 at 921600 baud
    uart8_init(921600);

    // Initialize DMA for RX
    uart8_dma_rx_init();

    // Set PWR_ON low (modem off)
    LTE_PWR_ON_LOW();

    // Set RTS high (not ready to receive)
    LTE_RTS_HIGH();

    return 0;
}

int lte_power_on(void) {
    if (modem.state != LTE_STATE_OFF) return -1;

    // LARA-R6 power-on sequence: toggle PWR_ON low for 500ms
    LTE_PWR_ON_LOW();

    // Wait 500ms
    for (volatile uint32_t i = 0; i < 240000000; i++);  // ~500ms @ 480MHz

    LTE_PWR_ON_HIGH();

    // Wait for modem to boot (up to 5 seconds)
    // Look for "RDY" or "AT" response
    modem.state = LTE_STATE_BOOTING;

    int result = lte_wait_response("RDY", NULL, 0, 5000);
    if (result != 0) {
        // Try sending AT to wake up
        lte_send_at("AT", NULL, 0, 2000);
    }

    // Disable echo
    lte_send_at("ATE0", NULL, 0, 1000);

    // Check SIM status
    if (lte_check_sim() != 0) {
        modem.state = LTE_STATE_ERROR;
        return -2;
    }

    // Set error reporting to numeric
    lte_send_at("AT+CMEE=1", NULL, 0, 1000);

    // Configure hardware flow control
    lte_send_at("AT+IFC=2,2", NULL, 0, 1000);  // RTS/CTS flow control

    // Enable network registration URC
    lte_send_at("AT+CREG=2", NULL, 0, 1000);

    modem.state = LTE_STATE_READY;

    // Lower RTS — ready to receive
    LTE_RTS_LOW();

    return 0;
}

int lte_power_off(void) {
    // Send AT+CFUN=0 to power down gracefully
    lte_send_at("AT+CFUN=0", NULL, 0, 3000);

    // Toggle PWR_ON for shutdown
    LTE_PWR_ON_LOW();
    for (volatile uint32_t i = 0; i < 120000000; i++);  // ~250ms
    LTE_PWR_ON_HIGH();

    modem.state = LTE_STATE_OFF;
    modem.socket_id = -1;
    return 0;
}

int lte_send_at(const char *cmd, char *response, uint16_t resp_len, uint32_t timeout_ms) {
    // Send AT command with \r\n terminator
    uint16_t cmd_len = strlen(cmd);
    const uint8_t *tx_data = (const uint8_t *)cmd;

    // Send command byte by byte (simple polling)
    for (uint16_t i = 0; i < cmd_len; i++) {
        // Wait for TX empty
        while (!(USART8->ISR & (1UL << 7)));  // TXE flag
        USART8->TDR = tx_data[i];
    }
    // Send \r\n
    while (!(USART8->ISR & (1UL << 7)));
    USART8->TDR = '\r';
    while (!(USART8->ISR & (1UL << 7)));
    USART8->TDR = '\n';

    // If caller wants response, wait for it
    if (response && resp_len > 0) {
        return lte_wait_response("OK", response, resp_len, timeout_ms);
    }

    return 0;
}

int lte_wait_response(const char *expected, char *response, uint16_t resp_len, uint32_t timeout_ms) {
    uint32_t start = 0;  // Would use systick in production
    uint16_t idx = 0;
    char match_buf[32];
    uint8_t match_len = strlen(expected);
    uint8_t match_idx = 0;

    if (match_len > sizeof(match_buf)) return -1;

    // Simple timeout loop (in production, use systick)
    uint32_t timeout_ticks = timeout_ms * 480000UL;  // Approximate

    while (1) {
        // Check if data available in DMA ring buffer
        uint16_t dma_pos = LTE_RX_BUF_SIZE -
            ((DMA_Stream_Type *)(DMA1_BASE + 0x10 * 3))->NDTR;
        uint16_t new_bytes = 0;

        if (dma_pos != modem.rx_head) {
            // New data available
            if (dma_pos > modem.rx_head) {
                new_bytes = dma_pos - modem.rx_head;
            } else {
                new_bytes = LTE_RX_BUF_SIZE - modem.rx_head + dma_pos;
            }

            // Process new bytes
            for (uint16_t i = 0; i < new_bytes; i++) {
                uint8_t byte = modem.rx_buffer[modem.rx_head];
                modem.rx_head = (modem.rx_head + 1) % LTE_RX_BUF_SIZE;

                if (response && idx < resp_len - 1) {
                    response[idx++] = byte;
                }

                // Match expected string
                if (byte == expected[match_idx]) {
                    match_idx++;
                    if (match_idx == match_len) {
                        if (response) response[idx] = '\0';
                        return 0;  // Match found
                    }
                } else {
                    match_idx = 0;
                }
            }
        }

        start++;
        if (start > timeout_ticks) {
            if (response) response[idx] = '\0';
            return -1;  // Timeout
        }
    }
}

int lte_check_sim(void) {
    char resp[128];
    int rc = lte_send_at("AT+CPIN?", resp, sizeof(resp), 3000);
    if (rc == 0 && strstr(resp, "+CPIN: READY")) {
        return 0;
    }
    return -1;
}

int lte_check_registration(void) {
    char resp[128];
    int rc = lte_send_at("AT+CREG?", resp, sizeof(resp), 3000);
    if (rc == 0) {
        // Parse +CREG: <mode>,<stat>
        char *creg = strstr(resp, "+CREG:");
        if (creg) {
            int stat = 0;
            if (sscanf(creg, "+CREG: %*d,%d", &stat) == 1) {
                if (stat == 1 || stat == 5) {  // Registered home or roaming
                    modem.state = LTE_STATE_REGISTERED;
                    return 0;
                }
            }
        }
    }
    return -1;
}

int lte_get_signal_quality(int8_t *rssi, int8_t *rsrp, int8_t *rsrq) {
    char resp[256];

    // Get RSSI
    if (lte_send_at("AT+CSQ", resp, sizeof(resp), 5000) == 0) {
        char *csq = strstr(resp, "+CSQ:");
        if (csq) {
            int val = 0;
            sscanf(csq, "+CSQ: %d,", &val);
            *rssi = (int8_t)(val == 99 ? 0 : -113 + val * 2);
        }
    }

    // Get RSRP/RSRQ (LTE-specific)
    if (lte_send_at("AT+CESQ", resp, sizeof(resp), 5000) == 0) {
        char *cesq = strstr(resp, "+CESQ:");
        if (cesq) {
            int rxlev, ber, rscp, ecno, rsrp_val, rsrq_val;
            if (sscanf(cesq, "+CESQ: %d,%d,%d,%d,%d,%d",
                       &rxlev, &ber, &rscp, &ecno, &rsrp_val, &rsrq_val) == 6) {
                *rsrp = (int8_t)(rsrp_val != 255 ? -140 + rsrp_val : 0);
                *rsrq = (int8_t)(rsrq_val != 255 ? -20 + rsrq_val * 2 : 0);
            }
        }
    }

    return 0;
}

int lte_open_socket(const char *host, uint16_t port, uint8_t protocol) {
    char cmd[128];
    char resp[256];

    // Create socket using AT+USOCR
    // protocol: 6=TCP, 17=UDP
    snprintf(cmd, sizeof(cmd), "AT+USOCR=%d", protocol);
    if (lte_send_at(cmd, resp, sizeof(resp), 5000) != 0) return -1;

    // Parse socket ID from +USOCR: <socket_id>
    int sock_id = -1;
    char *usocr = strstr(resp, "+USOCR:");
    if (usocr) {
        sscanf(usocr, "+USOCR: %d", &sock_id);
    }
    if (sock_id < 0) return -1;

    modem.socket_id = sock_id;

    // Connect socket to remote host
    snprintf(cmd, sizeof(cmd), "AT+USOCO=%d,\"%s\",%d", sock_id, host, port);
    if (lte_send_at(cmd, resp, sizeof(resp), 30000) != 0) {
        // Close socket on failure
        snprintf(cmd, sizeof(cmd), "AT+USOCL=%d", sock_id);
        lte_send_at(cmd, NULL, 0, 5000);
        modem.socket_id = -1;
        return -2;
    }

    modem.state = LTE_STATE_CONNECTED;
    return sock_id;
}

int lte_send_data(int socket, const uint8_t *data, uint16_t len) {
    char cmd[64];
    char resp[128];

    // AT+USOW=<socket_id>,<length>
    snprintf(cmd, sizeof(cmd), "AT+USOW=%d,%d", socket, len);
    lte_send_at(cmd, resp, sizeof(resp), 10000);

    // Send binary data (in production, use hex encoding with AT+USOW)
    for (uint16_t i = 0; i < len; i++) {
        while (!(USART8->ISR & (1UL << 7)));  // TXE
        USART8->TDR = data[i];
    }

    return 0;
}

int lte_receive_data(int socket, uint8_t *data, uint16_t max_len, uint32_t timeout_ms) {
    // Check for incoming data using AT+USORF or AT+USORD
    char cmd[64];
    char resp[512];

    snprintf(cmd, sizeof(cmd), "AT+USORD=%d,%d", socket, max_len);
    if (lte_send_at(cmd, resp, sizeof(resp), timeout_ms) != 0) return -1;

    // Parse response: +USORD: <socket>,<length>,"<data>"
    char *usord = strstr(resp, "+USORD:");
    if (usord) {
        int sock, length;
        if (sscanf(usord, "+USORD: %d,%d", &sock, &length) == 2) {
            if (length <= max_len) {
                // Find the data portion (after the comma and quote)
                char *data_start = strchr(usord, '"');
                if (data_start) {
                    data_start++;  // Skip quote
                    memcpy(data, data_start, length);
                    return length;
                }
            }
        }
    }
    return 0;
}

int lte_close_socket(int socket) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+USOCL=%d", socket);
    int rc = lte_send_at(cmd, NULL, 0, 10000);
    if (socket == modem.socket_id) {
        modem.socket_id = -1;
        modem.state = LTE_STATE_REGISTERED;
    }
    return rc;
}

// MAVLink-over-LTE send
int lte_send_mavlink(const uint8_t *mavlink_msg, uint16_t len) {
    if (modem.socket_id < 0) return -1;
    return lte_send_data(modem.socket_id, mavlink_msg, len);
}

// MAVLink-over-LTE receive
int lte_receive_mavlink(uint8_t *mavlink_msg, uint16_t max_len, uint32_t timeout_ms) {
    if (modem.socket_id < 0) return -1;
    return lte_receive_data(modem.socket_id, mavlink_msg, max_len, timeout_ms);
}

void lte_uart8_irqhandler(void) {
    // Handle UART8 interrupts (overrun, framing errors, etc.)
    if (USART8->ISR & (1UL << 3)) {  // ORE (overrun error)
        USART8->ICR = (1UL << 3);     // Clear overrun
    }
    if (USART8->ISR & (1UL << 1)) {  // FE (framing error)
        USART8->ICR = (1UL << 1);     // Clear framing error
    }
    if (USART8->ISR & (1UL << 2)) {  // NE (noise error)
        USART8->ICR = (1UL << 2);     // Clear noise error
    }
}
```

## 7. Device Tree Overlay

```dts
// skypilot.dts — Device tree overlay for SkyPilot flight controller

/dts-v1/;
/plugin/;

#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/interrupt-controller/irq.h>

/ {
    compatible = "skypilot,fc-v1";
    model = "SkyPilot Dual-IMU Flight Controller with LTE";

    chosen {
        stdout-path = &uart2;
    };

    memory@24000000 {
        device_type = "memory";
        reg = <0x24000000 0x00080000>;  // 512KB AXI SRAM
    };

    cpus {
        cpu0: cpu@0 {
            compatible = "arm,cortex-m7";
            clock-frequency = <480000000>;
        };
    };

    clocks {
        #address-cells = <1>;
        #size-cells = <0>;

        hse: clock@0 {
            compatible = "fixed-clock";
            clock-frequency = <24000000>;
            #clock-cells = <0>;
        };

        lse: clock@1 {
            compatible = "fixed-clock";
            clock-frequency = <32768>;
            #clock-cells = <0>;
        };
    };

    leds {
        compatible = "gpio-leds";

        led-power {
            label = "skypilot:red:power";
            gpios = <&gpiod 2 GPIO_ACTIVE_LOW>;
            default-state = "on";
        };

        led-status {
            label = "skypilot:green:status";
            gpios = <&gpioe 7 GPIO_ACTIVE_LOW>;
        };

        led-lte {
            label = "skypilot:blue:lte";
            gpios = <&gpiod 1 GPIO_ACTIVE_LOW>;
        };
    };

    gpio-keys {
        compatible = "gpio-keys";

        button-reset {
            label = "Reset Button";
            gpios = <&gpiod 3 GPIO_ACTIVE_LOW>;
            linux,code = <0x100>;  // KEY_RESTART
        };

        button-boot {
            label = "Boot Button";
            gpios = <&gpiod 4 GPIO_ACTIVE_LOW>;
            linux,code = <0x101>;  // KEY_BOOT
        };
    };

    /* I2C1 bus: BMP390 barometer + RV-3032 RTC */
    i2c1: i2c@40005400 {
        compatible = "st,stm32h7-i2c";
        reg = <0x40005400 0x400>;
        clocks = <&hse>;
        clock-frequency = <400000>;  /* 400 kHz Fast Mode */
        #address-cells = <1>;
        #size-cells = <0>;
        status = "okay";

        bmp390: pressure@77 {
            compatible = "bosch,bmp390";
            reg = <0x77>;
            interrupt-parent = <&gpioe>;
            interrupts = <0 IRQ_TYPE_EDGE_RISING>;
        };

        rv3032: rtc@32 {
            compatible = "microcrystal,rv3032";
            reg = <0x32>;
        };
    };

    /* SPI1: ICM-42688-P primary IMU */
    spi1: spi@40013000 {
        compatible = "st,stm32h7-spi";
        reg = <0x40013000 0x400>;
        clocks = <&hse>;
        spi-max-frequency = <30000000>;  /* 30 MHz */
        #address-cells = <1>;
        #size-cells = <0>;
        status = "okay";

        icm42688: imu@0 {
            compatible = "tdk,icm42688p";
            reg = <0>;
            spi-max-frequency = <30000000>;
            interrupt-parent = <&gpiob>;
            interrupts = <3 IRQ_TYPE_EDGE_RISING>;
        };
    };

    /* SPI2: BMI270 secondary IMU */
    spi2: spi@40003800 {
        compatible = "st,stm32h7-spi";
        reg = <0x40003800 0x400>;
        clocks = <&hse>;
        spi-max-frequency = <30000000>;  /* 30 MHz */
        #address-cells = <1>;
        #size-cells = <0>;
        status = "okay";

        bmi270: imu@0 {
            compatible = "bosch,bmi270";
            reg = <0>;
            spi-max-frequency = <30000000>;
            interrupt-parent = <&gpiob>;
            interrupts = <4 IRQ_TYPE_EDGE_RISING>;
        };
    };

    /* SPI4: W25Q128 config flash */
    spi4: spi@40013400 {
        compatible = "st,stm32h7-spi";
        reg = <0x40013400 0x400>;
        clocks = <&hse>;
        spi-max-frequency = <20000000>;  /* 20 MHz */
        #address-cells = <1>;
        #size-cells = <0>;
        status = "okay";

        flash@0 {
            compatible = "winbond,w25q128";
            reg = <0>;
            spi-max-frequency = <20000000>;
            size = <16777216>;  /* 16MB */
        };
    };

    /* UART1: u-blox SAM-M10Q GNSS */
    uart1: serial@40011000 {
        compatible = "st,stm32h7-uart";
        reg = <0x40011000 0x400>;
        clocks = <&hse>;
        current-speed = <460800>;
        status = "okay";

        gnss {
            compatible = "ublox,sam-m10q";
        };
    };

    /* UART4: ESP32-H2 companion */
    uart4: serial@40004400 {
        compatible = "st,stm32h7-uart";
        reg = <0x40004400 0x400>;
        clocks = <&hse>;
        current-speed = <921600>;
        status = "okay";

        companion {
            compatible = "espressif,esp32-h2";
        };
    };

    /* UART8: u-blox LARA-R6 LTE modem */
    uart8: serial@40007C00 {
        compatible = "st,stm32h7-uart";
        reg = <0x40007C00 0x400>;
        clocks = <&hse>;
        current-speed = <921600>;
        status = "okay";

        lte-modem {
            compatible = "ublox,lara-r6";
            pwr-on-gpios = <&gpioa 0 GPIO_ACTIVE_HIGH>;
            status-gpios = <&gpiod 9 GPIO_ACTIVE_LOW>;
            rts-gpios = <&gpiod 14 GPIO_ACTIVE_LOW>;
            cts-gpios = <&gpiod 15 GPIO_ACTIVE_LOW>;
            reset-gpios = <&gpiod 13 GPIO_ACTIVE_LOW>;  /* Not connected, pull-up */
        };
    };

    /* FDCAN1: CAN bus / DroneCAN */
    fdcan1: can@40006000 {
        compatible = "st,stm32h7-fdcan";
        reg = <0x40006000 0x400>;
        clocks = <&hse>;
        clock-frequency = <48000000>;
        bus-speed = <1000000>;
        status = "okay";
    };

    /* ADC1: Battery voltage/current sensing */
    adc1: adc@40022000 {
        compatible = "st,stm32h7-adc";
        reg = <0x40022000 0x400>;
        clocks = <&hse>;
        vref-mv = <3300>;
        #address-cells = <1>;
        #size-cells = <0>;
        status = "okay";

        channel@16 {
            reg = <16>;
            label = "battery_voltage";
        };

        channel@17 {
            reg = <17>;
            label = "battery_current";
        };
    };

    /* USB FS: Configuration port */
    usbotg_fs: usb@40080000 {
        compatible = "st,stm32h7-otg-fs";
        reg = <0x40080000 0x40000>;
        clocks = <&hse>;
        dr_mode = "peripheral";
        maximum-speed = "full-speed";
        status = "okay";
    };

    /* IWDG: Independent watchdog */
    iwdg: watchdog@58004C00 {
        compatible = "st,stm32h7-iwdg";
        reg = <0x58004C00 0x400>;
        timeout-sec = <2>;
        status = "okay";
    };
};

&pinctrl {
    spi1_pins: spi1-0 {
        pins {
            pinmux = <STM32H7_PA4_AF5>, <STM32H7_PA5_AF5>,
                      <STM32H7_PA6_AF5>, <STM32H7_PA7_AF5>;
            bias-pull-up;
            drive-push-pull;
            slew-rate = <3>;  /* Very high speed */
        };
    };

    spi2_pins: spi2-0 {
        pins {
            pinmux = <STM32H7_PC4_AF5>, <STM32H7_PC5_AF5>,
                      <STM32H7_PB13_AF5>, <STM32H7_PB14_AF5>;
            bias-pull-up;
            drive-push-pull;
            slew-rate = <3>;
        };
    };

    i2c1_pins: i2c1-0 {
        pins {
            pinmux = <STM32H7_PB5_AF4>, <STM32H7_PB6_AF4>;
            bias-pull-up;
            drive-open-drain;
            slew-rate = <3>;
        };
    };

    uart1_pins: uart1-0 {
        pins {
            pinmux = <STM32H7_PB7_AF4>, <STM32H7_PB8_AF4>;
            drive-push-pull;
            slew-rate = <3>;
        };
    };

    uart8_pins: uart8-0 {
        pins {
            pinmux = <STM32H7_PC9_AF8>, <STM32H7_PC10_AF8>,
                      <STM32H7_PD14_AF8>, <STM32H7_PD15_AF8>;
            drive-push-pull;
            slew-rate = <3>;
        };
    };
};
```

## 8. Build Instructions

### Prerequisites
- `arm-none-eabi-gcc` (version 12.2 or later)
- `arm-none-eabi-newlib`
- `make` (GNU Make 4.3+)
- `openocd` (for flashing, version 0.12+)
- `dfu-util` (alternative flashing via USB-C)

### Build Steps

```bash
# Clone the repository
cd /root/hardware-design-lab/skypilot/firmware

# Build the firmware
make clean
make all

# This produces:
#   build/skypilot.elf     — ELF binary with debug symbols
#   build/skypilot.bin     — Raw binary for flashing
#   build/skypilot.hex     — Intel HEX format

# Flash via OpenOCD (ST-Link)
make flash

# Flash via DFU (USB-C)
make dfu

# Flash via UART bootloader
make flash-uart

# Size report
make size

# Generate map file
make map
```

### Output Files
| File | Description |
|---|---|
| `build/skypilot.elf` | Full ELF with debug symbols |
| `build/skypilot.bin` | Raw binary (for STM32 flash) |
| `build/skypilot.hex` | Intel HEX format |
| `build/skypilot.map` | Linker map (memory layout) |