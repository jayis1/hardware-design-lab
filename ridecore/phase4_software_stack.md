# RideCore — Phase 4: Software Stack

## 1. Boot Strategy

### 1.1 Boot Sequence (detailed)

1. **POR (Power-On Reset):** TPS6521801 PMIC asserts PGOOD → STM32G474 NRST released
2. **Option bytes loaded:** BOOT0 = 0 → boot from Main Flash @ 0x0800_0000
3. **SystemInit():** Configure FPU, enable ART accelerator, set IRQ table @ 0x0800_0000
4. **Clock tree setup:**
   - HSE 8 MHz → PLL (M=1, N=85, R=2, Q=7) → SYSCLK = 170 MHz, AHB = 170 MHz
   - APB1 = 170 MHz (no prescaler), APB2 = 170 MHz
   - HSI48 for USB (48 MHz, CRS synchronized)
5. **PMIC initialization:** I2C1 → TPS6521801: enable DCDC1 (5V), LDO1 (3.3V), LDO2 (1.8V), set power-good mask
6. **GPIO init:** Configure all pins per `board.h` pin table
7. **SPI1 init:** Master, CPOL=0, CPHA=0, 20 MHz, MSB first, hardware NSS disabled
8. **SPI flash init:** W25Q128 JEDEC ID verify (0xEF4018), release deep power-down, read config sector
9. **CAN FD init:** MCP2518FD SPI reset → configure CLK, CNF1-3 for 5 Mbps FD, enable TX/RX FIFOs, enable INT
10. **BLE init:** UART2 @ 1 Mbps → nRF52832 AT handshake → set advertising name "RideCore-XXXX"
11. **ADC init:** Calibration (offset trim), configure 6 channels, set trigger from TIM1
12. **TIM1 init:** Center-aligned PWM mode 1, 20 kHz, dead-time 500 ns, enable COM interrupt
13. **FOC init:** Load parameters from SPI flash → Clarke/Park init → PI controllers init
14. **Gate enable:** Assert GATE_EN → wait 10 ms for gate drivers → de-assert DESAT_FAULT mask
15. **Ready:** Status LED heartbeat → send CAN frame 0x100 "READY" → enable FOC ISR

### 1.2 Fallback Boot

If SPI flash config sector is corrupt (CRC fail), fall back to hard-coded defaults:
- Motor: 7 pole pairs, 100 µH phase inductance, 0.05 Ω phase resistance
- Current limit: 80 A (conservative)
- PWM: 20 kHz, dead time 500 ns

## 2. MMIO Register Definitions

