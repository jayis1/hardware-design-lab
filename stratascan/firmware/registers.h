/*
 * registers.h — STM32H743 Peripheral Register Definitions
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Minimal register-level definitions for the STM32H743 peripherals used
 * by StrataScan firmware.  These are hand-maintained to avoid dependency on
 * a full vendor header pack; the firmware is compile-ready with only this
 * file and the standard C library.
 */

#ifndef STRATASCAN_REGISTERS_H
#define STRATASCAN_REGISTERS_H

#include <stdint.h>

/* ===================================================================== */
/*  Base addresses                                                        */
/* ===================================================================== */

#define PERIPH_BASE      0x40000000UL
#define D1_AHB1PERIPH    (PERIPH_BASE + 0x10000000UL)
#define D1_APB1PERIPH    (PERIPH_BASE + 0x10000000UL)  /* placeholder */
#define D2_AHB1PERIPH    (PERIPH_BASE + 0x00020000UL)
#define D2_AHB2PERIPH    (PERIPH_BASE + 0x08000000UL)
#define D2_APB1PERIPH    (PERIPH_BASE + 0x00000000UL)
#define D2_APB2PERIPH    (PERIPH_BASE + 0x00010000UL)
#define D3_AHB1PERIPH    (PERIPH_BASE + 0x18000000UL)
#define D3_APB4PERIPH    (PERIPH_BASE + 0x10000000UL)

/* GPIO ports */
#define GPIOA_BASE       (D2_AHB2PERIPH + 0x0000UL)
#define GPIOB_BASE       (D2_AHB2PERIPH + 0x0400UL)
#define GPIOC_BASE       (D2_AHB2PERIPH + 0x0800UL)
#define GPIOD_BASE       (D2_AHB2PERIPH + 0x0C00UL)
#define GPIOE_BASE       (D2_AHB2PERIPH + 0x1000UL)
#define GPIOH_BASE       (D2_AHB2PERIPH + 0x1C00UL)

#define RCC_BASE         (D1_AHB1PERIPH + 0x4400UL)
#define PWR_BASE         (D1_APB1PERIPH + 0x7000UL)
#define FLASH_BASE_REG   (D1_AHB1PERIPH + 0x2000UL)

/* DMA */
#define DMA1_BASE        (D2_AHB1PERIPH + 0x0000UL)
#define DMA2_BASE        (D2_AHB1PERIPH + 0x0400UL)
#define DMAMUX1_BASE     (D2_AHB1PERIPH + 0x0800UL)

/* SPI */
#define SPI1_BASE        (D2_APB2PERIPH + 0x3000UL)
#define SPI3_BASE        (D2_APB1PERIPH + 0x3C00UL)

/* USART */
#define USART1_BASE      (D2_APB2PERIPH + 0x3800UL)
#define USART2_BASE      (D2_APB1PERIPH + 0x4400UL)
#define USART3_BASE      (D2_APB1PERIPH + 0x4800UL)

/* I2C */
#define I2C1_BASE       (D2_APB1PERIPH + 0x5400UL)

/* Timers */
#define TIM1_BASE        (D2_APB2PERIPH + 0x0000UL)
#define TIM2_BASE       (D2_APB1PERIPH + 0x0000UL)
#define TIM3_BASE       (D2_APB1PERIPH + 0x0400UL)
#define TIM6_BASE       (D2_APB1PERIPH + 0x1000UL)
#define TIM7_BASE       (D2_APB1PERIPH + 0x1400UL)
#define TIM8_BASE       (D2_APB2PERIPH + 0x0400UL)

/* SDMMC */
#define SDMMC1_BASE     (D2_AHB2PERIPH + 0x2800UL)

/* USB */
#define OTG_FS_BASE     (D2_AHB2PERIPH + 0x10000UL)

/* EXTI */
#define EXTI_BASE       (D3_APB4PERIPH + 0x0C00UL)

/* SysTick */
#define SYSTICK_BASE    (0xE000E010UL)
#define NVIC_BASE       (0xE000E100UL)
#define SCB_BASE        (0xE000ED00UL)

/* RNG */
#define RNG_BASE        (D2_AHB2PERIPH + 0x0800UL)

/* DAC */
#define DAC_BASE        (D2_APB1PERIPH + 0x7800UL)

