/*
 * registers.h — STM32H743 Peripheral Register Definitions
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * This file provides low-level register address definitions and bit-field
 * constants for the STM32H743VIT6 MCU used in FerroProbe.  It is not a
 * complete SVD replacement — it covers only the peripherals that the
 * firmware actually touches.  Register base addresses follow the
 * STM32H743 Reference Manual (RM0433, Rev 7).
 */

#ifndef FERROPROBE_REGISTERS_H
#define FERROPROBE_REGISTERS_H

#include <stdint.h>

/* ===================================================================== */
/*  Bus base addresses (STM32H7 memory map)                              */
/* ===================================================================== */

#define AHB1_BASE       0x48000000U
#define AHB2_BASE       0x48021000U
#define AHB3_BASE       0x48021900U
#define AHB4_BASE       0x58020000U
#define APB1_BASE       0x40000000U
#define APB2_BASE      0x58007000U

/* ===================================================================== */
/*  Cortex-M7 system registers                                            */
/* ===================================================================== */

#define SCB_BASE        0xE000ED00U
#define SCB_VTOR        (*(volatile uint32_t *)(SCB_BASE + 0x008U))
#define SCB_CPACR      (*(volatile uint32_t *)(SCB_BASE + 0x088U))
#define SCB_CCR        (*(volatile uint32_t *)(SCB_BASE + 0x014U))

#define SCS_BASE        0xE000E000U
#define SYSTICK_BASE    0xE000E010U
#define SYSTICK_CTRL    (*(volatile uint32_t *)(SYSTICK_BASE + 0x00U))
#define SYSTICK_LOAD    (*(volatile uint32_t *)(SYSTICK_BASE + 0x04U))
#define SYSTICK_VAL     (*(volatile uint32_t *)(SYSTICK_BASE + 0x08U))
#define SYSTICK_CALIB   (*(volatile uint32_t *)(SYSTICK_BASE + 0x0CU))

#define NVIC_ISER0      (*(volatile uint32_t *)(0xE000E100U))
#define NVIC_ICER0      (*(volatile uint32_t *)(0xE000E180U))
#define NVIC_ISPR0      (*(volatile uint32_t *)(0xE000E200U))
#define NVIC_ICPR0      (*(volatile uint32_t *)(0xE000E280U))
#define NVIC_IPR_BASE   (0xE000E400U)

/* DWT cycle counter for timing */
#define DWT_BASE        0xE0001000U
#define DWT_CTRL        (*(volatile uint32_t *)(DWT_BASE + 0x000U))
#define DWT_CYCCNT      (*(volatile uint32_t *)(DWT_BASE + 0x004U))

/* ===================================================================== */
/*  RCC (Reset and Clock Control) — RM0433 §7                            */
/* ===================================================================== */

#define RCC_BASE        0x58024400U

#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x000U))
#define RCC_HSICFGR     (*(volatile uint32_t *)(RCC_BASE + 0x004U))
#define RCC_CRRCSR      (*(volatile uint32_t *)(RCC_BASE + 0x008U))
#define RCC_PLL1CFGR    (*(volatile uint32_t *)(RCC_BASE + 0x028U))
#define RCC_PLL1FRACR   (*(volatile uint32_t *)(RCC_BASE + 0x02CU))
#define RCC_PLLCKSELR   (*(volatile uint32_t *)(RCC_BASE + 0x024U))

/* RCC CR bit fields */
#define RCC_CR_HSION        (1U << 4)
#define RCC_CR_HSIRDY        (1U << 5)
#define RCC_CR_CSION         (1U << 6)
#define RCC_CR_CSIRDY        (1U << 7)
#define RCC_CR_HSEON         (1U << 16)
#define RCC_CR_HSERDY        (1U << 17)
#define RCC_CR_HSEBYP        (1U << 18)
#define RCC_CR_PLL1ON        (1U << 24)
#define RCC_CR_PLL1RDY       (1U << 25)

