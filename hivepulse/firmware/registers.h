/*
 * registers.h — Register-level definitions for HivePulse custom peripherals
 *
 * Target MCU: STM32H733VGT6 (Cortex-M7 @ 280 MHz)
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ---- Base Addresses (STM32H733) ---- */
#define PERIPH_BASE           0x40000000UL
#define D1_APB1_PERIPH_BASE   0x40000000UL
#define D1_APB2_PERIPH_BASE   0x40010000UL
#define D1_AHB1_PERIPH_BASE   0x50000000UL
#define D1_AHB2_PERIPH_BASE   0x58000000UL
#define D2_AHB1_PERIPH_BASE   0x48020000UL
#define D2_APB1_PERIPH_BASE   0x40002000UL
#define D2_APB2_PERIPH_BASE   0x40010000UL
#define D3_AHB1_PERIPH_BASE   0x51000000UL
#define D3_APB1_PERIPH_BASE   0x58000000UL

/* ---- GPIO Registers ---- */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 Mode register */
    volatile uint32_t OTYPER;   /* 0x04 Output type register */
    volatile uint32_t OSPEEDR;  /* 0x08 Output speed register */
    volatile uint32_t PUPDR;    /* 0x0C Pull-up/pull-down register */
    volatile uint32_t IDR;      /* 0x10 Input data register */
    volatile uint32_t ODR;      /* 0x14 Output data register */
    volatile uint32_t BSRR;     /* 0x18 Bit set/reset register */
    volatile uint32_t LCKR;     /* 0x1C Lock register */
    volatile uint32_t AFR[2];   /* 0x20-0x24 Alternate function registers */
    volatile uint32_t BRR;      /* 0x28 Bit reset register */
} GPIO_TypeDef;

#define GPIOA_BASE  (D2_AHB1_PERIPH_BASE + 0x0000)
#define GPIOB_BASE  (D2_AHB1_PERIPH_BASE + 0x0400)
#define GPIOC_BASE  (D2_AHB1_PERIPH_BASE + 0x0800)
#define GPIOD_BASE  (D2_AHB1_PERIPH_BASE + 0x0C00)
#define GPIOE_BASE  (D2_AHB1_PERIPH_BASE + 0x1000)
#define GPIOF_BASE  (D2_AHB1_PERIPH_BASE + 0x1400)
#define GPIOG_BASE  (D2_AHB1_PERIPH_BASE + 0x1800)
#define GPIOH_BASE  (D2_AHB1_PERIPH_BASE + 0x1C00)
#define GPIOI_BASE  (D2_AHB1_PERIPH_BASE + 0x2000)

#define GPIOA       ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB       ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC       ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD       ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE       ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF       ((GPIO_TypeDef *)GPIOF_BASE)
#define GPIOG       ((GPIO_TypeDef *)GPIOG_BASE)
#define GPIOH       ((GPIO_TypeDef *)GPIOH_BASE)
#define GPIOI       ((GPIO_TypeDef *)GPIOI_BASE)

#define GPIO_PIN_0    (1U << 0)
#define GPIO_PIN_1    (1U << 1)
#define GPIO_PIN_2    (1U << 2)
#define GPIO_PIN_3    (1U << 3)
#define GPIO_PIN_4    (1U << 4)
#define GPIO_PIN_5    (1U << 5)
#define GPIO_PIN_6    (1U << 6)
#define GPIO_PIN_7    (1U << 7)
#define GPIO_PIN_8    (1U << 8)
#define GPIO_PIN_9    (1U << 9)
#define GPIO_PIN_10   (1U << 10)
#define GPIO_PIN_11   (1U << 11)
#define GPIO_PIN_12   (1U << 12)
#define GPIO_PIN_13   (1U << 13)
#define GPIO_PIN_14   (1U << 14)
#define GPIO_PIN_15   (1U << 15)

/* GPIO mode values */
#define GPIO_MODE_INPUT    0x00
#define GPIO_MODE_OUTPUT   0x01
#define GPIO_MODE_AF       0x02
#define GPIO_MODE_ANALOG   0x03

/* GPIO output type */
#define GPIO_OTYPE_PP      0x00  /* Push-pull */
#define GPIO_OTYPE_OD      0x01  /* Open-drain */

