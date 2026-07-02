/*
 * registers.h — TensilGuard peripheral register convenience definitions
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Lightweight macros and base-address helpers for the peripherals used by
 * the firmware. These keep the driver files free of raw CMSIS noise and make
 * the register-level intent readable while remaining target-portable.
 */
#ifndef TENSILGUARD_REGISTERS_H
#define TENSILGUARD_REGISTERS_H

#include <stdint.h>

/* STM32G4 memory map base addresses (subset relevant to TensilGuard) */
#define PERIPH_BASE             (0x40000000UL)
#define AHB1PERIPH_BASE          (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE          (PERIPH_BASE + 0x08000000UL)
#define APB1PERIPH_BASE          (PERIPH_BASE)
#define APB2PERIPH_BASE          (PERIPH_BASE + 0x00010000UL)

/* GPIO ports — STM32G4 puts GPIO on AHB2 */
#define GPIOA_BASE_REG           (AHB2PERIPH_BASE + 0x0000)
#define GPIOB_BASE_REG           (AHB2PERIPH_BASE + 0x0400)
#define GPIOC_BASE_REG            (AHB2PERIPH_BASE + 0x0800)

/* RCC */
#define RCC_BASE_REG              (AHB1PERIPH_BASE + 0x1000)
#define RCC_CR                   (*(volatile uint32_t *)(RCC_BASE_REG + 0x00))
#define RCC_CFGR                 (*(volatile uint32_t *)(RCC_BASE_REG + 0x08))
#define RCC_PLLCFGR              (*(volatile uint32_t *)(RCC_BASE_REG + 0x0C))
#define RCC_AHB1ENR               (*(volatile uint32_t *)(RCC_BASE_REG + 0x48))
#define RCC_AHB2ENR               (*(volatile uint32_t *)(RCC_BASE_REG + 0x4C))
#define RCC_APB1ENR1             (*(volatile uint32_t *)(RCC_BASE_REG + 0x58))
#define RCC_APB1ENR2             (*(volatile uint32_t *)(RCC_BASE_REG + 0x5C))
#define RCC_APB2ENR              (*(volatile uint32_t *)(RCC_BASE_REG + 0x60))
#define RCC_BDCR                  (*(volatile uint32_t *)(RCC_BASE_REG + 0x90))

/* GPIO helpers */
#define GPIO_MODER(p)             (*(volatile uint32_t *)((p) + 0x00))
#define GPIO_OTYPER(p)           (*(volatile uint32_t *)((p) + 0x04))
#define GPIO_OSPEEDR(p)           (*(volatile uint32_t *)((p) + 0x08))
#define GPIO_PUPDR(p)             (*(volatile uint32_t *)((p) + 0x0C))
#define GPIO_IDR(p)               (*(volatile uint32_t *)((p) + 0x10))
#define GPIO_ODR(p)               (*(volatile uint32_t *)((p) + 0x14))
#define GPIO_BSRR(p)              (*(volatile uint32_t *)((p) + 0x18))
#define GPIO_AFRL(p)              (*(volatile uint32_t *)((p) + 0x20))
#define GPIO_AFRH(p)              (*(volatile uint32_t *)((p) + 0x24))

/* GPIO mode bits */
#define GPIO_MODE_IN              0x0
#define GPIO_MODE_OUT             0x1
#define GPIO_MODE_AF              0x2
#define GPIO_MODE_AN              0x3

/* PWR */
#define PWR_BASE_REG              (APB1PERIPH_BASE + 0x0000)
#define PWR_CR1                   (*(volatile uint32_t *)(PWR_BASE_REG + 0x00))
#define PWR_CR2                   (*(volatile uint32_t *)(PWR_BASE_REG + 0x04))
#define PWR_CR3                   (*(volatile uint32_t *)(PWR_BASE_REG + 0x08))
#define PWR_SR1                   (*(volatile uint32_t *)(PWR_BASE_REG + 0x10))

/* RTC */
#define RTC_BASE_REG              (APB1PERIPH_BASE + 0x2800)
#define RTC_TR                    (*(volatile uint32_t *)(RTC_BASE_REG + 0x00))
#define RTC_DR                     (*(volatile uint32_t *)(RTC_BASE_REG + 0x04))
#define RTC_CR                     (*(volatile uint32_t *)(RTC_BASE_REG + 0x08))
#define RTC_ISR                   (*(volatile uint32_t *)(RTC_BASE_REG + 0x0C))
#define RTC_PRER                   (*(volatile uint32_t *)(RTC_BASE_REG + 0x10))
#define RTC_ALRMAR                (*(volatile uint32_t *)(RTC_BASE_REG + 0x18))
#define RTC_ALRMASSR               (*(volatile uint32_t *)(RTC_BASE_REG + 0x1C))
#define RTC_WUTR                   (*(volatile uint32_t *)(RTC_BASE_REG + 0x24))
#define RTC_SCR                     (*(volatile uint32_t *)(RTC_BASE_REG + 0x5C))

