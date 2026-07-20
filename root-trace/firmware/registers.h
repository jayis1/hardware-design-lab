/*
 * registers.h — RootTrace STM32H743 & AD5940 Register Definitions
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Low-level register address definitions for the STM32H743 Cortex-M7
 * microcontroller and the AD5940 bioimpedance analog front-end.
 * These mirror the reference manual (RM0433 rev 8) and the AD5940
 * datasheet (rev B), but are kept self-contained so the firmware
 * build does not depend on vendor headers beyond the CMSIS core.
 */

#ifndef ROOTTRACE_REGISTERS_H
#define ROOTTRACE_REGISTERS_H

#include <stdint.h>

/* ===================================================================== *
 *  STM32H743 Memory Map (RM0433)
 * ===================================================================== */

#define PERIPH_BASE         (0x40000000UL)
#define PERIPH_BASE_AHB1    (0x40018000UL)
#define PERIPH_BASE_AHB4    (0x58020000UL)
#define PERIPH_BASE_APB1L   (0x40000000UL)
#define PERIPH_BASE_APB1H   (0x40001000UL)
#define PERIPH_BASE_APB2    (0x48010000UL)

/* --- AHB1 peripherals --- */
#define DMA1_BASE           (PERIPH_BASE_AHB1 + 0x0000)
#define DMA2_BASE           (PERIPH_BASE_AHB1 + 0x0400)
#define DMAMUX1_BASE        (PERIPH_BASE_AHB1 + 0x0800)
#define RCC_BASE            (0x58024400UL)
#define PWR_BASE            (0x58024800UL)

/* --- AHB4 peripherals (GPIO) --- */
#define GPIOA_BASE          (0x58020000UL)
#define GPIOB_BASE          (0x58020400UL)
#define GPIOC_BASE          (0x58020800UL)
#define GPIOD_BASE          (0x58020C00UL)
#define GPIOE_BASE          (0x58021000UL)
#define GPIOG_BASE          (0x58021800UL)
#define GPIOH_BASE          (0x58021C00UL)

/* --- APB1 peripherals --- */
#define SPI2_BASE            (0x58013000UL)  /* APB1P */
#define SPI3_BASE            (0x58002000UL)  /* APB1L */
#define USART2_BASE          (0x58000400UL)  /* APB1L */
#define USART3_BASE          (0x58000800UL)  /* APB1L */
#define UART4_BASE           (0x58000C00UL)  /* APB1L */
#define UART7_BASE           (0x58001400UL)  /* APB1L */
#define I2C1_BASE            (0x58005400UL)  /* APB1L */
#define I2C3_BASE            (0x58005C00UL)  /* APB1L */
#define I2C4_BASE            (0x58021C00UL)  /* APB4 */
#define TIM2_BASE            (0x58000000UL)  /* APB1L */
#define TIM4_BASE            (0x58000800UL)  /* APB1L -- not used, placeholder */
#define LPTIM1_BASE          (0x58000C00UL)

/* --- APB2 peripherals --- */
#define SPI1_BASE            (0x5800A400UL)  /* APB2L */
#define SPI4_BASE            (0x5800B400UL)
#define SPI5_BASE            (0x5800B800UL)
#define USART1_BASE          (0x5800AC00UL)
#define TIM1_BASE            (0x58009000UL)
#define TIM8_BASE            (0x58009400UL)
#define SDMMC1_BASE          (0x58002400UL)  /* APB2L */

/* --- System & misc --- */
#define SYSCFG_BASE          (0x58000000UL)   /* APB4 */
#define NVIC_VTOR            (0xE000ED08UL)
#define SCB_AIRCR            (0xE000ED0CUL)
#define FPU_CPACR            (0xE000ED88UL)

/* --- Interrupt numbers (IRQn) --- */
#define DMA1_STR0_IRQn       0
#define DMA1_STR1_IRQn       1
#define DMA1_STR2_IRQn       2
#define DMA1_STR3_IRQn       3
#define DMA1_STR4_IRQn       4
#define DMA1_STR5_IRQn       5
#define DMA1_STR6_IRQn       6
#define DMA1_STR7_IRQn       7
#define DMA2_STR0_IRQn       8
#define DMA1_STR8_IRQn       56
#define EXTI9_5_IRQn         23
#define SPI1_IRQn            24
#define SPI2_IRQn            25
#define SPI3_IRQn            51
#define SPI4_IRQn            52
#define SPI5_IRQn            53
#define USART1_IRQn          61
#define USART2_IRQn          62
#define USART3_IRQn          63
#define UART4_IRQn           64
#define UART7_IRQn           67
#define I2C1_EV_IRQn         31
#define I2C1_ER_IRQn         32
#define I2C3_EV_IRQn         36
#define I2C3_ER_IRQn         37
#define I2C4_EV_IRQn         82
#define I2C4_ER_IRQn         83
#define TIM2_IRQn            28
#define EXTI15_10_IRQn       40
#define SDMMC1_IRQn           49
#define LPTIM1_IRQn          86

