/*
 * registers.h — STM32L476RG Peripheral Register Definitions
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Low-level register address definitions and bit-field constants for the
 * STM32L476RGT6 MCU used in FloraPulse.  Covers only the peripherals the
 * firmware touches.  Register base addresses follow the STM32L476 Reference
 * Manual (RM0351, Rev 6).
 */

#ifndef FLORAPULSE_REGISTERS_H
#define FLORAPULSE_REGISTERS_H

#include <stdint.h>

/* ===================================================================== */
/*  System control block / Cortex-M4 core                               */
/* ===================================================================== */

#define SCB_BASE        0xE000ED00U
#define SCB_VTOR        (*(volatile uint32_t *)(SCB_BASE + 0x008U))
#define SCB_CPACR       (*(volatile uint32_t *)(SCB_BASE + 0x088U))

#define SYSTICK_BASE    0xE000E010U
#define SYSTICK_CTRL    (*(volatile uint32_t *)(SYSTICK_BASE + 0x00U))
#define SYSTICK_LOAD     (*(volatile uint32_t *)(SYSTICK_BASE + 0x04U))
#define SYSTICK_VAL      (*(volatile uint32_t *)(SYSTICK_BASE + 0x08U))

#define NVIC_ISER0      (*(volatile uint32_t *)(0xE000E100U))
#define NVIC_ISER1      (*(volatile uint32_t *)(0xE000E104U))
#define NVIC_ICER0      (*(volatile uint32_t *)(0xE000E180U))
#define NVIC_ICER1      (*(volatile uint32_t *)(0xE000E184U))
#define NVIC_IPR_BASE   (0xE000E400U)

#define EXTI_BASE       0x40010400U
#define EXTI_IMR1       (*(volatile uint32_t *)(EXTI_BASE + 0x00U))
#define EXTI_EMR1       (*(volatile uint32_t *)(EXTI_BASE + 0x04U))
#define EXTI_RTSR1      (*(volatile uint32_t *)(EXTI_BASE + 0x08U))
#define EXTI_FTSR1      (*(volatile uint32_t *)(EXTI_BASE + 0x0CU))
#define EXTI_SWIER1     (*(volatile uint32_t *)(EXTI_BASE + 0x10U))
#define EXTI_PR1        (*(volatile uint32_t *)(EXTI_BASE + 0x14U))

#define SYSCFG_BASE      0x40010000U
#define SYSCFG_EXTICR1  (*(volatile uint32_t *)(SYSCFG_BASE + 0x08U))
#define SYSCFG_EXTICR2  (*(volatile uint32_t *)(SYSCFG_BASE + 0x0CU))
#define SYSCFG_EXTICR3  (*(volatile uint32_t *)(SYSCFG_BASE + 0x10U))

/* ===================================================================== */
/*  RCC (Reset and Clock Control) — RM0351 §7                            */
/* ===================================================================== */

#define RCC_BASE        0x40021000U

#define RCC_CR           (*(volatile uint32_t *)(RCC_BASE + 0x00U))
#define RCC_ICSCR        (*(volatile uint32_t *)(RCC_BASE + 0x04U))
#define RCC_CFGR         (*(volatile uint32_t *)(RCC_BASE + 0x08U))
#define RCC_PLLCFGR      (*(volatile uint32_t *)(RCC_BASE + 0x0CU))
#define RCC_CIER         (*(volatile uint32_t *)(RCC_BASE + 0x18U))
#define RCC_CIFR         (*(volatile uint32_t *)(RCC_BASE + 0x1CU))
#define RCC_CICR         (*(volatile uint32_t *)(RCC_BASE + 0x20U))

/* RCC CR bits */
#define RCC_CR_HSION       (1U << 8)
#define RCC_CR_HSIRDY      (1U << 10)
#define RCC_CR_HSEON       (1U << 16)
#define RCC_CR_HSERDY      (1U << 17)
#define RCC_CR_HSEBYP      (1U << 18)
#define RCC_CR_PLLON       (1U << 24)
#define RCC_CR_PLLRDY      (1U << 25)

