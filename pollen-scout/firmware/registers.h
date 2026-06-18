/*
 * registers.h - Register-level definitions for Pollen Scout peripherals
 * STM32H743 reference: RM0433 rev 7
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef POLLEN_SCOUT_REGISTERS_H
#define POLLEN_SCOUT_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses (H743) ---- */
#define PERIPH_BASE         (0x40000000U)
#define AHB1_BASE           (PERIPH_BASE + 0x00020000U)
#define AHB2_BASE           (PERIPH_BASE + 0x08020000U)
#define AHB4_BASE           (PERIPH_BASE + 0x00010000U)
#define APB1_BASE           (PERIPH_BASE + 0x00000000U)
#define APB2_BASE           (PERIPH_BASE + 0x00010000U)
#define APB4_BASE           (PERIPH_BASE + 0x00030000U)

/* GPIO banks (AHB4) */
#define GPIOA_BASE          (AHB4_BASE + 0x0000U)
#define GPIOB_BASE          (AHB4_BASE + 0x0100U)
#define GPIOC_BASE          (AHB4_BASE + 0x0200U)
#define GPIOD_BASE          (AHB4_BASE + 0x0300U)
#define GPIOE_BASE          (AHB4_BASE + 0x0400U)
#define GPIOF_BASE          (AHB4_BASE + 0x0500U)
#define GPIOG_BASE          (AHB4_BASE + 0x0600U)
#define GPIOH_BASE          (AHB4_BASE + 0x0700U)

/* RCC (AHB1) */
#define RCC_BASE            (AHB1_BASE + 0x4400U)
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00U))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x08U))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10U))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xD0U))
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0U))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE8U))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF0U))
#define RCC_APB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xF8U))

/* PWR (AHB1) */
#define PWR_BASE            (AHB1_BASE + 0x4800U)
#define PWR_CR3             (*(volatile uint32_t *)(PWR_BASE + 0x10U))
#define PWR_SR1             (*(volatile uint32_t *)(PWR_BASE + 0x14U))

/* GPIO register offsets */
#define GPIO_MODER_OFFSET   0x00U
#define GPIO_OTYPER_OFFSET  0x04U
#define GPIO_OSPEEDR_OFFSET 0x08U
#define GPIO_PUPDR_OFFSET   0x0CU
#define GPIO_IDR_OFFSET     0x10U
#define GPIO_ODR_OFFSET     0x14U
#define GPIO_BSRR_OFFSET    0x18U
#define GPIO_AFRL_OFFSET    0x20U
#define GPIO_AFRH_OFFSET    0x24U

#define GPIO_REG(port, off) (*(volatile uint32_t *)((port) + (off)))

/* I2C base addresses */
#define I2C1_BASE           (APB1_BASE + 0x5400U)
#define I2C3_BASE           (APB1_BASE + 0x5C00U)
#define I2C4_BASE           (APB4_BASE + 0x5400U)

#define I2C_CR1(i2c)        (*(volatile uint32_t *)((i2c) + 0x00U))
#define I2C_CR2(i2c)        (*(volatile uint32_t *)((i2c) + 0x04U))
#define I2C_ISR(i2c)        (*(volatile uint32_t *)((i2c) + 0x10U))
#define I2C_TXDR(i2c)       (*(volatile uint32_t *)((i2c) + 0x28U))
#define I2C_RXDR(i2c)       (*(volatile uint32_t *)((i2c) + 0x24U))
#define I2C_TIMINGR(i2c)    (*(volatile uint32_t *)((i2c) + 0x08U))

/* SPI base */
#define SPI1_BASE           (APB2_BASE + 0x3000U)
#define SPI_CR1(spi)        (*(volatile uint32_t *)((spi) + 0x00U))
#define SPI_CR2(spi)        (*(volatile uint32_t *)((spi) + 0x04U))
#define SPI_SR(spi)         (*(volatile uint32_t *)((spi) + 0x08U))
#define SPI_DR(spi)         (*(volatile uint32_t *)((spi) + 0x0CU))

