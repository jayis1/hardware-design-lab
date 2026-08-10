/**
 * @file    registers.h
 * @brief   TideBand — MMIO register definitions, peripheral base addresses,
 *          IRQ priorities, and hardware constants for STM32H733VGT6.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TIDEBAND_REGISTERS_H
#define TIDEBAND_REGISTERS_H

#include <stdint.h>

/* ---- Cortex-M7 system registers ---- */
#define SCB_BASE            0xE000ED00u
#define SCB_VTOR            (*(volatile uint32_t *)(SCB_BASE + 0x08u))
#define SCB_AIRCR           (*(volatile uint32_t *)(SCB_BASE + 0x0Cu))
#define SCB_SCR             (*(volatile uint32_t *)(SCB_BASE + 0x10u))
#define NVIC_ISER0          (*(volatile uint32_t *)(0xE000E100u))
#define NVIC_ICER0          (*(volatile uint32_t *)(0xE000E180u))
#define NVIC_IPR_BASE       (0xE000E400u)

/* ---- RCC (Reset and Clock Control) ---- */
#define RCC_BASE            0x58024400u
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00u))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10u))
#define RCC_PLLCFGR          (*(volatile uint32_t *)(RCC_BASE + 0x14u))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x48u))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x4Cu))
#define RCC_AHB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0x50u))
#define RCC_APB1ENR1        (*(volatile uint32_t *)(RCC_BASE + 0x58u))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x60u))
#define RCC_AHB1LPENR       (*(volatile uint32_t *)(RCC_BASE + 0x70u))

/* RCC bit definitions */
#define RCC_CR_HSION        (1u << 8)
#define RCC_CR_HSIRDY       (1u << 10)
#define RCC_CR_HSEON        (1u << 16)
#define RCC_CR_HSERDY       (1u << 17)
#define RCC_CR_PLL1ON       (1u << 24)
#define RCC_CR_PLL1RDY      (1u << 25)

#define RCC_AHB1ENR_GPIOA   (1u << 0)
#define RCC_AHB1ENR_GPIOB   (1u << 1)
#define RCC_AHB1ENR_GPIOC   (1u << 2)
#define RCC_AHB1ENR_GPIOD   (1u << 3)
#define RCC_AHB1ENR_GPIOE   (1u << 4)
#define RCC_AHB1ENR_GPIOH   (1u << 7)
#define RCC_AHB1ENR_DMA1    (1u << 0)  /* in AHB1 for H7 — actually DMA1EN bit in RCC_AHB1ENR */
#define RCC_AHB1ENR_DMA2    (1u << 1)

#define RCC_APB1ENR1_TIM2   (1u << 0)
#define RCC_APB1ENR1_I2C1   (1u << 21)
#define RCC_APB1ENR1_I2C2   (1u << 22)
#define RCC_APB1ENR1_USART3 (1u << 18)
#define RCC_APB1ENR1_RTCAPB (1u << 10)

#define RCC_APB2ENR_TIM1    (1u << 0)
#define RCC_APB2ENR_SPI1    (1u << 12)
#define RCC_APB2ENR_SPI4    (1u << 13)
#define RCC_APB2ENR_USART1  (1u << 14)

/* ---- Power Controller (PWR) ---- */
#define PWR_BASE            0x58024800u
#define PWR_CR1             (*(volatile uint32_t *)(PWR_BASE + 0x00u))
#define PWR_CR3             (*(volatile uint32_t *)(PWR_BASE + 0x08u))
#define PWR_SR1             (*(volatile uint32_t *)(PWR_BASE + 0x10u))
#define PWR_CPUCR           (*(volatile uint32_t *)(0xE000EDF0u))

#define PWR_CR1_VOS_SHIFT   9
#define PWR_CR1_VOS_MASK    (3u << 9)
#define PWR_CR1_VOS_0       0u
#define PWR_CR1_VOS_1       1u
#define PWR_CR1_VOS_2       2u
#define PWR_CR1_VOS_3       3u  /* Maximum voltage scaling */

/* ---- GPIO ---- */
#define GPIOA_BASE          0x58020000u
#define GPIOB_BASE          0x58020400u
#define GPIOC_BASE          0x58020800u
#define GPIOD_BASE          0x58020C00u
#define GPIOE_BASE          0x58021000u
#define GPIOH_BASE          0x58021C00u