/* RCC CFGR SW bits */
#define RCC_CFGR_SW_HSI    (0U << 0)
#define RCC_CFGR_SW_HSE    (2U << 0)
#define RCC_CFGR_SW_PLL    (3U << 0)
#define RCC_CFGR_SWS_PLL   (3U << 2)
#define RCC_CFGR_HPRE_DIV1 (0U << 8)
#define RCC_CFGR_PPRE1_DIV1 (0x4U << 8)
#define RCC_CFGR_PPRE1_DIV2 (0x5U << 8)
#define RCC_CFGR_PPRE2_DIV1 (0x4U << 11)
#define RCC_CFGR_PPRE2_DIV2 (0x5U << 11)

/* PLLCFGR bits */
#define RCC_PLLCFGR_PLLSRC_HSI (2U << 0)
#define RCC_PLLCFGR_PLLSRC_HSE (3U << 0)

/* AHB/APB enable registers (L4 uses AHB1ENR1/APB1ENR1/APB2ENR/AHB2ENR/AHB3ENR) */
#define RCC_AHB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x48U))
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4CU))
#define RCC_AHB3ENR     (*(volatile uint32_t *)(RCC_BASE + 0x50U))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58U))
#define RCC_APB1ENR2    (*(volatile uint32_t *)(RCC_BASE + 0x5CU))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x60U))

/* Enable bits */
#define RCC_AHB1ENR1_FLASHEN    (1U << 8)
#define RCC_AHB1ENR1_DMA1EN     (1U << 0)
#define RCC_AHB1ENR1_DMA2EN     (1U << 1)
#define RCC_AHB1ENR1_CRCEN      (1U << 11)
#define RCC_AHB2ENR_GPIOAEN     (1U << 0)
#define RCC_AHB2ENR_GPIOBEN     (1U << 1)
#define RCC_AHB2ENR_GPIOCEN     (1U << 2)
#define RCC_AHB2ENR_GPIODEN     (1U << 3)
#define RCC_AHB2ENR_GPIOHEN     (1U << 7)
#define RCC_AHB2ENR_ADCEN       (1U << 13)
#define RCC_AHB3ENR_FLASH       (1U << 25)
#define RCC_APB1ENR1_TIM2EN     (1U << 0)
#define RCC_APB1ENR1_TIM3EN     (1U << 1)
#define RCC_APB1ENR1_TIM6EN     (1U << 4)
#define RCC_APB1ENR1_TIM7EN     (1U << 5)
#define RCC_APB1ENR1_SPI2EN     (1U << 14)
#define RCC_APB1ENR1_USART2EN  (1U << 17)
#define RCC_APB1ENR1_USART3EN   (1U << 18)
#define RCC_APB1ENR1_UART4EN    (1U << 19)
#define RCC_APB1ENR1_I2C1EN     (1U << 21)
#define RCC_APB1ENR1_I2C2EN     (1U << 22)
#define RCC_APB1ENR1_LPUART1EN  (1U << 28)
#define RCC_APB1ENR2_LPUART1EN  (1U << 0)
#define RCC_APB2ENR_TIM1EN      (1U << 11)
#define RCC_APB2ENR_TIM16EN     (1U << 17)
#define RCC_APB2ENR_USART1EN    (1U << 14)
#define RCC_APB2ENR_SPI1EN      (1U << 12)
#define RCC_APB2ENR_SYSCFGEN    (1U << 0)
#define RCC_APB2ENR_ADCEN       (1U << 20)

/* ===================================================================== */
/*  PWR controller — RM0351 §5                                           */
/* ===================================================================== */

#define PWR_BASE        0x40007000U
#define PWR_CR1         (*(volatile uint32_t *)(PWR_BASE + 0x00U))
#define PWR_CR2         (*(volatile uint32_t *)(PWR_BASE + 0x04U))
#define PWR_CR3         (*(volatile uint32_t *)(PWR_BASE + 0x08U))
#define PWR_CR4         (*(volatile uint32_t *)(PWR_BASE + 0x0CU))
#define PWR_SR1         (*(volatile uint32_t *)(PWR_BASE + 0x10U))
#define PWR_SR2         (*(volatile uint32_t *)(PWR_BASE + 0x14U))

