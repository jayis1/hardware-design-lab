/*
 * registers.h — STM32H743 register definitions
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Minimal register-level definitions for bare-metal programming.
 * Only the peripherals used by WattLens are defined here.
 */

#ifndef WATTLENS_REGISTERS_H
#define WATTLENS_REGISTERS_H

#include <stdint.h>

/* ========================================================================
 * Base addresses (from STM32H743 reference manual, RM0433)
 * ======================================================================== */
#define PERIPH_BASE        0x40000000UL
#define D1_AHB1PERIPH_BASE 0x48020000UL
#define D1_AHB2PERIPH_BASE 0x48020000UL
#define D1_APB1PERIPH_BASE 0x40000000UL
#define D1_APB2PERIPH_BASE 0x58000000UL
#define D2_AHB1PERIPH_BASE 0x48021000UL
#define D2_APB1PERIPH_BASE 0x40000000UL
#define D2_APB2PERIPH_BASE 0x48000000UL
#define D3_APB1PERIPH_BASE 0x58000000UL

/* RCC */
#define RCC_BASE           0x58024400UL
#define RCC_CR             (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_PLLCFGR        (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_CFGR           (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_D1CFGR         (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_D2CFGR         (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_D3CFGR         (*(volatile uint32_t *)(RCC_BASE + 0x20))
#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0xD0))
#define RCC_AHB2ENR        (*(volatile uint32_t *)(RCC_BASE + 0xD4))
#define RCC_AHB3ENR        (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_APB1LENR       (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_APB1HENR       (*(volatile uint32_t *)(RCC_BASE + 0xE4))
#define RCC_APB2ENR        (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_AHB4ENR        (*(volatile uint32_t *)(RCC_BASE + 0xE0 + 0x30))

/* PWR */
#define PWR_BASE           0x58024800UL
#define PWR_CR1            (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CSR1           (*(volatile uint32_t *)(PWR_BASE + 0x04))
#define PWR_D3CR           (*(volatile uint32_t *)(PWR_BASE + 0x14))

/* GPIO */
#define GPIOA_BASE         0x4802'2000UL  /* placeholder formatting */
#undef GPIOA_BASE
#define GPIOA_BASE         0x48022000UL
#define GPIOB_BASE         0x48022400UL
#define GPIOC_BASE         0x48022800UL
#define GPIOD_BASE         0x48022C00UL
#define GPIOE_BASE         0x48023000UL

#define GPIO_MODER_OFF     0x00
#define GPIO_OTYPER_OFF    0x04
#define GPIO_OSPEEDR_OFF   0x08
#define GPIO_PUPDR_OFF     0x0C
#define GPIO_IDR_OFF       0x10
#define GPIO_ODR_OFF       0x14
#define GPIO_BSRR_OFF      0x18
#define GPIO_AFRL_OFF      0x20
#define GPIO_AFRH_OFF      0x24

#define GPIO_REG(port, off) (*(volatile uint32_t *)((port) + (off)))

/* SPI */
#define SPI1_BASE          0x40013000UL
#define SPI2_BASE          0x40003800UL
#define SPI3_BASE          0x40003C00UL
#define SPI_CR1            0x00
#define SPI_CR2            0x04
#define SPI_CFG1           0x08
#define SPI_CFG2           0x0C
#define SPI_SR             0x14
#define SPI_IFCR           0x18
#define SPI_TXDR           0x20
#define SPI_RXDR           0x30
#define SPI_REG(port, off) (*(volatile uint32_t *)((port) + (off)))

/* USART */
#define USART1_BASE        0x40011000UL
#define USART2_BASE        0x40004400UL
#define USART3_BASE        0x40004800UL
#define USART_CR1          0x00
#define USART_CR2          0x04
#define USART_CR3          0x08
#define USART_BRR          0x0C
#define USART_ISR          0x1C
#define USART_RDR          0x24
#define USART_TDR          0x28
#define USART_REG(port, off) (*(volatile uint32_t *)((port) + (off)))