/* RCC PLL1CFGR bit fields */
#define RCC_PLL1CFGR_PLL1RGE_1_2M   (0x1U << 1)
#define RCC_PLL1CFGR_PLL1VCOSEL     (1U << 1U)
#define RCC_PLL1CFGR_PLL1FRACEN     (1U << 4)
#define RCC_PLL1CFGR_PLL1REN        (1U << 16)

/* RCC clock config registers (domain-specific) */
#define RCC_D1CFGR     (*(volatile uint32_t *)(RCC_BASE + 0x08CU))
#define RCC_D2CFGR     (*(volatile uint32_t *)(RCC_BASE + 0x090U))
#define RCC_D3CFGR     (*(volatile uint32_t *)(RCC_BASE + 0x094U))

#define RCC_D1CCIPR    (*(volatile uint32_t *)(RCC_BASE + 0x0A0U))
#define RCC_D2CCIP1R   (*(volatile uint32_t *)(RCC_BASE + 0x0A4U))
#define RCC_D2CCIP2R   (*(volatile uint32_t *)(RCC_BASE + 0x0A8U))
#define RCC_D3CCIPR    (*(volatile uint32_t *)(RCC_BASE + 0x0ACU))

#define RCC_CFGR       (*(volatile uint32_t *)(RCC_BASE + 0x0BCU))

/* Peripheral enable registers */
#define RCC_AHB1ENR    (*(volatile uint32_t *)(RCC_BASE + 0x0D8U))
#define RCC_AHB2ENR    (*(volatile uint32_t *)(RCC_BASE + 0x0DCU))
#define RCC_AHB3ENR    (*(volatile uint32_t *)(RCC_BASE + 0x0E0U))
#define RCC_AHB4ENR    (*(volatile uint32_t *)(RCC_BASE + 0x0E4U))
#define RCC_APB1LENR   (*(volatile uint32_t *)(RCC_BASE + 0x0E8U))
#define RCC_APB1HENR   (*(volatile uint32_t *)(RCC_BASE + 0x0ECU))
#define RCC_APB2ENR    (*(volatile uint32_t *)(RCC_BASE + 0x0F0U))
#define RCC_APB3ENR    (*(volatile uint32_t *)(RCC_BASE + 0x0F4U))
#define RCC_APB4ENR    (*(volatile uint32_t *)(RCC_BASE + 0x0F8U))

/* RCC enable bits */
#define RCC_AHB1ENR_DMA1EN       (1U << 0)
#define RCC_AHB1ENR_DMA2EN       (1U << 1)
#define RCC_AHB2ENR_ADC12EN      (1U << 5)
#define RCC_AHB3ENR_SDMMC1EN     (1U << 12)
#define RCC_AHB4ENR_GPIOAEN      (1U << 0)
#define RCC_AHB4ENR_GPIOBEN      (1U << 1)
#define RCC_AHB4ENR_GPIOCEN      (1U << 2)
#define RCC_AHB4ENR_GPIODEN      (1U << 3)
#define RCC_AHB4ENR_GPIOEEN      (1U << 4)
#define RCC_AHB4ENR_GPIOFEN      (1U << 5)
#define RCC_AHB4ENR_GPIOGEN      (1U << 6)
#define RCC_AHB4ENR_GPIOHEN      (1U << 7)
#define RCC_APB1LENR_TIM2EN      (1U << 0)
#define RCC_APB1LENR_USART3EN   (1U << 18)
#define RCC_APB1LENR_UART4EN     (1U << 19)
#define RCC_APB1HENR_SPI4EN      (1U << 4)
#define RCC_APB2ENR_TIM1EN      (1U << 0)
#define RCC_APB2ENR_USART1EN     (1U << 4)
#define RCC_APB2ENR_SPI1EN      (1U << 12)
#define RCC_APB2ENR_SPI2EN      (1U << 13)
#define RCC_APB4ENR_I2C1EN      (1U << 8)
#define RCC_APB4ENR_SYSCFGEN    (1U << 1)
#define RCC_APB4ENR_LPUART1EN   (1U << 3)