```c
// ============================================================================
// registers.h — RideCore STM32G474 MMIO Register Map
// ============================================================================

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

// ---------- Base addresses ----------
#define PERIPH_BASE         0x40000000UL
#define APB1PERIPH_BASE     PERIPH_BASE
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE     (PERIPH_BASE + 0x08000000UL)

// ---------- GPIO ----------
#define GPIOA_BASE          (AHB2PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE          (AHB2PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE          (AHB2PERIPH_BASE + 0x0800UL)

typedef struct {
    volatile uint32_t MODER;    // 0x00
    volatile uint32_t OTYPER;   // 0x04
    volatile uint32_t OSPEEDR;  // 0x08
    volatile uint32_t PUPDR;    // 0x0C
    volatile uint32_t IDR;      // 0x10
    volatile uint32_t ODR;      // 0x14
    volatile uint32_t BSRR;     // 0x18
    volatile uint32_t LCKR;     // 0x1C
    volatile uint32_t AFRL;    // 0x20
    volatile uint32_t AFRH;    // 0x24
    volatile uint32_t BRR;      // 0x28
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)

// ---------- TIM1 (Advanced timer — PWM) ----------
#define TIM1_BASE           (APB2PERIPH_BASE + 0x2C00UL)

typedef struct {
    volatile uint32_t CR1;      // 0x00
    volatile uint32_t CR2;      // 0x04
    volatile uint32_t SMCR;     // 0x08
    volatile uint32_t DIER;     // 0x0C
    volatile uint32_t SR;       // 0x10
    volatile uint32_t CCMR1;   // 0x14  (CH1, CH2)
    volatile uint32_t CCMR2;   // 0x18  (CH3, CH4)
    volatile uint32_t CCER;     // 0x1C
    volatile uint32_t CNT;      // 0x20
    volatile uint32_t PSC;      // 0x24
    volatile uint32_t ARR;      // 0x28
    volatile uint32_t RCR;      // 0x2C
    volatile uint32_t CCR1;    // 0x30
    volatile uint32_t CCR2;    // 0x34
    volatile uint32_t CCR3;    // 0x38
    volatile uint32_t CCR4;    // 0x3C
    volatile uint32_t BDTR;    // 0x40  (Break, dead-time)
    volatile uint32_t DCR;      // 0x44
    volatile uint32_t DMAR;    // 0x48
    volatile uint32_t AF1;      // 0x50 (G4 only)
    volatile uint32_t AF2;      // 0x54 (G4 only)
    volatile uint32_t TISEL;   // 0x58 (G4 only)
} TIM_TypeDef;

#define TIM1 ((TIM_TypeDef *)TIM1_BASE)

// ---------- ADC1/2 ----------
#define ADC1_BASE           (AHB2PERIPH_BASE + 0x0800UL)

typedef struct {
    volatile uint32_t ISR;      // 0x00
    volatile uint32_t IER;      // 0x04
    volatile uint32_t CR;       // 0x08
    volatile uint32_t CFGR;    // 0x0C
    volatile uint32_t CFGR2;   // 0x10
    volatile uint32_t SMPR1;   // 0x14
    volatile uint32_t SMPR2;   // 0x18
    volatile uint32_t LTR1;    // 0x20
    volatile uint32_t HTR1;    // 0x24
    volatile uint32_t RES1;    // 0x28 (reserved)
    volatile uint32_t OFR1;    // 0x30 (offset)
    volatile uint32_t OFR2;    // 0x34
    volatile uint32_t OFR3;    // 0x38
    volatile uint32_t OFR4;    // 0x3C
    volatile uint32_t JSQR;    // 0x40
    volatile uint32_t DR;      // 0x48 (data)
    volatile uint32_t AWD2CR;  // 0x4C
    volatile uint32_t AWD3CR;  // 0x50
    volatile uint32_t DIFSEL;  // 0x54
    volatile uint32_t CALFACT; // 0x58
    volatile uint32_t GCOMP;  // 0x60 (G4 gain comp)
} ADC_TypeDef;

#define ADC1 ((ADC_TypeDef *)ADC1_BASE)
#define ADC2 ((ADC_TypeDef *)(ADC1_BASE + 0x100UL))

// ---------- SPI1 ----------
#define SPI1_BASE           (APB2PERIPH_BASE + 0x3000UL)

typedef struct {
    volatile uint32_t CR1;      // 0x00
    volatile uint32_t CR2;      // 0x04
    volatile uint32_t SR;       // 0x08
    volatile uint32_t DR;       // 0x0C
    volatile uint32_t CRCPR;   // 0x10
    volatile uint32_t RXCRCR;  // 0x14
    volatile uint32_t TXCRCR;  // 0x18
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)

// ---------- I2C1 ----------
#define I2C1_BASE           (APB1PERIPH_BASE + 0x5400UL)

typedef struct {
    volatile uint32_t CR1;      // 0x00
    volatile uint32_t CR2;      // 0x04
    volatile uint32_t OAR1;    // 0x08
    volatile uint32_t OAR2;    // 0x0C
    volatile uint32_t TIMINGR; // 0x10
    volatile uint32_t TIMEOUTR; // 0x14
    volatile uint32_t ISR;      // 0x18
    volatile uint32_t ICR;      // 0x1C
    volatile uint32_t PECR;    // 0x20
    volatile uint32_t RXDR;    // 0x24
    volatile uint32_t TXDR;    // 0x28
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)

// ---------- USART2 ----------
#define USART2_BASE         (APB1PERIPH_BASE + 0x4400UL)

typedef struct {
    volatile uint32_t CR1;      // 0x00
    volatile uint32_t CR2;      // 0x04
    volatile uint32_t CR3;      // 0x08
    volatile uint32_t BRR;      // 0x0C
    volatile uint32_t GTPR;    // 0x10
    volatile uint32_t RTOR;    // 0x14
    volatile uint32_t RQR;      // 0x18
    volatile uint32_t ISR;      // 0x1C
    volatile uint32_t ICR;      // 0x20
    volatile uint32_t RDR;      // 0x24
    volatile uint32_t TDR;      // 0x28
} USART_TypeDef;

#define USART2 ((USART_TypeDef *)USART2_BASE)

// ---------- RCC ----------
#define RCC_BASE            (AHB1PERIPH_BASE + 0x0800UL)

typedef struct {
    volatile uint32_t CR;       // 0x00
    volatile uint32_t ICSCR;   // 0x04
    volatile uint32_t CFGR;    // 0x08
    volatile uint32_t PLLCFGR; // 0x0C (G4)
    volatile uint32_t PLL1DIVR; // 0x10
    volatile uint32_t PLL1FRACR; // 0x14
    volatile uint32_t PLL2DIVR; // 0x18
    volatile uint32_t PLL2FRACR; // 0x1C
    volatile uint32_t PLL3DIVR; // 0x20
    volatile uint32_t PLL3FRACR; // 0x24
    volatile uint32_t RESERVED[2];
    volatile uint32_t CIER;     // 0x30
    volatile uint32_t CIFR;    // 0x34
    volatile uint32_t CICR;    // 0x38
    volatile uint32_t BDCR;    // 0x3C
    volatile uint32_t CSR;      // 0x40
    volatile uint32_t CRRCR;   // 0x44
    volatile uint32_t CCIPR;   // 0x48
    volatile uint32_t CCIPR2;  // 0x4C
    volatile uint32_t DCKCFGR; // 0x50
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)RCC_BASE)

// ---------- DMA1 ----------
#define DMA1_BASE           (AHB1PERIPH_BASE + 0x0000UL)
#define DMA1_Channel1_BASE  (DMA1_BASE + 0x0008UL)

typedef struct {
    volatile uint32_t CCR;      // 0x00
    volatile uint32_t CNDTR;   // 0x04
    volatile uint32_t CPAR;    // 0x08
    volatile uint32_t CMAR;    // 0x0C
} DMA_Channel_TypeDef;

#define DMA1_Channel1 ((DMA_Channel_TypeDef *)DMA1_Channel1_BASE)
#define DMA1_Channel2 ((DMA_Channel_TypeDef *)(DMA1_Channel1_BASE + 0x14UL))
#define DMA1_Channel3 ((DMA_Channel_TypeDef *)(DMA1_Channel1_BASE + 0x28UL))

// ---------- EXTI ----------
#define EXTI_BASE           (APB2PERIPH_BASE + 0x0000UL)
#define EXTI                ((EXTI_TypeDef *)EXTI_BASE)

// ---------- SCB (Cortex-M4 system control) ----------
#define SCB_BASE            0xE000ED00UL
#define SCB                 ((SCB_TypeDef *)SCB_BASE)
#define SCB_VTOR            (*(volatile uint32_t *)0xE000ED08UL)

// ---------- NVIC ----------
#define NVIC_BASE           0xE000E100UL
#define NVIC                ((NVIC_TypeDef *)NVIC_BASE)

// ---------- SysTick ----------
#define SysTick_BASE        0xE000E010UL
#define SysTick             ((SysTick_TypeDef *)SysTick_BASE)

#endif // REGISTERS_H
```

## 3. Clock Configuration Code

```c
// ============================================================================
// clock_config.c — STM32G474 clock tree setup
// ============================================================================

#include "registers.h"
#include "board.h"

void SystemClock_Config(void) {
    // 1. Enable HSE (8 MHz external crystal)
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));  // Wait for HSE ready

    // 2. Configure PLL1: HSE → 170 MHz SYSCLK
    //    PLL1 source = HSE (8 MHz)
    //    M = 1, N = 85, R = 2 → VCO = 8/1 * 85 = 680 MHz → SYSCLK = 680/2 = 340... 
    //    Correct: M=1, N=85, R=4 → 8/1*85/4 = 170 MHz
    //    Actually STM32G4: PLLM = 1..8, PLLN = 8..127, PLLR = 2..8
    //    8 MHz / M=1 * N=85 / R=2 = 340 (too high, max VCO=344)
    //    8 / 1 * 85 / R: VCO = 680 too high
    //    Use: M=2, N=85, R=2 → VCO = 8/2*85 = 340, SYSCLK = 340/2 = 170 ✓
    RCC->PLLCFGR = (2 << RCC_PLLCFGR_PLLM_Pos)    // PLLM = 2
                 | (85 << RCC_PLLCFGR_PLLN_Pos)    // PLLN = 85
                 | (0 << RCC_PLLCFGR_PLLR_Pos)      // PLLR = 2 (0=div2)
                 | RCC_PLLCFGR_PLLREN               // Enable PLLR output
                 | (2 << RCC_PLLCFGR_PLLSRC_Pos);   // HSE source

    // 3. Enable PLL1
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));  // Wait for PLL lock

    // 4. Set flash latency for 170 MHz (4 wait states)
    FLASH->ACR = FLASH_ACR_LATENCY_4WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS);

    // 5. Switch SYSCLK to PLL1R
    RCC->CFGR = (RCC_CFGR_SW_PLL1) |   // SYSCLK = PLL1R
                (0 << RCC_CFGR_HPRE_Pos) |   // AHB = SYSCLK / 1 = 170 MHz
                (0 << RCC_CFGR_PPRE1_Pos) |   // APB1 = AHB / 1 = 170 MHz
                (0 << RCC_CFGR_PPRE2_Pos);    // APB2 = AHB / 1 = 170 MHz

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1R);

    // 6. Enable HSI48 for USB (48 MHz CRS-synced)
    RCC->CRRCR |= RCC_CRRCR_HSI48ON;
    while (!(RCC->CRRCR & RCC_CRRCR_HSI48RDY));
}

void Peripheral_Clock_Enable(void) {
    // GPIOA, GPIOB, GPIOC
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;

    // SPI1, TIM1, USART2
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_TIM1EN | RCC_APB2ENR_USART1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // I2C1
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // ADC12
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;

    // DMA1
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    // SYSCFG, COMP (for overcurrent hardware trigger)
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
}
```