/* ===================================================================== *
 *  RCC (Reset & Clock Control)  — RM0433 §7
 * ===================================================================== */

#define RCC_OFFSET(x)  (RCC_BASE + (x))

#define RCC_CR         (*(volatile uint32_t *)(RCC_OFFSET(0x000)))
#define RCC_HSICFGR    (*(volatile uint32_t *)(RCC_OFFSET(0x004)))
#define RCC_CRRCR      (*(volatile uint32_t *)(RCC_OFFSET(0x008)))
#define RCC_CFGR       (*(volatile uint32_t *)(RCC_OFFSET(0x010)))
#define RCC_D1CFGR     (*(volatile uint32_t *)(RCC_OFFSET(0x018)))
#define RCC_D2CFGR     (*(volatile uint32_t *)(RCC_OFFSET(0x01C)))
#define RCC_D3CFGR     (*(volatile uint32_t *)(RCC_OFFSET(0x020)))
#define RCC_PLLCKSELR  (*(volatile uint32_t *)(RCC_OFFSET(0x028)))
#define RCC_PLL1DIVR   (*(volatile uint32_t *)(RCC_OFFSET(0x02C)))
#define RCC_PLL1DIVP1  (*(volatile uint32_t *)(RCC_OFFSET(0x030)))
#define RCC_PLL1FRACR  (*(volatile uint32_t *)(RCC_OFFSET(0x034)))
#define RCC_PLLCFGR    (*(volatile uint32_t *)(RCC_OFFSET(0x038)))
#define RCC_C1_D1CCIPR (*(volatile uint32_t *)(RCC_OFFSET(0x04C)))
#define RCC_D2CCIP1R   (*(volatile uint32_t *)(RCC_OFFSET(0x050)))
#define RCC_D2CCIP2R   (*(volatile uint32_t *)(RCC_OFFSET(0x054)))
#define RCC_D3CCIPR    (*(volatile uint32_t *)(RCC_OFFSET(0x058)))
#define RCC_AHB3ENR    (*(volatile uint32_t *)(RCC_OFFSET(0x0D0)))
#define RCC_AHB1ENR    (*(volatile uint32_t *)(RCC_OFFSET(0x0D4)))
#define RCC_AHB2ENR    (*(volatile uint32_t *)(RCC_OFFSET(0x0D8)))
#define RCC_AHB4ENR    (*(volatile uint32_t *)(RCC_OFFSET(0x0DC)))
#define RCC_APB1LENR   (*(volatile uint32_t *)(RCC_OFFSET(0x0E0)))
#define RCC_APB1HENR   (*(volatile uint32_t *)(RCC_OFFSET(0x0E4)))
#define RCC_APB2ENR    (*(volatile uint32_t *)(RCC_OFFSET(0x0E8)))
#define RCC_APB3ENR    (*(volatile uint32_t *)(RCC_OFFSET(0x0EC)))
#define RCC_APB4ENR    (*(volatile uint32_t *)(RCC_OFFSET(0x0F0)))

/* RCC bits */
#define RCC_CR_HSIRDY          (1U << 4)
#define RCC_CR_HSERDY          (1U << 17)
#define RCC_CR_HSEON           (1U << 16)
#define RCC_CR_PLL1RDY         (1U << 25)
#define RCC_CR_PLL1ON          (1U << 24)
#define RCC_CR_CSIRDY          (1U << 8)
#define RCC_CR_CSION           (1U << 7)

/* ===================================================================== *
 *  PWR (Power Control) — RM0433 §6
 * ===================================================================== */

#define PWR_OFFSET(x)  (PWR_BASE + (x))
#define PWR_CR1        (*(volatile uint32_t *)(PWR_OFFSET(0x000)))
#define PWR_CSR1       (*(volatile uint32_t *)(PWR_OFFSET(0x004)))
#define PWR_CR3        (*(volatile uint32_t *)(PWR_OFFSET(0x00C)))
#define PWR_CPUCR      (*(volatile uint32_t *)(PWR_OFFSET(0x038)))

