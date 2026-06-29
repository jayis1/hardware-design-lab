/*
 * registers.h — STM32L432KC peripheral register map (bare-metal)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Minimal direct-register definitions for the peripherals HiveVox uses.
 * No ST HAL dependency. Addresses from RM0394 reference manual.
 */
#ifndef HIVEVOX_REGISTERS_H
#define HIVEVOX_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ------------------------------------------------- */
#define PERIPH_BASE        0x40000000UL
#define AHB1_BASE          0x48000000UL
#define AHB2_BASE          0x48010000UL
#define APB1_BASE          0x40000000UL
#define APB2_BASE          0x48000000UL

/* Cortex-M4 system registers */
#define SCB_BASE           0xE000ED00UL
#define SYSTICK_BASE       0xE000E010UL
#define NVIC_BASE          0xE000E100UL

/* ---- RCC (clock control) ------------------------------------------- */
#define RCC_BASE           (AHB1_BASE + 0x1000)
#define RCC_CR             (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_ICSCR          (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_CFGR           (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_PLLCFGR        (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x48))  /* GPIOA-E */
#define RCC_AHB2ENR        (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_APB1ENR1       (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB1ENR2       (*(volatile uint32_t *)(RCC_BASE + 0x5C))
#define RCC_APB2ENR        (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_CCIPR          (*(volatile uint32_t *)(RCC_BASE + 0x88))
#define RCC_BDCR           (*(volatile uint32_t *)(RCC_BASE + 0x90))  /* LSE + RTCEN */
#define RCC_CSR            (*(volatile uint32_t *)(RCC_BASE + 0x94))

/* RCC bit positions */
#define RCC_CR_MSIRANGE_SHIFT   4
#define RCC_CR_HSION            (1U << 8)
#define RCC_CR_HSIRDY           (1U << 10)
#define RCC_CR_HSEON           (1U << 16)
#define RCC_CR_HSERDY           (1U << 17)
#define RCC_CR_PLLON           (1U << 24)
#define RCC_CR_PLLRDY           (1U << 25)

#define RCC_CFGR_SW_MSI        0
#define RCC_CFGR_SW_HSI        1
#define RCC_CFGR_SW_HSE        2
#define RCC_CFGR_SW_PLL        3
#define RCC_CFGR_SWS_SHIFT      2

/* AHB1ENR enables — GPIO ports A,B,C */
#define RCC_AHB2ENR_GPIOAEN    (1U << 0)
#define RCC_AHB2ENR_GPIOBEN    (1U << 1)
#define RCC_AHB2ENR_GPIOCEN    (1U << 2)

/* ADC on AH2 */
#define RCC_AHB2ENR_ADCEN      (1U << 13)

/* APB1ENR1 enables */
#define RCC_APB1ENR1_TIM2EN    (1U << 0)
#define RCC_APB1ENR1_SPI2EN    (1U << 14)
#define RCC_APB1ENR1_USART2EN  (1U << 17)
#define RCC_APB1ENR1_I2C3EN    (1U << 23)

/* APB1ENR2 enables */
#define RCC_APB1ENR2_LPUART1EN (1U << 0)

/* APB2ENR enables */
#define RCC_APB2ENR_SYSCFGEN  (1U << 0)
#define RCC_APB2ENR_TIM1EN    (1U << 11)
#define RCC_APB2ENR_SPI1EN    (1U << 12)
#define RCC_APB2ENR_USART1EN  (1U << 14)

/* BDCR */
#define RCC_BDCR_LSEON        (1U << 0)
#define RCC_BDCR_LSERDY       (1U << 1)
#define RCC_BDCR_RTCEN        (1U << 15)
#define RCC_BDCR_RTCSEL_SHIFT 16

/* ---- GPIO ---------------------------------------------------------- */
#define GPIOA_BASE          (AHB2_BASE + 0x0000)
#define GPIOB_BASE          (AHB2_BASE + 0x0400)
#define GPIOC_BASE          (AHB2_BASE + 0x0800)

typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
    volatile uint32_t OTYPER;   /* 0x04 */
    volatile uint32_t OSPEEDR;   /* 0x08 */
    volatile uint32_t PUPDR;    /* 0x0C */
    volatile uint32_t IDR;      /* 0x10 */
    volatile uint32_t ODR;      /* 0x14 */
    volatile uint32_t BSRR;     /* 0x18 */
    volatile uint32_t LCKR;     /* 0x1C */
    volatile uint32_t AFRL;    /* 0x20 */
    volatile uint32_t AFRH;    /* 0x24 */
    volatile uint32_t BRR;      /* 0x28 */
    uint32_t RESERVED[2];
    volatile uint32_t SECCFGR;  /* 0x30 */
} gpio_regs_t;

#define GPIOA  ((volatile gpio_regs_t *)GPIOA_BASE)
#define GPIOB  ((volatile gpio_regs_t *)GPIOB_BASE)
#define GPIOC  ((volatile gpio_regs_t *)GPIOC_BASE)