/* ===================================================================== */
/*  GPIO — RM0433 §11                                                    */
/* ===================================================================== */

#define GPIOA_BASE      0x58020000U
#define GPIOB_BASE      0x58020400U
#define GPIOC_BASE      0x58020800U
#define GPIOD_BASE      0x58020C00U
#define GPIOE_BASE      0x58021000U
#define GPIOF_BASE      0x58021400U
#define GPIOG_BASE      0x58021800U
#define GPIOH_BASE      0x58021C00U

/* GPIO register offsets (per port) */
#define GPIO_MODER      0x000U  /* Mode: 00 Input, 01 Output, 10 AF, 11 Analog */
#define GPIO_OTYPER     0x004U  /* Output type: 0 push-pull, 1 open-drain */
#define GPIO_OSPEEDR    0x008U  /* Output speed */
#define GPIO_PUPDR      0x00CU  /* Pull-up / pull-down */
#define GPIO_IDR        0x010U  /* Input data register */
#define GPIO_ODR        0x014U  /* Output data register */
#define GPIO_BSRR       0x018U  /* Atomic bit set / reset */
#define GPIO_AFRL       0x020U  /* Alternate function (pins 0–7) */
#define GPIO_AFRH       0x024U  /* Alternate function (pins 8–15) */
#define GPIO_LOCKR      0x01CU  /* Lock configuration */

#define GPIO_AF0         0x0
#define GPIO_AF1         0x1   /* TIM1/TIM2 */
#define GPIO_AF4         0x4   /* I2C */
#define GPIO_AF5         0x5   /* SPI1/SPI2 */
#define GPIO_AF6         0x6   /* SPI3 */
#define GPIO_AF7         0x7   /* USART1/2/3 */
#define GPIO_AF8         0x8   /* UART4/5, LPUART1 */

/* GPIO helper macro */
#define GPIO_REG(port, offset) (*(volatile uint32_t *)((port) + (offset)))

/* ===================================================================== */
/*  TIM1 — Advanced timer (fluxgate excitation)                          */
/* ===================================================================== */

#define TIM1_BASE       0x40010000U

#define TIM1_CR1        (*(volatile uint32_t *)(TIM1_BASE + 0x000U))
#define TIM1_CR2        (*(volatile uint32_t *)(TIM1_BASE + 0x004U))
#define TIM1_SMCR      (*(volatile uint32_t *)(TIM1_BASE + 0x008U))
#define TIM1_DIER      (*(volatile uint32_t *)(TIM1_BASE + 0x00CU))
#define TIM1_SR        (*(volatile uint32_t *)(TIM1_BASE + 0x010U))
#define TIM1_EGR       (*(volatile uint32_t *)(TIM1_BASE + 0x014U))
#define TIM1_CCMR1     (*(volatile uint32_t *)(TIM1_BASE + 0x018U))
#define TIM1_CCMR2     (*(volatile uint32_t *)(TIM1_BASE + 0x01CU))
#define TIM1_CCER      (*(volatile uint32_t *)(TIM1_BASE + 0x020U))
#define TIM1_CNT       (*(volatile uint32_t *)(TIM1_BASE + 0x024U))
#define TIM1_PSC       (*(volatile uint32_t *)(TIM1_BASE + 0x028U))
#define TIM1_ARR       (*(volatile uint32_t *)(TIM1_BASE + 0x02CU))
#define TIM1_RCR       (*(volatile uint32_t *)(TIM1_BASE + 0x030U))
#define TIM1_CCR1      (*(volatile uint32_t *)(TIM1_BASE + 0x034U))
#define TIM1_CCR2      (*(volatile uint32_t *)(TIM1_BASE + 0x038U))
#define TIM1_CCR3      (*(volatile uint32_t *)(TIM1_BASE + 0x03CU))
#define TIM1_CCR4      (*(volatile uint32_t *)(TIM1_BASE + 0x040U))
#define TIM1_BDTR      (*(volatile uint32_t *)(TIM1_BASE + 0x044U))

