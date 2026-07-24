/*
 * registers.h — STM32G474 register base addresses and key register offsets.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * This file provides direct register-level access for the STM32G474CEU6.
 * It defines base addresses for all peripherals used by AlloyScan and
 * the key register offsets needed for low-level configuration without
 * relying on the full STM32 HAL or LL libraries.
 */

#ifndef ALLOYSCAN_REGISTERS_H
#define ALLOYSCAN_REGISTERS_H

#include <stdint.h>

/* ---- Cortex-M4 core registers ---- */
#define SCB_BASE            0xE000ED00UL
#define SCB_VTOR            (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_CCR             (*(volatile uint32_t *)(SCB_BASE + 0x14))
#define SCB_SHPR2           (*(volatile uint32_t *)(SCB_BASE + 0x1C))
#define SCB_SHPR3           (*(volatile uint32_t *)(SCB_BASE + 0x20))

#define NVIC_BASE            0xE000E100UL
#define NVIC_ISER(x)        (*(volatile uint32_t *)(NVIC_BASE + 0x00 + (x)*4))
#define NVIC_ICER(x)        (*(volatile uint32_t *)(NVIC_BASE + 0x80 + (x)*4))
#define NVIC_ISPR(x)        (*(volatile uint32_t *)(NVIC_BASE + 0x100 + (x)*4))
#define NVIC_ICPR(x)        (*(volatile uint32_t *)(NVIC_BASE + 0x180 + (x)*4))
#define NVIC_IPR(x)         (*(volatile uint8_t  *)(NVIC_BASE + 0x300 + (x)))

#define SYSTICK_BASE        0xE000E010UL
#define SYST_CSR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYST_RVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYST_CVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYST_CALIB          (*(volatile uint32_t *)(SYSTICK_BASE + 0x0C))