## 4. GPIO Init Code

```c
// ============================================================================
// gpio_init.c — RideCore pin configuration
// ============================================================================

#include "registers.h"
#include "board.h"

// GPIO mode values
#define GPIO_MODE_INPUT    0
#define GPIO_MODE_OUTPUT   1
#define GPIO_MODE_AF       2
#define GPIO_MODE_ANALOG   3

// Pull-up/pull-down
#define GPIO_NOPULL   0
#define GPIO_PULLUP   1
#define GPIO_PULLDOWN 2

// Speed
#define GPIO_SPEED_LOW    0
#define GPIO_SPEED_MED    1
#define GPIO_SPEED_HIGH   2
#define GPIO_SPEED_VHIGH  3

static void gpio_set_mode(GPIO_TypeDef *port, uint8_t pin, uint8_t mode) {
    port->MODER = (port->MODER & ~(3 << (pin*2))) | (mode << (pin*2));
}

static void gpio_set_af(GPIO_TypeDef *port, uint8_t pin, uint8_t af) {
    if (pin < 8)
        port->AFRL = (port->AFRL & ~(0xF << (pin*4))) | (af << (pin*4));
    else
        port->AFRH = (port->AFRH & ~(0xF << ((pin-8)*4))) | (af << ((pin-8)*4));
}

static void gpio_set_pull(GPIO_TypeDef *port, uint8_t pin, uint8_t pull) {
    port->PUPDR = (port->PUPDR & ~(3 << (pin*2))) | (pull << (pin*2));
}

static void gpio_set_speed(GPIO_TypeDef *port, uint8_t pin, uint8_t speed) {
    port->OSPEEDR = (port->OSPEEDR & ~(3 << (pin*2))) | (speed << (pin*2));
}

void GPIO_Init(void) {
    // ---- PWM outputs (TIM1 CH1-3 + CH1N-3N) ----
    // PA8  = TIM1_CH1  (AF6) → PWM_A_HIGH
    gpio_set_mode(GPIOA, 8, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 8, 6);
    gpio_set_speed(GPIOA, 8, GPIO_SPEED_VHIGH);

    // PA9  = TIM1_CH2  (AF6) → PWM_B_HIGH
    gpio_set_mode(GPIOA, 9, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 9, 6);
    gpio_set_speed(GPIOA, 9, GPIO_SPEED_VHIGH);

    // PA10 = TIM1_CH3  (AF6) → PWM_C_HIGH
    gpio_set_mode(GPIOA, 10, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 10, 6);
    gpio_set_speed(GPIOA, 10, GPIO_SPEED_VHIGH);

    // PA11 = TIM1_CH1N (AF6) → PWM_A_LOW
    gpio_set_mode(GPIOA, 11, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 11, 6);
    gpio_set_speed(GPIOA, 11, GPIO_SPEED_VHIGH);

    // PB14 = TIM1_CH2N (AF6) → PWM_B_LOW
    gpio_set_mode(GPIOB, 14, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 14, 6);
    gpio_set_speed(GPIOB, 14, GPIO_SPEED_VHIGH);

    // PB15 = TIM1_CH3N (AF6) → PWM_C_LOW
    gpio_set_mode(GPIOB, 15, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 15, 6);
    gpio_set_speed(GPIOB, 15, GPIO_SPEED_VHIGH);

    // ---- ADC inputs ----
    // PC0 = ADC1_IN1 → CUR_A
    gpio_set_mode(GPIOC, 0, GPIO_MODE_ANALOG);
    // PC1 = ADC1_IN2 → CUR_B
    gpio_set_mode(GPIOC, 1, GPIO_MODE_ANALOG);
    // PC2 = ADC1_IN3 → CUR_C
    gpio_set_mode(GPIOC, 2, GPIO_MODE_ANALOG);
    // PC3 = ADC2_IN4 → VBAT_ADC
    gpio_set_mode(GPIOC, 3, GPIO_MODE_ANALOG);

    // ---- SPI1 (CAN FD + Flash) ----
    // PA4 = SPI1_NSS → CAN_CS (manual GPIO)
    gpio_set_mode(GPIOA, 4, GPIO_MODE_OUTPUT);
    gpio_set_speed(GPIOA, 4, GPIO_SPEED_VHIGH);
    GPIOA->BSRR = (1 << 4);  // CS idle high

    // PA5 = SPI1_SCK (AF5)
    gpio_set_mode(GPIOA, 5, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 5, 5);
    gpio_set_speed(GPIOA, 5, GPIO_SPEED_VHIGH);

    // PA6 = SPI1_MISO (AF5)
    gpio_set_mode(GPIOA, 6, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 6, 5);

    // PA7 = SPI1_MOSI (AF5)
    gpio_set_mode(GPIOA, 7, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 7, 5);
    gpio_set_speed(GPIOA, 7, GPIO_SPEED_VHIGH);

    // PB2 = FLASH_CS (GPIO)
    gpio_set_mode(GPIOB, 2, GPIO_MODE_OUTPUT);
    gpio_set_speed(GPIOB, 2, GPIO_SPEED_VHIGH);
    GPIOB->BSRR = (1 << 2);  // CS idle high

    // ---- I2C1 ----
    // PB10 = I2C1_SCL (AF4)
    gpio_set_mode(GPIOB, 10, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 10, 4);
    gpio_set_pull(GPIOB, 10, GPIO_NOPULL);
    gpio_set_speed(GPIOB, 10, GPIO_SPEED_VHIGH);
    gpio_set_mode(GPIOB, 10, GPIO_MODE_AF | (1 << (10*2))); // open-drain

    // PB11 = I2C1_SDA (AF4)
    gpio_set_mode(GPIOB, 11, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 11, 4);
    gpio_set_pull(GPIOB, 11, GPIO_NOPULL);
    gpio_set_speed(GPIOB, 11, GPIO_SPEED_VHIGH);

    // ---- USART2 (BLE) ----
    // PA2 = USART2_TX (AF7)
    gpio_set_mode(GPIOA, 2, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 2, 7);
    gpio_set_speed(GPIOA, 2, GPIO_SPEED_HIGH);

    // PA3 = USART2_RX (AF7)
    gpio_set_mode(GPIOA, 3, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 3, 7);

    // ---- GPIO inputs ----
    // PC13 = STATUS_LED (output, active low)
    gpio_set_mode(GPIOC, 13, GPIO_MODE_OUTPUT);
    gpio_set_speed(GPIOC, 13, GPIO_SPEED_LOW);
    GPIOC->BSRR = (1 << (13+16));  // LED off (active low, set high)

    // PC14 = FAULT_LED (output, active low)
    gpio_set_mode(GPIOC, 14, GPIO_MODE_OUTPUT);
    GPIOC->BSRR = (1 << (14+16));

    // PC15 = HALL_A_IN (input, pull-up)
    gpio_set_mode(GPIOC, 15, GPIO_MODE_INPUT);
    gpio_set_pull(GPIOC, 15, GPIO_PULLUP);

    // PB5 = HALL_B_IN
    gpio_set_mode(GPIOB, 5, GPIO_MODE_INPUT);
    gpio_set_pull(GPIOB, 5, GPIO_PULLUP);

    // PB6 = HALL_C_IN
    gpio_set_mode(GPIOB, 6, GPIO_MODE_INPUT);
    gpio_set_pull(GPIOB, 6, GPIO_PULLUP);

    // PB0 = GATE_EN (output, default LOW = disabled)
    gpio_set_mode(GPIOB, 0, GPIO_MODE_OUTPUT);
    gpio_set_speed(GPIOB, 0, GPIO_SPEED_LOW);

    // PB1 = DESAT_FAULT (input, pull-up)
    gpio_set_mode(GPIOB, 1, GPIO_MODE_INPUT);
    gpio_set_pull(GPIOB, 1, GPIO_PULLUP);

    // PB7 = PMIC_IRQ (input, pull-up)
    gpio_set_mode(GPIOB, 7, GPIO_MODE_INPUT);
    gpio_set_pull(GPIOB, 7, GPIO_PULLUP);

    // PB4 = BLE_RST (output, default high = run)
    gpio_set_mode(GPIOB, 4, GPIO_MODE_OUTPUT);
    GPIOB->BSRR = (1 << 4);  // HIGH = nRF running

    // PA15 = FLASH_WP (output, high = write-protect off)
    gpio_set_mode(GPIOA, 15, GPIO_MODE_OUTPUT);
    GPIOA->BSRR = (1 << 15);  // WP off

    // PB3 = FLASH_HOLD (output, high = hold off)
    gpio_set_mode(GPIOB, 3, GPIO_MODE_OUTPUT);
    GPIOB->BSRR = (1 << 3);
}

void PWM_Output_Enable(void) {
    // Enable TIM1 outputs via CCER and BDTR MOE bit
    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1NE |
                  TIM_CCER_CC2E | TIM_CCER_CC2NE |
                  TIM_CCER_CC3E | TIM_CCER_CC3NE;
    TIM1->BDTR |= TIM_BDTR_MOE;  // Main Output Enable
}
```

