// ============================================================================
// registers.h — RideCore STM32G474 MMIO Register Map
// Full register definitions for all peripherals used
// ============================================================================

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

// ---------- Core Peripherals (Cortex-M4) ----------
#define SCB_BASE            0xE000ED00UL
#define NVIC_BASE           0xE000E100UL
#define SysTick_BASE        0xE000E010UL
#define SCB_VTOR            (*(volatile uint32_t *)0xE000ED08UL)

typedef struct {
    volatile uint32_t ISER[8];
    volatile uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];
    volatile uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];
    volatile uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];
    volatile uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];
    volatile uint32_t RESERVED4[56];
    volatile uint32_t IPR[60];
} NVIC_TypeDef;

#define NVIC ((NVIC_TypeDef *)NVIC_BASE)

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_TypeDef;

#define SysTick ((SysTick_TypeDef *)SysTick_BASE)

typedef struct {
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint32_t SHPCR;
    volatile uint32_t CFSR;
    volatile uint32_t HFSR;
    volatile uint32_t DFSR;
    volatile uint32_t MMFAR;
    volatile uint32_t BFAR;
} SCB_TypeDef;

#define SCB ((SCB_TypeDef *)SCB_BASE)

// ---------- Peripheral Base Addresses ----------
#define PERIPH_BASE         0x40000000UL
#define APB1PERIPH_BASE     PERIPH_BASE
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE     (PERIPH_BASE + 0x08000000UL)

// ---------- GPIO ----------
#define GPIOA_BASE          (AHB2PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE          (AHB2PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE          (AHB2PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE          (AHB2PERIPH_BASE + 0x0C00UL)

typedef struct {
    volatile uint32_t MODER;     // 0x00
    volatile uint32_t OTYPER;    // 0x04
    volatile uint32_t OSPEEDR;   // 0x08
    volatile uint32_t PUPDR;     // 0x0C
    volatile uint32_t IDR;       // 0x10
    volatile uint32_t ODR;       // 0x14
    volatile uint32_t BSRR;      // 0x18
    volatile uint32_t LCKR;      // 0x1C
    volatile uint32_t AFRL;     // 0x20
    volatile uint32_t AFRH;     // 0x24
    volatile uint32_t BRR;       // 0x28
    volatile uint32_t RESERVED;
    volatile uint32_t ASCR;     // 0x34 (G4 analog switch control)
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)

// GPIO bit set/reset helper
#define GPIO_BS_PIN(port, pin)  ((port)->BSRR = (1 << (pin)))
#define GPIO_BR_PIN(port, pin)  ((port)->BRR = (1 << (pin)))

// ---------- RCC ----------
#define RCC_BASE            (AHB1PERIPH_BASE + 0x0800UL)

typedef struct {
    volatile uint32_t CR;        // 0x00
    volatile uint32_t ICSCR;    // 0x04
    volatile uint32_t CFGR;     // 0x08
    volatile uint32_t PLLCFGR; // 0x0C
    volatile uint32_t PLL1DIVR; // 0x10
    volatile uint32_t PLL1FRACR; // 0x14
    volatile uint32_t PLL2DIVR; // 0x18
    volatile uint32_t PLL2FRACR; // 0x1C
    volatile uint32_t PLL3DIVR; // 0x20
    volatile uint32_t PLL3FRACR; // 0x24
    volatile uint32_t RESERVED[2];
    volatile uint32_t CIER;      // 0x30
    volatile uint32_t CIFR;     // 0x34
    volatile uint32_t CICR;      // 0x38
    volatile uint32_t BDCR;      // 0x3C
    volatile uint32_t CSR;       // 0x40
    volatile uint32_t CRRCR;    // 0x44
    volatile uint32_t CCIPR;    // 0x48
    volatile uint32_t CCIPR2;   // 0x4C
    volatile uint32_t DCKCFGR;  // 0x50
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)RCC_BASE)

// RCC CR bits
#define RCC_CR_HSION      (1 << 0)
#define RCC_CR_HSERDY     (1 << 17)
#define RCC_CR_HSEON      (1 << 16)
#define RCC_CR_PLLON      (1 << 24)
#define RCC_CR_PLLRDY     (1 << 25)

// RCC CFGR bits
#define RCC_CFGR_SW_PLL1  (3 << 0)
#define RCC_CFGR_SWS_PLL1 (3 << 2)
#define RCC_CFGR_HPRE_Pos  4
#define RCC_CFGR_PPRE1_Pos 8
#define RCC_CFGR_PPRE2_Pos 11

