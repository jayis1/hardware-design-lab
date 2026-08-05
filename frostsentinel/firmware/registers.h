/*
 * registers.h — STM32U575 peripheral register map (subset)
 *
 * Hand-written minimal register definitions for the peripherals used by
 * FrostSentinel.  Only the registers and bitfields actually touched by the
 * firmware are defined; this keeps the file small and auditable.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_REGISTERS_H
#define FROSTSENTINEL_REGISTERS_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Base addresses                                                     */
/* ------------------------------------------------------------------ */
#define RCC_BASE        0x46040C00u
#define PWR_BASE        0x46012800u
#define FLASH_BASE      0x40022000u
#define GPIOA_BASE      0x48020000u
#define GPIOB_BASE      0x48020400u
#define GPIOC_BASE      0x48020800u
#define GPIOD_BASE      0x48020C00u
#define GPIOH_BASE      0x48021C00u
#define I2C1_BASE       0x40005400u
#define I2C2_BASE       0x40005800u
#define SPI1_BASE       0x40013000u
#define SPI2_BASE       0x40003800u
#define USART2_BASE     0x40004400u
#define USART3_BASE     0x40004800u
#define ADC1_BASE       0x50040000u
#define TIM2_BASE       0x40000000u
#define TIM3_BASE       0x40000400u
#define TIM6_BASE       0x40000C00u
#define TIM7_BASE       0x40001000u
#define LPUART1_BASE    0x46002400u
#define AES1_BASE       0x500C1000u
#define DMA1_BASE       0x40020000u
#define DMAMUX1_BASE    0x40020800u
#define EXTI_BASE       0x46020000u

/* ------------------------------------------------------------------ */
/*  Generic register access macros                                     */
/* ------------------------------------------------------------------ */
#define REG32(addr)     (*(volatile uint32_t *)(addr))

#define CLR_BITS(reg, mask)  ((reg) = (reg) & ~(uint32_t)(mask))
#define SET_BITS(reg, mask)  ((reg) = (reg) |  (uint32_t)(mask))
#define SET_FIELD(reg, mask, val) \
    do { (reg) = ((reg) & ~(uint32_t)(mask)) | ((uint32_t)(val) & (mask)); } while (0)

/* ------------------------------------------------------------------ */
/*  RCC — Reset and Clock Control                                      */
/* ------------------------------------------------------------------ */
#define RCC_CR          REG32(RCC_BASE + 0x00)
#define RCC_CFGR        REG32(RCC_BASE + 0x10)
#define RCC_PLL1CFGR    REG32(RCC_BASE + 0x20)
#define RCC_PLL1CKSELR  REG32(RCC_BASE + 0x28)
#define RCC_AHB1ENR     REG32(RCC_BASE + 0x48)
#define RCC_AHB2ENR     REG32(RCC_BASE + 0x4C)
#define RCC_AHB3ENR     REG32(RCC_BASE + 0x50)
#define RCC_APB1ENR1    REG32(RCC_BASE + 0x58)
#define RCC_APB1ENR2    REG32(RCC_BASE + 0x5C)
#define RCC_APB2ENR     REG32(RCC_BASE + 0x60)
#define RCC_APB3ENR     REG32(RCC_BASE + 0x64)

/* AHB1 clock enable bits */
#define RCC_AHB1ENR_DMA1EN   (1u << 0)
#define RCC_AHB1ENR_DMAMUX1EN (1u << 2)
#define RCC_AHB1ENR_FLASHEN  (1u << 8)

/* AHB2 clock enable bits */
#define RCC_AHB2ENR_GPIOAEN  (1u << 0)
#define RCC_AHB2ENR_GPIOBEN  (1u << 1)
#define RCC_AHB2ENR_GPIOCEN  (1u << 2)
#define RCC_AHB2ENR_GPIODEN  (1u << 3)
#define RCC_AHB2ENR_GPIOHEN  (1u << 7)
#define RCC_AHB2ENR_AES1EN   (1u << 16)
#define RCC_AHB2ENR_ADC12EN  (1u << 24)

