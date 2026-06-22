/*
 * registers.h — STM32WB55CGU6 Peripheral Register Definitions
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Low-level register address definitions and bit-field constants for the
 * STM32WB55CGU6 used in LumiCast.  This is not a complete SVD replacement —
 * it covers only the peripherals the firmware actually touches.  Register
 * base addresses follow the STM32WB55 Reference Manual (RM0434, Rev 5).
 */

#ifndef LUMICAST_REGISTERS_H
#define LUMICAST_REGISTERS_H

#include <stdint.h>

/* ===================================================================== */
/*  Memory map bus bases                                                  */
/* ===================================================================== */

#define PERIPH_BASE     0x40000000U
#define PERIPH_BASE2    0x48000000U

#define AHB1PERIPH_BASE  (PERIPH_BASE + 0x00020000U)
#define AHB2PERIPH_BASE  (PERIPH_BASE + 0x08000000U)
#define AHB3PERIPH_BASE  (PERIPH_BASE2 + 0x00000000U)
#define APB1PERIPH_BASE  PERIPH_BASE
#define APB2PERIPH_BASE  (PERIPH_BASE + 0x00010000U)

/* ===================================================================== */
/*  Cortex-M4 system control                                              */
/* ===================================================================== */

#define SCB_BASE        0xE000ED00U
#define SCB_VTOR        (*(volatile uint32_t *)(SCB_BASE + 0x008U))
#define SCB_CPACR       (*(volatile uint32_t *)(SCB_BASE + 0x088U))
#define SCB_CCR         (*(volatile uint32_t *)(SCB_BASE + 0x014U))

#define SysTick_BASE    0xE000E010U
#define SYST_CSR        (*(volatile uint32_t *)(SysTick_BASE + 0x00U))
#define SYST_RVR        (*(volatile uint32_t *)(SysTick_BASE + 0x04U))
#define SYST_CVR        (*(volatile uint32_t *)(SysTick_BASE + 0x08U))

#define NVIC_ISER0      (*(volatile uint32_t *)(0xE000E100U))
#define NVIC_ISER1      (*(volatile uint32_t *)(0xE000E104U))
#define NVIC_ICPR0      (*(volatile uint32_t *)(0xE000E280U))
#define NVIC_ICPR1      (*(volatile uint32_t *)(0xE000E284U))

/* ===================================================================== */
/*  Reset and Clock Control (RCC)                                         */
/* ===================================================================== */

#define RCC_BASE        (AHB1PERIPH_BASE + 0x4800U)

#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00U))
#define RCC_CR_HSION    (1U << 8)
#define RCC_CR_HSIRDY   (1U << 10)
#define RCC_CR_HSEON    (1U << 16)
#define RCC_CR_HSERDY   (1U << 17)
#define RCC_CR_HSEBYP   (1U << 18)
#define RCC_CR_PLLON    (1U << 24)
#define RCC_CR_PLLRDY   (1U << 25)

#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x10U))
#define RCC_CFGR_SW_HSI         0U
#define RCC_CFGR_SW_HSE         2U
#define RCC_CFGR_SW_PLL         3U
#define RCC_CFGR_SWS_MASK       (3U << 2)

#define RCC_PLLCFGR     (*(volatile uint32_t *)(RCC_BASE + 0x0CU))
#define RCC_PLLCFGR_PLLM_Pos    4U
#define RCC_PLLCFGR_PLLN_Pos    8U
#define RCC_PLLCFGR_PLLP_Pos    17U
#define RCC_PLLCFGR_PLLQ_Pos    21U
#define RCC_PLLCFGR_PLLR_Pos    25U
#define RCC_PLLCFGR_PLLREN      (1U << 24U)

#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4CU))
#define RCC_AHB1ENR_DMA1EN     (1U << 0)
#define RCC_AHB1ENR_DMA2EN     (1U << 1)
#define RCC_AHB1ENR_CRCEN      (1U << 12)