#define GPIO_MODER(g)       (*(volatile uint32_t *)((g) + 0x00u))
#define GPIO_OTYPER(g)      (*(volatile uint32_t *)((g) + 0x04u))
#define GPIO_OSPEEDR(g)     (*(volatile uint32_t *)((g) + 0x08u))
#define GPIO_PUPDR(g)       (*(volatile uint32_t *)((g) + 0x0Cu))
#define GPIO_IDR(g)         (*(volatile uint32_t *)((g) + 0x10u))
#define GPIO_ODR(g)         (*(volatile uint32_t *)((g) + 0x14u))
#define GPIO_BSRR(g)        (*(volatile uint32_t *)((g) + 0x18u))
#define GPIO_AFRL(g)        (*(volatile uint32_t *)((g) + 0x20u))
#define GPIO_AFRH(g)        (*(volatile uint32_t *)((g) + 0x24u))

#define GPIO_MODE_INPUT      0u
#define GPIO_MODE_OUTPUT     1u
#define GPIO_MODE_AF         2u
#define GPIO_MODE_ANALOG     3u

#define GPIO_SPEED_LOW       0u
#define GPIO_SPEED_MED       1u
#define GPIO_SPEED_HIGH      2u
#define GPIO_SPEED_VHIGH     3u

#define GPIO_PUPD_NONE       0u
#define GPIO_PUPD_PU         1u
#define GPIO_PUPD_PD         2u

/* ---- Timer (TIM1 — Doppler TX PWM) ---- */
#define TIM1_BASE           0x40010000u
#define TIM1_CR1            (*(volatile uint32_t *)(TIM1_BASE + 0x00u))
#define TIM1_CR2            (*(volatile uint32_t *)(TIM1_BASE + 0x04u))
#define TIM1_SMCR           (*(volatile uint32_t *)(TIM1_BASE + 0x08u))
#define TIM1_DIER           (*(volatile uint32_t *)(TIM1_BASE + 0x0Cu))
#define TIM1_SR             (*(volatile uint32_t *)(TIM1_BASE + 0x10u))
#define TIM1_EGR            (*(volatile uint32_t *)(TIM1_BASE + 0x14u))
#define TIM1_CCMR1          (*(volatile uint32_t *)(TIM1_BASE + 0x18u))
#define TIM1_CCER           (*(volatile uint32_t *)(TIM1_BASE + 0x20u))
#define TIM1_CNT            (*(volatile uint32_t *)(TIM1_BASE + 0x24u))
#define TIM1_PSC            (*(volatile uint32_t *)(TIM1_BASE + 0x28u))
#define TIM1_ARR            (*(volatile uint32_t *)(TIM1_BASE + 0x2Cu))
#define TIM1_CCR1           (*(volatile uint32_t *)(TIM1_BASE + 0x34u))
#define TIM1_BDTR           (*(volatile uint32_t *)(TIM1_BASE + 0x44u))

#define TIM_CR1_CEN         (1u << 0)
#define TIM_CR1_ARPE        (1u << 7)
#define TIM_DIER_UIE        (1u << 0)
#define TIM_DIER_CC1IE      (1u << 1)
#define TIM_SR_UIF          (1u << 0)
#define TIM_CCMR1_OC1M_PWM1 (6u << 4)
#define TIM_CCMR1_OC1PE     (1u << 3)
#define TIM_CCER_CC1E       (1u << 0)
#define TIM_BDTR_MOE        (1u << 15)

/* ---- Timer (TIM2 — Haptic PWM) ---- */
#define TIM2_BASE           0x40000000u
#define TIM2_CR1            (*(volatile uint32_t *)(TIM2_BASE + 0x00u))
#define TIM2_DIER           (*(volatile uint32_t *)(TIM2_BASE + 0x0Cu))
#define TIM2_SR             (*(volatile uint32_t *)(TIM2_BASE + 0x10u))
#define TIM2_CCMR1          (*(volatile uint32_t *)(TIM2_BASE + 0x18u))
#define TIM2_CCER           (*(volatile uint32_t *)(TIM2_BASE + 0x20u))
#define TIM2_CNT            (*(volatile uint32_t *)(TIM2_BASE + 0x24u))
#define TIM2_PSC            (*(volatile uint32_t *)(TIM2_BASE + 0x28u))
#define TIM2_ARR            (*(volatile uint32_t *)(TIM2_BASE + 0x2Cu))
#define TIM2_CCR1           (*(volatile uint32_t *)(TIM2_BASE + 0x34u))