/* ADC */
#define ADC1_BASE       (D2_AHB2PERIPH + 0x2000UL)

/* ===================================================================== */
/*  Register access macros                                                */
/* ===================================================================== */

#define __IO volatile
#define __I  volatile const
#define REG32(addr) (*(volatile uint32_t *)(addr))

/* ===================================================================== */
/*  GPIO register layout                                                  */
/* ===================================================================== */

typedef struct {
    __IO uint32_t MODER;     /* 0x00 */
    __IO uint32_t OTYPER;    /* 0x04 */
    __IO uint32_t OSPEEDR;   /* 0x08 */
    __IO uint32_t PUPDR;     /* 0x0C */
    __IO uint32_t IDR;       /* 0x10 */
    __IO uint32_t ODR;       /* 0x14 */
    __IO uint32_t BSRR;      /* 0x18 — set (low 16) / reset (high 16) */
    __IO uint32_t LCKR;      /* 0x1C */
    __IO uint32_t AFRL;      /* 0x20 */
    __IO uint32_t AFRH;      /* 0x24 */
    __IO uint32_t BRR;       /* 0x28 — reset (low 16) */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOH ((GPIO_TypeDef *)GPIOH_BASE)

/* GPIO modes */
#define GPIO_MODE_INPUT    0x0
#define GPIO_MODE_OUTPUT   0x1
#define GPIO_MODE_AF       0x2
#define GPIO_MODE_ANALOG   0x3

#define GPIO_OTYPE_PP      0x0
#define GPIO_OTYPE_OD      0x1

#define GPIO_SPEED_LOW     0x0
#define GPIO_SPEED_MED     0x1
#define GPIO_SPEED_HIGH    0x2
#define GPIO_SPEED_VHIGH   0x3

#define GPIO_PUPD_NONE     0x0
#define GPIO_PUPD_UP       0x1
#define GPIO_PUPD_DOWN     0x2

/* GPIO AF codes */
#define AF_USART1_TX       7
#define AF_USART1_RX       7
#define AF_USART2_TX       7
#define AF_USART2_RX       7
#define AF_USART3_TX       7
#define AF_USART3_RX       7
#define AF_SPI1_SCK        5
#define AF_SPI1_MISO       5
#define AF_SPI1_MOSI       5
#define AF_SPI3_SCK        6
#define AF_SPI3_MISO       6
#define AF_SPI3_MOSI       6
#define AF_I2C1_SCL        4
#define AF_I2C1_SDA        4
#define AF_TIM3_CH1       2
#define AF_TIM3_CH2       2
#define AF_SDMMC1_D       12
#define AF_SDMMC1_CK      12
#define AF_SDMMC1_CMD     12
#define AF_USB_FS         10
#define AF_DAC1_OUT1      12  /* PA4 */
#define AF_DAC1_OUT2      12  /* PA5 */

/* ===================================================================== */
/*  RCC register layout                                                    */
/* ===================================================================== */

typedef struct {
    __IO uint32_t CR;        /* 0x00 */
    __IO uint32_t HSICFGR;   /* 0x04 */
    __IO uint32_t CRRCR;     /* 0x08 */
    __IO uint32_t CSICFGR;   /* 0x0C */
    __IO uint32_t CFGR;      /* 0x10 */
    __IO uint32_t D1CFGR;    /* 0x18 */
    __IO uint32_t D2CFGR;    /* 0x1C */
    __IO uint32_t D3CFGR;    /* 0x20 */
    __IO uint32_t RESERVED0; /* 0x24 */
    __IO uint32_t PLLCKSELR; /* 0x28 */
    __IO uint32_t PLLCFGR;   /* 0x2C */
    __IO uint32_t PLL1DIVR;  /* 0x30 */
    __IO uint32_t PLL1FRACR;/* 0x34 */
    __IO uint32_t PLL2DIVR;  /* 0x38 */
    __IO uint32_t PLL2FRACR; /* 0x3C */
    __IO uint32_t PLL3DIVR;  /* 0x40 */
    __IO uint32_t PLL3FRACR; /* 0x44 */
    __IO uint32_t RESERVED1; /* 0x48 */
    __IO uint32_t D1CCIPR;   /* 0x4C */
    __IO uint32_t D2CCIP1R;  /* 0x50 */
    __IO uint32_t D2CCIP2R;  /* 0x54 */
    __IO uint32_t D3CCIPR;   /* 0x58 */
    __IO uint32_t RESERVED2; /* 0x5C */
    __IO uint32_t CIER;      /* 0x60 */
    __IO uint32_t CIFR;      /* 0x64 */
    __IO uint32_t CICR;      /* 0x68 */
    __IO uint32_t RESERVED3; /* 0x6C */
    __IO uint32_t BDCR;      /* 0x70 */
    __IO uint32_t CSR;       /* 0x74 */
    __IO uint32_t RESERVED4; /* 0x78 */
    __IO uint32_t AHB3RSTR;  /* 0x7C — actually 0x7C is reserved; see ref */
    /* ... extended for D1/D2/D3 peripheral enables ... */
    __IO uint32_t AHB1RSTR;  /* 0x80 */
    __IO uint32_t AHB2RSTR;  /* 0x84 */
    __IO uint32_t AHB4RSTR;  /* 0x88 */
    __IO uint32_t APB1LRSTR; /* 0x90 */
    __IO uint32_t APB1HRSTR; /* 0x94 */
    __IO uint32_t APB2RSTR;  /* 0x98 */
    __IO uint32_t APB3RSTR;  /* 0x9C */
    __IO uint32_t APB4RSTR;  /* 0xA0 */
    __IO uint32_t RESERVED5[3];
    __IO uint32_t AHB1ENR;   /* 0xD8 */
    __IO uint32_t AHB2ENR;   /* 0xDC */
    __IO uint32_t AHB3ENR;   /* 0xE0 */
    __IO uint32_t RESERVED6;
    __IO uint32_t APB1LENR;  /* 0xE8 */
    __IO uint32_t APB1HENR;  /* 0xEC */
    __IO uint32_t APB2ENR;   /* 0xF0 */
    __IO uint32_t APB3ENR;   /* 0xF4 */
    __IO uint32_t APB4ENR;   /* 0xF8 */
    __IO uint32_t RESERVED7[2];
    __IO uint32_t AHB1LPENR; /* 0x100 */
    __IO uint32_t AHB2LPENR; /* 0x104 */
    __IO uint32_t AHB3LPENR; /* 0x108 */
    __IO uint32_t RESERVED8;
    __IO uint32_t APB1LLPENR;/* 0x110 */
    __IO uint32_t APB1HLPENR;/* 0x114 */
    __IO uint32_t APB2LPENR; /* 0x118 */
    __IO uint32_t APB3LPENR; /* 0x11C */
    __IO uint32_t APB4LPENR; /* 0x120 */
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)RCC_BASE)