/* USART/UART bases */
#define USART3_BASE         (APB1_BASE + 0x4800U)
#define UART4_BASE          (APB1_BASE + 0x5000U)
#define USART6_BASE         (APB2_BASE + 0x3800U)
#define USART_CR1(uart)     (*(volatile uint32_t *)((uart) + 0x00U))
#define USART_CR2(uart)     (*(volatile uint32_t *)((uart) + 0x04U))
#define USART_BRR(uart)     (*(volatile uint32_t *)((uart) + 0x0CU))
#define USART_ISR(uart)     (*(volatile uint32_t *)((uart) + 0x1CU))
#define USART_TDR(uart)     (*(volatile uint32_t *)((uart) + 0x28U))
#define USART_RDR(uart)     (*(volatile uint32_t *)((uart) + 0x24U))

/* Timer bases */
#define TIM1_BASE           (APB2_BASE + 0x0000U)
#define TIM3_BASE           (APB1_BASE + 0x0400U)
#define TIM4_BASE           (APB1_BASE + 0x0800U)
#define TIM_CR1(tim)        (*(volatile uint32_t *)((tim) + 0x00U))
#define TIM_CCMR1(tim)      (*(volatile uint32_t *)((tim) + 0x18U))
#define TIM_CCER(tim)       (*(volatile uint32_t *)((tim) + 0x20U))
#define TIM_PSC(tim)        (*(volatile uint32_t *)((tim) + 0x28U))
#define TIM_ARR(tim)        (*(volatile uint32_t *)((tim) + 0x2CU))
#define TIM_CCR1(tim)       (*(volatile uint32_t *)((tim) + 0x34U))
#define TIM_CCR2(tim)       (*(volatile uint32_t *)((tim) + 0x38U))
#define TIM_EGR(tim)        (*(volatile uint32_t *)((tim) + 0x14U))

/* RTC */
#define RTC_BASE            (APB4_BASE + 0x4800U)
#define RTC_TR              (*(volatile uint32_t *)(RTC_BASE + 0x00U))
#define RTC_DR              (*(volatile uint32_t *)(RTC_BASE + 0x04U))
#define RTC_ISR             (*(volatile uint32_t *)(RTC_BASE + 0x0CU))
#define RTC_PRER            (*(volatile uint32_t *)(RTC_BASE + 0x10U))

/* ADC */
#define ADC1_BASE           (AHB1_BASE + 0x4400U)
#define ADC_ISR(adc)        (*(volatile uint32_t *)((adc) + 0x00U))
#define ADC_CR(adc)         (*(volatile uint32_t *)((adc) + 0x08U))
#define ADC_SQR1(adc)       (*(volatile uint32_t *)((adc) + 0x30U))
#define ADC_DR(adc)         (*(volatile uint32_t *)((adc) + 0x40U))

/* DMA (DMA1 stream 0 used for DCMI) */
#define DMA1_BASE           (AHB1_BASE + 0x0000U)
#define DMA1_Stream0_BASE   (DMA1_BASE + 0x0100U)
#define DMA_SPAR(s)         (*(volatile uint32_t *)((s) + 0x00U))
#define DMA_SM0AR(s)        (*(volatile uint32_t *)((s) + 0x04U))
#define DMA_SNDTR(s)        (*(volatile uint32_t *)((s) + 0x08U))
#define DMA_SCR(s)          (*(volatile uint32_t *)((s) + 0x0CU))
#define DMA_SFCR(s)         (*(volatile uint32_t *)((s) + 0x10U))