/* ---- SPI1 (Doppler ADC + IMU) ---- */
#define SPI1_BASE           0x40013000u
#define SPI1_CR1            (*(volatile uint32_t *)(SPI1_BASE + 0x00u))
#define SPI1_CR2            (*(volatile uint32_t *)(SPI1_BASE + 0x04u))
#define SPI1_SR             (*(volatile uint32_t *)(SPI1_BASE + 0x08u))
#define SPI1_DR             (*(volatile uint32_t *)(SPI1_BASE + 0x0Cu))
#define SPI1_CFG1           (*(volatile uint32_t *)(SPI1_BASE + 0x00u))
#define SPI1_CFG2           (*(volatile uint32_t *)(SPI1_BASE + 0x04u))
#define SPI1_IER            (*(volatile uint32_t *)(SPI1_BASE + 0x0Cu))

#define SPI_CFG1_MBR_SHIFT  28
#define SPI_CFG1_CRCSIZE_8  (7u << 16)
#define SPI_CFG1_FTHLV_1    (0u << 5)
#define SPI_CFG1_DSIZE_16   (15u << 0)
#define SPI_CFG2_MASTER     (1u << 22)
#define SPI_CFG2_SSOE       (1u << 14)
#define SPI_CFG2_CPOL       (1u << 1)
#define SPI_CFG2_CPHA       (1u << 0)
#define SPI_CR1_SPE         (1u << 0)
#define SPI_CR1_CSTART      (1u << 9)
#define SPI_SR_RXP          (1u << 0)
#define SPI_SR_TXP          (1u << 1)
#define SPI_SR_EOT          (1u << 3)
#define SPI_SR_OVR          (1u << 6)

/* ---- SPI4 (NAND flash + LCD) ---- */
#define SPI4_BASE           0x40013400u
#define SPI4_CR1            (*(volatile uint32_t *)(SPI4_BASE + 0x00u))
#define SPI4_CR2            (*(volatile uint32_t *)(SPI4_BASE + 0x04u))
#define SPI4_SR             (*(volatile uint32_t *)(SPI4_BASE + 0x08u))
#define SPI4_DR             (*(volatile uint32_t *)(SPI4_BASE + 0x0Cu))

/* ---- I2C1 (MS5837 pressure, MAX17055 fuel gauge, RTC) ---- */
#define I2C1_BASE           0x40005400u
#define I2C1_CR1            (*(volatile uint32_t *)(I2C1_BASE + 0x00u))
#define I2C1_CR2            (*(volatile uint32_t *)(I2C1_BASE + 0x04u))
#define I2C1_OAR1           (*(volatile uint32_t *)(I2C1_BASE + 0x08u))
#define I2C1_TIMINGR        (*(volatile uint32_t *)(I2C1_BASE + 0x10u))
#define I2C1_ISR            (*(volatile uint32_t *)(I2C1_BASE + 0x18u))
#define I2C1_ICR            (*(volatile uint32_t *)(I2C1_BASE + 0x1Cu))
#define I2C1_PECR           (*(volatile uint32_t *)(I2C1_BASE + 0x20u))
#define I2C1_RXDR           (*(volatile uint32_t *)(I2C1_BASE + 0x24u))
#define I2C1_TXDR           (*(volatile uint32_t *)(I2C1_BASE + 0x28u))

#define I2C_CR1_PE          (1u << 0)
#define I2C_CR1_TXIE        (1u << 1)
#define I2C_CR1_RXIE        (1u << 2)
#define I2C_CR2_START       (1u << 13)
#define I2C_CR2_STOP        (1u << 14)
#define I2C_CR2_NACKSTOP    (3u << 13)
#define I2C_ISR_TXE         (1u << 0)
#define I2C_ISR_RXNE        (1u << 2)
#define I2C_ISR_TC          (1u << 6)
#define I2C_ISR_NACKF       (1u << 12)
#define I2C_ISR_BUSY        (1u << 15)