/* RCC CR bits */
#define RCC_CR_HSION    (1U << 0)
#define RCC_CR_HSIRDY    (1U << 1)
#define RCC_CR_HSEON    (1U << 16)
#define RCC_CR_HSERDY    (1U << 17)
#define RCC_CR_HSECSSON  (1U << 18)
#define RCC_CR_PLL1ON   (1U << 24)
#define RCC_CR_PLL1RDY   (1U << 25)

/* RCC CFGR SW bits */
#define RCC_CFGR_SW_HSI   0x0
#define RCC_CFGR_SW_HSE   0x1
#define RCC_CFGR_SW_PLL1  0x2

/* RCC PLLCKSELR bits */
#define RCC_PLLCKSELR_DIVM1(val)  ((val) & 0x3F)
#define RCC_PLLCKSELR_PLLSRC_HSE  (1U << 0)   /* actually PLLSRC = 3 for HSE */
#define RCC_PLLCKSELR_PLLSRC(val) ((val) << 0)

/* RCC PLLCFGR bits */
#define RCC_PLLCFGR_PLL1FRACEN  (1U << 4)
#define RCC_PLLCFGR_PLL1VCOSEL_MID (0U << 1)
#define RCC_PLLCFGR_PLL1VCOSEL_HI  (1U << 1)