## 5. Device Driver: I2C PMIC (TPS6521801)

```c
// ============================================================================
// drivers/pmic.h — TPS6521801 PMIC Driver
// ============================================================================

#ifndef PMIC_H
#define PMIC_H

#include <stdint.h>

// TPS6521801 I2C slave address (7-bit)
#define PMIC_I2C_ADDR       0x24

// Register map
#define PMIC_REG_CHIPID     0x00
#define PMIC_REG_INT1       0x01
#define PMIC_REG_INT2       0x02
#define PMIC_REG_INT_MASK1  0x03
#define PMIC_REG_INT_MASK2  0x04
#define PMIC_REG_STATUS     0x05
#define PMIC_REG_CTRL       0x06
#define PMIC_REG_PGOOD      0x07
#define PMIC_REG_SEQ        0x08
#define PMIC_REG_DCDC1_VSEL 0x10  // 5V DCDC
#define PMIC_REG_DCDC2_VSEL 0x11  // 3.3V DCDC (or LDO)
#define PMIC_REG_DCDC3_VSEL 0x12
#define PMIC_REG_LDO1_VSEL  0x20  // 3.3V LDO
#define PMIC_REG_LDO2_VSEL  0x21  // 1.8V LDO
#define PMIC_REG_LDO3_VSEL  0x22
#define PMIC_REG_LDO4_VSEL  0x23
#define PMIC_REG_DCDC_CTRL  0x30
#define PMIC_REG_LDO_CTRL   0x31
#define PMIC_REG_PASSWORD   0x3F  // Unlock for writes

// Power rail IDs
#define PMIC_RAIL_DCDC1    0  // 5V
#define PMIC_RAIL_DCDC2    1
#define PMIC_RAIL_LDO1     4  // 3.3V
#define PMIC_RAIL_LDO2     5  // 1.8V

typedef enum {
    PMIC_OK = 0,
    PMIC_I2C_ERROR,
    PMIC_TIMEOUT,
    PMIC_FAULT
} pmic_status_t;

void pmic_init(void);
pmic_status_t pmic_enable_rail(uint8_t rail);
pmic_status_t pmic_disable_rail(uint8_t rail);
pmic_status_t pmic_set_voltage(uint8_t rail, uint16_t mv);
uint16_t pmic_get_voltage(uint8_t rail);
uint8_t pmic_get_status(void);
uint8_t pmic_get_pgood(void);
void pmic_clear_faults(void);

#endif // PMIC_H
```

