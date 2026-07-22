/*
 * registers.h — STM32L4R5 register definitions used by RebarScope firmware
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Only the peripheral registers touched by the drivers are defined; the
 * full STM32L4 register set is huge, so we declare what we need and avoid
 * pulling in the vendor header in this open-source reference build.
 * The names follow the ST RM0351 reference manual conventions.
 */
#ifndef REBARSCOPE_REGISTERS_H
#define REBARSCOPE_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses (selected, STM32L4R5) ---- */
#define PERIPH_BASE       0x40000000u
#define AHB1PERIPH_BASE   0x40018000u
#define APB1PERIPH_BASE   0x40000000u
#define APB2PERIPH_BASE   0x40010000u
#define AHB2PERIPH_BASE   0x48000000u  /* GPIO ports */

/* ---- GPIO (AHB2) ---- */
typedef struct {
    volatile uint32_t MODER;     /* 0x00 */
    volatile uint32_t OTYPER;   /* 0x04 */
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

#define GPIOA_BASE  (AHB2PERIPH_BASE + 0x0000)
#define GPIOB_BASE  (AHB2PERIPH_BASE + 0x0400)
#define GPIOC_BASE  (AHB2PERIPH_BASE + 0x0800)
#define GPIOD_BASE  (AHB2PERIPH_BASE + 0x0C00)
#define GPIOE_BASE  (AHB2PERIPH_BASE + 0x1000)
#define GPIOH_BASE  (AHB2PERIPH_BASE + 0x1C00)

#define GPIOA       ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB       ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC       ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD       ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE       ((GPIO_TypeDef *) GPIOE_BASE)
#define GPIOH       ((GPIO_TypeDef *) GPIOH_BASE)

/* GPIO mode bits */
#define GPIO_MODE_INPUT    0
#define GPIO_MODE_OUTPUT   1
#define GPIO_MODE_AF       2
#define GPIO_MODE_ANALOG   3

#define GPIO_OTYPE_PP      0
#define GPIO_OTYPE_OD      1
#define GPIO_OSPEED_LOW    0
#define GPIO_OSPEED_HIGH   3
#define GPIO_PUPD_NONE     0
#define GPIO_PUPD_PU       1
#define GPIO_PUPD_PD       2

/* ---- RCC (AHB1) ---- */
#define RCC_BASE           (AHB1PERIPH_BASE + 0x1000)
typedef struct {
    volatile uint32_t CR;        /* 0x00 */
    volatile uint32_t ICSCR;     /* 0x04 */
    volatile uint32_t CRRCR;     /* 0x08 */
    volatile uint32_t CFGR;      /* 0x0C */
    volatile uint32_t RESERVED1; /* 0x10 */
    volatile uint32_t PLLCFGR;    /* 0x14 */
    volatile uint32_t PLLSAI1CFGR; /* 0x18 */
    volatile uint32_t PLLSAI2CFGR; /* 0x1C */
    volatile uint32_t CIER;       /* 0x20 */
    volatile uint32_t CIFR;       /* 0x24 */
    volatile uint32_t CICR;       /* 0x28 */
    volatile uint32_t RESERVED2;  /* 0x2C */
    volatile uint32_t RESERVED3;  /* 0x30 */
    volatile uint32_t CCIPR;      /* 0x34 */
    volatile uint32_t RESERVED4;  /* 0x38 */
    volatile uint32_t BDCR;       /* 0x3C */
    volatile uint32_t CSR;        /* 0x40 */
    volatile uint32_t CRRCR1;     /* 0x44 */
    volatile uint32_t CCIPR2;     /* 0x48 */
    /* AHB enable registers are further on; see RM0351 */
} RCC_TypeDef;

/* Simplified RCC enable register offsets (RM0351 §6.4.16+) */
#define RCC_AHB1ENR_OFFSET  0x4C
#define RCC_AHB2ENR_OFFSET  0x4C + 4   /* placeholder */
#define RCC_AHB3ENR_OFFSET  0x54
#define RCC_APB1ENR1_OFFSET 0x58
#define RCC_APB1ENR2_OFFSET 0x5C
#define RCC_APB2ENR_OFFSET  0x60
/* ... we only expose what the drivers touch ... */

#define RCC  ((RCC_TypeDef *) RCC_BASE)

#define RCC_AHB2ENR_GPIOA  (1u<<0)
#define RCC_AHB2ENR_GPIOB  (1u<<1)
#define RCC_AHB2ENR_GPIOC  (1u<<2)
#define RCC_AHB2ENR_GPIOD  (1u<<3)
#define RCC_AHB2ENR_GPIOE  (1u<<4)
#define RCC_AHB2ENR_GPIOH  (1u<<7)
#define RCC_AHB2ENR_ADC    (1u<<13)
#define RCC_AHB2ENR_RNG    (1u<<18)

#define RCC_APB1ENR1_TIM3   (1u<<1)
#define RCC_APB1ENR1_TIM6   (1u<<4)
#define RCC_APB1ENR1_SPI2   (1u<<14)
#define RCC_APB1ENR1_USART2 (1u<<17)
#define RCC_APB1ENR1_I2C1   (1u<<21)

#define RCC_APB2ENR_SPI1    (1u<<12)
#define RCC_APB2ENR_SPI4    (1u<<13)
#define RCC_APB2ENR_USART1  (1u<<14)
#define RCC_APB2ENR_TIM1    (1u<<11)

/* ---- SPI (APB) ---- */
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t SR;      /* 0x08 */
    volatile uint32_t DR;      /* 0x0C */
    volatile uint32_t CRCPR;    /* 0x10 */
    volatile uint32_t RXCRCR;  /* 0x14 */
    volatile uint32_t TXCRCR;  /* 0x18 */
} SPI_TypeDef;