/* TIM1 bit fields */
#define TIM_CR1_CEN        (1U << 0)   /* Counter enable */
#define TIM_CR1_ARPE       (1U << 7)   /* Auto-reload preload enable */
#define TIM_CR1_CMS_CENTER (0x3U << 5) /* Center-aligned mode 3 */
#define TIM_DIER_UIE       (1U << 0)   /* Update interrupt enable */
#define TIM_DIER_CC1IE     (1U << 1)   /* Capture/compare 1 interrupt */
#define TIM_SR_UIF         (1U << 0)   /* Update interrupt flag */
#define TIM_SR_CC1IF       (1U << 1)
#define TIM_CCMR1_OC1M_PWM1 (0x6U << 4)
#define TIM_CCMR1_OC1PE    (1U << 3)
#define TIM_CCMR1_OC2M_PWM1 (0x6U << 12)
#define TIM_CCMR1_OC2PE    (1U << 11)
#define TIM_CCER_CC1E      (1U << 0)
#define TIM_CCER_CC1NE     (1U << 2)
#define TIM_CCER_CC2E      (1U << 4)
#define TIM_CCER_CC2NE     (1U << 6)
#define TIM_BDTR_MOE       (1U << 15)

/* ===================================================================== */
/*  TIM2 — General timer (ADC trigger + 2f₀ reference)                    */
/* ===================================================================== */

#define TIM2_BASE       0x40000000U

#define TIM2_CR1        (*(volatile uint32_t *)(TIM2_BASE + 0x000U))
#define TIM2_DIER       (*(volatile uint32_t *)(TIM2_BASE + 0x00CU))
#define TIM2_SR         (*(volatile uint32_t *)(TIM2_BASE + 0x010U))
#define TIM2_CNT        (*(volatile uint32_t *)(TIM2_BASE + 0x024U))
#define TIM2_PSC        (*(volatile uint32_t *)(TIM2_BASE + 0x028U))
#define TIM2_ARR        (*(volatile uint32_t *)(TIM2_BASE + 0x02CU))
#define TIM2_CCMR1      (*(volatile uint32_t *)(TIM2_BASE + 0x018U))
#define TIM2_CCER       (*(volatile uint32_t *)(TIM2_BASE + 0x020U))
#define TIM2_CCR1       (*(volatile uint32_t *)(TIM2_BASE + 0x034U))

/* ===================================================================== */
/*  SPI — RM0433 §18                                                      */
/* ===================================================================== */

#define SPI1_BASE       0x58004000U  /* SPI1 — ADS1256 + microSD (mux) */
#define SPI2_BASE       0x5800B000U  /* SPI2 — DAC8568 */

#define SPI_CR1         0x000U
#define SPI_CR2         0x004U
#define SPI_CFG1        0x008U
#define SPI_CFG2        0x00CU
#define SPI_SR          0x014U
#define SPI_IFCR        0x018U
#define SPI_TXDR        0x020U
#define SPI_RXDR        0x030U
#define SPI_IER         0x028U

/* SPI bit fields (H7) */
#define SPI_CR1_CSTART   (1U << 1)
#define SPI_CR1_CSUSP    (1U << 4)
#define SPI_CR1_SPE      (1U << 0)
#define SPI_CFG1_MASTER  (1U << 2)
#define SPI_CFG1_DSIZE_8  (0x7U << 0)  /* 8-bit */
#define SPI_CFG1_DSIZE_16 (0xFU << 0)  /* 16-bit */
#define SPI_CFG1_CR1_8BIT (0x7U << 28)
#define SPI_CFG1_CR1_16BIT (0xFU << 28)
#define SPI_CFG2_CPOL    (1U << 0)
#define SPI_CFG2_CPHA    (1U << 1)
#define SPI_CFG2_LSBFIRST (1U << 2)
#define SPI_CFG2_SSOE    (1U << 3)
#define SPI_CFG2_AFCNTR  (1U << 5)
#define SPI_SR_TXP       (1U << 1)
#define SPI_SR_RXP       (1U << 2)
#define SPI_SR_CRCF      (1U << 4)
#define SPI_SR_TSERF     (1U << 5)
#define SPI_SR_SUSP      (1U << 6)
#define SPI_SR_OVR       (1U << 7)
#define SPI_SR_EOT       (1U << 3)
#define SPI_IFCR_ALL     (0x3FU)