```c
// ============================================================================
// drivers/pmic.c — TPS6521801 PMIC Driver Implementation
// ============================================================================

#include "pmic.h"
#include "registers.h"
#include "board.h"

#define I2C_TIMEOUT_MS  100

static volatile uint32_t i2c_tick = 0;

static uint32_t get_tick(void) {
    // Use SysTick or DWT cycle counter
    return i2c_tick;
}

static int i2c_write(uint8_t dev_addr, uint8_t reg, uint8_t val) {
    uint32_t timeout = get_tick() + I2C_TIMEOUT_MS;

    // Wait for bus ready
    while (I2C1->ISR & I2C_ISR_BUSY)
        if (get_tick() > timeout) return -1;

    // Set slave address, write 2 bytes (reg + val)
    I2C1->CR2 = (dev_addr << 1) | (2 << I2C_CR2_NBYTES_Pos) | I2C_CR2_START | I2C_CR2_AUTOEND;

    // Wait TXIS or NACK
    while (!(I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF)))
        if (get_tick() > timeout) return -1;

    if (I2C1->ISR & I2C_ISR_NACKF) {
        I2C1->ICR = I2C_ICR_NACKCF;
        return -1;
    }

    I2C1->TXDR = reg;

    while (!(I2C1->ISR & I2C_ISR_TXIS))
        if (get_tick() > timeout) return -1;

    I2C1->TXDR = val;

    // Wait STOP
    while (!(I2C1->ISR & I2C_ISR_STOPF))
        if (get_tick() > timeout) return -1;

    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}

static int i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *val) {
    uint32_t timeout = get_tick() + I2C_TIMEOUT_MS;

    while (I2C1->ISR & I2C_ISR_BUSY)
        if (get_tick() > timeout) return -1;

    // Phase 1: Write register address (1 byte, no stop = RELOAD)
    I2C1->CR2 = (dev_addr << 1) | (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXIS))
        if (get_tick() > timeout) return -1;
    I2C1->TXDR = reg;

    // Wait TC (transfer complete, not autoend)
    while (!(I2C1->ISR & I2C_ISR_TC))
        if (get_tick() > timeout) return -1;

    // Phase 2: Read 1 byte with re-start and stop
    I2C1->CR2 = (dev_addr << 1) | I2C_CR2_RD_WRN | (1 << I2C_CR2_NBYTES_Pos) |
                I2C_CR2_START | I2C_CR2_AUTOEND;

    while (!(I2C1->ISR & I2C_ISR_RXNE))
        if (get_tick() > timeout) return -1;

    *val = (uint8_t)I2C1->RXDR;

    while (!(I2C1->ISR & I2C_ISR_STOPF))
        if (get_tick() > timeout) return -1;

    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}

void pmic_init(void) {
    // Configure I2C1 timing for 400 kHz @ 170 MHz
    // PRESC=4, SCLDEL=0xC, SDADEL=0x2, SCLH=0xC, SCLL=0x18
    I2C1->TIMINGR = (4 << 28) | (0xC << 20) | (0x2 << 16) | (0xC << 8) | 0x18;
    I2C1->CR1 = I2C_CR1_PE;  // Enable peripheral

    // Read chip ID to verify
    uint8_t chipid;
    if (i2c_read(PMIC_I2C_ADDR, PMIC_REG_CHIPID, &chipid) == 0) {
        // Expected: 0x80 or similar for TPS65218
    }
}

pmic_status_t pmic_enable_rail(uint8_t rail) {
    uint8_t ctrl_val;
    if (i2c_read(PMIC_I2C_ADDR, PMIC_REG_DCDC_CTRL, &ctrl_val) < 0)
        return PMIC_I2C_ERROR;
    ctrl_val |= (1 << rail);
    if (i2c_write(PMIC_I2C_ADDR, PMIC_REG_DCDC_CTRL, ctrl_val) < 0)
        return PMIC_I2C_ERROR;
    return PMIC_OK;
}

pmic_status_t pmic_disable_rail(uint8_t rail) {
    uint8_t ctrl_val;
    if (i2c_read(PMIC_I2C_ADDR, PMIC_REG_DCDC_CTRL, &ctrl_val) < 0)
        return PMIC_I2C_ERROR;
    ctrl_val &= ~(1 << rail);
    if (i2c_write(PMIC_I2C_ADDR, PMIC_REG_DCDC_CTRL, ctrl_val) < 0)
        return PMIC_I2C_ERROR;
    return PMIC_OK;
}

pmic_status_t pmic_set_voltage(uint8_t rail, uint16_t mv) {
    // Map rail to VSEL register
    uint8_t vsel_reg;
    if (rail == PMIC_RAIL_DCDC1)      vsel_reg = PMIC_REG_DCDC1_VSEL;
    else if (rail == PMIC_RAIL_DCDC2) vsel_reg = PMIC_REG_DCDC2_VSEL;
    else if (rail == PMIC_RAIL_LDO1)  vsel_reg = PMIC_REG_LDO1_VSEL;
    else if (rail == PMIC_RAIL_LDO2)  vsel_reg = PMIC_REG_LDO2_VSEL;
    else return PMIC_I2C_ERROR;

    // TPS65218 VSEL: 0x80 = enable, bits[6:0] = voltage step
    // Step = 50 mV, base = 700 mV for DCDC1
    uint8_t vsel = 0x80 | ((mv - 700) / 50) & 0x7F;
    if (i2c_write(PMIC_I2C_ADDR, vsel_reg, vsel) < 0)
        return PMIC_I2C_ERROR;
    return PMIC_OK;
}

uint16_t pmic_get_voltage(uint8_t rail) {
    uint8_t vsel_reg;
    if (rail == PMIC_RAIL_DCDC1)      vsel_reg = PMIC_REG_DCDC1_VSEL;
    else if (rail == PMIC_RAIL_DCDC2) vsel_reg = PMIC_REG_DCDC2_VSEL;
    else if (rail == PMIC_RAIL_LDO1)  vsel_reg = PMIC_REG_LDO1_VSEL;
    else if (rail == PMIC_RAIL_LDO2)  vsel_reg = PMIC_REG_LDO2_VSEL;
    else return 0;

    uint8_t val;
    if (i2c_read(PMIC_I2C_ADDR, vsel_reg, &val) < 0) return 0;
    return 700 + (val & 0x7F) * 50;  // mV
}

uint8_t pmic_get_status(void) {
    uint8_t val;
    i2c_read(PMIC_I2C_ADDR, PMIC_REG_STATUS, &val);
    return val;
}

uint8_t pmic_get_pgood(void) {
    uint8_t val;
    i2c_read(PMIC_I2C_ADDR, PMIC_REG_PGOOD, &val);
    return val;
}

void pmic_clear_faults(void) {
    uint8_t int1, int2;
    i2c_read(PMIC_I2C_ADDR, PMIC_REG_INT1, &int1);
    i2c_read(PMIC_I2C_ADDR, PMIC_REG_INT2, &int2);
    // Reading INT registers clears them on TPS65218
}
```

## 6. Device Driver: SPI CAN FD (MCP2518FD) with DMA