#define PWR_CR3_EWUP1    (1U << 0)
#define PWR_CR3_EWUP2    (1U << 1)
#define PWR_CR3_EWUP3    (1U << 2)
#define PWR_CR3_EWUP4    (1U << 3)
#define PWR_CR3_EWUP5    (1U << 4)
#define PWR_CR4_WP1      (1U << 0)

#define PWR_SR1_WUF1     (1U << 0)
#define PWR_SR1_WUF2     (1U << 1)
#define PWR_SR1_WUF3     (1U << 2)
#define PWR_SR1_WUF4     (1U << 3)
#define PWR_SR1_WUF5     (1U << 4)

/* ===================================================================== */
/*  GPIO — RM0351 §8                                                     */
/* ===================================================================== */

#define GPIOA_BASE      0x48000000U
#define GPIOB_BASE      0x48000400U
#define GPIOC_BASE      0x48000800U
#define GPIOD_BASE      0x48000C00U
#define GPIOH_BASE      0x48001C00U

#define GPIO_MODER      0x000U
#define GPIO_OTYPER     0x004U
#define GPIO_OSPEEDR    0x008U
#define GPIO_PUPDR      0x00CU
#define GPIO_IDR        0x010U
#define GPIO_ODR        0x014U
#define GPIO_BSRR       0x018U
#define GPIO_LCKR       0x01CU
#define GPIO_AFRL       0x020U
#define GPIO_AFRH       0x024U
#define GPIO_BRR        0x028U

#define GPIO_AF0         0x0
#define GPIO_AF1         0x1   /* TIM1/TIM2/TIM16 */
#define GPIO_AF3         0x3   /* TIM8 / SAI */
#define GPIO_AF4         0x4   /* I2C */
#define GPIO_AF5         0x5   /* SPI1/SPI2 */
#define GPIO_AF6         0x6   /* SPI3 */
#define GPIO_AF7         0x7   /* USART1/2/3 */
#define GPIO_AF8         0x8   /* UART4/5, LPUART1 */

#define GPIO_REG(port, offset) (*(volatile uint32_t *)((port) + (offset)))

/* ===================================================================== */
/*  TIM1 — Advanced timer (sap-flow heater PWM)                          */
/* ===================================================================== */

#define TIM1_BASE       0x40012C00U
#define TIM1_CR1        (*(volatile uint32_t *)(TIM1_BASE + 0x00U))
#define TIM1_CR2        (*(volatile uint32_t *)(TIM1_BASE + 0x04U))
#define TIM1_SMCR       (*(volatile uint32_t *)(TIM1_BASE + 0x08U))
#define TIM1_DIER       (*(volatile uint32_t *)(TIM1_BASE + 0x0CU))
#define TIM1_SR         (*(volatile uint32_t *)(TIM1_BASE + 0x10U))
#define TIM1_EGR        (*(volatile uint32_t *)(TIM1_BASE + 0x14U))
#define TIM1_CCMR1      (*(volatile uint32_t *)(TIM1_BASE + 0x18U))
#define TIM1_CCMR2      (*(volatile uint32_t *)(TIM1_BASE + 0x1CU))
#define TIM1_CCER       (*(volatile uint32_t *)(TIM1_BASE + 0x20U))
#define TIM1_CNT        (*(volatile uint32_t *)(TIM1_BASE + 0x24U))
#define TIM1_PSC        (*(volatile uint32_t *)(TIM1_BASE + 0x28U))
#define TIM1_ARR        (*(volatile uint32_t *)(TIM1_BASE + 0x2CU))
#define TIM1_CCR1       (*(volatile uint32_t *)(TIM1_BASE + 0x34U))
#define TIM1_CCR2       (*(volatile uint32_t *)(TIM1_BASE + 0x38U))
#define TIM1_BDTR       (*(volatile uint32_t *)(TIM1_BASE + 0x44U))

#define TIM_CR1_CEN        (1U << 0)
#define TIM_CR1_ARPE       (1U << 7)
#define TIM_CR1_OPM        (1U << 3)
#define TIM_DIER_UIE       (1U << 0)
#define TIM_DIER_CC1IE     (1U << 1)
#define TIM_SR_UIF         (1U << 0)
#define TIM_SR_CC1IF       (1U << 1)
#define TIM_CCMR1_OC1M_PWM1 (0x6U << 4)
#define TIM_CCMR1_OC1PE    (1U << 3)
#define TIM_CCER_CC1E      (1U << 0)
#define TIM_BDTR_MOE       (1U << 15)