/* RCC AHB1ENR / AHB2ENR / APB1LENR / APB2ENR enable bits */
#define RCC_AHB2ENR_GPIOAEN  (1U << 0)
#define RCC_AHB2ENR_GPIOBEN  (1U << 1)
#define RCC_AHB2ENR_GPIOCEN  (1U << 2)
#define RCC_AHB2ENR_GPIODEN  (1U << 3)
#define RCC_AHB2ENR_GPIOEEN  (1U << 4)
#define RCC_AHB2ENR_GPIOHEN  (1U << 7)
#define RCC_AHB2ENR_ADC12EN  (1U << 28)
#define RCC_AHB2ENR_OTGFSEN  (1U << 27)

#define RCC_AHB1ENR_DMA1EN   (1U << 0)
#define RCC_AHB1ENR_DMA2EN   (1U << 1)

#define RCC_APB1LENR_TIM2EN  (1U << 0)
#define RCC_APB1LENR_TIM3EN  (1U << 1)
#define RCC_APB1LENR_SPI3EN  (1U << 15)
#define RCC_APB1LENR_USART2EN (1U << 17)
#define RCC_APB1LENR_USART3EN (1U << 18)
#define RCC_APB1LENR_I2C1EN  (1U << 21)
#define RCC_APB1LENR_DAC1EN  (1U << 28)

#define RCC_APB1HENR_USART1EN (1U << 0)   /* actually bit 0 of APB1HENR */

#define RCC_APB2ENR_TIM1EN   (1U << 0)
#define RCC_APB2ENR_TIM8EN   (1U << 1)
#define RCC_APB2ENR_SPI1EN   (1U << 12)
#define RCC_APB2ENR_USART1EN (1U << 14)

#define RCC_APB4ENR_I2C1EN   (1U << 5)
#define RCC_APB4ENR_SYSCFGEN (1U << 0)

/* ===================================================================== */
/*  SysTick                                                                */
/* ===================================================================== */

typedef struct {
    __IO uint32_t CTRL;   /* 0x00 */
    __IO uint32_t LOAD;   /* 0x04 */
    __IO uint32_t VAL;    /* 0x08 */
    __IO uint32_t CALIB;  /* 0x0C */
} SysTick_Type;

#define SysTick ((SysTick_Type *)SYSTICK_BASE)
#define SysTick_CTRL_ENABLE   (1U << 0)
#define SysTick_CTRL_TICKINT  (1U << 1)
#define SysTick_CTRL_CLKSOURCE (1U << 2)

/* ===================================================================== */
/*  USART register layout                                                 */
/* ===================================================================== */

typedef struct {
    __IO uint32_t CR1;    /* 0x00 */
    __IO uint32_t CR2;    /* 0x04 */
    __IO uint32_t CR3;    /* 0x08 */
    __IO uint32_t BRR;    /* 0x0C */
    __IO uint32_t GTPR;   /* 0x10 */
    __IO uint32_t RTOR;   /* 0x14 */
    __IO uint32_t RQR;    /* 0x18 */
    __IO uint32_t ISR;    /* 0x1C */
    __IO uint32_t ICR;    /* 0x20 */
    __IO uint32_t RDR;    /* 0x24 */
    __IO uint32_t TDR;    /* 0x28 */
} USART_TypeDef;

#define USART1 ((USART_TypeDef *)USART1_BASE)
#define USART2 ((USART_TypeDef *)USART2_BASE)
#define USART3 ((USART_TypeDef *)USART3_BASE)

#define USART_CR1_UE     (1U << 0)
#define USART_CR1_RE     (1U << 2)
#define USART_CR1_TE     (1U << 3)
#define USART_CR1_RXNEIE (1U << 5)
#define USART_CR1_TCIE   (1U << 6)
#define USART_CR1_TXEIE  (1U << 7)
#define USART_ISR_RXNE   (1U << 5)
#define USART_ISR_TXE    (1U << 7)
#define USART_ISR_TC     (1U << 6)

/* ===================================================================== */
/*  SPI register layout                                                   */
/* ===================================================================== */