/* APB1 clock enable bits */
#define RCC_APB1ENR1_TIM2EN  (1u << 0)
#define RCC_APB1ENR1_TIM3EN  (1u << 1)
#define RCC_APB1ENR1_TIM6EN  (1u << 4)
#define RCC_APB1ENR1_TIM7EN  (1u << 5)
#define RCC_APB1ENR1_SPI2EN  (1u << 14)
#define RCC_APB1ENR1_USART2EN (1u << 17)
#define RCC_APB1ENR1_USART3EN (1u << 18)
#define RCC_APB1ENR1_I2C1EN  (1u << 21)
#define RCC_APB1ENR1_I2C2EN  (1u << 22)

/* APB2 clock enable bits */
#define RCC_APB2ENR_TIM1EN   (1u << 0)
#define RCC_APB2ENR_SPI1EN   (1u << 12)

/* APB3 clock enable bits */
#define RCC_APB3ENR_LPUART1EN (1u << 0)

/* ------------------------------------------------------------------ */
/*  PWR — Power Control                                                */
/* ------------------------------------------------------------------ */
#define PWR_CR1         REG32(PWR_BASE + 0x00)
#define PWR_CR3         REG32(PWR_BASE + 0x08)
#define PWR_SR1         REG32(PWR_BASE + 0x10)
#define PWR_SR2         REG32(PWR_BASE + 0x14)
#define PWR_DBPCR       REG32(PWR_BASE + 0x40)   /* Stop2 / Standby config */

#define PWR_CR1_LPMS_STOP2   (0x2u << 0)        /* LPMS = 010 -> Stop2 */

/* ------------------------------------------------------------------ */
/*  GPIO                                                               */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
    volatile uint32_t OTYPER;   /* 0x04 */
    volatile uint32_t OSPEEDR;  /* 0x08 */
    volatile uint32_t PUPDR;    /* 0x0C */
    volatile uint32_t IDR;      /* 0x10 */
    volatile uint32_t ODR;      /* 0x14 */
    volatile uint32_t BSRR;     /* 0x18 */
    volatile uint32_t LCKR;     /* 0x1C */
    volatile uint32_t AFRL;     /* 0x20 */
    volatile uint32_t AFRH;     /* 0x24 */
    volatile uint32_t BRR;      /* 0x28 */
} GPIO_TypeDef;

#define GPIOA  ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB  ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC  ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD  ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOH  ((GPIO_TypeDef *)GPIOH_BASE)

#define GPIO_MODE_INPUT   0u
#define GPIO_MODE_OUTPUT  1u
#define GPIO_MODE_AF      2u
#define GPIO_MODE_ANALOG  3u

#define GPIO_OTYPE_PP     0u
#define GPIO_OTYPE_OD     1u

#define GPIO_SPEED_LOW    0u
#define GPIO_SPEED_MID    1u
#define GPIO_SPEED_HIGH   2u
#define GPIO_SPEED_VHIGH  3u

#define GPIO_PUPD_NONE    0u
#define GPIO_PUPD_UP      1u
#define GPIO_PUPD_DOWN    2u

/* Helper: configure one GPIO pin in one line */
#define GPIO_CONFIG(port, pin, mode, otype, speed, pupd, af) \
    do { \
        (port)->MODER   = ((port)->MODER   & ~(3u << (2*(pin)))) | ((mode) << (2*(pin))); \
        (port)->OTYPER  = ((port)->OTYPER  & ~(1u << (pin)))    | ((otype) << (pin)); \
        (port)->OSPEEDR = ((port)->OSPEEDR & ~(3u << (2*(pin)))) | ((speed) << (2*(pin))); \
        (port)->PUPDR   = ((port)->PUPDR   & ~(3u << (2*(pin)))) | ((pupd)  << (2*(pin))); \
        if ((pin) < 8) (port)->AFRL = ((port)->AFRL & ~(0xFu << (4*(pin)))) | ((af) << (4*(pin))); \
        else           (port)->AFRH = ((port)->AFRH & ~(0xFu << (4*((pin)-8)))) | ((af) << (4*((pin)-8))); \
    } while (0)