/* ===================================================================== */
/*  TIM2 — 32-bit timer (1 Hz system tick / sample scheduler)            */
/* ===================================================================== */

#define TIM2_BASE       0x40000000U
#define TIM2_CR1        (*(volatile uint32_t *)(TIM2_BASE + 0x00U))
#define TIM2_DIER       (*(volatile uint32_t *)(TIM2_BASE + 0x0CU))
#define TIM2_SR         (*(volatile uint32_t *)(TIM2_BASE + 0x10U))
#define TIM2_CNT        (*(volatile uint32_t *)(TIM2_BASE + 0x24U))
#define TIM2_PSC        (*(volatile uint32_t *)(TIM2_BASE + 0x28U))
#define TIM2_ARR        (*(volatile uint32_t *)(TIM2_BASE + 0x2CU))

/* ===================================================================== */
/*  TIM3 — Leaf-wetness capacitive frequency measurement                 */
/* ===================================================================== */

#define TIM3_BASE       0x40000400U
#define TIM3_CR1        (*(volatile uint32_t *)(TIM3_BASE + 0x00U))
#define TIM3_SMCR       (*(volatile uint32_t *)(TIM3_BASE + 0x08U))
#define TIM3_CCMR1      (*(volatile uint32_t *)(TIM3_BASE + 0x18U))
#define TIM3_CCMR2      (*(volatile uint32_t *)(TIM3_BASE + 0x1CU))
#define TIM3_CCER       (*(volatile uint32_t *)(TIM3_BASE + 0x20U))
#define TIM3_CNT        (*(volatile uint32_t *)(TIM3_BASE + 0x24U))
#define TIM3_PSC        (*(volatile uint32_t *)(TIM3_BASE + 0x28U))
#define TIM3_ARR        (*(volatile uint32_t *)(TIM3_BASE + 0x2CU))
#define TIM3_CCR1       (*(volatile uint32_t *)(TIM3_BASE + 0x34U))
#define TIM3_CCR2       (*(volatile uint32_t *)(TIM3_BASE + 0x38U))

#define TIM_SMCR_SMS_EC  (0x7U << 0)  /* External clock mode 1 */
#define TIM_CCMR1_CC1S_TI1 (0x1U << 0)
#define TIM_CCER_CC1E   (1U << 0)
#define TIM_CCER_CC1P   (1U << 1)

/* ===================================================================== */
/*  SPI1 — ADS1292 biopotential ADC (master)                             */
/*  SPI2 — ADS1220 sap-flow thermistor ADC + microSD (CS-mux)           */
/* ===================================================================== */

#define SPI1_BASE       0x40013000U
#define SPI2_BASE       0x40003800U

#define SPI_CR1          0x00U
#define SPI_CR2          0x04U
#define SPI_SR           0x08U
#define SPI_DR           0x0CU
#define SPI_CRCPR        0x10U
#define SPI_RXCRCR       0x14U
#define SPI_TXCRCR       0x18U

#define SPI_CR1_CPHA     (1U << 0)
#define SPI_CR1_CPOL     (1U << 1)
#define SPI_CR1_MSTR     (1U << 2)
#define SPI_CR1_BR_DIV2  (0U << 3)
#define SPI_CR1_BR_DIV8  (2U << 3)
#define SPI_CR1_BR_DIV16 (3U << 3)
#define SPI_CR1_BR_DIV32 (4U << 3)
#define SPI_CR1_SPE      (1U << 6)
#define SPI_CR1_LSBFIRST (1U << 7)
#define SPI_CR1_SSI       (1U << 8)
#define SPI_CR1_SSM       (1U << 9)

#define SPI_CR2_DS_8BIT  (0x7U << 8)
#define SPI_CR2_DS_16BIT  (0xFU << 8)
#define SPI_CR2_FRXTH    (1U << 12)
#define SPI_CR2_NSSP     (1U << 3)
#define SPI_CR2_RXNEIE   (1U << 6)
#define SPI_CR2_TXEIE    (1U << 7)

