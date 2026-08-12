/*
 * registers.h — STM32WL55JC register map (subset, bare-metal)
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Minimal register definitions for the STM32WL55 SoC used by GrainGuard.
 * Not a complete header — only the registers and bitfields the firmware
 * actually touches.
 */

#ifndef GRAINGUARD_REGISTERS_H
#define GRAINGUARD_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ---- */
#define PERIPH_BASE          0x40000000UL
#define PERIPH_APB2_BASE     0x48000000UL  /* GPIOA-GPIOC */
#define PERIPH_APB1_BASE     0x40000000UL
#define PERIPH_AHB1_BASE     0x58000000UL  /* DMA, AES, RNG */
#define PERIPH_AHB2_BASE     0x58001400UL  /* Flash, SRAM */

/* ---- GPIO (same layout as STM32 family) ---- */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
    volatile uint32_t OTYPER;   /* 0x04 */
    volatile uint32_t OSPEEDR;   /* 0x08 */
    volatile uint32_t PUPDR;    /* 0x0C */
    volatile uint32_t IDR;      /* 0x10 */
    volatile uint32_t ODR;      /* 0x14 */
    volatile uint32_t BSRR;     /* 0x18 */
    volatile uint32_t LCKR;     /* 0x1C */
    volatile uint32_t AFRL;     /* 0x20 */
    volatile uint32_t AFRH;     /* 0x24 */
    volatile uint32_t BRR;      /* 0x28 */
} GPIO_t;

#define GPIOA_BASE  (PERIPH_APB2_BASE + 0x0000)
#define GPIOB_BASE  (PERIPH_APB2_BASE + 0x0400)
#define GPIOC_BASE  (PERIPH_APB2_BASE + 0x0800)
#define GPIOA      ((GPIO_t *) GPIOA_BASE)
#define GPIOB      ((GPIO_t *) GPIOB_BASE)
#define GPIOC      ((GPIO_t *) GPIOC_BASE)

/* GPIO mode bits (2 bits per pin) */
#define GPIO_MODE_INPUT     0x00
#define GPIO_MODE_OUTPUT     0x01
#define GPIO_MODE_AF         0x02
#define GPIO_MODE_ANALOG     0x03

#define GPIO_OTYPE_PP        0  /* push-pull */
#define GPIO_OTYPE_OD        1  /* open-drain */
#define GPIO_OSPEED_LOW      0x00
#define GPIO_OSPEED_HIGH     0x01
#define GPIO_PUPD_NONE       0x00
#define GPIO_PUPD_PU         0x01
#define GPIO_PUPD_PD         0x02

/* ---- RCC (Reset and Clock Control) ---- */
#define RCC_BASE    (PERIPH_AHB1_BASE + 0x0000)
typedef struct {
    volatile uint32_t CR;          /* 0x00 */
    volatile uint32_t ICSCR;       /* 0x04 */
    volatile uint32_t CFGR;        /* 0x08 */
    volatile uint32_t RESERVED0;   /* 0x0C */
    volatile uint32_t PLLCFGR;     /* 0x10 */
    volatile uint32_t RESERVED1[5];/* 0x14..0x28 */
    volatile uint32_t CIER;        /* 0x2C */
    volatile uint32_t CIFR;        /* 0x30 */
    volatile uint32_t CICR;        /* 0x34 */
    volatile uint32_t RESERVED2;   /* 0x38 */
    volatile uint32_t AHB1ENR;     /* 0x3C? (offset varies) */
    /* ... (many more; we define only what we use) */
    volatile uint32_t APB1ENR;     /* approximate */
    volatile uint32_t APB2ENR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t CCIPR;       /* peripheral clock sel */
} RCC_t;

#define RCC ((RCC_t *) RCC_BASE)

/* RCC enable bits (subset) */
#define RCC_AHB1ENR_GPIOAEN   (1 << 0)
#define RCC_AHB1ENR_GPIOBEN   (1 << 1)
#define RCC_AHB1ENR_GPIOCEN   (1 << 2)
#define RCC_AHB1ENR_DMA1EN    (1 << 8)
#define RCC_AHB1ENR_AESEN     (1 << 16)
#define RCC_AHB1ENR_RNGEN     (1 << 18)

#define RCC_APB1ENR_I2C1EN    (1 << 21)
#define RCC_APB1ENR_SPI2EN    (1 << 14)
#define RCC_APB1ENR_RTCAPBEN  (1 << 10)
#define RCC_APB1ENR_TIM2EN   (1 << 0)

#define RCC_APB2ENR_SPI1EN    (1 << 12)
#define RCC_APB2ENR_USART2EN  (1 << 14)
#define RCC_APB2ENR_ADCEN    (1 << 13)
#define RCC_APB2ENR_SYSCFGEN  (1 << 0)

/* ---- I2C1 ---- */
#define I2C1_BASE  (PERIPH_APB1_BASE + 0x5400)
typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t OAR1;   /* 0x08 */
    volatile uint32_t OAR2;   /* 0x0C */
    volatile uint32_t TIMINGR;/* 0x10 */
    volatile uint32_t TIMEOUTr;/* 0x14 */
    volatile uint32_t ISR;    /* 0x18 */
    volatile uint32_t ICR;    /* 0x1C */
    volatile uint32_t PECR;   /* 0x20 */
    volatile uint32_t RXDR;   /* 0x24 */
    volatile uint32_t TXDR;   /* 0x28 */
} I2C_t;