#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4CU + 0x04U))
#define RCC_AHB2ENR_GPIOAEN    (1U << 0)
#define RCC_AHB2ENR_GPIOBEN    (1U << 1)
#define RCC_AHB2ENR_GPIOCEN    (1U << 2)
#define RCC_AHB2ENR_GPIOHEN    (1U << 7)
#define RCC_AHB2ENR_AES1EN     (1U << 17)

#define RCC_AHB3ENR     (*(volatile uint32_t *)(RCC_BASE + 0x54U))
#define RCC_AHB3ENR_QSPIEN     (1U << 8)
#define RCC_AHB3ENR_HSEMEN     (1U << 19)

#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58U))
#define RCC_APB1ENR1_TIM2EN   (1U << 0)
#define RCC_APB1ENR1_I2C1EN   (1U << 21)
#define RCC_APB1ENR1_I2C3EN   (1U << 23)
#define RCC_APB1ENR1_USART2EN (1U << 17)
#define RCC_APB1ENR1_RTCAPBEN (1U << 10)

#define RCC_APB1ENR2    (*(volatile uint32_t *)(RCC_BASE + 0x5CU))
#define RCC_APB1ENR2_LPUART1EN (1U << 0)

#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x60U))
#define RCC_APB2ENR_TIM1EN   (1U << 11)
#define RCC_APB2ENR_TIM16EN  (1U << 17)
#define RCC_APB2ENR_TIM17EN  (1U << 18)
#define RCC_APB2ENR_SPI1EN   (1U << 12)
#define RCC_APB2ENR_ADC1EN   (1U << 20)

#define RCC_APB3ENR     (*(volatile uint32_t *)(RCC_BASE + 0x64U))
#define RCC_APB3ENR_RFEN      (1U << 0)

#define RCC_CCIPR       (*(volatile uint32_t *)(RCC_BASE + 0x88U))

/* ===================================================================== */
/*  Power control (PWR)                                                  */
/* ===================================================================== */

#define PWR_BASE        (AHB1PERIPH_BASE + 0x5000U)
#define PWR_CR1         (*(volatile uint32_t *)(PWR_BASE + 0x00U))
#define PWR_CR3         (*(volatile uint32_t *)(PWR_BASE + 0x08U))
#define PWR_CR4         (*(volatile uint32_t *)(PWR_BASE + 0x0CU))
#define PWR_CR4_VBE     (1U << 16)

#define PWR_SR1         (*(volatile uint32_t *)(PWR_BASE + 0x10U))
#define PWR_SR2         (*(volatile uint32_t *)(PWR_BASE + 0x14U))

/* ===================================================================== */
/*  GPIO                                                                 */
/* ===================================================================== */

typedef struct {
    volatile uint32_t MODER;     /* 0x00 */
    volatile uint32_t OTYPER;    /* 0x04 */
    volatile uint32_t OSPEEDR;   /* 0x08 */
    volatile uint32_t PUPDR;     /* 0x0C */
    volatile uint32_t IDR;       /* 0x10 */
    volatile uint32_t ODR;       /* 0x14 */
    volatile uint32_t BSRR;      /* 0x18 */
    volatile uint32_t LCKR;      /* 0x1C */
    volatile uint32_t AFRL;      /* 0x20 */
    volatile uint32_t AFRH;      /* 0x24 */
    volatile uint32_t BRR;       /* 0x28 */
} GPIO_TypeDef;

#define GPIOA           ((GPIO_TypeDef *)(AHB2PERIPH_BASE + 0x0000U))
#define GPIOB           ((GPIO_TypeDef *)(AHB2PERIPH_BASE + 0x0400U))
#define GPIOC           ((GPIO_TypeDef *)(AHB2PERIPH_BASE + 0x0800U))
#define GPIOH           ((GPIO_TypeDef *)(AHB2PERIPH_BASE + 0x1C00U))