/* GPIO speed */
#define GPIO_SPEED_LOW     0x00
#define GPIO_SPEED_MED     0x01
#define GPIO_SPEED_HIGH    0x02
#define GPIO_SPEED_VHIGH   0x03

/* GPIO pull */
#define GPIO_NOPULL        0x00
#define GPIO_PULLUP        0x01
#define GPIO_PULLDOWN      0x02

/* ---- RCC (Reset & Clock Control) ---- */
#define RCC_BASE   (D1_AHB1_PERIPH_BASE + 0x4400)
typedef struct {
    volatile uint32_t CR;       /* 0x00 Clock control */
    volatile uint32_t HSICFGR;  /* 0x04 HSI config */
    volatile uint32_t CSICFGR;  /* 0x08 CSI config */
    volatile uint32_t CRRCR;    /* 0x0C CRSI config */
    volatile uint32_t HSIDIV;   /* 0x10 HSI divider */
} RCC_CR_TypeDef;

#define RCC_CR    (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_PLLCFGR (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_CFGR  (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_D1CFGR (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_D2CFGR (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_D3CFGR (*(volatile uint32_t *)(RCC_BASE + 0x20))

/* RCC enable registers */
#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_AHB2ENR (*(volatile uint32_t *)(RCC_BASE + 0xDC))
#define RCC_AHB3ENR (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_APB1LENR (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_APB1HENR (*(volatile uint32_t *)(RCC_BASE + 0xEC))
#define RCC_APB2ENR (*(volatile uint32_t *)(RCC_BASE + 0xF0))
#define RCC_APB3ENR (*(volatile uint32_t *)(RCC_BASE + 0xE4))
#define RCC_APB4ENR (*(volatile uint32_t *)(RCC_BASE + 0xF4))

/* RCC bit positions for peripheral enables */
#define RCC_AHB1ENR_DMA1EN    (1U << 0)
#define RCC_AHB1ENR_DMA2EN    (1U << 1)
#define RCC_AHB1ENR_CRCEN     (1U << 19)
#define RCC_AHB2ENR_GPIOAEN   (1U << 0)
#define RCC_AHB2ENR_GPIOBEN   (1U << 1)
#define RCC_AHB2ENR_GPIOCEN   (1U << 2)
#define RCC_AHB2ENR_GPIODEN   (1U << 3)
#define RCC_AHB2ENR_GPIOEEN   (1U << 4)
#define RCC_AHB2ENR_GPIOGEN   (1U << 6)
#define RCC_AHB2ENR_GPIOHEN   (1U << 7)
#define RCC_AHB2ENR_GPIOIEN   (1U << 8)
#define RCC_AHB4ENR_ADC12EN   (1U << 24)
#define RCC_APB1LENR_USART3EN (1U << 18)
#define RCC_APB1LENR_UART4EN  (1U << 24)
#define RCC_APB1LENR_I2C1EN   (1U << 21)
#define RCC_APB1LENR_I2C2EN   (1U << 22)
#define RCC_APB2ENR_SPI1EN    (1U << 12)
#define RCC_APB2ENR_SPI2EN    (1U << 14)
#define RCC_APB2ENR_SPI3EN    (1U << 15)
#define RCC_APB2ENR_USART1EN  (1U << 14)
#define RCC_APB4ENR_LPUART1EN (1U << 3)

/* ---- DMA Controller ---- */
#define DMA1_BASE  (D2_AHB1_PERIPH_BASE + 0x0000)
typedef struct {
    volatile uint32_t LISR;     /* Low interrupt status */
    volatile uint32_t HISR;     /* High interrupt status */
    volatile uint32_t LIFCR;    /* Low interrupt flag clear */
    volatile uint32_t HIFCR;    /* High interrupt flag clear */
} DMA_Common_TypeDef;

#define DMA1 ((DMA_Common_TypeDef *)DMA1_BASE)

typedef struct {
    volatile uint32_t CR;       /* 0x00 Configuration */
    volatile uint32_t NDTR;     /* 0x04 Number of data items */
    volatile uint32_t PAR;      /* 0x08 Peripheral address */
    volatile uint32_t M0AR;     /* 0x0C Memory address 0 */
    volatile uint32_t M1AR;     /* 0x10 Memory address 1 (double buffer) */
    volatile uint32_t FCR;      /* 0x14 FIFO control */
} DMA_Stream_TypeDef;