#define SPI_SR_RXNE      (1U << 0)
#define SPI_SR_TXE       (1U << 1)
#define SPI_SR_BSY       (1U << 7)
#define SPI_SR_MODF      (1U << 5)
#define SPI_SR_OVR       (1U << 6)
#define SPI_SR_CRCERR    (1U << 4)

#define SPI_REG(port, offset) (*(volatile uint32_t *)((port) + (offset)))

/* ===================================================================== */
/*  I2C1 — BME280 + APDS-9301 + MAX17048 fuel gauge                     */
/* ===================================================================== */

#define I2C1_BASE       0x40005400U
#define I2C_CR1         (*(volatile uint32_t *)(I2C1_BASE + 0x00U))
#define I2C_CR2         (*(volatile uint32_t *)(I2C1_BASE + 0x04U))
#define I2C_OAR1        (*(volatile uint32_t *)(I2C1_BASE + 0x08U))
#define I2C_OAR2        (*(volatile uint32_t *)(I2C1_BASE + 0x0CU))
#define I2C_TIMINGR     (*(volatile uint32_t *)(I2C1_BASE + 0x10U))
#define I2C_ISR         (*(volatile uint32_t *)(I2C1_BASE + 0x14U))
#define I2C_ICR         (*(volatile uint32_t *)(I2C1_BASE + 0x1CU))
#define I2C_PECR        (*(volatile uint32_t *)(I2C1_BASE + 0x20U))
#define I2C_RXDR        (*(volatile uint32_t *)(I2C1_BASE + 0x24U))
#define I2C_TXDR        (*(volatile uint32_t *)(I2C1_BASE + 0x28U))

#define I2C_ISR_TXE     (1U << 0)
#define I2C_ISR_TXIS    (1U << 1)
#define I2C_ISR_RXNE    (1U << 2)
#define I2C_ISR_ADDR    (1U << 3)
#define I2C_ISR_NACKF   (1U << 4)
#define I2C_ISR_STOPF   (1U << 5)
#define I2C_ISR_TC      (1U << 6)
#define I2C_ISR_TCR     (1U << 7)
#define I2C_ISR_BUSY    (1U << 15)

#define I2C_CR1_PE      (1U << 0)
#define I2C_CR2_START   (1U << 13)
#define I2C_CR2_STOP    (1U << 14)
#define I2C_CR2_NBYTES_1 (0x1U << 16)
#define I2C_CR2_NBYTES_2 (0x2U << 16)
#define I2C_CR2_AUTOEND (1U << 25)
#define I2C_CR2_RELOAD  (1U << 24)
#define I2C_CR2_RD_WRN  (1U << 10)

/* ===================================================================== */
/*  USART1 — nRF52840 BLE module                                         */
/*  UART4 — SX1276 LoRa (via DIO/interrupt, config via SPI — see below)  */
/*  LPUART1 — GPS backup (optional)                                      */
/* ===================================================================== */

#define USART1_BASE     0x40013800U
#define UART4_BASE      0x40004C00U

#define USART_CR1        0x00U
#define USART_CR2        0x04U
#define USART_CR3        0x08U
#define USART_BRR        0x0CU
#define USART_RQR        0x18U
#define USART_ISR        0x1CU
#define USART_ICR        0x20U
#define USART_RDR        0x24U
#define USART_TDR        0x28U

#define USART_CR1_UE     (1U << 0)
#define USART_CR1_RE     (1U << 2)
#define USART_CR1_TE     (1U << 3)
#define USART_CR1_RXNEIE (1U << 5)
#define USART_CR1_TCIE   (1U << 6)
#define USART_CR1_IDLEIE (1U << 4)

#define USART_ISR_TXE    (1U << 7)
#define USART_ISR_TC     (1U << 6)
#define USART_ISR_RXNE   (1U << 5)
#define USART_ISR_IDLE   (1U << 4)
#define USART_ISR_ORE    (1U << 3)

#define USART_REG(port, offset) (*(volatile uint32_t *)((port) + (offset)))