/* ---- RCC (Reset and Clock Control) ---- */
#define RCC_BASE            0x40021000UL
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x48))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_APB1ENR1        (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB1ENR2        (*(volatile uint32_t *)(RCC_BASE + 0x5C))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_CCIPR1          (*(volatile uint32_t *)(RCC_BASE + 0x88))
#define RCC_CCIPR2          (*(volatile uint32_t *)(RCC_BASE + 0x8C))

/* AHB1ENR bits */
#define RCC_AHB1ENR_DMA1    (1UL << 0)
#define RCC_AHB1ENR_DMA2    (1UL << 1)
#define RCC_AHB1ENR_CRCEN   (1UL << 11)
#define RCC_AHB1ENR_FLASHEN (1UL << 25)

/* AHB2ENR bits */
#define RCC_AHB2ENR_GPIOA   (1UL << 0)
#define RCC_AHB2ENR_GPIOB   (1UL << 1)
#define RCC_AHB2ENR_GPIOC   (1UL << 2)
#define RCC_AHB2ENR_ADC12   (1UL << 5)
#define RCC_AHB2ENR_DAC1    (1UL << 16)

/* APB1ENR1 bits */
#define RCC_APB1ENR1_USART2 (1UL << 17)
#define RCC_APB1ENR1_I2C3   (1UL << 23)
#define RCC_APB1ENR1_SPI2   (1UL << 14)

/* APB2ENR bits */
#define RCC_APB2ENR_SPI1    (1UL << 12)
#define RCC_APB2ENR_SPI4    (1UL << 15)
#define RCC_APB2ENR_TIM1    (1UL << 0)
#define RCC_APB2ENR_TIM8    (1UL << 1)
#define RCC_APB2ENR_SYSCFG  (1UL << 0)

/* ---- GPIO ---- */
#define GPIOA_BASE          0x48000000UL
#define GPIOB_BASE          0x48000400UL
#define GPIOC_BASE          0x48000800UL

typedef struct {
    volatile uint32_t MODER;   /* Offset 0x00 */
    volatile uint32_t OTYPER;  /* Offset 0x04 */
    volatile uint32_t OSPEEDR;  /* Offset 0x08 */
    volatile uint32_t PUPDR;   /* Offset 0x0C */
    volatile uint32_t IDR;    /* Offset 0x10 */
    volatile uint32_t ODR;    /* Offset 0x14 */
    volatile uint32_t BSRR;   /* Offset 0x18 */
    volatile uint32_t LCKR;   /* Offset 0x1C */
    volatile uint32_t AFRL;   /* Offset 0x20 */
    volatile uint32_t AFRH;   /* Offset 0x24 */
    volatile uint32_t BRR;    /* Offset 0x28 */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)

#define GPIO_MODE_INPUT      0x00
#define GPIO_MODE_OUTPUT     0x01
#define GPIO_MODE_AF         0x02
#define GPIO_MODE_ANALOG     0x03

#define GPIO_OTYPE_PP        0x00
#define GPIO_OTYPE_OD        0x01

#define GPIO_OSPEED_LOW       0x00
#define GPIO_OSPEED_MED       0x01
#define GPIO_OSPEED_HIGH      0x02
#define GPIO_OSPEED_VHIGH     0x03

#define GPIO_PUPD_NONE        0x00
#define GPIO_PUPD_PU          0x01
#define GPIO_PUPD_PD          0x02

/* ---- DMA1 ---- */
#define DMA1_BASE            0x40020000UL
#define DMA1_Channel1_BASE   (DMA1_BASE + 0x00)
#define DMA1_Channel2_BASE   (DMA1_BASE + 0x08)
#define DMA1_Channel3_BASE   (DMA1_BASE + 0x10)
#define DMA1_Channel4_BASE   (DMA1_BASE + 0x18)
#define DMA1_Channel5_BASE   (DMA1_BASE + 0x20)
#define DMA1_Channel6_BASE   (DMA1_BASE + 0x28)
#define DMA1_Channel7_BASE   (DMA1_BASE + 0x30)

typedef struct {
    volatile uint32_t CCR;   /* Channel config */
    volatile uint32_t CNDR;  /* Number of data to transfer */
    volatile uint32_t CPAR;  /* Peripheral address */
    volatile uint32_t CM0AR; /* Memory 0 address */
    volatile uint32_t CM1AR; /* Memory 1 address */
} DMA_Channel_TypeDef;

#define DMA1_Ch1 ((DMA_Channel_TypeDef *)DMA1_Channel1_BASE)
#define DMA1_Ch2 ((DMA_Channel_TypeDef *)DMA1_Channel2_BASE)
#define DMA1_Ch3 ((DMA_Channel_TypeDef *)DMA1_Channel3_BASE)
#define DMA1_Ch4 ((DMA_Channel_TypeDef *)DMA1_Channel4_BASE)
#define DMA1_Ch5 ((DMA_Channel_TypeDef *)DMA1_Channel5_BASE)
#define DMA1_Ch6 ((DMA_Channel_TypeDef *)DMA1_Channel6_BASE)
#define DMA1_Ch7 ((DMA_Channel_TypeDef *)DMA1_Channel7_BASE)

/* DMA ISR register */
#define DMA1_ISR            (*(volatile uint32_t *)(DMA1_BASE + 0x80))
#define DMA1_IFCR           (*(volatile uint32_t *)(DMA1_BASE + 0x84))

/* DMA CCR bits */
#define DMA_CCR_EN          (1UL << 0)
#define DMA_CCR_TCIE        (1UL << 1)
#define DMA_CCR_HTIE        (1UL << 2)
#define DMA_CCR_TEIE        (1UL << 3)
#define DMA_CCR_DIR_P2M     (0UL << 4)
#define DMA_CCR_DIR_M2P     (1UL << 4)
#define DMA_CCR_CIRC        (1UL << 5)
#define DMA_CCR_MINC        (1UL << 6)
#define DMA_CCR_PINC        (1UL << 7)
#define DMA_CCR_PSIZE_8B    (0UL << 8)
#define DMA_CCR_PSIZE_16B   (1UL << 8)
#define DMA_CCR_PSIZE_32B   (2UL << 8)
#define DMA_CCR_MSIZE_8B    (0UL << 10)
#define DMA_CCR_MSIZE_16B   (1UL << 10)
#define DMA_CCR_MSIZE_32B   (2UL << 10)
#define DMA_CCR_PL_LOW      (0UL << 12)
#define DMA_CCR_PL_MED      (1UL << 12)
#define DMA_CCR_PL_HIGH     (2UL << 12)
#define DMA_CCR_PL_VHIGH    (3UL << 12)
#define DMA_CCR_M2M         (1UL << 14)

/* ---- DAC1 ---- */
#define DAC1_BASE           0x50000800UL
#define DAC1_CR            (*(volatile uint32_t *)(DAC1_BASE + 0x00))
#define DAC1_DHR12R1       (*(volatile uint32_t *)(DAC1_BASE + 0x08))
#define DAC1_DHR12R2       (*(volatile uint32_t *)(DAC1_BASE + 0x0C))
#define DAC1_DOR1          (*(volatile uint32_t *)(DAC1_BASE + 0x10))
#define DAC1_MCR           (*(volatile uint32_t *)(DAC1_BASE + 0x3C))

#define DAC_CR_EN1          (1UL << 0)
#define DAC_CR_DMAEN1       (1UL << 12)

/* ---- SPI ---- */
#define SPI1_BASE           0x40013000UL
#define SPI2_BASE           0x40003800UL
#define SPI3_BASE           0x40003C00UL

typedef struct {
    volatile uint32_t CR1;   /* 0x00 Control register 1 */
    volatile uint32_t CR2;   /* 0x04 Control register 2 */
    volatile uint32_t SR;    /* 0x08 Status register */
    volatile uint32_t DR;    /* 0x0C Data register */
    volatile uint32_t CRCPR;  /* 0x10 CRC polynomial */
    volatile uint32_t RXCRCR; /* 0x14 RX CRC */
    volatile uint32_t TXCRCR; /* 0x18 TX CRC */
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)
#define SPI2 ((SPI_TypeDef *)SPI2_BASE)
#define SPI3 ((SPI_TypeDef *)SPI3_BASE)

#define SPI_CR1_CPHA        (1UL << 0)
#define SPI_CR1_CPOL        (1UL << 1)
#define SPI_CR1_MSTR        (1UL << 2)
#define SPI_CR1_BR_DIV2     (0UL << 3)
#define SPI_CR1_BR_DIV4     (1UL << 3)
#define SPI_CR1_BR_DIV8     (2UL << 3)
#define SPI_CR1_BR_DIV16    (3UL << 3)
#define SPI_CR1_BR_DIV32    (4UL << 3)
#define SPI_CR1_BR_DIV64    (5UL << 3)
#define SPI_CR1_BR_DIV128   (6UL << 3)
#define SPI_CR1_BR_DIV256   (7UL << 3)
#define SPI_CR1_LSBFIRST    (1UL << 7)
#define SPI_CR1_SSI         (1UL << 8)
#define SPI_CR1_SSM         (1UL << 9)
#define SPI_CR1_RXONLY      (1UL << 10)
#define SPI_CR1_CRCL_8B     (0UL << 11)
#define SPI_CR1_CRCL_16B    (1UL << 11)
#define SPI_CR1_CPOL_LOW    0
#define SPI_CR1_CPHA_1EDGE  0

#define SPI_CR2_DS_8B       (7UL << 8)
#define SPI_CR2_DS_16B      (15UL << 8)
#define SPI_CR2_FRF         (1UL << 4)
#define SPI_CR2_NSSP        (1UL << 3)
#define SPI_CR2_SSOE        (1UL << 2)
#define SPI_CR2_TXDMAEN     (1UL << 1)
#define SPI_CR2_RXDMAEN     (1UL << 0)

#define SPI_SR_RXNE         (1UL << 0)
#define SPI_SR_TXE          (1UL << 1)
#define SPI_SR_BSY          (1UL << 7)
#define SPI_SR_FRE          (1UL << 8)
#define SPI_SR_OVR          (1UL << 6)
#define SPI_SR_MODF         (1UL << 5)
#define SPI_SR_CRCERR       (1UL << 4)

/* ---- USART2 (BLE) ---- */
#define USART2_BASE         0x40004400UL

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

#define USART2 ((USART_TypeDef *)USART2_BASE)

#define USART_CR1_UE        (1UL << 0)
#define USART_CR1_RE        (1UL << 2)
#define USART_CR1_TE        (1UL << 3)
#define USART_CR1_RXNEIE    (1UL << 5)
#define USART_CR1_TCIE      (1UL << 6)
#define USART_CR1_TXEIE    (1UL << 7)

#define USART_ISR_RXNE      (1UL << 5)
#define USART_ISR_TXE       (1UL << 7)
#define USART_ISR_TC        (1UL << 6)

/* ---- I2C3 (VL53L0X) ---- */
#define I2C3_BASE           0x40005C00UL

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t TIMINGR;
    volatile uint32_t TIMEOUTR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t PECR;
    volatile uint32_t RXDR;
    volatile uint32_t TXDR;
} I2C_TypeDef;