// RCC PLLCFGR bits
#define RCC_PLLCFGR_PLLM_Pos   4
#define RCC_PLLCFGR_PLLN_Pos   8
#define RCC_PLLCFGR_PLLR_Pos   28
#define RCC_PLLCFGR_PLLREN     (1 << 24)
#define RCC_PLLCFGR_PLLSRC_Pos 0

// RCC AHB2ENR bits
#define RCC_AHB2ENR_GPIOAEN  (1 << 0)
#define RCC_AHB2ENR_GPIOBEN  (1 << 1)
#define RCC_AHB2ENR_GPIOCEN  (1 << 2)
#define RCC_AHB2ENR_ADC12EN  (1 << 13)

// RCC AHB1ENR bits
#define RCC_AHB1ENR_DMA1EN   (1 << 0)

// RCC APB1ENR bits
#define RCC_APB1ENR_I2C1EN   (1 << 21)
#define RCC_APB1ENR_USART2EN (1 << 17)

// RCC APB2ENR bits
#define RCC_APB2ENR_SPI1EN   (1 << 12)
#define RCC_APB2ENR_TIM1EN   (1 << 11)
#define RCC_APB2ENR_USART1EN (1 << 14)
#define RCC_APB2ENR_SYSCFGEN (1 << 0)

// RCC CRRCR bits
#define RCC_CRRCR_HSI48ON    (1 << 0)
#define RCC_CRRCR_HSI48RDY   (1 << 1)

// ---------- FLASH ----------
#define FLASH_BASE          0x40022000UL
#define FLASH_ACR           (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_ACR_LATENCY_4WS  (4 << 0)
#define FLASH_ACR_PRFTEN      (1 << 8)
#define FLASH_ACR_ICEN        (1 << 9)
#define FLASH_ACR_DCEN        (1 << 10)

// ---------- TIM1 (Advanced Timer - PWM) ----------
#define TIM1_BASE           (APB2PERIPH_BASE + 0x2C00UL)

typedef struct {
    volatile uint32_t CR1;       // 0x00
    volatile uint32_t CR2;       // 0x04
    volatile uint32_t SMCR;      // 0x08
    volatile uint32_t DIER;      // 0x0C
    volatile uint32_t SR;        // 0x10
    volatile uint32_t CCMR1;    // 0x14
    volatile uint32_t CCMR2;    // 0x18
    volatile uint32_t CCER;      // 0x1C
    volatile uint32_t CNT;       // 0x20
    volatile uint32_t PSC;       // 0x24
    volatile uint32_t ARR;       // 0x28
    volatile uint32_t RCR;       // 0x2C
    volatile uint32_t CCR1;     // 0x30
    volatile uint32_t CCR2;     // 0x34
    volatile uint32_t CCR3;     // 0x38
    volatile uint32_t CCR4;     // 0x3C
    volatile uint32_t BDTR;     // 0x40
    volatile uint32_t DCR;       // 0x44
    volatile uint32_t DMAR;     // 0x48
    volatile uint32_t AF1;       // 0x50 (G4)
    volatile uint32_t AF2;       // 0x54 (G4)
    volatile uint32_t TISEL;    // 0x58 (G4)
} TIM_TypeDef;

#define TIM1 ((TIM_TypeDef *)TIM1_BASE)

// TIM1 CR1 bits
#define TIM_CR1_CEN          (1 << 0)
#define TIM_CR1_ARPE         (1 << 7)
#define TIM_CR1_CMS_CENTER1  (1 << 5)  // Center-aligned mode 1

// TIM1 CCER bits
#define TIM_CCER_CC1E        (1 << 0)
#define TIM_CCER_CC1NE       (1 << 2)
#define TIM_CCER_CC2E        (1 << 4)
#define TIM_CCER_CC2NE       (1 << 6)
#define TIM_CCER_CC3E        (1 << 8)
#define TIM_CCER_CC3NE       (1 << 10)

// TIM1 BDTR bits
#define TIM_BDTR_MOE         (1 << 15)
#define TIM_BDTR_OSSR        (1 << 11)
#define TIM_BDTR_OSSI        (1 << 10)

// TIM1 DIER bits
#define TIM_DIER_UIE         (1 << 0)
#define TIM_DIER_COMIE       (1 << 5)