/* ===================================================================== */
/*  DMA1 — for SPI1 ADS1292 streaming + SD card                         */
/* ===================================================================== */

#define DMA1_BASE        0x40026000U
#define DMA1_CSELR       (*(volatile uint32_t *)(DMA1_BASE + 0xA8U))

#define DMA1_Channel1    (DMA1_BASE + 0x10U)
#define DMA1_Channel2    (DMA1_BASE + 0x24U)
#define DMA1_Channel3    (DMA1_BASE + 0x38U)
#define DMA1_Channel4    (DMA1_BASE + 0x4CU)
#define DMA1_Channel5    (DMA1_BASE + 0x60U)

#define DMA_ISR          (*(volatile uint32_t *)(DMA1_BASE + 0x00U))
#define DMA_IFCR         (*(volatile uint32_t *)(DMA1_BASE + 0x04U))

#define DMA_CCR          0x08U
#define DMA_CNDTR        0x0CU
#define DMA_CPAR         0x10U
#define DMA_CMAR         0x14U

#define DMA_CCR_EN       (1U << 0)
#define DMA_CCR_TCIE     (1U << 1)
#define DMA_CCR_TEIE     (1U << 2)
#define DMA_CCR_MINC     (1U << 7)
#define DMA_CCR_PINC     (1U << 6)
#define DMA_CCR_PSIZE_8  (0x0U << 4)
#define DMA_CCR_MSIZE_8  (0x0U << 8)
#define DMA_CCR_PSIZE_16 (0x1U << 4)
#define DMA_CCR_MSIZE_16 (0x1U << 8)
#define DMA_CCR_CIRC     (1U << 5)
#define DMA_CCR_DIR_R2P  (1U << 4)
#define DMA_CCR_HTIE     (1U << 3)

#define DMA_REG(ch, offset) (*(volatile uint32_t *)((ch) + (offset)))

/* ===================================================================== */
/*  ADC1 — internal (battery voltage, temperature)                      */
/* ===================================================================== */

#define ADC1_BASE        0x50040000U
#define ADC_CR           (*(volatile uint32_t *)(ADC1_BASE + 0x08U))
#define ADC_CFGR         (*(volatile uint32_t *)(ADC1_BASE + 0x0CU))
#define ADC_SQR1         (*(volatile uint32_t *)(ADC1_BASE + 0x30U))
#define ADC_DR           (*(volatile uint32_t *)(ADC1_BASE + 0x40U))
#define ADC_ISR          (*(volatile uint32_t *)(ADC1_BASE + 0x00U))
#define ADC_ISR_ADRDY    (1U << 0)
#define ADC_ISR_EOC      (1U << 2)
#define ADC_CR_ADVREN    (1U << 28)
#define ADC_CR_ADEN      (1U << 0)
#define ADC_CR_ADCAL     (1U << 31)

/* ===================================================================== */
/*  Flash controller — for option bytes / boot config                   */
/* ===================================================================== */

#define FLASH_BASE      0x40022000U
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x08U))
#define FLASH_SR        (*(volatile uint32_t *)(FLASH_BASE + 0x10U))
#define FLASH_CR        (*(volatile uint32_t *)(FLASH_BASE + 0x14U))

/* ===================================================================== */
/*  RTC — for timestamping and wakeup (deep sleep)                      */
/* ===================================================================== */

#define RTC_BASE        0x40002800U
#define RTC_TR          (*(volatile uint32_t *)(RTC_BASE + 0x00U))
#define RTC_DR          (*(volatile uint32_t *)(RTC_BASE + 0x04U))
#define RTC_CR          (*(volatile uint32_t *)(RTC_BASE + 0x08U))
#define RTC_ISR         (*(volatile uint32_t *)(RTC_BASE + 0x0CU))
#define RTC_PRER        (*(volatile uint32_t *)(RTC_BASE + 0x10U))
#define RTC_ALRMAR      (*(volatile uint32_t *)(RTC_BASE + 0x1CU))
#define RTC_WPR         (*(volatile uint32_t *)(RTC_BASE + 0x24U))

#define RTC_ISR_INIT     (1U << 7)
#define RTC_ISR_ALRAF    (1U << 0)

