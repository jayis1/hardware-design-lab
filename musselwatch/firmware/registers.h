/*
 * registers.h — MusselWatch hardware register / peripheral map
 *
 * MusselWatch: Open Bivalve Valvometric Biosensor Network
 * A solar-powered LoRaWAN biosensor that monitors freshwater-mussel
 * shell-gape (valve opening) activity via a Hall-effect sensor array as a
 * biological early-warning system for water contamination.
 *
 * MCU: STM32L432KC (Ultra-low-power Cortex-M4, 80 MHz, 256 KB flash)
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#ifndef MUSSelWATCH_REGISTERS_H
#define MUSSelWATCH_REGISTERS_H

#include <stdint.h>

/* ---- ARM Cortex-M4 system control registers --------------------------- */
#define SCB_VTOR        (*(volatile uint32_t *)0xE000ED08u)
#define SCB_SCR_SLEEPDEEP_Msk  (1u << 2)
#define NVIC_STIR       (*(volatile uint32_t *)0xE000EF00u)

/* ---- SysTick --------------------------------------------------------- */
#define SYST_CSR        (*(volatile uint32_t *)0xE000E010u)
#define SYST_RVR        (*(volatile uint32_t *)0xE000E014u)
#define SYST_CVR        (*(volatile uint32_t *)0xE000E018u)
#define SYSTICK_CLKSOURCE_Msk (1u << 2)
#define SYSTICK_ENABLE_Msk   (1u << 0)
#define SYSTICK_TICKINT_Msk  (1u << 1)

/* ---- RCC (Reset & Clock Control) STM32L4 base 0x40021000 ------------- */
#define RCC_BASE        0x40021000u
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00u))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x08u))
#define RCC_PLLCFGR     (*(volatile uint32_t *)(RCC_BASE + 0x0Cu))
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x48u))
#define RCC_AHB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x4Cu))
#define RCC_APB1ENR1    (*(volatile uint32_t *)(RCC_BASE + 0x58u))
#define RCC_APB1ENR2    (*(volatile uint32_t *)(RCC_BASE + 0x5Cu))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x60u))

#define RCC_AHB1ENR_DMA1EN  (1u << 0)
#define RCC_AHB2ENR_GPIOAEN  (1u << 0)
#define RCC_AHB2ENR_GPIOBEN  (1u << 1)
#define RCC_AHB2ENR_GPIOCEN  (1u << 2)
#define RCC_AHB2ENR_ADCEN   (1u << 13)
#define RCC_APB1ENR1_TIM2EN (1u << 0)
#define RCC_APB1ENR1_I2C1EN (1u << 21)
#define RCC_APB1ENR1_LPTIM1EN (1u << 31)
#define RCC_APB1ENR2_LPUART1EN (1u << 0)
#define RCC_APB2ENR_SPI1EN (1u << 12)
#define RCC_APB2ENR_USART1EN (1u << 14)
#define RCC_APB2ENR_TIM16EN (1u << 17)

/* ---- Flash latency (for 80 MHz) -------------------------------------- */
#define FLASH_ACR       (*(volatile uint32_t *)0x40022000u)
#define FLASH_ACR_LATENCY_4WS  4u
#define FLASH_ACR_PRFTEN      (1u << 8)

/* ---- PWR (Power Control) -------------------------------------------- */
#define PWR_BASE        0x40007000u
#define PWR_CR1         (*(volatile uint32_t *)(PWR_BASE + 0x00u))
#define PWR_CR3         (*(volatile uint32_t *)(PWR_BASE + 0x08u))
#define PWR_SR1         (*(volatile uint32_t *)(PWR_BASE + 0x10u))
#define PWR_CR1_VOS_MASK     (0x3u << 9)
#define PWR_CR1_VOS_RANGE1   (1u << 9)   /* 1.2 V range, up to 80 MHz */

/* ---- GPIO base addresses ------------------------------------------- */
#define GPIOA_BASE      0x48000000u
#define GPIOB_BASE      0x48000400u
#define GPIOC_BASE      0x48000800u
#define GPIOH_BASE      0x48001C00u