#define SPI_REG(port, offset) (*(volatile uint32_t *)((port) + (offset)))

/* ===================================================================== */
/*  I2C1 — RM0433 §16 (BMI270 + SSD1306 OLED)                            */
/* ===================================================================== */

#define I2C1_BASE       0x58020C00U

#define I2C_CR1         (*(volatile uint32_t *)(I2C1_BASE + 0x000U))
#define I2C_CR2         (*(volatile uint32_t *)(I2C1_BASE + 0x004U))
#define I2C_OAR1        (*(volatile uint32_t *)(I2C1_BASE + 0x008U))
#define I2C_OAR2        (*(volatile uint32_t *)(I2C1_BASE + 0x00CU))
#define I2C_TIMINGR     (*(volatile uint32_t *)(I2C1_BASE + 0x010U))
#define I2C_ISR         (*(volatile uint32_t *)(I2C1_BASE + 0x014U))
#define I2C_ICR         (*(volatile uint32_t *)(I2C1_BASE + 0x020U))
#define I2C_PECR        (*(volatile uint32_t *)(I2C1_BASE + 0x024U))
#define I2C_RXDR        (*(volatile uint32_t *)(I2C1_BASE + 0x028U))
#define I2C_TXDR        (*(volatile uint32_t *)(I2C1_BASE + 0x028U))

/* I2C ISR flags */
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
/*  USART1 (GPS NEO-M9N NMEA)                                            */
/*  UART4 (nRF52840 BLE module)                                          */
/* ===================================================================== */

#define USART1_BASE     0x58008000U
#define UART4_BASE      0x50004400U

#define USART_CR1       0x000U
#define USART_CR2       0x004U
#define USART_CR3       0x008U
#define USART_BRR       0x00CU
#define USART_RQR       0x018U
#define USART_ISR       0x01CU
#define USART_ICR       0x020U
#define USART_RDR       0x024U
#define USART_TDR       0x028U

/* USART ISR flags */
#define USART_ISR_TXE   (1U << 7)
#define USART_ISR_TC    (1U << 6)
#define USART_ISR_RXNE  (1U << 5)
#define USART_ISR_IDLE  (1U << 4)
#define USART_ISR_ORE   (1U << 3)
#define USART_ISR_FE    (1U << 1)
#define USART_ISR_NE    (1U << 2)

#define USART_CR1_UE    (1U << 0)
#define USART_CR1_TE    (1U << 3)
#define USART_CR1_RE    (1U << 2)
#define USART_CR1_RXNEIE (1U << 5)
#define USART_CR1_TCIE  (1U << 6)
#define USART_CR1_IDLEIE (1U << 4)

#define USART_REG(port, offset) (*(volatile uint32_t *)((port) + (offset)))

/* ===================================================================== */
/*  DMA1 (for SPI1 ADC transfers and SD card)                             */
/* ===================================================================== */

#define DMA1_BASE       0x40020000U
#define DMA1_Stream0    0x40020010U
#define DMA1_Stream1    0x40020028U
#define DMA1_Stream2    0x40020040U
#define DMA1_Stream3    0x40020058U
#define DMA1_Stream4    0x40020070U
#define DMA1_Stream5    0x40020088U