// TIM1 SR bits
#define TIM_SR_UIF           (1 << 0)

// TIM1 CCMR output mode
#define TIM_CCMR1_OC1M_PWM1  (6 << 4)  // PWM mode 1 on CH1
#define TIM_CCMR1_OC2M_PWM1  (6 << 12) // PWM mode 1 on CH2
#define TIM_CCMR1_OC1PE      (1 << 3)
#define TIM_CCMR1_OC2PE      (1 << 11)
#define TIM_CCMR2_OC3M_PWM1  (6 << 4)  // PWM mode 1 on CH3
#define TIM_CCMR2_OC3PE      (1 << 3)

// ---------- ADC ----------
#define ADC1_BASE            (AHB2PERIPH_BASE + 0x0800UL)

typedef struct {
    volatile uint32_t ISR;       // 0x00
    volatile uint32_t IER;       // 0x04
    volatile uint32_t CR;        // 0x08
    volatile uint32_t CFGR;     // 0x0C
    volatile uint32_t CFGR2;    // 0x10
    volatile uint32_t SMPR1;    // 0x14
    volatile uint32_t SMPR2;    // 0x18
    volatile uint32_t RESERVED0[2];
    volatile uint32_t LTR1;     // 0x20
    volatile uint32_t HTR1;     // 0x24
    volatile uint32_t RESERVED1;
    volatile uint32_t OFR1;     // 0x30
    volatile uint32_t OFR2;     // 0x34
    volatile uint32_t OFR3;     // 0x38
    volatile uint32_t OFR4;     // 0x3C
    volatile uint32_t RESERVED2[1];
    volatile uint32_t JSQR;      // 0x40
    volatile uint32_t RESERVED3[2];
    volatile uint32_t DR;        // 0x48
} ADC_TypeDef;

#define ADC1 ((ADC_TypeDef *)ADC1_BASE)

// ADC CR bits
#define ADC_CR_ADEN          (1 << 0)
#define ADC_CR_ADCAL         (1 << 31)
#define ADC_CR_ADSTART       (1 << 2)

// ADC ISR bits
#define ADC_ISR_ADRDY        (1 << 0)
#define ADC_ISR_EOC          (1 << 2)
#define ADC_ISR_EOS          (1 << 3)

// ADC CFGR bits
#define ADC_CFGR_CONT        (1 << 13)
#define ADC_CFGR_EXTEN_RISING (1 << 10)
#define ADC_CFGR_EXTSEL_TIM1 (1 << 5)  // TIM1_TRGO

// ---------- SPI1 ----------
#define SPI1_BASE            (APB2PERIPH_BASE + 0x3000UL)

typedef struct {
    volatile uint32_t CR1;       // 0x00
    volatile uint32_t CR2;       // 0x04
    volatile uint32_t SR;        // 0x08
    volatile uint32_t DR;        // 0x0C
    volatile uint32_t CRCPR;    // 0x10
    volatile uint32_t RXCRCR;   // 0x14
    volatile uint32_t TXCRCR;   // 0x18
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)

// SPI CR1 bits
#define SPI_CR1_SPE          (1 << 6)
#define SPI_CR1_MSTR         (1 << 2)
#define SPI_CR1_BR_DIV8      (2 << 3)
#define SPI_CR1_CPHA         (1 << 0)
#define SPI_CR1_CPOL         (1 << 1)
#define SPI_CR1_LSBFIRST     (1 << 7)
#define SPI_CR1_SSM          (1 << 9)
#define SPI_CR1_SSI          (1 << 8)

// SPI CR2 bits
#define SPI_CR2_DS_8BIT      (7 << 8)
#define SPI_CR2_RXDMAEN      (1 << 0)
#define SPI_CR2_TXDMAEN      (1 << 1)
#define SPI_CR2_SSOE         (1 << 2)

// SPI SR bits
#define SPI_SR_TXE           (1 << 1)
#define SPI_SR_RXNE          (1 << 0)
#define SPI_SR_BSY           (1 << 7)

// ---------- I2C1 ----------
#define I2C1_BASE            (APB1PERIPH_BASE + 0x5400UL)

typedef struct {
    volatile uint32_t CR1;        // 0x00
    volatile uint32_t CR2;        // 0x04
    volatile uint32_t OAR1;      // 0x08
    volatile uint32_t OAR2;      // 0x0C
    volatile uint32_t TIMINGR;   // 0x10
    volatile uint32_t TIMEOUTR;  // 0x14
    volatile uint32_t ISR;        // 0x18
    volatile uint32_t ICR;        // 0x1C
    volatile uint32_t PECR;      // 0x20
    volatile uint32_t RXDR;      // 0x24
    volatile uint32_t TXDR;      // 0x28
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)