#define GPIO_MODE_INPUT   0U
#define GPIO_MODE_OUTPUT  1U
#define GPIO_MODE_AF      2U
#define GPIO_MODE_ANALOG  3U

#define GPIO_OTYPE_PP     0U
#define GPIO_OTYPE_OD     1U

#define GPIO_OSPEED_LOW   0U
#define GPIO_OSPEED_HIGH  2U

#define GPIO_PUPD_NONE    0U
#define GPIO_PUPD_PU      1U
#define GPIO_PUPD_PD      2U

#define GPIO_PIN_SET(n)   (1U << (n))
#define GPIO_PIN_RST(n)   (1U << ((n) + 16U))

/* ===================================================================== */
/*  I2C1 — for AS7343 spectrometer + SSD1306 OLED                         */
/* ===================================================================== */

typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t OAR1;      /* 0x08 */
    volatile uint32_t OAR2;      /* 0x0C */
    volatile uint32_t TIMINGR;   /* 0x10 */
    volatile uint32_t TIMEOUTR;  /* 0x14 */
    volatile uint32_t ISR;       /* 0x18 */
    volatile uint32_t ICR;       /* 0x1C */
    volatile uint32_t PECR;      /* 0x20 */
    volatile uint32_t RXDR;      /* 0x24 */
    volatile uint32_t TXDR;      /* 0x28 */
} I2C_TypeDef;

#define I2C1            ((I2C_TypeDef *)(APB1PERIPH_BASE + 0x5400U))

#define I2C_CR1_PE      (1U << 0)
#define I2C_CR1_TXIE    (1U << 1)
#define I2C_CR1_RXIE    (1U << 2)
#define I2C_CR1_NACKIE  (1U << 4)
#define I2C_CR1_STOPIE  (1U << 5)
#define I2C_CR1_ERRIE   (1U << 8)

#define I2C_CR2_SADD_Pos 1U
#define I2C_CR2_NBYTES_Pos 16U
#define I2C_CR2_NBYTES(n) ((n) << 16U)
#define I2C_CR2_RELOAD  (1U << 24U)
#define I2C_CR2_AUTOEND (1U << 25U)
#define I2C_CR2_START   (1U << 13U)
#define I2C_CR2_STOP    (1U << 14U)
#define I2C_CR2_RD_WRN  (1U << 10U)

#define I2C_ISR_TXE     (1U << 0)
#define I2C_ISR_TXIS     (1U << 1)
#define I2C_ISR_RXNE     (1U << 2)
#define I2C_ISR_ADDR    (1U << 3)
#define I2C_ISR_NACKF   (1U << 4)
#define I2C_ISR_STOPF   (1U << 5)
#define I2C_ISR_TC      (1U << 6)
#define I2C_ISR_TCR     (1U << 7)
#define I2C_ISR_BUSY    (1U << 15)

#define I2C_ICR_NACKCF  (1U << 4)
#define I2C_ICR_STOPCF  (1U << 5)
#define I2C_ICR_ALLCF   0x3F38U

/* ===================================================================== */
/*  ADC1 — photodiode flicker sampling                                   */
/* ===================================================================== */

typedef struct {
    volatile uint32_t ISR;       /* 0x00 */
    volatile uint32_t IER;       /* 0x04 */
    volatile uint32_t CR;        /* 0x08 */
    volatile uint32_t CFGR;      /* 0x0C */
    volatile uint32_t CFGR2;     /* 0x10 */
    volatile uint32_t SMPR1;     /* 0x14 */
    volatile uint32_t SMPR2;     /* 0x18 */
    volatile uint32_t TR1;       /* 0x20 */
    volatile uint32_t TR2;      /* 0x24 */
    volatile uint32_t TR3;      /* 0x28 */
    volatile uint32_t DR;       /* 0x40 */
    volatile uint32_t AWD2CR;   /* 0xA0 */
    volatile uint32_t AWD3CR;   /* 0xA4 */
} ADC_TypeDef;