/* DMA stream register offsets */
#define DMA_PAR         0x00U   /* Peripheral address */
#define DMA_M0AR        0x04U   /* Memory 0 address */
#define DMA_M1AR        0x08U   /* Memory 1 address */
#define DMA_SPAR        0x0CU   /* Spare address (H7) */
#define DMA_NDTR        0x10U   /* Number of data to transfer */
#define DMA_CR          0x18U   /* Configuration register (H7 uses CR) */
#define DMA_FCR         0x1CU   /* FIFO control register */

/* DMA1 LIFCR and HISR (interrupt clear/status) */
#define DMA1_LIFCR      (*(volatile uint32_t *)(DMA1_BASE + 0x000U))
#define DMA1_LISR       (*(volatile uint32_t *)(DMA1_BASE + 0x008U))
#define DMA1_HIFCR      (*(volatile uint32_t *)(DMA1_BASE + 0x004U))
#define DMA1_HISR       (*(volatile uint32_t *)(DMA1_BASE + 0x00CU))

#define DMA_REG(stream, offset) (*(volatile uint32_t *)((stream) + (offset)))

/* ===================================================================== */
/*  ADS1256 24-bit ADC command definitions (SPI)                         */
/* ===================================================================== */

#define ADS1256_CMD_WREG    0x50U   /* Write registers (followed by data) */
#define ADS1256_CMD_RREG    0x10U   /* Read registers */
#define ADS1256_CMD_SYNC    0xFCU   /* Synchronize */
#define ADS1256_CMD_STANDBY 0xFDU   /* Standby mode */
#define ADS1256_CMD_RESET   0xFEU   /* Reset */
#define ADS1256_CMD_WAKEUP  0x00U   /* Wakeup (same as NOP/NOP) */
#define ADS1256_CMD_RDATAC  0x01U   /* Read data continuous */
#define ADS1256_CMD_SDATAC  0x0FU   /* Stop read data continuous */
#define ADS1256_CMD_RDATA   0x01U   /* Read data once */

/* ADS1256 register map */
#define ADS1256_REG_STATUS  0x00U
#define ADS1256_REG_MUX     0x01U
#define ADS1256_REG_ADCON   0x02U
#define ADS1256_REG_DRATE   0x03U
#define ADS1256_REG_IO      0x04U
#define ADS1256_REG_OFC0    0x05U
#define ADS1256_REG_OFC1    0x06U
#define ADS1256_REG_OFC2    0x07U
#define ADS1256_REG_FSC0   0x08U
#define ADS1256_REG_FSC1   0x09U
#define ADS1256_REG_FSC2   0x0AU

/* ADS1256 data rates */
#define ADS1256_DRATE_30000SPS  0xF0U
#define ADS1256_DRATE_15000SPS  0xE0U
#define ADS1256_DRATE_7500SPS   0xC0U
#define ADS1256_DRATE_1000SPS   0x82U
#define ADS1256_DRATE_500SPS    0x63U
#define ADS1256_DRATE_100SPS    0x23U

/* ADS1256 PGA gain settings */
#define ADS1256_PGA_1X    0x00U
#define ADS1256_PGA_2X    0x01U
#define ADS1256_PGA_4X    0x02U
#define ADS1256_PGA_8X    0x03U
#define ADS1256_PGA_16X   0x04U
#define ADS1256_PGA_32X   0x05U

/* ===================================================================== */
/*  DAC8568 16-bit 8-channel DAC (feedback nulling coils)               */
/* ===================================================================== */

#define DAC8568_CMD_WRITE_UPDATE  0x00U  /* Write to input reg and update DAC */
#define DAC8568_CMD_UPDATE_ONLY   0x20U  /* Update DAC register (no write) */
#define DAC8568_CMD_WRITE_LATCH   0x40U  /* Write to input reg only */
#define DAC8568_CMD_POWER_DOWN    0x70U  /* Power down DAC(s) */
#define DAC8568_CMD_LDAC_ALL      0x60U  /* LDAC all channels */