// I2C CR1 bits
#define I2C_CR1_PE            (1 << 0)

// I2C CR2 bits
#define I2C_CR2_START         (1 << 13)
#define I2C_CR2_STOP          (1 << 14)
#define I2C_CR2_RD_WRN        (1 << 10)
#define I2C_CR2_AUTOEND       (1 << 25)
#define I2C_CR2_NBYTES_Pos    16

// I2C ISR bits
#define I2C_ISR_TXIS          (1 << 1)
#define I2C_ISR_RXNE          (1 << 2)
#define I2C_ISR_BUSY          (1 << 15)
#define I2C_ISR_TC            (1 << 6)
#define I2C_ISR_NACKF         (1 << 4)
#define I2C_ISR_STOPF         (1 << 5)

// I2C ICR bits
#define I2C_ICR_NACKCF       (1 << 4)
#define I2C_ICR_STOPCF       (1 << 5)

// ---------- USART2 ----------
#define USART2_BASE          (APB1PERIPH_BASE + 0x4400UL)

typedef struct {
    volatile uint32_t CR1;        // 0x00
    volatile uint32_t CR2;        // 0x04
    volatile uint32_t CR3;        // 0x08
    volatile uint32_t BRR;        // 0x0C
    volatile uint32_t GTPR;      // 0x10
    volatile uint32_t RTOR;      // 0x14
    volatile uint32_t RQR;        // 0x18
    volatile uint32_t ISR;        // 0x1C
    volatile uint32_t ICR;        // 0x20
    volatile uint32_t RDR;        // 0x24
    volatile uint32_t TDR;        // 0x28
} USART_TypeDef;

#define USART2 ((USART_TypeDef *)USART2_BASE)

// USART CR1 bits
#define USART_CR1_UE          (1 << 0)
#define USART_CR1_TE          (1 << 3)
#define USART_CR1_RE          (1 << 2)
#define USART_CR1_RXNEIE      (1 << 5)
#define USART_CR1_TXEIE       (1 << 7)

// USART ISR bits
#define USART_ISR_TXE         (1 << 7)
#define USART_ISR_RXNE        (1 << 5)
#define USART_ISR_TC          (1 << 6)

// ---------- DMA1 ----------
#define DMA1_BASE             (AHB1PERIPH_BASE + 0x0000UL)
#define DMA1_Channel1_BASE    (DMA1_BASE + 0x0008UL)

typedef struct {
    volatile uint32_t CCR;        // 0x00
    volatile uint32_t CNDTR;     // 0x04
    volatile uint32_t CPAR;      // 0x08
    volatile uint32_t CMAR;      // 0x0C
} DMA_Channel_TypeDef;

#define DMA1_Channel1 ((DMA_Channel_TypeDef *)DMA1_Channel1_BASE)
#define DMA1_Channel2 ((DMA_Channel_TypeDef *)(DMA1_Channel1_BASE + 0x14UL))
#define DMA1_Channel3 ((DMA_Channel_TypeDef *)(DMA1_Channel1_BASE + 0x28UL))

// DMA CCR bits
#define DMA_CCR_EN            (1 << 0)
#define DMA_CCR_MINC          (1 << 10)
#define DMA_CCR_CIRC          (1 << 8)
#define DMA_CCR_DIR           (1 << 4)  // 0 = periph-to-mem, 1 = mem-to-periph

// ---------- NVIC Interrupt Numbers ----------
#define TIM1_UP_IRQn          24
#define TIM1_TRG_COM_IRQn     25
#define ADC1_2_IRQn           18
#define SPI1_IRQn             35
#define I2C1_EV_IRQn          31
#define USART2_IRQn           38
#define DMA1_Channel2_IRQn    12
#define DMA1_Channel3_IRQn    13

// ---------- NVIC Helpers ----------
static inline void NVIC_EnableIRQ(int irqn) {
    NVIC->ISER[irqn >> 5] = (1 << (irqn & 0x1F));
}

static inline void NVIC_SetPriority(int irqn, uint32_t priority) {
    NVIC->IPR[irqn] = (priority << 4) & 0xFF;
}

#endif // REGISTERS_H