/* ===================================================================== */
/*  ADS1292 24-bit biopotential ADC command set (SPI)                   */
/* ===================================================================== */

#define ADS1292_CMD_WAKEUP   0x02U
#define ADS1292_CMD_STANDBY  0x04U
#define ADS1292_CMD_RESET    0x06U
#define ADS1292_CMD_START    0x08U
#define ADS1292_CMD_STOP     0x0AU
#define ADS1292_CMD_RDATAC   0x10U
#define ADS1292_CMD_SDATAC   0x11U
#define ADS1292_CMD_RDATA    0x12U
#define ADS1292_CMD_RREG     0x20U   /* + reg addr */
#define ADS1292_CMD_WREG     0x40U   /* + reg addr */

/* ADS1292 register map */
#define ADS1292_REG_ID       0x00U
#define ADS1292_REG_CONFIG1  0x01U
#define ADS1292_REG_CONFIG2  0x02U
#define ADS1292_REG_LOFF     0x03U
#define ADS1292_REG_CH1SET  0x04U
#define ADS1292_REG_CH2SET  0x05U
#define ADS1292_REG_RLDSENS 0x06U
#define ADS1292_REG_LOFFSENS 0x07U
#define ADS1292_REG_LOFFSTAT 0x08U
#define ADS1292_REG_RESP1   0x09U
#define ADS1292_REG_RESP2   0x0AU
#define ADS1292_REG_GPIO    0x0BU

/* CONFIG1 */
#define ADS1292_CONFIG1_HR      (0x0U << 0)  /* High-resolution 250 SPS */
#define ADS1292_CONFIG1_LP      (0x1U << 0)
#define ADS1292_CONFIG1_DR_125  (0x0U << 2)
#define ADS1292_CONFIG1_DR_250  (0x4U << 2)
#define ADS1292_CONFIG1_DR_500  (0x8U << 2)
#define ADS1292_CONFIG1_DR_1K   (0xCU << 2)

/* CONFIG2 */
#define ADS1292_CONFIG2_WCT     (1U << 7)
#define ADS1292_CONFIG2_INT_TEST (1U << 4)

/* CHxSET */
#define ADS1292_CHSET_GAIN_1   (0x0U << 4)
#define ADS1292_CHSET_GAIN_2   (0x1U << 4)
#define ADS1292_CHSET_GAIN_4   (0x2U << 4)
#define ADS1292_CHSET_GAIN_6   (0x3U << 4)
#define ADS1292_CHSET_GAIN_8   (0x4U << 4)
#define ADS1292_CHSET_GAIN_12  (0x5U << 4)
#define ADS1292_CHSET_MUX_NORMAL (0x0U << 0)

/* ID */
#define ADS1292_ID_DEV_ID_MASK 0x0FU
#define ADS1292_ID_DEV_ADS1292 0x5U

/* ===================================================================== */
/*  ADS1220 24-bit general-purpose ADC (sap-flow thermistors)            */
/* ===================================================================== */

#define ADS1220_CMD_RESET    0x06U
#define ADS1220_CMD_START    0x08U
#define ADS1220_CMD_POWERDN  0x02U
#define ADS1220_CMD_RDATA    0x10U
#define ADS1220_CMD_RREG     0x20U
#define ADS1220_CMD_WREG     0x40U

#define ADS1220_REG_CFG0     0x00U
#define ADS1220_REG_CFG1     0x01U
#define ADS1220_REG_CFG2     0x02U
#define ADS1220_REG_CFG3     0x03U

/* ===================================================================== */
/*  BME280 I2C addresses / registers                                     */
/* ===================================================================== */

#define BME280_I2C_ADDR       0x76U
#define BME280_REG_ID         0xD0U
#define BME280_REG_CTRL_HUM   0xF2U
#define BME280_REG_STATUS     0xF3U
#define BME280_REG_CTRL_MEAS  0xF4U
#define BME280_REG_CONFIG     0xF5U
#define BME280_REG_CALIB00    0x88U
#define BME280_REG_DATA       0xF7U   /* 8 bytes: press(3) temp(3) hum(2) */
#define BME280_ID_VAL         0x60U