#define DMA1_Stream0 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x010))
#define DMA1_Stream1 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x028))
#define DMA1_Stream2 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x040))
#define DMA1_Stream3 ((DMA_Stream_TypeDef *)(DMA1_BASE + 0x058))

#define DMA_CHANNEL_0   0x0
#define DMA_CHANNEL_1   0x1
#define DMA_CHANNEL_2   0x2
#define DMA_CHANNEL_3   0x3
#define DMA_CHANNEL_4   0x4
#define DMA_CHANNEL_5   0x5
#define DMA_CHANNEL_6   0x6
#define DMA_CHANNEL_7   0x7

/* DMA CR register bits */
#define DMA_CR_EN        (1U << 0)
#define DMA_CR_DMEIE     (1U << 2)
#define DMA_CR_TEIE      (1U << 3)
#define DMA_CR_HTIE      (1U << 4)
#define DMA_CR_TCIE      (1U << 5)
#define DMA_CR_PFCTRL    (1U << 5)
#define DMA_CR_DBM       (1U << 18)
#define DMA_CR_CT        (1U << 19)
#define DMA_CR_MINC      (1U << 10)
#define DMA_CR_PINC      (1U << 9)
#define DMA_CR_CIRC      (1U << 8)
#define DMA_CR_DIR_P2M   (0x0 << 6)
#define DMA_CR_DIR_M2P   (0x1 << 6)
#define DMA_CR_DIR_M2M   (0x2 << 6)

/* DMA priority */
#define DMA_PRIO_LOW     (0x0 << 16)
#define DMA_PRIO_MED     (0x1 << 16)
#define DMA_PRIO_HIGH    (0x2 << 16)
#define DMA_PRIO_VHIGH   (0x3 << 16)

/* ---- SPI / I2S Registers ---- */
#define SPI1_BASE  (D2_APB2_PERIPH_BASE + 0x3000)
#define SPI2_BASE  (D2_APB1_PERIPH_BASE + 0x3800)
#define SPI3_BASE  (D2_APB2_PERIPH_BASE + 0x3C00)

typedef struct {
    volatile uint32_t CR1;      /* 0x00 Control register 1 */
    volatile uint32_t CR2;      /* 0x04 Control register 2 */
    volatile uint32_t SR;       /* 0x08 Status register */
    volatile uint32_t DR;       /* 0x0C Data register */
    volatile uint32_t CRCPR;    /* 0x10 CRC polynomial */
    volatile uint32_t RXCRCR;   /* 0x14 RX CRC */
    volatile uint32_t TXCRCR;   /* 0x18 TX CRC */
    volatile uint32_t I2SCFGR;  /* 0x1C I2S config */
    volatile uint32_t I2SPR;    /* 0x20 I2S prescaler */
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)
#define SPI2 ((SPI_TypeDef *)SPI2_BASE)
#define SPI3 ((SPI_TypeDef *)SPI3_BASE)

/* SPI CR1 bits */
#define SPI_CR1_SPE      (1U << 6)
#define SPI_CR1_MSTR     (1U << 2)
#define SPI_CR1_CPOL     (1U << 1)
#define SPI_CR1_CPHA     (1U << 0)
#define SPI_CR1_BR_DIV2  (0x0 << 3)
#define SPI_CR1_BR_DIV4  (0x1 << 3)
#define SPI_CR1_BR_DIV8  (0x2 << 3)
#define SPI_CR1_BR_DIV16 (0x3 << 3)
#define SPI_CR1_BR_DIV32 (0x4 << 3)
#define SPI_CR1_BR_DIV64 (0x5 << 3)
#define SPI_CR1_LSBFIRST (1U << 7)
#define SPI_CR1_SSM      (1U << 9)
#define SPI_CR1_SSI      (1U << 10)
#define SPI_CR1_RXONLY   (1U << 10)

/* SPI SR bits */
#define SPI_SR_RXNE   (1U << 0)
#define SPI_SR_TXE    (1U << 1)
#define SPI_SR_BSY    (1U << 7)
#define SPI_SR_FRE    (1U << 8)
#define SPI_SR_OVR    (1U << 6)