```c
// ============================================================================
// drivers/canfd.h — MCP2518FD CAN FD Controller Driver
// ============================================================================

#ifndef CANFD_H
#define CANFD_H

#include <stdint.h>

// MCP2518FD SPI opcodes
#define MCP2518_SPI_RESET       0x00
#define MCP2518_SPI_WRITE       0x02
#define MCP2518_SPI_READ        0x03
#define MCP2518_SPI_WRITE_CRC   0x12
#define MCP2518_SPI_READ_CRC    0x13

// MCP2518FD register addresses
#define MCP2518_REG_CON         0x000  // Configuration
#define MCP2518_REG_NBTCFG      0x004  // Nominal bit timing
#define MCP2518_REG_DBTCFG     0x008  // Data bit timing (FD)
#define MCP2518_REG_TSCON      0x00C  // Timestamp
#define MCP2518_REG_IOCON      0x010  // I/O control
#define MCP2518_REG_INT        0x01C  // Interrupt flags
#define MCP2518_REG_INTEN      0x020  // Interrupt enable
#define MCP2518_REG_ECCCON     0x028  // ECC (RAM)
#define MCP2518_REG_TXQCON     0x050  // TX queue config
#define MCP2518_REG_TXQUA      0x054  // TX queue user address
#define MCP2518_REG_FIFOCON1   0x058  // RX FIFO 1 config
#define MCP2518_REG_FIFOCON2   0x060  // TX FIFO config
#define MCP2518_REG_FIFOUA1    0x05C  // FIFO 1 user address
#define MCP2518_REG_FIFOUA2    0x064  // FIFO 2 user address
#define MCP2518_REG_CiFLIFOCON(i) (0x050 + (i * 0x08))

// CAN frame structure
typedef struct {
    uint32_t id;           // 11-bit or 29-bit ID
    uint8_t  dlc;          // Data length code (0-8, or FD: 12/16/20/32/48/64)
    uint8_t  data[64];     // Payload (up to 64 bytes in FD mode)
    uint8_t  ide;          // 0=standard, 1=extended
    uint8_t  fdf;          // 0=classic, 1=FD
    uint8_t  brs;          // Bit rate switch
    uint8_t  esi;          // Error state indicator
} canfd_frame_t;

// CAN FD bit timing for 5 Mbps data phase
// With 20 MHz oscillator:
//   Nominal: SJW=16, BRP=1, TSEG1=63, TSEG2=16 → 500 kbps
//   Data:    SJW=4, BRP=1, TSEG1=7, TSEG2=2 → 5 Mbps
#define CAN_NOM_BRP      1
#define CAN_NOM_TSEG1    63
#define CAN_NOM_TSEG2    16
#define CAN_NOM_SJW      16
#define CAN_DATA_BRP     1
#define CAN_DATA_TSEG1   7
#define CAN_DATA_TSEG2  2
#define CAN_DATA_SJW     4

// RideCore CAN message IDs
#define CAN_MSG_READY      0x100
#define CAN_MSG_STATUS     0x101
#define CAN_MSG_CURRENTS    0x102
#define CAN_MSG_VOLTAGES    0x103
#define CAN_MSG_TEMPS       0x104
#define CAN_MSG_FAULT       0x10F
#define CAN_MSG_CMD_THROTTLE 0x200
#define CAN_MSG_CMD_BRAKE   0x201
#define CAN_MSG_CONFIG      0x300

void canfd_init(void);
int canfd_tx(const canfd_frame_t *frame);
int canfd_rx(canfd_frame_t *frame);
uint32_t canfd_get_int_flags(void);
void canfd_clear_int_flags(uint32_t flags);
void canfd_set_filter(uint8_t fid, uint32_t mask, uint32_t id);

// DMA-based SPI transfer for bulk CAN FIFO reads
void canfd_spi_dma_init(void);
int canfd_rx_dma(canfd_frame_t *frames, uint8_t max_count, uint8_t *rx_count);

#endif // CANFD_H
```