#define I2C3 ((I2C_TypeDef *)I2C3_BASE)

#define I2C_CR1_PE          (1UL << 0)
#define I2C_CR1_TXIE        (1UL << 1)
#define I2C_CR1_RXIE        (1UL << 2)
#define I2C_CR1_NACKIE      (1UL << 4)
#define I2C_CR1_STOPIE      (1UL << 5)

#define I2C_ISR_TXE         (1UL << 0)
#define I2C_ISR_RXNE        (1UL << 2)
#define I2C_ISR_NACKF       (1UL << 4)
#define I2C_ISR_STOPF       (1UL << 5)
#define I2C_ISR_BUSY        (1UL << 15)

#define I2C_CR2_AUTOEND     (1UL << 25)
#define I2C_CR2_RD_WRN      (1UL << 10)
#define I2C_CR2_START       (1UL << 13)
#define I2C_CR2_STOP        (1UL << 14)
#define I2C_CR2_NBYTES_SHIFT 16

/* ---- ADC1 (Battery monitoring) ---- */
#define ADC1_BASE           0x50000000UL
#define ADC1_ISR            (*(volatile uint32_t *)(ADC1_BASE + 0x00))
#define ADC1_CR             (*(volatile uint32_t *)(ADC1_BASE + 0x08))
#define ADC1_CFGR           (*(volatile uint32_t *)(ADC1_BASE + 0x0C))
#define ADC1_SQR1           (*(volatile uint32_t *)(ADC1_BASE + 0x30))
#define ADC1_DR             (*(volatile uint32_t *)(ADC1_BASE + 0x40))

