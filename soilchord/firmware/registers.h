/*
 * registers.h — Soilchord peripheral register convenience definitions
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Lightweight macros and base-address helpers for the peripherals used by
 * the firmware. These keep the driver files free of raw CMSIS noise and make
 * the register-level intent readable.
 */
#ifndef SOILCHORD_REGISTERS_H
#define SOILCHORD_REGISTERS_H

#include <stdint.h>

/* STM32L4 memory map base addresses (subset relevant to Soilchord) */
#define PERIPH_BASE             (0x40000000UL)
#define AHB1PERIPH_BASE         (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE         (PERIPH_BASE + 0x08000000UL)
#define APB1PERIPH_BASE         (PERIPH_BASE)
#define APB2PERIPH_BASE         (PERIPH_BASE + 0x00010000UL)

/* GPIO ports */
#define GPIOA_BASE_REG          (AHB2PERIPH_BASE + 0x0000)
#define GPIOB_BASE_REG          (AHB2PERIPH_BASE + 0x0400)
#define GPIOC_BASE_REG          (AHB2PERIPH_BASE + 0x0800)

/* RCC */
#define RCC_BASE_REG            (AHB1PERIPH_BASE + 0x1000)
#define RCC_CR                  (*(volatile uint32_t *)(RCC_BASE_REG + 0x00))
#define RCC_CFGR                (*(volatile uint32_t *)(RCC_BASE_REG + 0x08))
#define RCC_AHB1ENR             (*(volatile uint32_t *)(RCC_BASE_REG + 0x48))
#define RCC_AHB2ENR             (*(volatile uint32_t *)(RCC_BASE_REG + 0x4C))
#define RCC_APB1ENR1            (*(volatile uint32_t *)(RCC_BASE_REG + 0x58))
#define RCC_APB2ENR             (*(volatile uint32_t *)(RCC_BASE_REG + 0x60))
#define RCC_BDCR                (*(volatile uint32_t *)(RCC_BASE_REG + 0x90))

/* GPIO helpers */
#define GPIO_MODER(p)           (*(volatile uint32_t *)((p) + 0x00))
#define GPIO_OTYPER(p)          (*(volatile uint32_t *)((p) + 0x04))
#define GPIO_OSPEEDR(p)         (*(volatile uint32_t *)((p) + 0x08))
#define GPIO_PUPDR(p)           (*(volatile uint32_t *)((p) + 0x0C))
#define GPIO_IDR(p)             (*(volatile uint32_t *)((p) + 0x10))
#define GPIO_ODR(p)             (*(volatile uint32_t *)((p) + 0x14))
#define GPIO_BSRR(p)            (*(volatile uint32_t *)((p) + 0x18))
#define GPIO_AFR(p, n)         (*(volatile uint32_t *)((p) + 0x20 + 4*(n)))

/* ADC1 */
#define ADC1_BASE_REG           (AHB2PERIPH_BASE + 0x0800)
#define ADC_ISR                 (*(volatile uint32_t *)(ADC1_BASE_REG + 0x00))
#define ADC_CR                  (*(volatile uint32_t *)(ADC1_BASE_REG + 0x08))
#define ADC_CFGR                (*(volatile uint32_t *)(ADC1_BASE_REG + 0x0C))
#define ADC_SQR1                (*(volatile uint32_t *)(ADC1_BASE_REG + 0x30))
#define ADC_DR                  (*(volatile uint32_t *)(ADC1_BASE_REG + 0x40))
#define ADC_ISR_ADRDY          (1U << 0)
#define ADC_ISR_EOC            (1U << 2)
#define ADC_CR_ADEN            (1U << 0)
#define ADC_CR_ADSTART         (1U << 2)

/* TIM1 (solenoid gates) */
#define TIM1_BASE_REG           (APB2PERIPH_BASE + 0x0000)
#define TIM_CR1(t)             (*(volatile uint32_t *)((t) + 0x00))
#define TIM_CCER(t)            (*(volatile uint32_t *)((t) + 0x20))
#define TIM_CCMR1(t)           (*(volatile uint32_t *)((t) + 0x18))
#define TIM_CCR1(t)            (*(volatile uint32_t *)((t) + 0x34))

/* SPI2 (LoRa) */
#define SPI2_BASE_REG           (APB1PERIPH_BASE + 0x3800)
#define SPI_CR1(s)             (*(volatile uint32_t *)((s) + 0x00))
#define SPI_SR(s)              (*(volatile uint32_t *)((s) + 0x08))
#define SPI_DR(s)              (*(volatile uint32_t *)((s) + 0x0C))
#define SPI_SR_TXE             (1U << 1)
#define SPI_SR_RXNE           (1U << 0)
#define SPI_SR_BSY            (1U << 7)

/* I2C1 (MAX17048) */
#define I2C1_BASE_REG           (APB1PERIPH_BASE + 0x5400)
#define I2C_CR1(i)             (*(volatile uint32_t *)((i) + 0x00))
#define I2C_CR2(i)             (*(volatile uint32_t *)((i) + 0x04))
#define I2C_ISR(i)             (*(volatile uint32_t *)((i) + 0x18))
#define I2C_TXDR(i)            (*(volatile uint32_t *)((i) + 0x28))
#define I2C_RXDR(i)            (*(volatile uint32_t *)((i) + 0x24))
#define I2C_ISR_TXE            (1U << 0)
#define I2C_ISR_RXNE          (1U << 2)
#define I2C_ISR_TC            (1U << 6)
#define I2C_ISR_NACKF         (1U << 4)
#define I2C_ISR_BUSY           (1U << 15)

/* RTC */
#define RTC_BASE_REG            (APB1PERIPH_BASE + 0x2800)
#define RTC_TR                 (*(volatile uint32_t *)(RTC_BASE_REG + 0x00))
#define RTC_DR                 (*(volatile uint32_t *)(RTC_BASE_REG + 0x04))
#define RTC_CR                 (*(volatile uint32_t *)(RTC_BASE_REG + 0x18))
#define RTC_ISR                (*(volatile uint32_t *)(RTC_BASE_REG + 0x0C))
#define RTC_ALRMAR             (*(volatile uint32_t *)(RTC_BASE_REG + 0x1C))
#define RTC_WPR                (*(volatile uint32_t *)(RTC_BASE_REG + 0x24))

/* PWR */
#define PWR_BASE_REG            (APB1PERIPH_BASE + 0x0000)
#define PWR_CR1                (*(volatile uint32_t *)(PWR_BASE_REG + 0x00))
#define PWR_SR1                (*(volatile uint32_t *)(PWR_BASE_REG + 0x04))
#define PWR_CR1_LPMS_STOP2     (0x2U << 0)

/* EXTI (for DIO1 and RTC alarm wake) */
#define EXTI_BASE_REG          (APB2PERIPH_BASE + 0x3C00)
#define EXTI_IMR1             (*(volatile uint32_t *)(EXTI_BASE_REG + 0x00))
#define EXTI_RTSR1            (*(volatile uint32_t *)(EXTI_BASE_REG + 0x08))
#define EXTI_PR1              (*(volatile uint32_t *)(EXTI_BASE_REG + 0x14))

/* Flash option/option-bytes for storing a small configuration blob without
 * wearing the main flash. We use the last page of user flash as a config
 * record (the HAL flash_unlock/program sequence is in flash_log.c). */
#define FLASH_CONFIG_ADDR      (0x0803F800UL)        /* last 2 KB page on L432 */
#define FLASH_MAGIC_OK         (0xA5C0FFEEUL)

#endif /* SOILCHORD_REGISTERS_H */