#define I2C1 ((I2C_t *) I2C1_BASE)
#define I2C_ISR_TXE    (1 << 0)
#define I2C_ISR_TXIS   (1 << 1)
#define I2C_ISR_RXNE   (1 << 2)
#define I2C_ISR_NACKF  (1 << 4)
#define I2C_ISR_STOPF  (1 << 5)
#define I2C_ISR_BUSY   (1 << 15)

/* ---- SPI1 ---- */
#define SPI1_BASE  (PERIPH_APB2_BASE + 0x3000)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t SR;      /* 0x08 */
    volatile uint32_t DR;      /* 0x0C */
    volatile uint32_t CRCPR;    /* 0x10 */
    volatile uint32_t RXCRCR;   /* 0x14 */
    volatile uint32_t TXCRCR;   /* 0x18 */
} SPI_t;

#define SPI1 ((SPI_t *) SPI1_BASE)
#define SPI_SR_RXNE   (1 << 0)
#define SPI_SR_TXE    (1 << 1)
#define SPI_SR_BSY    (1 << 7)

/* ---- ADC1 ---- */
#define ADC1_BASE  (PERIPH_APB2_BASE + 0x4000)
typedef struct {
    volatile uint32_t ISR;     /* 0x00 */
    volatile uint32_t IER;     /* 0x04 */
    volatile uint32_t CR;      /* 0x08 */
    volatile uint32_t CFGR;    /* 0x0C */
    volatile uint32_t CFGR2;   /* 0x10 */
    volatile uint32_t SMPR1;   /* 0x14 */
    volatile uint32_t SMPR2;   /* 0x18 */
    volatile uint32_t RESERVED0[2]; /* 0x1C-0x20 */
    volatile uint32_t TR1;     /* 0x24 */
    volatile uint32_t RESERVED1[3]; /* 0x28-0x30 */
    volatile uint32_t DR;      /* 0x40? (offset varies) */
    volatile uint32_t RESERVED2[30];
    volatile uint32_t SQR1;     /* regular sequence */
    volatile uint32_t SQR2;
    volatile uint32_t SQR3;
    volatile uint32_t SQR4;
} ADC_t;

#define ADC1 ((ADC_t *) ADC1_BASE)
#define ADC_ISR_ADRDY  (1 << 0)
#define ADC_ISR_EOC    (1 << 2)
#define ADC_CR_ADEN    (1 << 0)
#define ADC_CR_ADSTART (1 << 2)
#define ADC_CR_ADSTOP  (1 << 4)

/* ---- RTC (real-time clock) ---- */
#define RTC_BASE  (PERIPH_APB1_BASE + 0x4C00)
typedef struct {
    volatile uint32_t TR;      /* 0x00 time register */
    volatile uint32_t DR;      /* 0x04 date register */
    volatile uint32_t SSR;     /* 0x08 sub-second */
    volatile uint32_t RESERVED0[2];
    volatile uint32_t ISR;     /* 0x0C? (offset varies) */
    volatile uint32_t PRER;    /* prescaler */
    volatile uint32_t WUTR;    /* wake-up timer */
    volatile uint32_t CR;      /* control */
    volatile uint32_t WPR;     /* write protect */
    volatile uint32_t CALR;    /* calibration */
    volatile uint32_t SHIFTR;
    volatile uint32_t TSTR;
    volatile uint32_t TSDR;
    volatile uint32_t ALRMAR;
    volatile uint32_t ALRMASSR;
    volatile uint32_t RESERVED1[2];
    volatile uint32_t ALRMBR;
    volatile uint32_t ALRMBSSR;
} RTC_t;

#define RTC ((RTC_t *) RTC_BASE)
#define RTC_ISR_INIT      (1 << 6)
#define RTC_ISR_WUTF      (1 << 2)
#define RTC_CR_WUTE       (1 << 10)
#define RTC_CR_WUTIE      (1 << 14)

/* RTC wake-up clock selection */
#define RTC_CR_WUCKSEL_MASK  0x7
#define RTC_CR_WUCKSEL_RTCDIV16  0x2  /* 2048 Hz */

/* ---- PWR (power control) ---- */
#define PWR_BASE  (PERIPH_APB1_BASE + 0x4800)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t CR3;    /* 0x08 */
    volatile uint32_t CR4;    /* 0x0C */
    volatile uint32_t SR1;    /* 0x10 */
    volatile uint32_t SR2;    /* 0x14 */
    volatile uint32_t SCR;    /* 0x18 */
    volatile uint32_t RESERVED0[2];
    volatile uint32_t PUCRA;
    volatile uint32_t PDCRA;
    volatile uint32_t PUCRB;
    volatile uint32_t PDCRB;
    volatile uint32_t PUCRC;
    volatile uint32_t PDCRC;
} PWR_t;