/* ------------------------------------------------------------------ */
/*  I2C (master, 7-bit addressing)                                     */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t OAR1;     /* 0x08 */
    volatile uint32_t OAR2;     /* 0x0C */
    volatile uint32_t TIMINGR;  /* 0x10 */
    volatile uint32_t TIMEOUTR; /* 0x14 */
    volatile uint32_t ISR;      /* 0x18 */
    volatile uint32_t PECR;     /* 0x1C */
    volatile uint32_t RXDR;     /* 0x20 */
    volatile uint32_t TXDR;     /* 0x24 */
} I2C_TypeDef;

#define I2C1  ((I2C_TypeDef *)I2C1_BASE)
#define I2C2  ((I2C_TypeDef *)I2C2_BASE)

#define I2C_CR1_PE          (1u << 0)
#define I2C_CR1_TXIE        (1u << 1)
#define I2C_CR1_RXIE        (1u << 2)
#define I2C_CR1_STOPIE      (1u << 4)
#define I2C_CR1_NACKIE      (1u << 3)

#define I2C_CR2_START       (1u << 13)
#define I2C_CR2_STOP        (1u << 14)
#define I2C_CR2_RD_WRN      (1u << 10)
#define I2C_CR2_AUTOEND     (1u << 15)
#define I2C_CR2_NBYTES_MASK (0xFFu << 16)

#define I2C_ISR_TXE         (1u << 0)
#define I2C_ISR_RXNE        (1u << 2)
#define I2C_ISR_TC          (1u << 6)
#define I2C_ISR_STOPF       (1u << 5)
#define I2C_ISR_NACKF       (1u << 4)
#define I2C_ISR_BUSY        (1u << 15)

/* ------------------------------------------------------------------ */
/*  SPI (master)                                                       */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t SR;       /* 0x08 */
    volatile uint32_t DR;       /* 0x0C */
    volatile uint32_t CRCPR;    /* 0x10 */
    volatile uint32_t RXCRCR;   /* 0x14 */
    volatile uint32_t TXCRCR;   /* 0x18 */
} SPI_TypeDef;

#define SPI1  ((SPI_TypeDef *)SPI1_BASE)
#define SPI2  ((SPI_TypeDef *)SPI2_BASE)

#define SPI_CR1_SPE         (1u << 0)
#define SPI_CR1_MSTR        (1u << 2)
#define SPI_CR1_BR_DIV256   (7u << 3)
#define SPI_CR1_BR_DIV16    (2u << 3)
#define SPI_CR1_CPHA        (1u << 0)
#define SPI_CR1_CPOL        (1u << 1)
#define SPI_CR1_LSBFIRST    (1u << 7)
#define SPI_CR1_SSI         (1u << 8)
#define SPI_CR1_SSM         (1u << 9)

#define SPI_CR2_DS_8BIT     (7u << 8)
#define SPI_CR2_FRXTH       (1u << 2)
#define SPI_CR2_SSOE        (1u << 2)

#define SPI_SR_RXNE         (1u << 0)
#define SPI_SR_TXE          (1u << 1)
#define SPI_SR_BSY          (1u << 7)
#define SPI_SR_MODF         (1u << 4)
#define SPI_SR_OVR          (1u << 6)