#define ADC1            ((ADC_TypeDef *)(APB2PERIPH_BASE + 0x0800U))

#define ADC_ISR_ADRDY   (1U << 0)
#define ADC_ISR_EOC     (1U << 2)
#define ADC_ISR_EOS     (1U << 3)
#define ADC_ISR_OVR     (1U << 4)

#define ADC_CR_ADEN     (1U << 0)
#define ADC_CR_DISON    (1U << 1)
#define ADC_CR_ADCAL    (1U << 16)
#define ADC_CR_ADVREG   (1U << 12)

#define ADC_CFGR_CONT   (1U << 13)
#define ADC_CFGR_DMAEN  (1U << 15)
#define ADC_CFGR_DMACFG (1U << 14)
#define ADC_CFGR_EXTEN_RISING (1U << 10)

/* ADC channel assignments (STM32WB55) */
#define ADC_CH_VREFINT  13U
#define ADC_CH_TEMP     17U
#define ADC_CH_VBAT     18U

/* ===================================================================== */
/*  TIM2 — 1 kHz ADC trigger / system tick                                */
/* ===================================================================== */

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t BDTR;
} TIM_TypeDef;

#define TIM2            ((TIM_TypeDef *)(APB1PERIPH_BASE + 0x0000U))
#define TIM1            ((TIM_TypeDef *)(APB2PERIPH_BASE + 0x2C00U))
#define TIM16           ((TIM_TypeDef *)(APB2PERIPH_BASE + 0x4400U))
#define TIM17           ((TIM_TypeDef *)(APB2PERIPH_BASE + 0x4800U))

#define TIM_CR1_CEN     (1U << 0)
#define TIM_DIER_UIE    (1U << 0)
#define TIM_DIER_CC1IE  (1U << 1)
#define TIM_SR_UIF      (1U << 0)
#define TIM_SR_CC1IF    (1U << 1)

/* ===================================================================== */
/*  DMA1 Channel 1 — ADC streaming buffer                                 */
/* ===================================================================== */

typedef struct {
    volatile uint32_t CCR;
    volatile uint32_t CNDTR;
    volatile uint32_t CPAR;
    volatile uint32_t CMAR;
    volatile uint32_t CCR2;
    volatile uint32_t CNDTR2;
    volatile uint32_t CPAR2;
    volatile uint32_t CMAR2;
    volatile uint32_t CCR3;
    volatile uint32_t CNDTR3;
    volatile uint32_t CPAR3;
    volatile uint32_t CMAR3;
} DMA_Channel_TypeDef;

#define DMA1_Channel1   ((DMA_Channel_TypeDef *)(AHB1PERIPH_BASE + 0x0000U))
#define DMA1_CSELR       (*(volatile uint32_t *)(AHB1PERIPH_BASE + 0x0800U + 0xA8U))

#define DMA_CCR_EN      (1U << 0)
#define DMA_CCR_TCIE    (1U << 1)
#define DMA_CCR_HTIE    (1U << 2)
#define DMA_CCR_TEIE    (1U << 3)
#define DMA_CCR_DIR_P2M  (0U << 4)
#define DMA_CCR_CIRC    (1U << 5)
#define DMA_CCR_PINC    (1U << 6)
#define DMA_CCR_MINC    (1U << 7)
#define DMA_CCR_PSIZE_16 (1U << 8)
#define DMA_CCR_MSIZE_16 (1U << 10)
#define DMA_CCR_PR_HI   (3U << 12)

/* ===================================================================== */
/*  SPI1 — microSD card (SDIO via SPI)                                  */
/* ===================================================================== */

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
} SPI_TypeDef;

#define SPI1            ((SPI_TypeDef *)(APB2PERIPH_BASE + 0x3000U))