/* I2S CFGR bits */
#define I2S_CFGR_I2SMOD  (1U << 11)
#define I2S_CFGR_I2SCFG  (1U << 8)
#define I2S_CFGR_I2SE    (1U << 10)

/* ---- UART Registers ---- */
#define USART3_BASE (D2_APB1_PERIPH_BASE + 0x4800)
#define UART4_BASE  (D2_APB1_PERIPH_BASE + 0x4C00)
#define LPUART1_BASE (D3_APB1_PERIPH_BASE + 0x0C00)

typedef struct {
    volatile uint32_t CR1;  /* 0x00 */
    volatile uint32_t CR2;  /* 0x04 */
    volatile uint32_t CR3;  /* 0x08 */
    volatile uint32_t BRR;  /* 0x0C Baud rate */
    volatile uint32_t GTPR; /* 0x10 */
    volatile uint32_t RTOR; /* 0x14 */
    volatile uint32_t RQR;  /* 0x18 Request */
    volatile uint32_t ISR;  /* 0x1C Interrupt status */
    volatile uint32_t ICR;  /* 0x20 Interrupt flag clear */
    volatile uint32_t RDR;  /* 0x24 Receive data */
    volatile uint32_t TDR;  /* 0x28 Transmit data */
} USART_TypeDef;

#define USART3  ((USART_TypeDef *)USART3_BASE)
#define UART4   ((USART_TypeDef *)UART4_BASE)
#define LPUART1 ((USART_TypeDef *)LPUART1_BASE)

/* UART CR1 bits */
#define USART_CR1_UE     (1U << 0)
#define USART_CR1_RE     (1U << 2)
#define USART_CR1_TE     (1U << 3)
#define USART_CR1_RXNEIE (1U << 5)
#define USART_CR1_TCIE   (1U << 6)
#define USART_CR1_TXEIE  (1U << 7)
#define USART_CR1_OVER8  (1U << 15)

/* UART ISR bits */
#define USART_ISR_RXNE  (1U << 5)
#define USART_ISR_TXE   (1U << 7)
#define USART_ISR_TC    (1U << 6)
#define USART_ISR_BUSY  (1U << 16)

/* ---- I2C Registers ---- */
#define I2C1_BASE  (D2_APB1_PERIPH_BASE + 0x5400)
#define I2C2_BASE  (D2_APB1_PERIPH_BASE + 0x5800)

typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t OAR1;  /* 0x08 Own address 1 */
    volatile uint32_t OAR2;  /* 0x0C Own address 2 */
    volatile uint32_t TIMINGR; /* 0x10 Timing */
    volatile uint32_t TIMEOUTR; /* 0x14 */
    volatile uint32_t ISR;   /* 0x18 Interrupt status */
    volatile uint32_t ICR;   /* 0x1C */
    volatile uint32_t PECR;  /* 0x20 */
    volatile uint32_t RXDR;  /* 0x24 */
    volatile uint32_t TXDR;  /* 0x28 */
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)
#define I2C2 ((I2C_TypeDef *)I2C2_BASE)

/* I2C ISR bits */
#define I2C_ISR_TXE    (1U << 0)
#define I2C_ISR_RXNE   (1U << 2)
#define I2C_ISR_TC     (1U << 6)
#define I2C_ISR_BUSY   (1U << 15)
#define I2C_ISR_NACKF  (1U << 12)

/* I2C CR1 bits */
#define I2C_CR1_PE     (1U << 0)
#define I2C_CR1_TXIE   (1U << 1)
#define I2C_CR1_RXIE   (1U << 2)
#define I2C_CR1_NACKIE (1U << 4)
#define I2C_CR1_TCIE   (1U << 6)

/* ---- ADC Registers ---- */
#define ADC1_BASE  (D2_AHB2_PERIPH_BASE + 0x0000)

typedef struct {
    volatile uint32_t ISR;   /* 0x00 */
    volatile uint32_t IER;   /* 0x04 */
    volatile uint32_t CR;    /* 0x08 */
    volatile uint32_t CFGR;  /* 0x0C */
    volatile uint32_t CFGR2; /* 0x10 */
    volatile uint32_t SMPR1; /* 0x14 */
    volatile uint32_t SMPR2; /* 0x18 */
    volatile uint32_t PCSEL; /* 0x1C */
    volatile uint32_t LFSR;  /* 0x20 */
    volatile uint32_t AWD1TR; /* 0x24 */
    volatile uint32_t AWD2TR; /* 0x28 */
    volatile uint32_t CHSELR; /* 0x30 — channel selection */
    volatile uint32_t AWD3TR; /* 0x2C */
    uint32_t reserved[4];
    volatile uint32_t DR;    /* 0x40 Data register */
} ADC_TypeDef;