/* ------------------------------------------------------------------ */
/*  USART / LPUART                                                     */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t CR3;      /* 0x08 */
    volatile uint32_t BRR;      /* 0x0C */
    volatile uint32_t GTPR;     /* 0x10 */
    volatile uint32_t RTOR;     /* 0x14 */
    volatile uint32_t RQR;      /* 0x18 */
    volatile uint32_t ISR;      /* 0x1C */
    volatile uint32_t ICR;      /* 0x20 */
    volatile uint32_t RDR;      /* 0x24 */
    volatile uint32_t TDR;      /* 0x28 */
} USART_TypeDef;

#define USART2 ((USART_TypeDef *)USART2_BASE)
#define USART3 ((USART_TypeDef *)USART3_BASE)
#define LPUART1 ((USART_TypeDef *)LPUART1_BASE)

#define USART_CR1_UE        (1u << 0)
#define USART_CR1_RE        (1u << 2)
#define USART_CR1_TE        (1u << 3)
#define USART_CR1_RXNEIE    (1u << 5)
#define USART_CR1_TCIE      (1u << 6)
#define USART_ISR_RXNE      (1u << 5)
#define USART_ISR_TXE       (1u << 7)
#define USART_ISR_TC        (1u << 6)

/* ------------------------------------------------------------------ */
/*  ADC (12-bit fast mode for acoustic emission)                       */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t ISR;      /* 0x00 */
    volatile uint32_t IER;      /* 0x04 */
    volatile uint32_t CR;       /* 0x08 */
    volatile uint32_t CFGR;     /* 0x0C */
    volatile uint32_t CFGR2;    /* 0x10 */
    volatile uint32_t SMPR1;    /* 0x14 */
    volatile uint32_t SMPR2;    /* 0x18 */
    volatile uint32_t TR1;      /* 0x20 */
    volatile uint32_t TR2;      /* 0x24 */
    volatile uint32_t TR3;      /* 0x28 */
    volatile uint32_t DR1;      /* 0x40 */
    volatile uint32_t DR2;      /* 0x44 */
} ADC_TypeDef;

#define ADC1  ((ADC_TypeDef *)ADC1_BASE)

#define ADC_ISR_ADRDY       (1u << 0)
#define ADC_ISR_EOC         (1u << 2)
#define ADC_ISR_AWD1        (1u << 7)
#define ADC_CR_ADEN         (1u << 0)
#define ADC_CR_ADSTART      (1u << 2)
#define ADC_CR_ADDIS        (1u << 1)
#define ADC_CR_ADVREGEN     (1u << 28)
#define ADC_CFGR_CONT       (1u << 13)
#define ADC_CFGR_DMAEN      (1u << 8)
#define ADC_CFGR_RES_12BIT  (0u << 3)

/* ------------------------------------------------------------------ */
/*  TIMERS                                                             */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t SMCR;     /* 0x08 */
    volatile uint32_t DIER;     /* 0x0C */
    volatile uint32_t SR;       /* 0x10 */
    volatile uint32_t EGR;      /* 0x14 */
    volatile uint32_t CCMR1;    /* 0x18 */
    volatile uint32_t CCMR2;    /* 0x1C */
    volatile uint32_t CCER;     /* 0x20 */
    volatile uint32_t CNT;      /* 0x24 */
    volatile uint32_t PSC;      /* 0x28 */
    volatile uint32_t ARR;      /* 0x2C */
    volatile uint32_t CCR1;     /* 0x30 */
    volatile uint32_t CCR2;     /* 0x34 */
    volatile uint32_t CCR3;     /* 0x38 */
    volatile uint32_t CCR4;     /* 0x3C */
} TIM_TypeDef;

#define TIM2  ((TIM_TypeDef *)TIM2_BASE)
#define TIM3  ((TIM_TypeDef *)TIM3_BASE)
#define TIM6  ((TIM_TypeDef *)TIM6_BASE)
#define TIM7  ((TIM_TypeDef *)TIM7_BASE)

#define TIM_CR1_CEN         (1u << 0)
#define TIM_CR1_ARPE        (1u << 7)
#define TIM_DIER_UIE        (1u << 0)
#define TIM_DIER_CC1IE      (1u << 1)
#define TIM_SR_UIF          (1u << 0)
#define TIM_SR_CC1IF        (1u << 1)
#define TIM_CCMR1_IC1_INPUT  (1u << 0)