#define SPI1_BASE (APB2PERIPH_BASE + 0x3000)
#define SPI2_BASE (APB1PERIPH_BASE + 0x3800)
#define SPI3_BASE (APB1PERIPH_BASE + 0x3C00)

#define SPI1  ((SPI_TypeDef *) SPI1_BASE)
#define SPI2  ((SPI_TypeDef *) SPI2_BASE)
#define SPI3  ((SPI_TypeDef *) SPI3_BASE)

#define SPI_CR1_SPE      (1u<<6)
#define SPI_CR1_MSTR     (1u<<2)
#define SPI_CR1_BR_DIV8   (0x2u<<3)
#define SPI_CR1_BR_DIV16  (0x3u<<3)
#define SPI_CR1_BR_DIV32  (0x4u<<3)
#define SPI_CR1_CPHA     (1u<<0)
#define SPI_CR1_CPOL     (1u<<1)
#define SPI_CR2_FRXTH    (1u<<12)
#define SPI_CR2_DS_8BIT  (0x7u<<8)
#define SPI_CR2_SSOE     (1u<<2)
#define SPI_CR2_RXNEIE   (1u<<6)
#define SPI_SR_RXNE      (1u<<0)
#define SPI_SR_TXE       (1u<<1)
#define SPI_SR_BSY       (1u<<7)

/* ---- USART (BLE) ---- */
typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t CR3;    /* 0x08 */
    volatile uint32_t BRR;    /* 0x0C */
    volatile uint32_t GTPR;   /* 0x10 */
    volatile uint32_t RTOR;   /* 0x14 */
    volatile uint32_t RQR;    /* 0x18 */
    volatile uint32_t ISR;    /* 0x1C */
    volatile uint32_t ICR;    /* 0x20 */
    volatile uint32_t RDR;    /* 0x24 */
    volatile uint32_t TDR;    /* 0x28 */
} USART_TypeDef;

#define USART2_BASE (APB1PERIPH_BASE + 0x4400)
#define USART2  ((USART_TypeDef *) USART2_BASE)

#define USART_CR1_UE     (1u<<0)
#define USART_CR1_TE     (1u<<3)
#define USART_CR1_RE     (1u<<2)
#define USART_CR1_RXNEIE (1u<<5)
#define USART_ISR_RXNE   (1u<<5)
#define USART_ISR_TXE    (1u<<7)