#define SPI_CR1_CPHA    (1U << 0)
#define SPI_CR1_CPOL    (1U << 1)
#define SPI_CR1_MSTR    (1U << 2)
#define SPI_CR1_BR_Pos  3U
#define SPI_CR1_SPE     (1U << 6)
#define SPI_CR1_LSBFIRST (1U << 7)
#define SPI_CR1_SSI     (1U << 8)
#define SPI_CR1_SSM     (1U << 9)

#define SPI_CR2_DS_8BIT 7U
#define SPI_CR2_DS_Pos  8U
#define SPI_CR2_FRXTH   (1U << 12)
#define SPI_CR2_TXEIE   (1U << 1)
#define SPI_CR2_RXNEIE  (1U << 0)

#define SPI_SR_RXNE     (1U << 0)
#define SPI_SR_TXE      (1U << 1)
#define SPI_SR_BSY      (1U << 7)
#define SPI_SR_MODF     (1U << 9)
#define SPI_SR_OVR      (1U << 6)

/* ===================================================================== */
/*  LPUART1 — debug console                                               */
/* ===================================================================== */

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t BRR;
    volatile uint32_t GTPR;
    volatile uint32_t RTOR;
    volatile uint32_t RQR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t RDR;
    volatile uint32_t TDR;
} USART_TypeDef;

#define LPUART1         ((USART_TypeDef *)(APB1PERIPH_BASE + 0x0800U + 0x8000U))

#define USART_CR1_UE    (1U << 0)
#define USART_CR1_TE    (1U << 3)
#define USART_CR1_RE    (1U << 2)
#define USART_ISR_TXE   (1U << 7)
#define USART_ISR_TC    (1U << 6)
#define USART_ISR_RXNE  (1U << 5)

/* ===================================================================== */
/*  RTC — date / time stamping                                            */
/* ===================================================================== */

#define RTC_BASE        (APB1PERIPH_BASE + 0x2800U)
#define RTC_TR           (*(volatile uint32_t *)(RTC_BASE + 0x00U))
#define RTC_DR           (*(volatile uint32_t *)(RTC_BASE + 0x04U))
#define RTC_CR           (*(volatile uint32_t *)(RTC_BASE + 0x08U))
#define RTC_ISR          (*(volatile uint32_t *)(RTC_BASE + 0x0CU))
#define RTC_PRER         (*(volatile uint32_t *)(RTC_BASE + 0x10U))
#define RTC_WUTR         (*(volatile uint32_t *)(RTC_BASE + 0x14U))
#define RTC_SSR          (*(volatile uint32_t *)(RTC_BASE + 0x18U))

#define RTC_ISR_INIT     (1U << 7)
#define RTC_ISR_RSF      (1U << 5)
#define RTC_CR_FMT       (1U << 6)

/* ===================================================================== */
/*  Flash interface                                                      */
/* ===================================================================== */

#define FLASH_BASE      (AHB1PERIPH_BASE + 0x2000U)
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE + 0x00U))
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x08U))
#define FLASH_SR        (*(volatile uint32_t *)(FLASH_BASE + 0x10U))
#define FLASH_CR        (*(volatile uint32_t *)(FLASH_BASE + 0x14U))

#define FLASH_ACR_LATENCY_3WS  (3U << 0)
#define FLASH_ACR_PRFTEN       (1U << 8)

/* ===================================================================== */
/*  Independent watchdog                                                  */
/* ===================================================================== */

#define IWDG_BASE       0x40003000U
#define IWDG_KR         (*(volatile uint32_t *)(IWDG_BASE + 0x00U))
#define IWDG_PR         (*(volatile uint32_t *)(IWDG_BASE + 0x04U))
#define IWDG_RLR        (*(volatile uint32_t *)(IWDG_BASE + 0x08U))
#define IWDG_SR          (*(volatile uint32_t *)(IWDG_BASE + 0x0CU))

#endif /* LUMICAST_REGISTERS_H */