#define PWR_CPUCR_PDDS_D2    (1U << 7)
#define PWR_CPUCR_RUN_PD     (1U << 10)
#define PWR_CPUCR_SBF_D2     (1U << 9)
#define PWR_CR1_LPDS         (1U << 0)
#define PWR_CR3_SCPEN        (1U << 4)
#define PWR_CR3_USB33DEN     (1U << 24)

/* ===================================================================== *
 *  GPIO — RM0433 §11
 * ===================================================================== */

typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
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
} gpio_reg_t;

#define GPIOA  ((gpio_reg_t *)GPIOA_BASE)
#define GPIOB  ((gpio_reg_t *)GPIOB_BASE)
#define GPIOC  ((gpio_reg_t *)GPIOC_BASE)
#define GPIOD  ((gpio_reg_t *)GPIOD_BASE)
#define GPIOE  ((gpio_reg_t *)GPIOE_BASE)
#define GPIOG  ((gpio_reg_t *)GPIOG_BASE)
#define GPIOH  ((gpio_reg_t *)GPIOH_BASE)

/* GPIO mode constants */
#define GPIO_MODE_IN        0
#define GPIO_MODE_OUT        1
#define GPIO_MODE_AF         2
#define GPIO_MODE_ANALOG     3

#define GPIO_OTYPE_PP        0
#define GPIO_OTYPE_OD        1

#define GPIO_SPEED_LOW       0
#define GPIO_SPEED_MID       1
#define GPIO_SPEED_HIGH      2
#define GPIO_SPEED_VHIGH     3

#define GPIO_PUPD_NONE        0
#define GPIO_PUPD_PU          1
#define GPIO_PUPD_PD          2

/* ===================================================================== *
 *  DMA (DMA1 / DMA2) — RM0433 §16
 * ===================================================================== */

typedef struct {
    volatile uint32_t ISR;          /* 0x00 */
    volatile uint32_t Reserved0;    /* 0x04 */
    volatile uint32_t IFCR;          /* 0x08 */
} dma_common_reg_t;

typedef struct {
    volatile uint32_t CR;            /* 0x00 — channel/stream control */
    volatile uint32_t NDTR;          /* 0x04 — transfer count */
    volatile uint32_t PAR;           /* 0x08 — peripheral address */
    volatile uint32_t M0AR;          /* 0x0C — memory 0 address */
    volatile uint32_t M1AR;          /* 0x10 — memory 1 address */
    volatile uint32_t FCR;           /* 0x14 — FIFO control */
} dma_stream_t;

#define DMA1_STREAM(n)  ((dma_stream_t *)(DMA1_BASE + (n) * 0x18))
#define DMA2_STREAM(n)  ((dma_stream_t *)(DMA2_BASE + (n) * 0x18))
#define DMA1_COMMON    (*(dma_common_reg_t *)(DMA1_BASE + 0x00))
#define DMA2_COMMON    (*(dma_common_reg_t *)(DMA2_BASE + 0x00))

#define DMA_SxCR_EN         (1U << 0)
#define DMA_SxCR_DMEIE      (1U << 1)
#define DMA_SxCR_TEIE      (1U << 2)
#define DMA_SxCR_HTIE      (1U << 3)
#define DMA_SxCR_TCIE      (1U << 4)
#define DMA_SxCR_PFCTRL    (1U << 5)
#define DMA_SxCR_MINC      (1U << 10)
#define DMA_SxCR_PINC      (1U << 9)
#define DMA_SxCR_CIRC      (1U << 8)

/* ===================================================================== *
 *  SPI — RM0433 §39
 * ===================================================================== */

typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t CFG1;      /* 0x08 */
    volatile uint32_t CFG2;      /* 0x0C */
    volatile uint32_t IER;       /* 0x10 */
    volatile uint32_t SR;        /* 0x14 */
    volatile uint32_t IFCR;      /* 0x18 */
    volatile uint32_t TXDR;      /* 0x1C (packed) */
    volatile uint32_t RXDR;      /* 0x20 (packed) */
} spi_reg_t;

#define SPI1  ((spi_reg_t *)SPI1_BASE)
#define SPI2  ((spi_reg_t *)SPI2_BASE)
#define SPI3  ((spi_reg_t *)SPI3_BASE)
#define SPI4  ((spi_reg_t *)SPI4_BASE)
#define SPI5  ((spi_reg_t *)SPI5_BASE)