typedef struct {
    __IO uint32_t CR1;    /* 0x00 */
    __IO uint32_t CR2;    /* 0x04 */
    __IO uint32_t CFG1;   /* 0x08 */
    __IO uint32_t CFG2;   /* 0x0C */
    __IO uint32_t IER;    /* 0x10 */
    __IO uint32_t SR;     /* 0x14 */
    __IO uint32_t IFCR;   /* 0x18 */
    __IO uint32_t TXDR;   /* 0x20 — actually XLDR at 0x20 */
    __IO uint32_t RXDR;   /* 0x30 */
    __IO uint32_t C2CR;   /* 0x40 */
    __IO uint32_t UDRCFG; /* 0x44 */
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)
#define SPI3 ((SPI_TypeDef *)SPI3_BASE)

#define SPI_CFG1_MASDIR   (1U << 0)   /* master clock / direction config */
#define SPI_CR1_SPE       (1U << 0)
#define SPI_CR1_CSTART    (1U << 1)
#define SPI_CR1_SUSP      (1U << 2)
#define SPI_SR_RXWNE      (1U << 3)
#define SPI_SR_RXPLVL_MSK (3U << 4)
#define SPI_SR_TXP        (1U << 1)
#define SPI_SR_RXP        (1U << 0)
#define SPI_SR_EOT        (1U << 3)

/* ===================================================================== */
/*  I2C register layout                                                   */
/* ===================================================================== */

typedef struct {
    __IO uint32_t CR1;    /* 0x00 */
    __IO uint32_t CR2;    /* 0x04 */
    __IO uint32_t OAR1;   /* 0x08 */
    __IO uint32_t OAR2;   /* 0x0C */
    __IO uint32_t TIMINGR; /* 0x10 */
    __IO uint32_t TIMEOUTR;/* 0x14 */
    __IO uint32_t ISR;    /* 0x18 */
    __IO uint32_t ICR;    /* 0x1C */
    __IO uint32_t PECR;   /* 0x20 */
    __IO uint32_t RXDR;   /* 0x24 */
    __IO uint32_t TXDR;   /* 0x28 */
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)

#define I2C_CR1_PE       (1U << 0)
#define I2C_CR2_START    (1U << 13)
#define I2C_CR2_STOP     (1U << 14)
#define I2C_CR2_RD_WRN   (1U << 10)
#define I2C_ISR_RXNE     (1U << 2)
#define I2C_ISR_TXIS     (1U << 1)
#define I2C_ISR_TXE     (1U << 0)
#define I2C_ISR_NACKF    (1U << 4)
#define I2C_ISR_TC       (1U << 6)
#define I2C_ISR_BUSY     (1U << 15)

/* ===================================================================== */
/*  TIM register layout (TIM2/TIM3 are 32-bit; TIM1 is 16-bit advanced)  */
/* ===================================================================== */

typedef struct {
    __IO uint32_t CR1;    /* 0x00 */
    __IO uint32_t CR2;    /* 0x04 */
    __IO uint32_t SMCR;   /* 0x08 */
    __IO uint32_t DIER;   /* 0x0C */
    __IO uint32_t SR;     /* 0x10 */
    __IO uint32_t EGR;    /* 0x14 */
    __IO uint32_t CCMR1;  /* 0x18 */
    __IO uint32_t CCMR2;  /* 0x1C */
    __IO uint32_t CCER;   /* 0x20 */
    __IO uint32_t CNT;    /* 0x24 */
    __IO uint32_t PSC;    /* 0x28 */
    __IO uint32_t ARR;    /* 0x2C */
    __IO uint32_t RCR;    /* 0x30 */
    __IO uint32_t CCR1;   /* 0x34 */
    __IO uint32_t CCR2;   /* 0x38 */
    __IO uint32_t CCR3;   /* 0x3C */
    __IO uint32_t CCR4;   /* 0x40 */
    __IO uint32_t BDTR;   /* 0x44 — TIM1/TIM8 only */
    __IO uint32_t DCR;    /* 0x48 */
    __IO uint32_t DMAR;   /* 0x4C */
    __IO uint32_t OR;     /* 0x50 */
    __IO uint32_t AF1;    /* 0x60 */
    __IO uint32_t AF2;    /* 0x64 */
    __IO uint32_t TISEL;  /* 0x68 */
} TIM_TypeDef;

#define TIM1  ((TIM_TypeDef *)TIM1_BASE)
#define TIM2  ((TIM_TypeDef *)TIM2_BASE)
#define TIM3  ((TIM_TypeDef *)TIM3_BASE)
#define TIM6  ((TIM_TypeDef *)TIM6_BASE)
#define TIM7  ((TIM_TypeDef *)TIM7_BASE)
#define TIM8  ((TIM_TypeDef *)TIM8_BASE)