#define ADC_CR_ADEN         (1UL << 0)
#define ADC_CR_ADSTART      (1UL << 2)
#define ADC_ISR_ADRDY       (1UL << 0)

/* ---- DMA request numbers (for DMAMUX) ---- */
#define DMAMUX1_BASE        0x40020800UL
#define DMAMUX1_Channel0   (*(volatile uint32_t *)(DMAMUX1_BASE + 0x00))
#define DMAMUX1_Channel1   (*(volatile uint32_t *)(DMAMUX1_BASE + 0x04))
#define DMAMUX1_Channel2   (*(volatile uint32_t *)(DMAMUX1_BASE + 0x08))
#define DMAMUX1_Channel3   (*(volatile uint32_t *)(DMAMUX1_BASE + 0x0C))
#define DMAMUX1_Channel4   (*(volatile uint32_t *)(DMAMUX1_BASE + 0x10))
#define DMAMUX1_Channel5   (*(volatile uint32_t *)(DMAMUX1_BASE + 0x14))
#define DMAMUX1_Channel6   (*(volatile uint32_t *)(DMAMUX1_BASE + 0x18))
#define DMAMUX1_Channel7   (*(volatile uint32_t *)(DMAMUX1_BASE + 0x1C))

/* DMA request selection IDs for DMAMUX */
#define DMAMUX_REQ_DAC1_CH1    6
#define DMAMUX_REQ_SPI1_RX      11
#define DMAMUX_REQ_SPI1_TX      12
#define DMAMUX_REQ_SPI2_RX      14
#define DMAMUX_REQ_SPI2_TX      15
#define DMAMUX_REQ_SPI3_RX      17
#define DMAMUX_REQ_SPI3_TX      18
#define DMAMUX_REQ_USART2_RX    33
#define DMAMUX_REQ_USART2_TX    34

/* ---- Interrupt numbers ---- */
#define IRQ_DMA1_Ch1        11
#define IRQ_DMA1_Ch2_3      12
#define IRQ_DMA1_Ch4_7      13
#define IRQ_SPI1            35
#define IRQ_SPI2            36
#define IRQ_SPI3            38
#define IRQ_USART2          27
#define IRQ_I2C3_EV         30
#define IRQ_TIM1_UP         15
#define IRQ_ADC1_2          18

/* ---- CORDIC peripheral ---- */
#define CORDIC_BASE         0x40020C00UL
#define CORDIC_CSR          (*(volatile uint32_t *)(CORDIC_BASE + 0x00))
#define CORDIC_WDATA        (*(volatile uint32_t *)(CORDIC_BASE + 0x04))
#define CORDIC_RDATA        (*(volatile uint32_t *)(CORDIC_BASE + 0x08))

/* CORDIC function codes */
#define CORDIC_FUNC_COSINE  0
#define CORDIC_FUNC_SINE    1
#define CORDIC_FUNC_PHASE   2
#define CORDIC_FUNC_MODUL   3
#define CORDIC_FUNC_ATAN    5

/* ---- Flash controller (for erasing calibration sector) ---- */
#define FLASH_BASE         0x40022000UL
#define FLASH_KEYR         (*(volatile uint32_t *)(FLASH_BASE + 0x04))
#define FLASH_CR           (*(volatile uint32_t *)(FLASH_BASE + 0x14))
#define FLASH_SR           (*(volatile uint32_t *)(FLASH_BASE + 0x18))
#define FLASH_AR           (*(volatile uint32_t *)(FLASH_BASE + 0x1C))

#define FLASH_CR_LOCK       (1UL << 31)
#define FLASH_CR_PER        (1UL << 1)
#define FLASH_CR_STRT      (1UL << 16)
#define FLASH_SR_BSY        (1UL << 16)
#define FLASH_SR_EOP        (1UL << 0)

#define FLASH_KEY1         0x45670123UL
#define FLASH_KEY2         0xCDEF89ABUL

#endif /* ALLOYSCAN_REGISTERS_H */