```c
// ============================================================================
// drivers/canfd.c — MCP2518FD CAN FD Controller Driver with DMA
// ============================================================================

#include "canfd.h"
#include "registers.h"
#include "board.h"
#include <string.h>

// ---- SPI1 low-level ----
static void spi1_cs_can_low(void) {
    GPIOA->BRR = (1 << 4);   // PA4 low
}

static void spi1_cs_can_high(void) {
    GPIOA->BSRR = (1 << 4);  // PA4 high
}

static uint8_t spi1_transfer(uint8_t tx_byte) {
    // Wait TX empty
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = tx_byte;
    // Wait RX ready
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)SPI1->DR;
}

static void spi1_write_reg(uint16_t addr, uint32_t val, uint8_t len) {
    spi1_cs_can_low();
    spi1_transfer(MCP2518_SPI_WRITE);
    spi1_transfer((addr >> 8) & 0xFF);
    spi1_transfer(addr & 0xFF);
    for (int i = 0; i < len; i++) {
        spi1_transfer((val >> (i*8)) & 0xFF);
    }
    spi1_cs_can_high();
}

static uint32_t spi1_read_reg(uint16_t addr, uint8_t len) {
    uint32_t val = 0;
    spi1_cs_can_low();
    spi1_transfer(MCP2518_SPI_READ);
    spi1_transfer((addr >> 8) & 0xFF);
    spi1_transfer(addr & 0xFF);
    for (int i = 0; i < len; i++) {
        val |= (uint32_t)spi1_transfer(0xFF) << (i*8);
    }
    spi1_cs_can_high();
    return val;
}

// ---- Driver API ----

void canfd_init(void) {
    // 1. Hardware reset
    GPIOB->BRR = (1 << 13);   // CAN_RST low
    for (volatile int i = 0; i < 10000; i++);
    GPIOB->BSRR = (1 << 13);  // CAN_RST high
    for (volatile int i = 0; i < 50000; i++);

    // 2. SPI software reset
    spi1_cs_can_low();
    spi1_transfer(MCP2518_SPI_RESET);
    spi1_cs_can_high();
    for (volatile int i = 0; i < 100000; i++);

    // 3. Configure CON register: enable CAN FD, request config mode
    spi1_write_reg(MCP2518_REG_CON,
        (1 << 31) |  // OPMOD = Configuration (0b100)
        (1 << 23) |  // CLKODIS = 1 (disable clock output)
        (0 << 15),   // SCLK2X = 0
        4);

    // 4. Nominal bit timing (500 kbps with 20 MHz clock)
    //   NBTCFG: BRP=0 (div1), TSEG1=63, TSEG2=16, SJW=16
    uint32_t nbtcfg = ((CAN_NOM_SJW - 1) << 24) |
                      ((CAN_NOM_TSEG1 - 1) << 8) |
                      ((CAN_NOM_BRP - 1) << 0) |
                      ((CAN_NOM_TSEG2 - 1) << 16);
    spi1_write_reg(MCP2518_REG_NBTCFG, nbtcfg, 4);

    // 5. Data bit timing (5 Mbps FD)
    //   DBTCFG: BRP=0, TSEG1=7, TSEG2=2, SJW=4
    uint32_t dbtcfg = ((CAN_DATA_SJW - 1) << 24) |
                      ((CAN_DATA_TSEG1 - 1) << 16) |
                      ((CAN_DATA_TSEG2 - 1) << 8) |
                      ((CAN_DATA_BRP - 1) << 0);
    spi1_write_reg(MCP2518_REG_DBTCFG, dbtcfg, 4);

    // 6. Configure TX queue (FIFO 2): 6 messages, priority=0, TX request
    spi1_write_reg(MCP2518_REG_FIFOCON2,
        (0 << 24) |   // Priority
        (0 << 21) |   // TX enable
        (5 << 16) |   // FIFO depth = 6 (5+1)
        (1 << 26),    // TXEVIE = enable TX event interrupt
        4);

    // 7. Configure RX FIFO 1: 8 messages, overflow to FIFO 2
    spi1_write_reg(MCP2518_REG_FIFOCON1,
        (7 << 16) |   // FIFO depth = 8 (7+1)
        (1 << 25),    // RX overflow interrupt
        4);

    // 8. Interrupt enables
    spi1_write_reg(MCP2518_REG_INTEN,
        (1 << 0) |    // TX FIFO interrupt
        (1 << 1) |    // RX FIFO 1 interrupt
        (1 << 11),    // Bus error interrupt
        4);

    // 9. Switch to Normal mode (CAN FD enabled)
    uint32_t con = spi1_read_reg(MCP2518_REG_CON, 4);
    con &= ~(7 << 24);          // Clear OPMOD
    con |= (0 << 24);           // OPMOD = Normal
    con |= (1 << 12);           // CANFD = enable FD
    con |= (1 << 11);           // BRSDIS = 0 (enable BRS)
    spi1_write_reg(MCP2518_REG_CON, con, 4);

    // 10. Wait for mode change
    for (volatile int i = 0; i < 100000; i++);
}

int canfd_tx(const canfd_frame_t *frame) {
    // Write frame to TX FIFO via RAM region (0x400+)
    // Simplified: write header + data
    uint32_t header = (frame->id & 0x1FFFFFFF) << 1;
    if (frame->ide) header |= (1 << 0);   // IDE
    header |= (frame->fdf ? (1 << 1) : 0); // FDF
    header |= (frame->brs ? (1 << 2) : 0); // BRS
    header |= (frame->esi ? (1 << 3) : 0); // ESI

    // DLC encoding
    uint8_t dlc_enc = frame->dlc;
    if (frame->dlc > 8) {
        // FD DLC encoding: 9→12, 10→16, 11→20, 12→32, 13→48, 14→64
        const uint8_t fd_dlc[] = {0,1,2,3,4,5,6,7,8,9,9,9,9,9,9,9};
        dlc_enc = fd_dlc[frame->dlc > 15 ? 15 : frame->dlc];
    }
    uint32_t header2 = (dlc_enc & 0xF);

    // Write to TX RAM area (FIFO 2 base = 0x400 + offset)
    uint16_t ram_addr = 0x400; // TX FIFO base (simplified)
    spi1_write_reg(ram_addr, header, 4);
    spi1_write_reg(ram_addr + 4, header2, 1);

    // Write data bytes
    for (int i = 0; i < frame->dlc && i < 64; i++) {
        spi1_write_reg(ram_addr + 8 + i, frame->data[i], 1);
    }

    // Trigger TX: set TXREQ in FIFOCON2
    uint32_t fifocon = spi1_read_reg(MCP2518_REG_FIFOCON2, 4);
    fifocon |= (1 << 26);  // TXREQ
    spi1_write_reg(MCP2518_REG_FIFOCON2, fifocon, 4);

    return 0;
}

int canfd_rx(canfd_frame_t *frame) {
    // Check RX FIFO 1 status
    uint32_t fifocon = spi1_read_reg(MCP2518_REG_FIFOCON1, 4);
    if (fifocon & (1 << 0)) {  // FIFO not empty
        // Read from FIFO 1 user address
        uint32_t ua = spi1_read_reg(MCP2518_REG_FIFOUA1, 4);
        uint16_t ram_addr = (uint16_t)(ua & 0xFFF);

        uint32_t hdr = spi1_read_reg(ram_addr, 4);
        uint32_t hdr2 = spi1_read_reg(ram_addr + 4, 4);

        frame->id = (hdr >> 1) & 0x1FFFFFFF;
        frame->ide = (hdr >> 0) & 1;
        frame->fdf = (hdr >> 1) & 1;
        frame->brs = (hdr >> 2) & 1;
        frame->esi = (hdr >> 3) & 1;
        frame->dlc = hdr2 & 0xF;

        // Read data
        for (int i = 0; i < frame->dlc && i < 64; i++) {
            frame->data[i] = (uint8_t)spi1_read_reg(ram_addr + 8 + i, 1);
        }

        // Increment FIFO
        fifocon = spi1_read_reg(MCP2518_REG_FIFOCON1, 4);
        fifocon |= (1 << 0); // UINC
        spi1_write_reg(MCP2518_REG_FIFOCON1, fifocon, 4);

        return 0;
    }
    return -1;  // No message
}

uint32_t canfd_get_int_flags(void) {
    return spi1_read_reg(MCP2518_REG_INT, 4);
}

void canfd_clear_int_flags(uint32_t flags) {
    spi1_write_reg(MCP2518_REG_INT, flags, 4);
}

void canfd_set_filter(uint8_t fid, uint32_t mask, uint32_t id) {
    // Configure filter object in MCP2518FD filter RAM
    uint16_t base = 0x1C0 + (fid * 0x08);
    spi1_write_reg(base, mask, 4);
    spi1_write_reg(base + 4, id, 4);
}

// ---- DMA-based bulk CAN read ----

static volatile uint8_t dma_rx_buffer[256];
static volatile uint8_t dma_busy = 0;

void canfd_spi_dma_init(void) {
    // Configure DMA1 Channel 3 for SPI1_RX (MISO → RAM)
    // DMA1 Channel 2 for SPI1_TX (RAM → MOSI)
    
    // Enable DMA clocks (already done in Peripheral_Clock_Enable)
    
    // SPI1 RX: DMA1 Channel 2 (SPI1_RX on G4)
    DMA1_Channel2->CPAR = (uint32_t)&SPI1->DR;
    DMA1_Channel2->CMAR = (uint32_t)dma_rx_buffer;
    DMA1_Channel2->CCR = DMA_CCR_MINC |   // Memory increment
                         DMA_CCR_CIRC;     // Circular (optional)
    
    // Enable SPI1 DMA requests
    SPI1->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
}

int canfd_rx_dma(canfd_frame_t *frames, uint8_t max_count, uint8_t *rx_count) {
    // Bulk read from RX FIFO via DMA
    // 1. Determine number of messages pending
    uint32_t fifocon = spi1_read_reg(MCP2518_REG_FIFOCON1, 4);
    uint8_t pending = (fifocon >> 25) & 0x1F;  // FIFOCI (message count)
    
    if (pending == 0) {
        *rx_count = 0;
        return 0;
    }
    
    uint8_t to_read = (pending > max_count) ? max_count : pending;
    
    // 2. Configure DMA for bulk SPI transfer
    //    Each CAN FD frame in RAM is 72 bytes (header + 64 data + status)
    uint16_t total_bytes = to_read * 72;
    
    DMA1_Channel2->CNDTR = total_bytes;
    DMA1_Channel2->CMAR = (uint32_t)dma_rx_buffer;
    DMA1_Channel2->CCR |= DMA_CCR_EN;
    
    // 3. Initiate SPI read from FIFO user address
    uint32_t ua = spi1_read_reg(MCP2518_REG_FIFOUA1, 4);
    
    dma_busy = 1;
    spi1_cs_can_low();
    spi1_transfer(MCP2518_SPI_READ);
    spi1_transfer(((ua >> 8) & 0xFF));
    spi1_transfer((ua & 0xFF));
    
    // Dummy clock out bytes, DMA captures MISO
    for (uint16_t i = 0; i < total_bytes; i++) {
        spi1_transfer(0xFF);
    }
    
    spi1_cs_can_high();
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    dma_busy = 0;
    
    // 4. Parse received frames
    for (int i = 0; i < to_read; i++) {
        uint16_t offset = i * 72;
        uint32_t hdr = dma_rx_buffer[offset] | (dma_rx_buffer[offset+1] << 8) |
                       (dma_rx_buffer[offset+2] << 16) | (dma_rx_buffer[offset+3] << 24);
        frames[i].id = (hdr >> 1) & 0x1FFFFFFF;
        frames[i].dlc = dma_rx_buffer[offset + 4] & 0xF;
        for (int j = 0; j < frames[i].dlc && j < 64; j++) {
            frames[i].data[j] = dma_rx_buffer[offset + 8 + j];
        }
    }
    
    // 5. Advance FIFO pointer
    for (int i = 0; i < to_read; i++) {
        fifocon = spi1_read_reg(MCP2518_REG_FIFOCON1, 4);
        fifocon |= (1 << 0); // UINC
        spi1_write_reg(MCP2518_REG_FIFOCON1, fifocon, 4);
    }
    
    *rx_count = to_read;
    return 0;
}
```