#define GPIO_MODER(g)   (*(volatile uint32_t *)((g) + 0x00u))
#define GPIO_OTYPER(g)  (*(volatile uint32_t *)((g) + 0x04u))
#define GPIO_OSPEEDR(g) (*(volatile uint32_t *)((g) + 0x08u))
#define GPIO_PUPDR(g)   (*(volatile uint32_t *)((g) + 0x0Cu))
#define GPIO_IDR(g)     (*(volatile uint32_t *)((g) + 0x10u))
#define GPIO_ODR(g)     (*(volatile uint32_t *)((g) + 0x14u))
#define GPIO_BSRR(g)    (*(volatile uint32_t *)((g) + 0x18u))
#define GPIO_AFRL(g)    (*(volatile uint32_t *)((g) + 0x20u))
#define GPIO_AFRH(g)    (*(volatile uint32_t *)((g) + 0x24u))
#define GPIO_AFRL_AFR_MASK 0xFu

#define GPIO_MODE_INPUT   0u
#define GPIO_MODE_OUTPUT  1u
#define GPIO_MODE_AF      2u
#define GPIO_MODE_ANALOG  3u

/* ---- ADC (12-bit, single-ended) base 0x50040000 ---------------------- */
#define ADC1_BASE       0x50040000u
#define ADC_ISR          (*(volatile uint32_t *)(ADC1_BASE + 0x00u))
#define ADC_IER          (*(volatile uint32_t *)(ADC1_BASE + 0x04u))
#define ADC_CR           (*(volatile uint32_t *)(ADC1_BASE + 0x08u))
#define ADC_CFGR         (*(volatile uint32_t *)(ADC1_BASE + 0x0Cu))
#define ADC_SQR1         (*(volatile uint32_t *)(ADC1_BASE + 0x30u))
#define ADC_DR           (*(volatile uint32_t *)(ADC1_BASE + 0x40u))
#define ADC_SMPR1        (*(volatile uint32_t *)(ADC1_BASE + 0x14u))
#define ADC_SMPR2        (*(volatile uint32_t *)(ADC1_BASE + 0x18u))
#define ADC_CCR          (*(volatile uint32_t *)(ADC1_BASE + 0x300u+0x04u)) /* ADC common */

#define ADC_CR_ADVREG_EN (1u << 28)
#define ADC_CR_ADCAL    (1u << 31)
#define ADC_CR_ADCAL_BIT (1u << 31)
#define ADC_CR_ADEN     (1u << 0)
#define ADC_CR_ADSTART  (1u << 2)
#define ADC_ISR_ADRDY   (1u << 0)
#define ADC_IER_ADRDYIE (1u << 0)

/* ---- LPTIM1 (low-power timer, 32 kHz tick) base 0x40007800 ----------- */
#define LPTIM1_BASE     0x40007800u
#define LPTIM_ISR       (*(volatile uint32_t *)(LPTIM1_BASE + 0x00u))
#define LPTIM_ICR       (*(volatile uint32_t *)(LPTIM1_BASE + 0x04u))
#define LPTIM_CFGR      (*(volatile uint32_t *)(LPTIM1_BASE + 0x08u))
#define LPTIM_CR        (*(volatile uint32_t *)(LPTIM1_BASE + 0x0Cu))
#define LPTIM_CMP       (*(volatile uint32_t *)(LPTIM1_BASE + 0x14u))
#define LPTIM_ARR       (*(volatile uint32_t *)(LPTIM1_BASE + 0x18u))
#define LPTIM_CNT       (*(volatile uint32_t *)(LPTIM1_BASE + 0x1Cu))
#define LPTIM_CR_ENABLE  (1u << 0)
#define LPTIM_CFGR_CKSEL_LSI (0u << 0)
#define LPTIM_CMPM       (1u << 0)