/* DCMI (camera interface) */
#define DCMI_BASE           (AHB2_BASE + 0x4000U)
#define DCMI_CR             (*(volatile uint32_t *)(DCMI_BASE + 0x00U))
#define DCMI_SR             (*(volatile uint32_t *)(DCMI_BASE + 0x04U))
#define DCMI_RIS            (*(volatile uint32_t *)(DCMI_BASE + 0x08U))
#define DCMI_DR             (*(volatile uint32_t *)(DCMI_BASE + 0x20U))
#define DCMI_FSCR           (*(volatile uint32_t *)(DCMI_BASE + 0x14U))
#define DCMI_CWSTRT         (*(volatile uint32_t *)(DCMI_BASE + 0x10U))
#define DCMI_CWSIZE         (*(volatile uint32_t *)(DCMI_BASE + 0x0CU))

/* SDMMC */
#define SDMMC1_BASE         (AHB2_BASE + 0x4800U)
#define SDMMC_POWER         (*(volatile uint32_t *)(SDMMC1_BASE + 0x00U))
#define SDMMC_CLKCR         (*(volatile uint32_t *)(SDMMC1_BASE + 0x04U))
#define SDMMC_CMD           (*(volatile uint32_t *)(SDMMC1_BASE + 0x08U))
#define SDMMC_RESP1         (*(volatile uint32_t *)(SDMMC1_BASE + 0x10U))

/* Cortex-M7 SysTick / NVIC / SCB */
#define SCB_BASE            (0xE000ED00U)
#define SCB_AIRCR           (*(volatile uint32_t *)(SCB_BASE + 0x0CU))
#define NVIC_ISER           (*(volatile uint32_t *)(0xE000E100U))
#define NVIC_ICER           (*(volatile uint32_t *)(0xE000E180U))
#define SYSTICK_BASE        (0xE000E010U)
#define SYST_CSR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x00U))
#define SYST_RVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x04U))
#define SYST_CVR            (*(volatile uint32_t *)(SYSTICK_BASE + 0x08U))

/* ---- Bit helpers ---- */
#define BIT(n)              (1U << (n))

/* RCC enable bits */
#define RCC_AHB1ENR_DCMIEN  BIT(0)
#define RCC_AHB1ENR_DMA1EN  BIT(0)
#define RCC_AHB1ENR_ADC12EN BIT(24)
#define RCC_AHB4ENR_GPIOAEN BIT(0)
#define RCC_AHB4ENR_GPIOBEN BIT(1)
#define RCC_AHB4ENR_GPIOCEN BIT(2)
#define RCC_AHB4ENR_GPIODEN BIT(3)
#define RCC_AHB4ENR_GPIOEEN BIT(4)
#define RCC_AHB4ENR_GPIOFEN BIT(5)
#define RCC_AHB2ENR_SDMMC1EN BIT(3)
#define RCC_APB1ENR_I2C1EN  BIT(21)
#define RCC_APB1ENR_USART3EN BIT(18)
#define RCC_APB1ENR_UART4EN BIT(12)
#define RCC_APB1ENR_TIM3EN  BIT(1)
#define RCC_APB1ENR_TIM4EN  BIT(2)
#define RCC_APB2ENR_SPI1EN  BIT(12)
#define RCC_APB2ENR_USART6EN BIT(5)
#define RCC_APB2ENR_TIM1EN  BIT(0)
#define RCC_APB4ENR_I2C3EN  BIT(3)
#define RCC_APB4ENR_I2C4EN  BIT(4)

/* USART ISR flags */
#define USART_ISR_TXE       BIT(7)
#define USART_ISR_RXNE      BIT(5)
#define USART_ISR_TC        BIT(6)
#define USART_ISR_BUSY      BIT(25)

/* I2C ISR flags */
#define I2C_ISR_TXIS        BIT(1)
#define I2C_ISR_RXNE        BIT(2)
#define I2C_ISR_TXE         BIT(0)
#define I2C_ISR_NACKF       BIT(4)
#define I2C_ISR_STOPF       BIT(5)
#define I2C_ISR_BUSY        BIT(15)

/* SPI SR flags */
#define SPI_SR_TXE          BIT(1)
#define SPI_SR_RXNE         BIT(0)
#define SPI_SR_BSY          BIT(7)

#endif /* POLLEN_SCOUT_REGISTERS_H */