/* I2C */
#define I2C1_BASE          0x40005400UL
#define I2C_CR1            0x00
#define I2C_CR2            0x04
#define I2C_ISR            0x10
#define I2C_TXDR           0x28
#define I2C_RXDR           0x2C
#define I2C_REG(port, off) (*(volatile uint32_t *)((port) + (off)))

/* DMA (BDMA / DMA1/DMA2) */
#define DMA1_BASE          0x48026000UL
#define DMA2_BASE          0x48027000UL
#define DMA_S0CR_OFF       0x10
#define DMA_S0NDTR_OFF     0x14
#define DMA_S0PAR_OFF      0x18
#define DMA_S0M0AR_OFF     0x1C
#define DMA_S0FCR_OFF      0x20

/* DMA stream register block (offset by stream number × 0x18) */
typedef struct {
    volatile uint32_t CR;      /* 0x00 */
    volatile uint32_t NDTR;    /* 0x04 */
    volatile uint32_t PAR;     /* 0x08 */
    volatile uint32_t M0AR;    /* 0x0C */
    volatile uint32_t M1AR;    /* 0x10 */
    volatile uint32_t FCR;     /* 0x14 */
} dma_stream_t;

#define DMA_STREAM(base, n) ((dma_stream_t *)((base) + 0x10 + (n) * 0x18))
#define DMA_LIFCR           (*(volatile uint32_t *)(DMA2_BASE + 0x08))
#define DMA_HIFCR           (*(volatile uint32_t *)(DMA2_BASE + 0x0C))
#define DMA_LISR            (*(volatile uint32_t *)(DMA2_BASE + 0x00))
#define DMA_HISR            (*(volatile uint32_t *)(DMA2_BASE + 0x04))

/* NVIC */
#define NVIC_BASE           0xE000E100UL
#define NVIC_ISER(n)        (*(volatile uint32_t *)(NVIC_BASE + 0x00 + (n)*4))
#define NVIC_ICER(n)        (*(volatile uint32_t *)(NVIC_BASE + 0x80 + (n)*4))
#define NVIC_ISPR(n)        (*(volatile uint32_t *)(NVIC_BASE + 0x100 + (n)*4))
#define NVIC_IP(n)          (*(volatile uint8_t *)(0xE000E400UL + (n)))

/* SysTick */
#define SYSTICK_BASE        0xE000E010UL
#define SYST_CSR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYST_RVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYST_CVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYST_CALIB          (*(volatile uint32_t *)(SYSTICK_BASE + 0x0C))

/* EXTI */
#define EXTI_BASE           0x5800'7800UL
#undef EXTI_BASE
#define EXTI_BASE           0x58007800UL
#define EXTI_IMR1           (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_EMR1           (*(volatile uint32_t *)(EXTI_BASE + 0x04))
#define EXTI_RTSR1          (*(volatile uint32_t *)(EXTI_BASE + 0x08))
#define EXTI_FTSR1          (*(volatile uint32_t *)(EXTI_BASE + 0x0C))
#define EXTI_SWIER1         (*(volatile uint32_t *)(EXTI_BASE + 0x10))
#define EXTI_PR1            (*(volatile uint32_t *)(EXTI_BASE + 0x14))

/* Flash / CRC */
#define FLASH_BASE          0x5200'2000UL
#undef FLASH_BASE
#define FLASH_BASE          0x52002000UL
#define CRC_BASE            0x58024000UL
#define CRC_DR              (*(volatile uint32_t *)(CRC_BASE + 0x00))
#define CRC_CR              (*(volatile uint32_t *)(CRC_BASE + 0x08))
#define CRC_INIT            (*(volatile uint32_t *)(CRC_BASE + 0x10))

/* USB (OTG1) */
#define USB1_OTG_BASE       0x40080000UL