#define PWR ((PWR_t *) PWR_BASE)
#define PWR_CR3_LSEDRV_MASK   (0x3 << 10)

/* ---- AES (hardware accelerator) ---- */
#define AES_BASE  (PERIPH_AHB1_BASE + 0x0800)
typedef struct {
    volatile uint32_t CR;     /* 0x00 */
    volatile uint32_t SR;     /* 0x04 */
    volatile uint32_t DINR;   /* 0x08 */
    volatile uint32_t DOUTR;  /* 0x0C */
    volatile uint32_t KEYR0;  /* 0x10 */
    volatile uint32_t KEYR1;  /* 0x14 */
    volatile uint32_t KEYR2;  /* 0x18 */
    volatile uint32_t KEYR3;  /* 0x1C */
    volatile uint32_t IVR0;   /* 0x20 */
    volatile uint32_t IVR1;   /* 0x24 */
    volatile uint32_t IVR2;   /* 0x28 */
    volatile uint32_t IVR3;   /* 0x2C */
    volatile uint32_t SUSPR0; /* 0x30 */
} AES_t;

#define AES ((AES_t *) AES_BASE)
#define AES_SR_BUSY   (1 << 0)
#define AES_SR_CCF    (1 << 3)
#define AES_CR_EN     (1 << 0)

/* ---- DMA1 channel 1 ---- */
#define DMA1_BASE  (PERIPH_AHB1_BASE + 0x0600)
typedef struct {
    volatile uint32_t ISR;     /* 0x00 */
    volatile uint32_t RESERVED0;
    volatile uint32_t CCR1;    /* channel 1 */
    volatile uint32_t CNDTR1;
    volatile uint32_t CPAR1;
    volatile uint32_t CMAR1;
    volatile uint32_t RESERVED1;
    volatile uint32_t CCR2;    /* channel 2 */
    volatile uint32_t CNDTR2;
    volatile uint32_t CPAR2;
    volatile uint32_t CMAR2;
} DMA_t;

#define DMA1 ((DMA_t *) DMA1_BASE)
#define DMA_CCR_EN   (1 << 0)
#define DMA_CCR_TCIE (1 << 1)
#define DMA_CCR_MINC (1 << 10)
#define DMA_CCR_CIRC (1 << 8)

/* ---- Sub-GHz radio (SX1262 integrated in STM32WL55) ---- */
/* The radio is accessed via the internal SPI (Radio SPI) — on STM32WL55
 * it is a peripheral inside the SoC, accessed through dedicated
 * commands.  The SubGHz peripheral base: */
#define SUBGHZ_BASE  (PERIPH_APB1_BASE + 0x5800)
typedef struct {
    volatile uint32_t CR;       /* 0x00 — not exact; simplified */
    volatile uint32_t RESERVED0[3];
    volatile uint32_t IR;       /* interrupt */
    volatile uint32_t RESERVED1[3];
} SubGHz_t;

#define SUBGHZ ((SubGHz_t *) SUBGHZ_BASE)

/* Radio commands (subset for SX1262) */
#define RADIO_CMD_SET_STANDBY     0x80
#define RADIO_CMD_SET_PACKET_TYPE 0x8A
#define RADIO_CMD_SET_RF_FREQ     0x86
#define RADIO_CMD_SET_MOD_PARAMS  0x8B
#define RADIO_CMD_SET_TX_PARAMS   0x8E
#define RADIO_CMD_SET_PACKET_PARAMS 0x9C
#define RADIO_CMD_WRITE_BUFFER    0x0E
#define RADIO_CMD_READ_BUFFER     0x1E
#define RADIO_CMD_CLEAR_IRQ       0x02
#define RADIO_CMD_GET_IRQ         0x12
#define RADIO_CMD_SET_TX           0x83
#define RADIO_CMD_SET_RX           0x82

#define RADIO_PACKET_TYPE_LORA    0x01
#define RADIO_IRQ_TX_DONE         0x01
#define RADIO_IRQ_RX_DONE         0x02
#define RADIO_IRQ_CRC_ERR         0x04
#define RADIO_IRQ_TIMEOUT         0x80

/* ---- Interrupt vector helpers ---- */
#define NVIC_ISER0  (*(volatile uint32_t *)0xE000E100)
#define NVIC_ICPR0  (*(volatile uint32_t *)0xE000E280)

#define IRQ_RTC_WUT        3    /* RTC wake-up through EXTI line 19 */
#define IRQ_I2C1_EV        23
#define IRQ_SPI1           25
#define IRQ_ADC1           18
#define IRQ_EXTI_LINE_11   11   /* NFC IRQ */

static inline void nvic_enable_irq(uint32_t irq) {
    NVIC_ISER0 = (1 << (irq & 31));
}
static inline void nvic_clear_pending(uint32_t irq) {
    NVIC_ICPR0 = (1 << (irq & 31));
}

/* ---- System tick for coarse delay ---- */
#define SYST_CSR   (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR   (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018)
#define SYST_CSR_ENABLE (1 << 0)
#define SYST_CSR_CLKSRC  (1 << 2)
#define SYST_CSR_TICKINT (1 << 1)

#endif /* GRAINGUARD_REGISTERS_H */