## 7. Device Tree Overlay

```dts
// ridecore-overlay.dts — RideCore Motor Controller Device Tree Overlay
// Target: STM32G474 (though bare-metal, this documents the HW map)

/dts-v1/;
/plugin/;

/ {
    compatible = "st,stm32g474";
    
    fragment@0 {
        target = <&spi1>;
        __overlay__ {
            status = "okay";
            pinctrl-0 = <&spi1_pins>;
            cs-gpios = <&gpioa 4 0>, <&gpiob 2 0>;
            
            canfd: mcp2518fd@0 {
                compatible = "microchip,mcp2518fd";
                reg = <0>;
                spi-max-frequency = <20000000>;
                interrupts = <&gpiob 12 IRQ_TYPE_LEVEL_LOW>;
                reset-gpios = <&gpiob 13 GPIO_ACTIVE_LOW>;
                clock-frequency = <0>;  /* internal oscillator */
            };
            
            spiflash: w25q128@1 {
                compatible = "winbond,w25q128";
                reg = <1>;
                spi-max-frequency = <20000000>;
                wp-gpios = <&gpioa 15 GPIO_ACTIVE_HIGH>;
                hold-gpios = <&gpiob 3 GPIO_ACTIVE_HIGH>;
            };
        };
    };
    
    fragment@1 {
        target = <&i2c1>;
        __overlay__ {
            status = "okay";
            pinctrl-0 = <&i2c1_pins>;
            
            pmic: tps65218@24 {
                compatible = "ti,tps6521801";
                reg = <0x24>;
                interrupts = <&gpiob 7 IRQ_TYPE_LEVEL_LOW>;
            };
            
            temp: at30ts74@48 {
                compatible = "microchip,at30ts74";
                reg = <0x48>;
            };
        };
    };
    
    fragment@2 {
        target = <&usart2>;
        __overlay__ {
            status = "okay";
            pinctrl-0 = <&usart2_pins>;
            current-speed = <1000000>;
            
            ble: nrf52832 {
                compatible = "nordic,nrf52832-uart";
                reset-gpios = <&gpiob 4 GPIO_ACTIVE_LOW>;
            };
        };
    };
    
    fragment@3 {
        target = <&tim1>;
        __overlay__ {
            status = "okay";
            pinctrl-0 = <&pwm_pins>;
            st,pwm-mode = "center-aligned-1";
            st,pwm-freq = <20000>;
            st,dead-time-ns = <500>;
        };
    };
    
    fragment@4 {
        target = <&adc1>;
        __overlay__ {
            status = "okay";
            pinctrl-0 = <&adc_pins>;
            st,adc-channels = <1 2 3 4>;
            st,adc-resolution = <12>;
            st,adc-oversampling = <4>;
        };
    };
    
    fragment@5 {
        target-path = "/";
        __overlay__ {
            motor-controller {
                compatible = "ridecore,pmsm-foc";
                gate-en-gpios = <&gpiob 0 GPIO_ACTIVE_HIGH>;
                desat-fault-gpios = <&gpiob 1 GPIO_ACTIVE_LOW>;
                hall-a-gpios = <&gpioc 15 GPIO_ACTIVE_HIGH>;
                hall-b-gpios = <&gpiob 5 GPIO_ACTIVE_HIGH>;
                hall-c-gpios = <&gpiob 6 GPIO_ACTIVE_HIGH>;
                status-led-gpios = <&gpioc 13 GPIO_ACTIVE_LOW>;
                fault-led-gpios = <&gpioc 14 GPIO_ACTIVE_LOW>;
                
                num-pole-pairs = <7>;
                phase-resistance-mohm = <50>;
                phase-inductance-uh = <100>;
                max-current-ma = <200000>;
                cont-current-ma = <120000>;
                pwm-freq-hz = <20000>;
                dead-time-ns = <500>;
            };
        };
    };
};
```

## 8. Build Instructions

### 8.1 Prerequisites

- **Toolchain:** `arm-none-eabi-gcc` (GNU Arm Embedded Toolchain 12.3 or later)
- **Flasher:** OpenOCD 0.12+ or ST-Link CLI
- **Make:** GNU Make 4.3+
- **Python 3:** For code generation scripts (optional)

### 8.2 Build Firmware

```bash
cd firmware
make clean
make all
# Output: build/ridecore.bin, build/ridecore.hex, build/ridecore.elf
```

### 8.3 Flash via ST-Link

```bash
openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
    -c "program build/ridecore.elf verify reset exit"
```

### 8.4 Flash via USB DFU (bootloader mode)

```bash
dfu-util -a 0 -s 0x08000000:leave -D build/ridecore.bin
```

### 8.5 Build Companion App

```bash
cd app
npm install
npx react-native run-android   # or run-ios
```

### 8.6 Serial Console

Connect USB-C or UART (115200 8N1):
```
> ridecore info
  FW: 1.0.0  MCU: STM32G474 @ 170 MHz
  Motor: 7pp, 100µH, 50mΩ
  Status: FOC running, 45.2A, 48.1V
> ridecore set current_limit 120000
  OK
> ridecore faults
  No active faults
> ridecore save
  Configuration saved to SPI flash
```