/* ===================================================================== */
/*  BMI270 IMU register addresses                                        */
/* ===================================================================== */

#define BMI270_I2C_ADDR   0x68U   /* SDO = G → 0x68, SDO = VCC → 0x69 */
#define BMI270_REG_CHIPID 0x00U
#define BMI270_CHIPID_VAL 0x24U

#define BMI270_REG_DATA_8   0x0CU   /* Accel/Gyro data, 12 bytes */
#define BMI270_REG_STATUS   0x1DU
#define BMI270_REG_TEMP_0   0x20U
#define BMI270_REG_CMD      0x7EU
#define BMI270_REG_PWR_CTRL 0x7DU
#define BMI270_REG_INT1_IO_CTRL 0x53U

/* BMI270 power control bits */
#define BMI270_PWR_AUX_EN   (1U << 0)
#define BMI270_PWR_GYR_EN   (1U << 1)
#define BMI270_PWR_ACC_EN   (1U << 2)
#define BMI270_PWR_TEMP_EN  (1U << 3)

/* ===================================================================== */
/*  SSD1306 OLED display (128×64, I2C addr 0x3C)                        */
/* ===================================================================== */

#define SSD1306_I2C_ADDR   0x3CU
#define SSD1306_REG_LOW_COLUMN  0x00U
#define SSD1306_REG_HIGH_COLUMN 0x10U
#define SSD1306_REG_DISPLAY_START_LINE 0x40U
#define SSD1306_CMD_CONTRAST   0x81U
#define SSD1306_CMD_ENTIRE_ON  0xA4U
#define SSD1306_CMD_ENTIRE_OFF 0xA5U
#define SSD1306_CMD_NORMAL     0xA6U
#define SSD1306_CMD_INVERT     0xA7U
#define SSD1306_CMD_DISPLAY_OFF 0xAEU
#define SSD1306_CMD_DISPLAY_ON  0xAFU
#define SSD1306_CMD_COL_ADDR_SET 0x21U
#define SSD1306_CMD_PAGE_ADDR_SET 0x22U

/* ===================================================================== */
/*  PWR controller — RM0433 §9                                            */
/* ===================================================================== */

#define PWR_BASE        0x58024800U
#define PWR_CR1        (*(volatile uint32_t *)(PWR_BASE + 0x000U))
#define PWR_CR2        (*(volatile uint32_t *)(PWR_BASE + 0x004U))
#define PWR_CR3        (*(volatile uint32_t *)(PWR_BASE + 0x008U))
#define PWR_CPUCR      (*(volatile uint32_t *)(PWR_BASE + 0x010U))
#define PWR_D3CR       (*(volatile uint32_t *)(PWR_BASE + 0x018U))

#define PWR_CR3_SCUEN    (1U << 2)
#define PWR_CR3_LDOEN    (1U << 1)
#define PWR_CR3_SMPSEN   (1U << 0)
#define PWR_CR3_USB33DEN (1U << 24)

/* ===================================================================== */
/*  Flash controller (for option bytes / boot config)                    */
/* ===================================================================== */

#define FLASH_BASE      0x52002000U
#define FLASH_KEYR      (*(volatile uint32_t *)(FLASH_BASE + 0x004U))
#define FLASH_OPTKEYR   (*(volatile uint32_t *)(FLASH_BASE + 0x008U))
#define FLASH_SR        (*(volatile uint32_t *)(FLASH_BASE + 0x010U))
#define FLASH_CR        (*(volatile uint32_t *)(FLASH_BASE + 0x014U))
#define FLASH_OPTCR     (*(volatile uint32_t *)(FLASH_BASE + 0x018U))
#define FLASH_OPTSR_CUR (*(volatile uint32_t *)(FLASH_BASE + 0x01CU))
#define FLASH_OPTCCR    (*(volatile uint32_t *)(FLASH_BASE + 0x024U))

#endif /* FERROPROBE_REGISTERS_H */