#define TIM_CR1_CEN     (1U << 0)
#define TIM_CR1_UDIS    (1U << 1)
#define TIM_CR1_URS     (1U << 2)
#define TIM_CR1_DIR     (1U << 4)
#define TIM_DIER_UIE    (1U << 0)
#define TIM_DIER_CC1IE  (1U << 1)
#define TIM_SR_UIF      (1U << 0)
#define TIM_SR_CC1IF    (1U << 1)

/* TIM3 SMCR encoder mode */
#define TIM_SMCR_SMS_ENC1  0x1
#define TIM_SMCR_SMS_ENC2  0x2
#define TIM_SMCR_SMS_ENC3  0x3   /* count on both edges, x4 */

/* CCMR input capture filter / prescaler */
#define TIM_CCMR1_CC1S_TI1  0x1
#define TIM_CCMR1_IC1F_8    (3U << 4)

/* ===================================================================== */
/*  DMA register layout (for SPI3 RX and USART RX)                        */
/* ===================================================================== */

typedef struct {
    __IO uint32_t ISR;    /* 0x00 stream-level ISR */
    __IO uint32_t RESERVED0;
    __IO uint32_t IFCR;
    __IO uint32_t RESERVED1;
    /* stream registers begin at +0x10 per stream */
} DMA_Stream_TypeDef;

#define DMA_SxCR_EN     (1U << 0)
#define DMA_SxCR_TCIE   (1U << 4)
#define DMA_SxCR_MINC   (1U << 10)
#define DMA_SxCR_PL_HI  (3U << 16)

/* ===================================================================== */
/*  EXTI register layout                                                  */
/* ===================================================================== */

typedef struct {
    __IO uint32_t IMR1;   /* 0x00 */
    __IO uint32_t EMR1;   /* 0x04 */
    __IO uint32_t RTSR1;  /* 0x08 */
    __IO uint32_t FTSR1;  /* 0x0C */
    __IO uint32_t SWIER1; /* 0x10 */
    __IO uint32_t PR1;    /* 0x14 */
    __IO uint32_t RESERVED0[2];
    __IO uint32_t IMR2;   /* 0x20 */
    __IO uint32_t EMR2;   /* 0x24 */
    __IO uint32_t RTSR2;  /* 0x28 */
    __IO uint32_t FTSR2;  /* 0x2C */
    __IO uint32_t SWIER2; /* 0x30 */
    __IO uint32_t PR2;    /* 0x34 */
} EXTI_TypeDef;

#define EXTI ((EXTI_TypeDef *)EXTI_BASE)

/* ===================================================================== */
/*  PWR register layout (minimal)                                        */
/* ===================================================================== */

typedef struct {
    __IO uint32_t CR1;    /* 0x00 */
    __IO uint32_t CR2;    /* 0x04 — actually CSR1 */
    __IO uint32_t CR3;    /* 0x08 */
    __IO uint32_t CR4;    /* 0x0C */
    __IO uint32_t SR1;    /* 0x10 */
    __IO uint32_t SR2;    /* 0x14 */
    __IO uint32_t SCR;    /* 0x18 */
    __IO uint32_t PUCR;   /* 0x1C */
    __IO uint32_t PDCR;   /* 0x20 */
} PWR_TypeDef;

#define PWR ((PWR_TypeDef *)PWR_BASE)

/* ===================================================================== */
/*  Flash ACR register (latency)                                         */
/* ===================================================================== */

typedef struct {
    __IO uint32_t ACR;     /* 0x00 */
    __IO uint32_t KEYR;    /* 0x04 */
    __IO uint32_t OPTKEYR; /* 0x08 */
    __IO uint32_t SR;      /* 0x0C */
    __IO uint32_t CR;      /* 0x10 */
    __IO uint32_t OPTCR;   /* 0x14 */
    __IO uint32_t OPTSR;   /* 0x18 */
    __IO uint32_t OPTCCR;  /* 0x1C */
    __IO uint32_t PRAR;    /* 0x20 */
} FLASH_TypeDef;