/* ---- I2C2 (MMC5983 magnetometer) ---- */
#define I2C2_BASE           0x40005800u
#define I2C2_CR1            (*(volatile uint32_t *)(I2C2_BASE + 0x00u))
#define I2C2_CR2            (*(volatile uint32_t *)(I2C2_BASE + 0x04u))
#define I2C2_TIMINGR        (*(volatile uint32_t *)(I2C2_BASE + 0x10u))
#define I2C2_ISR            (*(volatile uint32_t *)(I2C2_BASE + 0x18u))
#define I2C2_ICR            (*(volatile uint32_t *)(I2C2_BASE + 0x1Cu))
#define I2C2_RXDR           (*(volatile uint32_t *)(I2C2_BASE + 0x24u))
#define I2C2_TXDR           (*(volatile uint32_t *)(I2C2_BASE + 0x28u))

/* ---- USART1 (BLE module nRF52840) ---- */
#define USART1_BASE         0x40011000u
#define USART1_CR1          (*(volatile uint32_t *)(USART1_BASE + 0x00u))
#define USART1_CR2          (*(volatile uint32_t *)(USART1_BASE + 0x04u))
#define USART1_CR3          (*(volatile uint32_t *)(USART1_BASE + 0x08u))
#define USART1_BRR          (*(volatile uint32_t *)(USART1_BASE + 0x0Cu))
#define USART1_RQR          (*(volatile uint32_t *)(USART1_BASE + 0x18u))
#define USART1_ISR          (*(volatile uint32_t *)(USART1_BASE + 0x1Cu))
#define USART1_ICR          (*(volatile uint32_t *)(USART1_BASE + 0x20u))
#define USART1_RDR          (*(volatile uint32_t *)(USART1_BASE + 0x24u))
#define USART1_TDR          (*(volatile uint32_t *)(USART1_BASE + 0x28u))

#define USART_CR1_UE        (1u << 0)
#define USART_CR1_TE        (1u << 3)
#define USART_CR1_RE        (1u << 2)
#define USART_CR1_RXNEIE    (1u << 5)
#define USART_CR1_TXEIE     (1u << 7)
#define USART_ISR_RXNE      (1u << 5)
#define USART_ISR_TXE       (1u << 7)
#define USART_ISR_TC        (1u << 6)
#define USART_ISR_BUSY      (1u << 16)

/* ---- DMA2 (for ADC and SPI streaming) ---- */
#define DMA2_BASE           0x60020000u
#define DMA2_STREAM0_BASE   (DMA2_BASE + 0x10u)
#define DMA2_STREAM1_BASE   (DMA2_BASE + 0x28u)
#define DMA2_STREAM2_BASE   (DMA2_BASE + 0x40u)
#define DMA2_STREAM3_BASE   (DMA2_BASE + 0x58u)

#define DMA_S_PAR(s)        (*(volatile uint32_t *)((s) + 0x00u))
#define DMA_S_M0AR(s)       (*(volatile uint32_t *)((s) + 0x04u))
#define DMA_S_M1AR(s)       (*(volatile uint32_t *)((s) + 0x08u))
#define DMA_S_NDT(s)        (*(volatile uint16_t *)((s) + 0x0Cu))
#define DMA_S_CR(s)         (*(volatile uint32_t *)((s) + 0x10u))
#define DMA_S_FCR(s)        (*(volatile uint32_t *)((s) + 0x14u))

#define DMA_CR_EN           (1u << 0)
#define DMA_CR_DMEIE        (1u << 2)
#define DMA_CR_TEIE         (1u << 3)
#define DMA_CR_HTIE         (1u << 3)
#define DMA_CR_TCIE         (1u << 4)
#define DMA_CR_DBM          (1u << 5) /* CH bit in H7 */
#define DMA_CR_CT           (1u << 6) /* TCIE in different position — see H7 RM */
#define DMA_CR_MINC         (1u << 10)
#define DMA_CR_PINC         (1u << 9)
#define DMA_CR_PSIZE_16     (1u << 8)
#define DMA_CR_MSIZE_16     (1u << 13)
#define DMA_CR_PSIZE_32     (2u << 8)
#define DMA_CR_MSIZE_32     (2u << 13)
#define DMA_CR_CIRC         (1u << 8)
#define DMA_CR_DIR_P2M      (0u << 6)
#define DMA_CR_DIR_M2P      (1u << 6)
#define DMA_CR_DIR_M2M      (2u << 6)
#define DMA_CR_PL_VHIGH     (3u << 16)
#define DMA_CR_PL_HIGH      (2u << 16)
#define DMA_CR_CHSEL_SHIFT  16  /* In H7, channel sel is in DMAMUX */