/* ---- USART1 (LoRa host console / debug) base 0x40013800 ------------ */
#define USART1_BASE     0x40013800u
#define USART1_CR1      (*(volatile uint32_t *)(USART1_BASE + 0x00u))
#define USART1_CR2      (*(volatile uint32_t *)(USART1_BASE + 0x04u))
#define USART1_BRR      (*(volatile uint32_t *)(USART1_BASE + 0x0Cu))
#define USART1_ISR      (*(volatile uint32_t *)(USART1_BASE + 0x1Cu))
#define USART1_TDR      (*(volatile uint32_t *)(USART1_BASE + 0x28u))
#define USART1_RDR      (*(volatile uint32_t *)(USART1_BASE + 0x24u))
#define USART_CR1_UE    (1u << 0)
#define USART_CR1_TE    (1u << 3)
#define USART_CR1_RE    (1u << 2)
#define USART_ISR_TXE   (1u << 7)
#define USART_ISR_RXNE  (1u << 5)

/* ---- SPI1 (SX1262 LoRa radio) base 0x40013000 ---------------------- */
#define SPI1_BASE       0x40013000u
#define SPI1_CR1         (*(volatile uint32_t *)(SPI1_BASE + 0x00u))
#define SPI1_CR2         (*(volatile uint32_t *)(SPI1_BASE + 0x04u))
#define SPI1_SR          (*(volatile uint32_t *)(SPI1_BASE + 0x08u))
#define SPI1_DR          (*(volatile uint32_t *)(SPI1_BASE + 0x0Cu))
#define SPI_CR1_SPE     (1u << 6)
#define SPI_CR1_MSTR    (1u << 2)
#define SPI_CR1_BR_DIV8 (2u << 3)
#define SPI_SR_TXE      (1u << 1)
#define SPI_SR_RXNE     (1u << 0)
#define SPI_SR_BSY      (1u << 7)

/* ---- I2C1 (PMIC + FRAM) base 0x40005400 ---------------------------- */
#define I2C1_BASE       0x40005400u
#define I2C1_CR1         (*(volatile uint32_t *)(I2C1_BASE + 0x00u))
#define I2C1_CR2         (*(volatile uint32_t *)(I2C1_BASE + 0x04u))
#define I2C1_ISR         (*(volatile uint32_t *)(I2C1_BASE + 0x10u))
#define I2C1_TXDR        (*(volatile uint32_t *)(I2C1_BASE + 0x28u))
#define I2C1_RXDR        (*(volatile uint32_t *)(I2C1_BASE + 0x24u))
#define I2C1_TIMINGR     (*(volatile uint32_t *)(I2C1_BASE + 0x08u))
#define I2C_CR1_PE       (1u << 0)
#define I2C_CR2_START    (1u << 13)
#define I2C_CR2_STOP     (1u << 14)
#define I2C_CR2_AUTOEND  (1u << 25)
#define I2C_CR2_RD_WRN   (1u << 10)
#define I2C_ISR_TXIS     (1u << 1)
#define I2C_ISR_RXNE     (1u << 2)
#define I2C_ISR_TC       (1u << 6)
#define I2C_ISR_BUSY     (1u << 5)
#define I2C_ISR_NACKF    (1u << 4)

/* ---- TIM16 (one-wire timing / debug PWM) base 0x40014400 ----------- */
#define TIM16_BASE      0x40014400u
#define TIM16_CR1       (*(volatile uint32_t *)(TIM16_BASE + 0x00u))
#define TIM16_CNT       (*(volatile uint32_t *)(TIM16_BASE + 0x24u))
#define TIM16_PSC       (*(volatile uint32_t *)(TIM16_BASE + 0x28u))
#define TIM16_ARR       (*(volatile uint32_t *)(TIM16_BASE + 0x2Cu))
#define TIM16_CCR1      (*(volatile uint32_t *)(TIM16_BASE + 0x34u))
#define TIM16_CR1_CEN   (1u << 0)

/* ---- External interrupt (EXTI) ------------------------------------- */
#define EXTI_BASE       0x40010420u
#define EXTI_IMR1       (*(volatile uint32_t *)(EXTI_BASE + 0x00u))
#define EXTI_RTSR1      (*(volatile uint32_t *)(EXTI_BASE + 0x08u))
#define EXTI_FTSR1      (*(volatile uint32_t *)(EXTI_BASE + 0x0Cu))
#define EXTI_PR1        (*(volatile uint32_t *)(EXTI_BASE + 0x14u))

/* ---- Unique device ID (96-bit factory) ---------------------------- */
#define UID64_BASE      0x1FFF7590u

#endif /* MUSSelWATCH_REGISTERS_H */