#define SPI_CR1_SPE          (1U << 0)
#define SPI_CR1_MASRX        (1U << 8)
#define SPI_CR1_CSTART        (1U << 9)
#define SPI_CFG1_BAUD_SHIFT   0
#define SPI_CFG1_BAUD_MASK   0x1F
#define SPI_CFG1_DSIZE_SHIFT  0
#define SPI_CFG2_MASTER       (1U << 22)
#define SPI_CFG2_SSM          (1U << 8)
#define SPI_CFG2_SSOE         (1U << 7)
#define SPI_CFG2_CPOL         (1U << 0)
#define SPI_CFG2_CPHA         (1U << 1)
#define SPI_CFG2_LSBFRST      (1U << 4)
#define SPI_SR_TXP            (1U << 1)
#define SPI_SR_RXP            (1U << 2)
#define SPI_SR_EOT            (1U << 3)
#define SPI_SR_OVR            (1U << 6)
#define SPI_SR_TIFRE          (1U << 8)
#define SPI_SR_SUSP           (1U << 11)

/* ===================================================================== *
 *  USART — RM0433 §42
 * ===================================================================== */

typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t CR3;      /* 0x08 */
    volatile uint32_t BRR;      /* 0x0C */
    volatile uint32_t GTPR;     /*0x10 */
    volatile uint32_t RTOR;     /* 0x14 */
    volatile uint32_t RQR;      /* 0x18 */
    volatile uint32_t ISR;      /* 0x1C */
    volatile uint32_t ICR;      /* 0x20 */
    volatile uint32_t RDR;      /* 0x24 */
    volatile uint32_t TDR;      /* 0x28 */
} usart_reg_t;

#define USART1  ((usart_reg_t *)USART1_BASE)
#define USART2  ((usart_reg_t *)USART2_BASE)
#define USART3  ((usart_reg_t *)USART3_BASE)
#define UART4   ((usart_reg_t *)UART4_BASE)
#define UART7   ((usart_reg_t *)UART7_BASE)

#define USART_CR1_UE          (1U << 0)
#define USART_CR1_RE          (1U << 2)
#define USART_CR1_TE          (1U << 3)
#define USART_CR1_RXNEIE      (1U << 5)
#define USART_CR1_TCIE        (1U << 6)
#define USART_CR1_TXEIE       (1U << 7)
#define USART_CR1_PCE         (1U << 10)
#define USART_CR1_M0          (1U << 12)
#define USART_CR1_M1          (1U << 28)
#define USART_ISR_RXNE        (1U << 5)
#define USART_ISR_TXE         (1U << 7)
#define USART_ISR_TC          (1U << 6)
#define USART_ISR_PE          (1U << 0)
#define USART_ISR_FE          (1U << 1)
#define USART_ISR_ORE         (1U << 3)
#define USART_RQR_RXFRQ       (1U << 3)
#define USART_RQR_TXFRQ       (1U << 4)

/* ===================================================================== *
 *  TIM (Timers) — RM0433 §43
 * ===================================================================== */

typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t SMCR;    /* 0x08 */
    volatile uint32_t DIER;    /* 0x0C */
    volatile uint32_t SR;      /* 0x10 */
    volatile uint32_t EGR;     /* 0x14 */
    volatile uint32_t CCMR1;   /* 0x18 */
    volatile uint32_t CCMR2;   /* 0x1C */
    volatile uint32_t CCER;    /* 0x20 */
    volatile uint32_t CNT;     /* 0x24 */
    volatile uint32_t PSC;     /* 0x28 */
    volatile uint32_t ARR;     /* 0x2C */
    volatile uint32_t RCR;     /* 0x30 */
    volatile uint32_t CCR1;    /* 0x34 */
    volatile uint32_t CCR2;    /* 0x38 */
    volatile uint32_t CCR3;    /* 0x3C */
    volatile uint32_t CCR4;    /* 0x40 */
} tim_reg_t;

#define TIM1   ((tim_reg_t *)TIM1_BASE)
#define TIM2   ((tim_reg_t *)TIM2_BASE)

#define TIM_CR1_CEN    (1U << 0)
#define TIM_CR1_ARPE   (1U << 7)
#define TIM_SR_UIF     (1U << 0)
#define TIM_DIER_UIE   (1U << 0)

/* ===================================================================== *
 *  I2C — RM0433 §41
 * ===================================================================== */

typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t OAR1;     /* 0x08 */
    volatile uint32_t OAR2;     /* 0x0C */
    volatile uint32_t TIMINGR;  /* 0x10 */
    volatile uint32_t TIMEOUTR; /* 0x14 */
    volatile uint32_t ISR;      /* 0x18 */
    volatile uint32_t ICR;      /* 0x1C */
    volatile uint32_t PECR;     /* 0x20 */
    volatile uint32_t RXDR;     /* 0x24 */
    volatile uint32_t TXDR;     /* 0x28 */
} i2c_reg_t;

#define I2C1  ((i2c_reg_t *)I2C1_BASE)
#define I2C3  ((i2c_reg_t *)I2C3_BASE)
#define I2C4  ((i2c_reg_t *)I2C4_BASE)

#define I2C_CR1_PE          (1U << 0)
#define I2C_CR2_START       (1U << 13)
#define I2C_CR2_STOP         (1U << 14)
#define I2C_CR2_RD_WRN       (1U << 10)
#define I2C_CR2_NACK         (1U << 15)
#define I2C_ISR_TXE          (1U << 0)
#define I2C_ISR_RXNE         (1U << 2)
#define I2C_ISR_TC           (1U << 6)
#define I2C_ISR_NACKF        (1U << 4)
#define I2C_ISR_BUSY         (1U << 15)

/* ===================================================================== *
 *  SCB / NVIC / SysTick — ARMv7-M Architecture Ref Manual
 * ===================================================================== */

#define SCB_ICSR            (*(volatile uint32_t *)0xE000ED04UL)
#define SCB_SCR             (*(volatile uint32_t *)0xE000ED10UL)
#define SCB_SCR_SLEEPDEEP   (1U << 2)

#define NVIC_ISER0          (*(volatile uint32_t *)0xE000E100UL)
#define NVIC_ICER0          (*(volatile uint32_t *)0xE000E180UL)
#define NVIC_IPR_BASE       ((volatile uint8_t *)0xE000E400UL)

#define SysTick_BASE        0xE000E010UL
#define SysTick_CSR         (*(volatile uint32_t *)(SysTick_BASE + 0x00))
#define SysTick_RVR         (*(volatile uint32_t *)(SysTick_BASE + 0x04))
#define SysTick_CVR         (*(volatile uint32_t *)(SysTick_BASE + 0x08))
#define SysTick_CSR_ENABLE  (1U << 0)
#define SysTick_CSR_TICKINT (1U << 1)
#define SysTick_CSR_CLKSRC  (1U << 2)

/* ===================================================================== *
 *  Flash controller — for calibration storage
 * ===================================================================== */

#define FLASH_BASE_REG      (0x58022000UL)
#define FLASH_OFFSET(x)     (FLASH_BASE_REG + (x))
#define FLASH_KEYR         (*(volatile uint32_t *)(FLASH_OFFSET(0x04)))
#define FLASH_SR           (*(volatile uint32_t *)(FLASH_OFFSET(0x0C)))
#define FLASH_CR           (*(volatile uint32_t *)(FLASH_OFFSET(0x10)))
#define FLASH_OPTCR        (*(volatile uint32_t *)(FLASH_OFFSET(0x14)))
#define FLASHCCR1          (*(volatile uint32_t *)(FLASH_OFFSET(0x38)))

#define FLASH_SR_BSY        (1U << 0)
#define FLASH_SR_EOP        (1U << 16)
#define FLASH_CR_LOCK       (1U << 0)
#define FLASH_CR_PG         (1U << 1)
#define FLASH_CR_SER        (1U << 2)
#define FLASH_CR_PSIZE_8    (0U << 4)
#define FLASH_CR_PSIZE_16   (1U << 4)
#define FLASH_CR_PSIZE_32   (2U << 4)
#define FLASH_CR_PSIZE_64   (3U << 4)

/* ===================================================================== *
 *  AD5940 Bioimpedance AFE — Key Register Addresses
 *  (AD5940 Datasheet Rev B, §15 Register Map)
 * ===================================================================== */

#define AD5940_REG_TOPCTL0      0x0000
#define AD5940_REG_TOPCTL1      0x0004
#define AD5940_REG_TOPCTL2      0x0008
#define AD5940_REG_TOPCTL3      0x000C
#define AD5940_REG_TOPCTL4      0x0010
#define AD5940_REG_TOPCTL5      0x0014

#define AD5940_REG_AFECON       0x2000
#define AD5940_REG_FREQWORD       0x2004
#define AD5940_REG_SWCON        0x2008
#define AD5940_REG_SWVSEL0      0x200C
#define AD5940_REG_SWVSEL1      0x2010
#define AD5940_REG_SWVSEL2      0x2014
#define AD5940_REG_SWVSEL3      0x2018