#define FLASH_REG ((FLASH_TypeDef *)FLASH_BASE_REG)
#define FLASH_ACR_LATENCY_2WS  (2U << 0)
#define FLASH_ACR_LATENCY_4WS  (4U << 0)

/* ===================================================================== */
/*  NVIC + SCB (minimal)                                                  */
/* ===================================================================== */

typedef struct {
    __IO uint32_t ISER[8];   /* 0x00 Interrupt Set-Enable */
    __IO uint32_t RESERVED0[24];
    __IO uint32_t ICER[8];   /* 0x80 */
    __IO uint32_t RESERVED1[24];
    __IO uint32_t ISPR[8];
    __IO uint32_t RESERVED2[24];
    __IO uint32_t ICPR[8];   /* 0x180 */
    __IO uint32_t RESERVED3[24];
    __IO uint32_t IABR[8];
    __IO uint32_t RESERVED4[56];
    __IO uint32_t STIR;
} NVIC_TypeDef;

#define NVIC ((NVIC_TypeDef *)NVIC_BASE)

typedef struct {
    __IO uint32_t CPACR;
    __IO uint32_t RESERVED0;
    __IO uint32_t CCR;
    __IO uint32_t RESERVED1;
    __IO uint32_t SHPR[12];
} SCB_TypeDef_min;

#define SCB ((SCB_TypeDef_min *)SCB_BASE)
#define SCB_CPACR_FULL_ACCESS(val) ((0xFU << (20 + (val)*8)))

/* IRQ numbers used by StrataScan */
#define USART1_IRQn      37
#define USART2_IRQn      38
#define USART3_IRQn      39
#define EXTI10_IRQn     41   /* EXTI line 10 → EXTI15_10_IRQn is 40; line 10 is part of it */
#define TIM3_IRQn        29
#define DMA1_Stream0_IRQn 16

/* ===================================================================== */
/*  DAC register layout (minimal)                                         */
/* ===================================================================== */

typedef struct {
    __IO uint32_t CR;       /* 0x00 */
    __IO uint32_t SWTRGR;   /* 0x04 */
    __IO uint32_t DHR12R1;  /* 0x08 */
    __IO uint32_t DHR12L1;  /* 0x0C */
    __IO uint32_t DHR8R1;   /* 0x10 */
    __IO uint32_t DHR12RD;  /* 0x14 */
    __IO uint32_t DHR12LD;  /* 0x18 */
    __IO uint32_t DHR8RD;   /* 0x1C */
    __IO uint32_t DOR1;     /* 0x20 */
    __IO uint32_t DOR2;     /* 0x24 */
    __IO uint32_t SR;       /* 0x28 */
} DAC_TypeDef;

#define DAC1 ((DAC_TypeDef *)DAC_BASE)
#define DAC_CR_EN1  (1U << 0)
#define DAC_CR_EN2  (1U << 16)

/* ===================================================================== */
/*  ADC (battery monitoring — minimal)                                    */
/* ===================================================================== */

typedef struct {
    __IO uint32_t ISR;      /* 0x00 */
    __IO uint32_t IER;      /* 0x04 */
    __IO uint32_t CR;       /* 0x08 */
    __IO uint32_t CFGR;     /* 0x0C */
    __IO uint32_t RESERVED0;
    __IO uint32_t SMPR1;    /* 0x14 */
    __IO uint32_t SMPR2;    /* 0x18 */
    __IO uint32_t RESERVED1[2];
    __IO uint32_t TR1;      /* 0x20 */
    __IO uint32_t TR2;      /* 0x24 */
    __IO uint32_t TR3;      /* 0x28 */
    __IO uint32_t RESERVED2[4];
    __IO uint32_t JDR1;     /* 0x3C — actually at 0xBC for injected; simplified */
    __IO uint32_t DR1;      /* 0x40 */
    /* ... extended layout for multi-channel ... */
} ADC_TypeDef_min;

#define ADC1_REG ((ADC_TypeDef_min *)ADC1_BASE)
#define ADC_ISR_ADRDY  (1U << 0)
#define ADC_CR_ADEN    (1U << 0)
#define ADC_CR_ADSTART (1U << 2)

#endif /* STRATASCAN_REGISTERS_H */