/* ---- DMA interrupt clear register ---- */
#define DMA2_LIFCR          (*(volatile uint32_t *)(DMA2_BASE + 0x08u))
#define DMA2_HIFCR          (*(volatile uint32_t *)(DMA2_BASE + 0x0Cu))

/* ---- DMAMUX ---- */
#define DMAMUX1_BASE        0x60020800u
#define DMAMUX_CxCR(x)      (*(volatile uint32_t *)(DMAMUX1_BASE + (x) * 4u))

/* ---- Flash controller (for calibration storage) ---- */
#define FLASH_BASE          0x52002000u
#define FLASH_KEYR          (*(volatile uint32_t *)(FLASH_BASE + 0x08u))
#define FLASH_SR            (*(volatile uint32_t *)(FLASH_BASE + 0x10u))
#define FLASH_CR            (*(volatile uint32_t *)(FLASH_BASE + 0x14u))
#define FLASH_OPT_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x0Cu))

/* ---- NVIC IRQ numbers (STM32H733) ---- */
#define IRQ_DMA2_STREAM0    56u
#define IRQ_DMA2_STREAM1    57u
#define IRQ_DMA2_STREAM2    58u
#define IRQ_DMA2_STREAM3    59u
#define IRQ_SPI1            60u
#define IRQ_SPI4            61u
#define IRQ_USART1          37u
#define IRQ_I2C1_EV         31u
#define IRQ_I2C1_ER         32u
#define IRQ_I2C2_EV         33u
#define IRQ_I2C2_ER         34u
#define IRQ_TIM1_UP         25u
#define IRQ_TIM2            28u
#define IRQ_EXTI0           6u

/* ---- IRQ Priority levels ---- */
#define IRQ_PRIO_DOPPLER_DMA    0u  /* Highest — must not miss ADC samples */
#define IRQ_PRIO_DOPPLER_SPI    1u
#define IRQ_PRIO_BLE_UART       2u
#define IRQ_PRIO_I2C            3u
#define IRQ_PRIO_DISPLAY        4u
#define IRQ_PRIO_HAPTIC         5u
#define IRQ_PRIO_NAND           6u
#define IRQ_PRIO_NONE           7u  /* No preemption */

/* ---- Doppler system constants ---- */
#define DOPPLER_TX_FREQ_HZ      1000000u   /* 1 MHz transmit */
#define DOPPLER_ADC_RATE_HZ     20000u     /* 20 kSPS baseband */
#define DOPPLER_FFT_SIZE        4096u      /* 4096-point FFT */
#define DOPPLER_NUM_RX_CH       3u         /* Three receiver channels */
#define DOPPLER_SOUND_SPEED     1480.0f    /* m/s in water at ~20C */
#define DOPPLER_RX_ANGLE_DEG    30.0f      /* Receiver angle from normal */
#define DOPPLER_RX_ANGLE_RAD    0.5235988f /* 30 deg in radians */

/* ---- Depth and dive detection ---- */
#define DIVE_IMMERSION_DEPTH_M  0.5f       /* Depth > 0.5m = diving */
#define DIVE_SURFACE_TIMEOUT_S  300u       /* 5 min at surface ends dive */

/* ---- Storage ---- */
#define NAND_PAGE_SIZE          2048u
#define NAND_PAGES_PER_BLOCK    64u
#define NAND_BLOCK_SIZE         (NAND_PAGE_SIZE * NAND_PAGES_PER_BLOCK)
#define NAND_TOTAL_BLOCKS       2048u      /* 2Gbit = 256MB */
#define NAND_RECORD_SIZE        48u
#define NAND_RECORDS_PER_PAGE   (NAND_PAGE_SIZE / NAND_RECORD_SIZE)

/* ---- Calibration flash sector ---- */
#define CAL_FLASH_SECTOR        127u       /* Last 128KB sector */
#define CAL_FLASH_ADDR          0x0807F000u
#define CAL_MAGIC               0x54424341u  /* "TBCA" */

/* ---- BLE protocol constants ---- */
#define BLE_SYNC_BYTE           0xA5u
#define BLE_MAX_PAYLOAD         247u
#define BLE_MTU                 251u
#define BLE_RX_BUF_SIZE         512u
#define BLE_TX_BUF_SIZE         512u

#endif /* TIDEBAND_REGISTERS_H */