/* ---- I2C (IMU, fuel gauge) ---- */
typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t OAR1;   /* 0x08 */
    volatile uint32_t OAR2;   /* 0x0C */
    volatile uint32_t TIMINGR;/* 0x10 */
    volatile uint32_t TIMEOUTR;/* 0x14 */
    volatile uint32_t ISR;    /* 0x18 */
    volatile uint32_t ICR;    /* 0x1C */
    volatile uint32_t PECR;   /* 0x20 */
    volatile uint32_t RXDR;   /* 0x24 */
    volatile uint32_t TXDR;   /* 0x28 */
} I2C_TypeDef;

#define I2C1_BASE (APB1PERIPH_BASE + 0x5400)
#define I2C1  ((I2C_TypeDef *) I2C1_BASE)

#define I2C_CR1_PE       (1u<<0)
#define I2C_CR2_START    (1u<<13)
#define I2C_CR2_STOP     (1u<<14)
#define I2C_CR2_NBYTES_SHIFT 16
#define I2C_ISR_TXIS     (1u<<1)
#define I2C_ISR_RXNE     (1u<<2)
#define I2C_ISR_TCR      (1u<<4)
#define I2C_ISR_TC       (1u<<6)
#define I2C_ISR_BUSY     (1u<<15)

/* ---- TIM3 (encoder) ---- */
typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t SMCR;   /* 0x08 */
    volatile uint32_t DIER;   /* 0x0C */
    volatile uint32_t SR;     /* 0x10 */
    volatile uint32_t EGR;    /* 0x14 */
    volatile uint32_t CCMR1;  /* 0x18 */
    volatile uint32_t CCMR2;  /* 0x1C */
    volatile uint32_t CCER;   /* 0x20 */
    volatile uint32_t CNT;    /* 0x24 */
    volatile uint32_t PSC;    /* 0x28 */
    volatile uint32_t ARR;    /* 0x2C */
    volatile uint32_t CCR1;   /* 0x30 */
    volatile uint32_t CCR2;   /* 0x34 */
    volatile uint32_t CCR3;   /* 0x38 */
    volatile uint32_t CCR4;   /* 0x3C */
} TIM_TypeDef;

#define TIM3_BASE (APB1PERIPH_BASE + 0x0400)
#define TIM3  ((TIM_TypeDef *) TIM3_BASE)

#define TIM_CR1_CEN      (1u<<0)
#define TIM_SMCR_SMS_ENC3 0x3u    /* encoder mode 3 */
#define TIM_CCMR1_CC1S_TI2 2
#define TIM_CCMR1_CC2S_TI2 2

/* ---- DMA (selected, channel 3 for SPI1 RX) ---- */
#define DMA1_BASE        (AHB1PERIPH_BASE + 0x2000)
typedef struct {
    volatile uint32_t ISR;   /* 0x00 */
    volatile uint32_t IFCCR; /* 0x04 */
} DMA_COMMON_TypeDef;
#define DMA1 ((DMA_COMMON_TypeDef *) DMA1_BASE)

/* ---- Flash (option bytes / calibration) ---- */
#define FLASH_BASE       0x40022000u
#define FLASH_KEYR       (*(volatile uint32_t *)(FLASH_BASE + 0x08))
#define FLASH_SR         (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_CR         (*(volatile uint32_t *)(FLASH_BASE + 0x14))

/* Calibration storage at the last 2 KB sector of 2 MB flash */
#define CAL_FLASH_BASE    0x0807F800u
#define CAL_MAGIC         0x52534341u   /* 'RSCA' */

/* ---- Helper macros ---- */
#define BIT(n)        (1u << (n))
#define REG_WRITE(a, v)   (*(volatile uint32_t *)(a) = (v))
#define REG_READ(a)       (*(volatile uint32_t *)(a))
#define REG_OR(a, m)      (*(volatile uint32_t *)(a) |= (m))
#define REG_ANDN(a, m)    (*(volatile uint32_t *)(a) &= ~(m))

#endif /* REBARSCOPE_REGISTERS_H */