/* ===================================================================== */
/*  APDS-9301 ambient light sensor (PAR proxy)                          */
/* ===================================================================== */

#define APDS9301_I2C_ADDR     0x39U
#define APDS9301_REG_CTRL     0x80U
#define APDS9301_REG_TIMING   0x81U
#define APDS9301_REG_THRESHLOW 0x82U
#define APDS9301_REG_ID       0x8AU
#define APDS9301_REG_DATA0    0x8CU
#define APDS9301_CMD_CMD      0x80U
#define APDS9301_POWER_ON     0x03U

/* ===================================================================== */
/*  MAX17048 fuel gauge                                                  */
/* ===================================================================== */

#define MAX17048_I2C_ADDR     0x36U
#define MAX17048_REG_VCELL    0x02U
#define MAX17048_REG_SOC      0x04U
#define MAX17048_REG_VERSION  0x08U
#define MAX17048_REG_HIBRT    0x0AU
#define MAX17048_REG_CONFIG   0x0CU
#define MAX17048_REG_VALRT    0x14U
#define MAX17048_REG_CRATE    0x16U

/* ===================================================================== */
/*  SX1276 LoRa transceiver register addresses (via SPI2, CS-mux)        */
/* ===================================================================== */

#define SX1276_REG_FIFO          0x00U
#define SX1276_REG_OP_MODE       0x01U
#define SX1276_REG_FRF_MSB       0x06U
#define SX1276_REG_FRF_MID       0x07U
#define SX1276_REG_FRF_LSB       0x08U
#define SX1276_REG_PA_CONFIG      0x09U
#define SX1276_REG_FIFO_ADDR_PTR 0x0DU
#define SX1276_REG_FIFO_TX_BASE  0x0EU
#define SX1276_REG_FIFO_RX_BASE  0x0FU
#define SX1276_REG_FIFO_RX_CURR  0x10U
#define SX1276_REG_IRQ_FLAGS     0x12U
#define SX1276_REG_RX_NB_BYTES   0x13U
#define SX1276_REG_PKT_SNR       0x19U
#define SX1276_REG_PKT_RSSI      0x1AU
#define SX1276_REG_MODEM_CONFIG1 0x1DU
#define SX1276_REG_MODEM_CONFIG2 0x1EU
#define SX1276_REG_SYMB_TIMEOUT  0x1FU
#define SX1276_REG_PREAMBLE_MSB 0x20U
#define SX1276_REG_PREAMBLE_LSB 0x21U
#define SX1276_REG_PAYLOAD_LEN   0x22U
#define SX1276_REG_MODEM_CONFIG3 0x26U
#define SX1276_REG_RSSI_WIDE     0x2AU
#define SX1276_REG_DIO_MAPPING1  0x40U
#define SX1276_REG_VERSION       0x42U
#define SX1276_REG_PADAC         0x4DU

#define SX1276_MODE_LONG_RANGE   (1U << 7)
#define SX1276_MODE_SLEEP        0x00U
#define SX1276_MODE_STDBY        0x01U
#define SX1276_MODE_TX            0x03U
#define SX1276_MODE_RX_CONTINUOUS 0x05U

#define SX1276_IRQ_TX_DONE       (1U << 3)
#define SX1276_IRQ_RX_DONE       (1U << 6)

#define SX1276_SPI_WRITE         0x00U
#define SX1276_SPI_READ          0x80U

/* ===================================================================== */
/*  SD card commands (SPI mode)                                          */
/* ===================================================================== */

#define SD_CMD0_GO_IDLE          0x40U
#define SD_CMD8_SEND_IF_COND     0x48U
#define SD_CMD9_SEND_CSD         0x49U
#define SD_CMD16_SET_BLOCKLEN    0x50U
#define SD_CMD17_READ_SINGLE    0x51U
#define SD_CMD24_WRITE_SINGLE   0x58U
#define SD_CMD55_APP            0x77U
#define SD_ACMD41_SD_SEND_OP    0x41U

#define SD_TOKEN_START_BLOCK     0xFEU
#define SD_TOKEN_DATA_ACCEPTED   0x05U

#endif /* FLORAPULSE_REGISTERS_H */