#define ADC1 ((ADC_TypeDef *)ADC1_BASE)

/* ---- Timer Registers ---- */
#define TIM2_BASE  (D2_APB1_PERIPH_BASE + 0x0000)
#define TIM6_BASE  (D2_APB1_PERIPH_BASE + 0x1000)
#define TIM7_BASE  (D2_APB1_PERIPH_BASE + 0x1400)

typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t SMCR;  /* 0x08 */
    volatile uint32_t DIER;  /* 0x0C DMA/Interrupt enable */
    volatile uint32_t SR;    /* 0x10 Status */
    volatile uint32_t EGR;   /* 0x14 Event generation */
    volatile uint32_t CCMR1; /* 0x18 */
    volatile uint32_t CCMR2; /* 0x1C */
    volatile uint32_t CCER;  /* 0x20 */
    volatile uint32_t CNT;   /* 0x24 Counter */
    volatile uint32_t PSC;   /* 0x28 Prescaler */
    volatile uint32_t ARR;   /* 0x2C Auto-reload */
    volatile uint32_t CCR1;  /* 0x30 */
    volatile uint32_t CCR2;  /* 0x34 */
    volatile uint32_t CCR3;  /* 0x38 */
    volatile uint32_t CCR4;  /* 0x3C */
} TIM_TypeDef;

#define TIM2 ((TIM_TypeDef *)TIM2_BASE)
#define TIM6 ((TIM_TypeDef *)TIM6_BASE)
#define TIM7 ((TIM_TypeDef *)TIM7_BASE)

/* Timer CR1 bits */
#define TIM_CR1_CEN   (1U << 0)
#define TIM_CR1_ARPE  (1U << 7)
#define TIM_DIER_UIE  (1U << 0)
#define TIM_SR_UIF    (1U << 0)

/* ---- EXTI (External Interrupt) ---- */
#define EXTI_BASE  (D3_AHB1_PERIPH_BASE + 0x0000)

typedef struct {
    volatile uint32_t RTSR1;  /* 0x00 Rising trigger */
    volatile uint32_t FTSR1;  /* 0x04 Falling trigger */
    volatile uint32_t SWIER1; /* 0x08 Software interrupt */
    volatile uint32_t PR1;    /* 0x0C Pending register */
    volatile uint32_t IMR1;   /* 0x80 Interrupt mask (H7) */
} EXTI_TypeDef;

#define EXTI ((EXTI_TypeDef *)(EXTI_BASE))

/* ---- Flash / RTC / CRC / WDG ---- */
#define FLASH_BASE  (D1_AHB1_PERIPH_BASE + 0x2000)
#define RTC_BASE    (D3_APB1_PERIPH_BASE + 0x2800)
#define WDG_BASE    (D3_AHB1_PERIPH_BASE + 0x0C00)

/* Independent Watchdog (IWDG) */
typedef struct {
    volatile uint32_t KR;   /* 0x00 Key register */
    volatile uint32_t PR;   /* 0x04 Prescaler */
    volatile uint32_t RLR;  /* 0x08 Reload */
    volatile uint32_t SR;   /* 0x0C Status */
    volatile uint32_t WINR; /* 0x10 Window */
} IWDG_TypeDef;

#define IWDG ((IWDG_TypeDef *)(WDG_BASE))

#define IWDG_KEY_ENABLE   0xCCCC
#define IWDG_KEY_RELOAD   0xAAAA
#define IWDG_KEY_WRITE    0x5555