/* MODER values */
#define GPIO_MODE_INPUT   0
#define GPIO_MODE_OUTPUT  1
#define GPIO_MODE_AF       2
#define GPIO_MODE_ANALOG  3

/* ---- RTC (real-time clock, LSE-driven) ----------------------------- */
#define RTC_BASE            (APB1_BASE + 0x4C00)
#define RTC_TR              (*(volatile uint32_t *)(RTC_BASE + 0x00))
#define RTC_DR              (*(volatile uint32_t *)(RTC_BASE + 0x04))
#define RTC_CR              (*(volatile uint32_t *)(RTC_BASE + 0x08))
#define RTC_ISR            (*(volatile uint32_t *)(RTC_BASE + 0x0C))
#define RTC_PRER           (*(volatile uint32_t *)(RTC_BASE + 0x10))
#define RTC_ALRMAR          (*(volatile uint32_t *)(RTC_BASE + 0x1C))
#define RTC_ALRMASSR        (*(volatile uint32_t *)(RTC_BASE + 0x24))
#define RTC_SCR             (*(volatile uint32_t *)(RTC_BASE + 0x5C))
#define RTC_WUTR           (*(volatile uint32_t *)(RTC_BASE + 0x14))
#define RTC_WPR            (*(volatile uint32_t *)(RTC_BASE + 0x28))  /* write protect */

/* RTC ISR bits */
#define RTC_ISR_ALRAF       (1U << 8)   /* Alarm A flag */
#define RTC_ISR_INIT        (1U << 6)   /* initialization mode */
#define RTC_ISR_INITF       (1U << 6)
#define RTC_ISR_RSF         (1U << 5)
#define RTC_ISR_WUTWF       (1U << 2)

/* RTC CR bits */
#define RTC_CR_ALRAIE       (1U << 12)
#define RTC_CR_ALRAE       (1U << 8)
#define RTC_CR_WUTE        (1U << 10)
#define RTC_CR_WUTIE       (1U << 14)
#define RTC_CR_BYPSHAD     (1U << 5)

/* ---- EXTI (external interrupt) ------------------------------------- */
#define EXTI_BASE           (APB2_BASE + 0x0000)
#define EXTI_IMR1           (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_RPR1           (*(volatile uint32_t *)(EXTI_BASE + 0x20))
#define EXTI_FPR1           (*(volatile uint32_t *)(EXTI_BASE + 0x24))

/* ---- SPI2 (for SX1276 LoRa) ---------------------------------------- */
#define SPI2_BASE           (APB1_BASE + 0x3800)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t SR;      /* 0x08 */
    volatile uint32_t DR;      /* 0x0C */
    volatile uint32_t CRCPR;   /* 0x10 */
    volatile uint32_t RXCRCR;  /* 0x14 */
    volatile uint32_t TXCRCR;  /* 0x18 */
    volatile uint32_t I2SCFGR; /* 0x1C */
    volatile uint32_t I2SPR;   /* 0x20 */
} spi_regs_t;
#define SPI2  ((volatile spi_regs_t *)SPI2_BASE)

#define SPI_CR1_SPE         (1U << 6)
#define SPI_CR1_MSTR        (1U << 2)
#define SPI_CR1_CPOL        (1U << 1)
#define SPI_CR1_CPHA        (1U << 0)
#define SPI_CR1_BR_SHIFT    3
#define SPI_CR1_SSI         (1U << 8)
#define SPI_CR1_SSM         (1U << 9)
#define SPI_CR2_FRXTH       (1U << 12)
#define SPI_CR2_DS_8BIT     (7U << 8)
#define SPI_SR_TXE          (1U << 1)
#define SPI_SR_RXNE         (1U << 0)
#define SPI_SR_BSY          (1U << 7)

/* ---- I2C3 (SHT45 + VEML7700) --------------------------------------- */
#define I2C3_BASE           (APB1_BASE + 0x5C00)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t OAR1;    /* 0x08 */
    volatile uint32_t OAR2;    /* 0x0C */
    volatile uint32_t TIMINGR; /* 0x10 */
    volatile uint32_t TIMEOUTR;/* 0x14 */
    volatile uint32_t ISR;     /* 0x18 */
    volatile uint32_t ICR;     /* 0x1C */
    volatile uint32_t PECR;    /* 0x20 */
    volatile uint32_t RXDR;    /* 0x24 */
    volatile uint32_t TXDR;    /* 0x28 */
} i2c_regs_t;
#define I2C3  ((volatile i2c_regs_t *)I2C3_BASE)