/* EXTI */
#define EXTI_BASE_REG             (APB2PERIPH_BASE + 0x0000)
#define EXTI_IMR1                 (*(volatile uint32_t *)(EXTI_BASE_REG + 0x00))
#define EXTI_EMR1                 (*(volatile uint32_t *)(EXTI_BASE_REG + 0x04))
#define EXTI_RTSR1                 (*(volatile uint32_t *)(EXTI_BASE_REG + 0x08))
#define EXTI_FTSR1                 (*(volatile uint32_t *)(EXTI_BASE_REG + 0x0C))
#define EXTI_SWIER1                (*(volatile uint32_t *)(EXTI_BASE_REG + 0x10))
#define EXTI_PR1                   (*(volatile uint32_t *)(EXTI_BASE_REG + 0x14))

/* TIM1 (magnetoelastic coil drive) */
#define TIM1_BASE_REG              (APB2PERIPH_BASE + 0x0000)
#define TIM1_CR1                   (*(volatile uint32_t *)(TIM1_BASE_REG + 0x00))
#define TIM1_DIER                  (*(volatile uint32_t *)(TIM1_BASE_REG + 0x0C))
#define TIM1_SR                    (*(volatile uint32_t *)(TIM1_BASE_REG + 0x10))
#define TIM1_CCMR1                 (*(volatile uint32_t *)(TIM1_BASE_REG + 0x18))
#define TIM1_CCER                  (*(volatile uint32_t *)(TIM1_BASE_REG + 0x20))
#define TIM1_CCR1                  (*(volatile uint32_t *)(TIM1_BASE_REG + 0x34))
#define TIM1_CCR2                  (*(volatile uint32_t *)(TIM1_BASE_REG + 0x38))
#define TIM1_ARR                    (*(volatile uint32_t *)(TIM1_BASE_REG + 0x2C))
#define TIM1_PSC                    (*(volatile uint32_t *)(TIM1_BASE_REG + 0x28))

/* ADC1 */
#define ADC1_BASE_REG              (AHB2PERIPH_BASE + 0x0800)
#define ADC_ISR                   (*(volatile uint32_t *)(ADC1_BASE_REG + 0x00))
#define ADC_IER                    (*(volatile uint32_t *)(ADC1_BASE_REG + 0x04))
#define ADC_CR                     (*(volatile uint32_t *)(ADC1_BASE_REG + 0x08))
#define ADC_CFGR                   (*(volatile uint32_t *)(ADC1_BASE_REG + 0x0C))
#define ADC_SQR1                   (*(volatile uint32_t *)(ADC1_BASE_REG + 0x30))
#define ADC_SQR2                    (*(volatile uint32_t *)(ADC1_BASE_REG + 0x34))
#define ADC_DR                     (*(volatile uint32_t *)(ADC1_BASE_REG + 0x40))

/* SPI1 (LoRa) / SPI2 (flash) / SPI3 (accel) share a common layout */
#define SPI_CR1(p)                  (*(volatile uint32_t *)((p) + 0x00))
#define SPI_CR2(p)                  (*(volatile uint32_t *)((p) + 0x04))
#define SPI_SR(p)                   (*(volatile uint32_t *)((p) + 0x08))
#define SPI_DR(p)                   (*(volatile uint32_t *)((p) + 0x0C))

#define SPI1_BASE_REG              (APB2PERIPH_BASE + 0x3000)
#define SPI2_BASE_REG              (APB1PERIPH_BASE + 0x3800)
#define SPI3_BASE_REG              (APB1PERIPH_BASE + 0x3C00)

/* I2C1 (TMP117, MAX17055) */
#define I2C1_BASE_REG              (APB1PERIPH_BASE + 0x5400)
#define I2C_CR1(p)                  (*(volatile uint32_t *)((p) + 0x00))
#define I2C_CR2(p)                  (*(volatile uint32_t *)((p) + 0x04))
#define I2C_ISR(p)                  (*(volatile uint32_t *)((p) + 0x18))
#define I2C_TXDR(p)                (*(volatile uint32_t *)((p) + 0x28))
#define I2C_RXDR(p)                (*(volatile uint32_t *)((p) + 0x2C))

/* DMA */
#define DMA1_BASE_REG              (AHB1PERIPH_BASE + 0x0000)

/* Flash option / calibration area */
#define FLASH_BASE_REG             (0x40022000UL)
#define FLASH_KEYR                 (*(volatile uint32_t *)(FLASH_BASE_REG + 0x08))
#define FLASH_SR                   (*(volatile uint32_t *)(FLASH_BASE_REG + 0x10))
#define FLASH_CR                    (*(volatile uint32_t *)(FLASH_BASE_REG + 0x14))

/* CMP / OPAMP */
#define COMP1_BASE_REG              (0x40010200UL)
#define OPAMP1_CSR                 (*(volatile uint32_t *)(0x40003000UL))

/* IRQ numbers (subset) */
#define IRQ_EXTI0                   6
#define IRQ_EXTI1                   7
#define IRQ_RTCALARM                41
#define IRQ_DMA1_CH1                11
#define IRQ_TIM1_UP                16

/* helper: bit */
#define BIT(n)                      (1UL << (n))

#endif /* TENSILGUARD_REGISTERS_H */