/* TIM6 (basic timer for DRDY / general timing) */
#define TIM6_BASE           0x40004000UL
#define TIM6_CR1            (*(volatile uint32_t *)(TIM6_BASE + 0x00))
#define TIM6_DIER           (*(volatile uint32_t *)(TIM6_BASE + 0x0C))
#define TIM6_SR             (*(volatile uint32_t *)(TIM6_BASE + 0x10))
#define TIM6_PSC            (*(volatile uint32_t *)(TIM6_BASE + 0x28))
#define TIM6_ARR            (*(volatile uint32_t *)(TIM6_BASE + 0x2C))
#define TIM6_CNT            (*(volatile uint32_t *)(TIM6_BASE + 0x24))

/* ========================================================================
 * RCC enable bit definitions (for AHB/APB peripheral enables)
 * ======================================================================== */
/* RCC_AHB4ENR (GPIO ports) */
#define RCC_AHB4ENR_GPIOAEN  (1 << 0)
#define RCC_AHB4ENR_GPIOBEN  (1 << 1)
#define RCC_AHB4ENR_GPIOCEN  (1 << 2)
#define RCC_AHB4ENR_GPIODEN  (1 << 3)
#define RCC_AHB4ENR_GPIOEEN  (1 << 4)

/* RCC_APB1LENR */
#define RCC_APB1LENR_SPI2EN  (1 << 14)
#define RCC_APB1LENR_SPI3EN  (1 << 15)
#define RCC_APB1LENR_USART2EN (1 << 17)
#define RCC_APB1LENR_USART3EN (1 << 18)
#define RCC_APB1LENR_I2C1EN  (1 << 21)
#define RCC_APB1LENR_TIM6EN  (1 << 4)

/* RCC_APB2ENR */
#define RCC_APB2ENR_SPI1EN   (1 << 12)
#define RCC_APB2ENR_USART1EN (1 << 14)
#define RCC_APB2ENR_TIM1EN   (1 << 0)

/* RCC_AHB1ENR (DMA) */
#define RCC_AHB1ENR_DMA1EN   (1 << 0)
#define RCC_AHB1ENR_DMA2EN   (1 << 1)

/* ========================================================================
 * GPIO mode constants
 * ======================================================================== */
#define GPIO_MODE_IN         0x00
#define GPIO_MODE_OUT        0x01
#define GPIO_MODE_AF         0x02
#define GPIO_MODE_ANALOG     0x03

#define GPIO_SPEED_LOW       0x00
#define GPIO_SPEED_MED       0x01
#define GPIO_SPEED_HIGH      0x02
#define GPIO_SPEED_VHIGH     0x03

#define GPIO_PUPD_NONE       0x00
#define GPIO_PUPD_PU         0x01
#define GPIO_PUPD_PD         0x02

#define GPIO_OTYPE_PP        0x00
#define GPIO_OTYPE_OD        0x01

/* ========================================================================
 * Bit manipulation helpers
 * ======================================================================== */
#define BIT(n)               (1UL << (n))
#define SET_BIT(reg, bit)    ((reg) |= (bit))
#define CLR_BIT(reg, bit)    ((reg) &= ~(bit))
#define RD_BIT(reg, bit)     ((reg) & (bit))

/* ========================================================================
 * IRQ numbers (STM32H743)
 * ======================================================================== */
#define IRQ_EXTI0            6    /* EXTI line 0 (touch) */
#define IRQ_EXTI4            12   /* EXTI line 4 (LoRa DIO1) */
#define IRQ_EXTI9_5          23   /* EXTI lines 9-5 (DRDY on PC5) */
#define IRQ_DMA2_STREAM0     56   /* SPI2 RX DMA */
#define IRQ_DMA2_STREAM1     57   /* SPI2 TX DMA */
#define IRQ_SPI2             36
#define IRQ_USART2           38   /* BLE */
#define IRQ_USART3           39   /* LoRa */
#define IRQ_TIM6_DAC         54   /* basic timer */

#endif /* WATTLENS_REGISTERS_H */