#define I2C_CR1_PE           (1U << 0)
#define I2C_CR2_START        (1U << 13)
#define I2C_CR2_STOP         (1U << 14)
#define I2C_CR2_RD_WRN       (1U << 10)
#define I2C_CR2_NBYTES_SHIFT 16
#define I2C_CR2_AUTOEND      (1U << 20)
#define I2C_ISR_TXE         (1U << 0)
#define I2C_ISR_RXNE        (1U << 2)
#define I2C_ISR_NACKF       (1U << 4)
#define I2C_ISR_STOPF       (1U << 5)
#define I2C_ISR_BUSY        (1U << 15)
#define I2C_ICR_NACKCF      (1U << 4)
#define I2C_ICR_STOPCF      (1U << 5)

/* ---- ADC (battery voltage) ----------------------------------------- */
#define ADC_BASE            (AHB2_BASE + 0x0800)
#define ADC_CR              (*(volatile uint32_t *)(ADC_BASE + 0x08))
#define ADC_CFGR           (*(volatile uint32_t *)(ADC_BASE + 0x0C))
#define ADC_SMPR1          (*(volatile uint32_t *)(ADC_BASE + 0x14))
#define ADC_ISR            (*(volatile uint32_t *)(ADC_BASE + 0x20))
#define ADC_DR             (*(volatile uint32_t *)(ADC_BASE + 0x40))
#define ADC_ISR_ADRDY      (1U << 0)
#define ADC_CR_ADEN        (1U << 0)
#define ADC_CR_ADSTART     (1U << 2)
#define ADC_ISR_EOC        (1U << 2)

/* ---- PWR (power control) ------------------------------------------- */
#define PWR_BASE            (APB1_BASE + 0x0000)
#define PWR_CR1            (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CR2            (*(volatile uint32_t *)(PWR_BASE + 0x04))
#define PWR_CR3            (*(volatile uint32_t *)(PWR_BASE + 0x08))
#define PWR_CR4            (*(volatile uint32_t *)(PWR_BASE + 0x0C))
#define PWR_SR1            (*(volatile uint32_t *)(PWR_BASE + 0x10))
#define PWR_SR2            (*(volatile uint32_t *)(PWR_BASE + 0x14))
#define PWR_SCR            (*(volatile uint32_t *)(PWR_BASE + 0x18))

#define PWR_CR1_LPMS_STOP2  2

/* ---- SCB (low-power mode entry) ----------------------------------- */
#define SCB_SCR            (*(volatile uint32_t *)(SCB_BASE + 0x10))
#define SCB_SCR_SLEEPDEEP   (1U << 2)
#define SCB_SCR_SLEEPONEXIT (1U << 1)

/* ---- SysTick / NVIC ------------------------------------------------ */
#define SYSTICK_CSR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYSTICK_CSR_CLKSOURCE (1U << 2)
#define SYSTICK_CSR_ENABLE    (1U << 0)
#define SYSTICK_CSR_TICKINT   (1U << 1)

/* NVIC — RTC alarm IRQ is EXTI line 18 */
#define NVIC_ISER0          (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ICER0          (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ICPR0          (*(volatile uint32_t *)(NVIC_BASE + 0x180))

/* RTC_Alarm_IRQn on STM32L4 = 41 → ISER0 bit 41 (in word index 1) */
#define NVIC_RTC_ALARM_VEC  41
#define NVIC_ISER1          (*(volatile uint32_t *)(NVIC_BASE + 0x004))
#define NVIC_ICER1          (*(volatile uint32_t *)(NVIC_BASE + 0x084))
#define NVIC_ICPR1          (*(volatile uint32_t *)(NVIC_BASE + 0x184))

/* ---- DMA (for I²S/PDM microphone capture) ------------------------- */
#define DMA1_BASE           0x40026000UL
#define DMA1_Channel1_BASE  (DMA1_BASE + 0x08)
typedef struct {
    volatile uint32_t CCR;     /* channel config */
    volatile uint32_t CNDTR;   /* number of data */
    volatile uint32_t CPAR;    /* peripheral addr */
    volatile uint32_t CMAR;   /* memory addr */
    volatile uint32_t CCR2;    /* reserved / clear flag */
} dma_channel_regs_t;
#define DMA1_CH1 ((volatile dma_channel_regs_t *)DMA1_Channel1_BASE)
#define DMA1_IFCR (*(volatile uint32_t *)(DMA1_BASE + 0x04))

#define DMA_CCR_EN          (1U << 0)
#define DMA_CCR_TCIE        (1U << 1)
#define DMA_CCR_MINC        (1U << 10)
#define DMA_CCR_PSIZE_16    (1U << 9)
#define DMA_CCR_MSIZE_16    (1U << 13)
#define DMA_CCR_DIR_P2M     (0U << 4)
#define DMA_CCR_CIRC        (1U << 5)

/* ---- Flash (for wait states) --------------------------------------- */
#define FLASH_BASE          0x40022000UL
#define FLASH_ACR           (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_ACR_LATENCY_MASK 0x07
#define FLASH_ACR_PRFTEN     (1U << 8)
#define FLASH_ACR_RUNPD      (1U << 9)

#endif /* HIVEVOX_REGISTERS_H */