#define AD5940_REG_DACSEQ0       0x2100
#define AD5940_REG_DACSEQ1       0x2104
#define AD5940_REG_DACSEQ2       0x2108
#define AD5940_REG_DACCON        0x210C
#define AD5940_REG_WGCON         0x2100
#define AD5940_REG_WGTYPE        0x2104
#define AD5940_REG_WGSEQ0        0x2108
#define AD5940_REG_WGSEQ1        0x210C
#define AD5940_REG_WGSEQ2        0x2110

#define AD5940_REG_ADCDAT0       0x2200
#define AD5940_REG_ADCDAT1       0x2204
#define AD5940_REG_ADCDAT2       0x2208
#define AD5940_REG_ADCDAT3       0x220C
#define AD5940_REG_ADCFILTER     0x2210
#define AD5940_REG_ADCCON        0x2214
#define AD5940_REG_PGA           0x2218
#define AD5940_REG_ADCCNT0       0x2240
#define AD5940_REG_ADCCNT1       0x2244

#define AD5940_REG_DFTCON        0x2300
#define AD5940_REG_DFTINSEL      0x2304
#define AD5940_REG_DFTOUTSEL     0x2308
#define AD5940_REG_DFTCALC       0x230C
#define AD5940_REG_DFTREAL       0x2310
#define AD5940_REG_DFTIMAG       0x2314
#define AD5940_REG_DFTCALCLEN    0x2318
#define AD5940_REG_SFTCON        0x231C

#define AD5940_REG_AFE_STATUS    0x2F00
#define AD5940_REG_INTCPOL       0x2F04
#define AD5940_REG_INTCLR        0x2F08
#define AD5940_REG_INTCFG0       0x2F0C
#define AD5940_REG_INTCFG1       0x2F10

#define AD5940_REG_SEQ0          0x3000
#define AD5940_REG_SEQ1          0x3004
#define AD5940_REG_SEQ2          0x3008
#define AD5940_REG_SEQ3          0x300C
#define AD5940_REG_SEQINFO0      0x3010
#define AD5940_REG_SEQINFO1      0x3014
#define AD5940_REG_SEQINFO2      0x3018
#define AD5940_REG_SEQINFO3      0x301C

#define AD5940_REG_CMD_FIFO      0x4000
#define AD5940_REG_DATA_FIFO     0x4004
#define AD5940_REG_FIFO_STATUS   0x4008
#define AD5940_REG_FIFO_CONFIG   0x400C

/* AD5940 command opcodes */
#define AD5940_CMD_REG_WRITE     0x00
#define AD5940_CMD_REG_READ      0x01
#define AD5940_CMD_FIFO_WRITE     0x02
#define AD5940_CMD_FIFO_READ     0x03
#define AD5940_CMD_SEQ_WRITE      0x04
#define AD5940_CMD_SEQ_RUN        0x05
#define AD5940_CMD_SEQ_STOP       0x06
#define AD5940_CMD_WAKEUP         0x07
#define AD5940_CMD_SLEEP         0x08

#define AD5940_AFECON_AFERESET   (1U << 3)
#define AD5940_AFECON_ADCAFEEN   (1U << 4)
#define AD5940_AFECON_WGAFEEN    (1U << 5)
#define AD5940_AFECON_DFTEN      (1U << 6)
#define AD5940_AFECON_INAMPMODE  (1U << 7)
#define AD5940_AFECON_SEQSTART   (1U << 10)

/* ===================================================================== *
 *  Watchdog (IWDG)
 * ===================================================================== */

#define IWDG_BASE               (0x58004800UL)
#define IWDG_KR                 (*(volatile uint32_t *)(IWDG_BASE + 0x00))
#define IWDG_PR                 (*(volatile uint32_t *)(IWDG_BASE + 0x04))
#define IWDG_RLR                (*(volatile uint32_t *)(IWDG_BASE + 0x08))
#define IWDG_SR                 (*(volatile uint32_t *)(IWDG_BASE + 0x0C))
#define IWDG_KR_KEY             0xCCCC
#define IWDG_KR_UNLOCK          0x5555
#define IWDG_KR_RELOAD          0xAAAA

/* ===================================================================== *
 *  Interrupt vector table types
 * ===================================================================== */

typedef void (*isr_handler_t)(void);

#endif /* ROOTTRACE_REGISTERS_H */