/* RTC */
typedef struct {
    volatile uint32_t TR;    /* 0x00 Time register */
    volatile uint32_t DR;    /* 0x04 Date register */
    volatile uint32_t CR;    /* 0x08 Control */
    volatile uint32_t ISR;   /* 0x0C Init/status */
    volatile uint32_t PRER;  /* 0x10 Prescaler */
    volatile uint32_t WUTR;  /* 0x14 Wakeup timer */
    volatile uint32_t CALR;  /* 0x18 Calibration */
    uint32_t reserved[2];
    volatile uint32_t ALRMAR; /* 0x1C Alarm A */
    volatile uint32_t ALRMBR; /* 0x20 Alarm B */
    volatile uint32_t WPR;   /* 0x24 Write protection */
    volatile uint32_t SSR;   /* 0x28 Sub-second */
    volatile uint32_t SHIFTR;/* 0x2C */
    volatile uint32_t TSTR;  /* 0x30 Tamper time */
    volatile uint32_t TSDR;  /* 0x34 */
    volatile uint32_t TSSSR; /* 0x38 */
} RTC_TypeDef;

#define RTC ((RTC_TypeDef *)RTC_BASE)

/* RTC WPR keys */
#define RTC_WPR_KEY1   0xCA
#define RTC_WPR_KEY2   0x53

/* RTC ISR bits */
#define RTC_ISR_INIT   (1U << 7)
#define RTC_ISR_RSF    (1U << 5)
#define RTC_ISR_ALRAF  (1U << 8)

/* ---- PWR / SYSCFG ---- */
#define PWR_BASE   (D1_APB1_PERIPH_BASE + 0x4000)
#define SYSCFG_BASE (D2_APB2_PERIPH_BASE + 0x4400)

/* ---- NVIC ---- */
#define NVIC_BASE  (0xE000E100UL)
#define NVIC_ISER0 (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ISER1 (*(volatile uint32_t *)(NVIC_BASE + 0x004))
#define NVIC_ICER0 (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ICER1 (*(volatile uint32_t *)(NVIC_BASE + 0x084))
#define NVIC_IPR   ((volatile uint8_t *)(NVIC_BASE + 0x300))

/* IRQ numbers (STM32H733) */
#define SPI2_IRQn              36
#define DMA1_Stream0_IRQn      11
#define DMA1_Stream1_IRQn      12
#define DMA1_Stream2_IRQn      13
#define DMA1_Stream3_IRQn      14
#define USART3_IRQn            39
#define UART4_IRQn             45
#define EXTI9_5_IRQn           23
#define EXTI15_10_IRQn         40
#define I2C1_EV_IRQn           31
#define I2C2_EV_IRQn           32
#define TIM6_DAC_IRQn          54
#define LPUART1_IRQn           63

/* ---- SysTick ---- */
#define SysTick_BASE  (0xE000E010UL)
typedef struct {
    volatile uint32_t CTRL;    /* 0x00 */
    volatile uint32_t LOAD;    /* 0x04 */
    volatile uint32_t VAL;     /* 0x08 */
    volatile uint32_t CALIB;   /* 0x0C */
} SysTick_TypeDef;
#define SysTick ((SysTick_TypeDef *)SysTick_BASE)

#define SysTick_CTRL_ENABLE  (1U << 0)
#define SysTick_CTRL_TICKINT (1U << 1)
#define SysTick_CTRL_CLKSRC  (1U << 2)

/* ---- SCB (System Control Block) ---- */
#define SCB_BASE  (0xE000ED00UL)
#define SCB_VTOR  (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_AIRCR (*(volatile uint32_t *)(SCB_BASE + 0x0C))
#define SCB_SCR   (*(volatile uint32_t *)(SCB_BASE + 0x10))
#define SCB_CCR   (*(volatile uint32_t *)(SCB_BASE + 0x14))

#define SCB_SCR_SLEEPDEEP  (1U << 2)
#define SCB_SCR_SLEEPONEXIT (1U << 1)

/* ---- Global helper macros ---- */
#define BIT(n)  (1U << (n))

static inline void __DSB(void) {
    __asm volatile ("dsb 0xF" ::: "memory");
}

static inline void __ISB(void) {
    __asm volatile ("isb 0xF" ::: "memory");
}

static inline void __WFI(void) {
    __asm volatile ("wfi");
}

static inline void __WFE(void) {
    __asm volatile ("wfe");
}

static inline void __enable_irq(void) {
    __asm volatile ("cpsie i" ::: "memory");
}

static inline void __disable_irq(void) {
    __asm volatile ("cpsid i" ::: "memory");
}

#endif /* REGISTERS_H */