/* ------------------------------------------------------------------ */
/*  AES-256 hardware core (for mesh packet authentication)             */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t SR;       /* 0x04 */
    volatile uint32_t DINR;     /* 0x08 */
    volatile uint32_t DOUTR;    /* 0x0C */
    volatile uint32_t KEYR0;    /* 0x10 - 0x1C (8 regs for 256-bit) */
    volatile uint32_t KEYR1;
    volatile uint32_t KEYR2;
    volatile uint32_t KEYR3;
    volatile uint32_t KEYR4;
    volatile uint32_t KEYR5;
    volatile uint32_t KEYR6;
    volatile uint32_t KEYR7;
    volatile uint32_t IVR0;     /* 0x20 - 0x2C (4 regs for 128-bit IV) */
    volatile uint32_t IVR1;
    volatile uint32_t IVR2;
    volatile uint32_t IVR3;
} AES_TypeDef;

#define AES1  ((AES_TypeDef *)AES1_BASE)

#define AES_CR_EN           (1u << 0)
#define AES_CR_GCMPH        (0x3u << 13)   /* GCM phase mask */
#define AES_CR_MODE_GCM     (0xCu << 0)    /* GCM mode */
#define AES_CR_DATATYPE_8B  (1u << 1)
#define AES_CR_KEYSIZE_256  (0u << 2)
#define AES_SR_BUSY         (1u << 0)
#define AES_SR_WRERR        (1u << 2)
#define AES_SR_CCF          (1u << 7)      /* computation complete flag */

/* ------------------------------------------------------------------ */
/*  DMA (channel-level, simplified)                                    */
/* ------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CCR;      /* 0x00 + 0x14*ch */
    volatile uint32_t CNDTR;
    volatile uint32_t CPAR;
    volatile uint32_t CMAR;
    volatile uint32_t RESERVED;
} DMA_Channel_TypeDef;

#define DMA1_CH1 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x08))
#define DMA1_CH2 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x1C))
#define DMA1_CH3 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x30))

#define DMA_CCR_EN          (1u << 0)
#define DMA_CCR_TCIE        (1u << 1)
#define DMA_CCR_MINC        (1u << 10)
#define DMA_CCR_PINC        (1u << 9)
#define DMA_CCR_CIRC        (1u << 8)
#define DMA_CCR_DIR_P2M     (0u << 4)
#define DMA_CCR_DIR_M2P     (1u << 4)
#define DMA_CCR_PSIZE_16B   (1u << 9)
#define DMA_CCR_MSIZE_16B   (1u << 11)

/* ------------------------------------------------------------------ */
/*  EXTI (for RTC alarm wake and radio DIO interrupts)                 */
/* ------------------------------------------------------------------ */
#define EXTI_RTSR1       REG32(EXTI_BASE + 0x00)
#define EXTI_FTSR1       REG32(EXTI_BASE + 0x04)
#define EXTI_C1IMR1      REG32(EXTI_BASE + 0x80)
#define EXTI_C1EMR1      REG32(EXTI_BASE + 0x84)
#define EXTI_C1PR1       REG32(EXTI_BASE + 0x88)

/* ------------------------------------------------------------------ */
/*  Interrupt vector table offsets (used by startup.s)                 */
/* ------------------------------------------------------------------ */
#define IRQ_RTC_ALARM        42u
#define IRQ_TIM6_DAC         54u
#define IRQ_TIM7             55u
#define IRQ_DMA1_CH1         11u
#define IRQ_ADC1             37u
#define IRQ_USART2           38u
#define IRQ_USART3           39u
#define IRQ_LPUART1          91u
#define IRQ_AES1             78u

#endif /* FROSTSENTINEL_REGISTERS_H */