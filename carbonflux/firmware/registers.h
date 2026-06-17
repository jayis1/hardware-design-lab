/**
 * @file    registers.h
 * @brief   CarbonFlux MMIO register definitions and bitfield unions.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * Peripheral register map for the STM32U5A9ZG. Only peripherals used by
 * CarbonFlux are defined here. Each definition uses volatile structs with
 * bitfield unions for register-level access.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* ======================================================================== */
/*  BASE ADDRESSES (STM32U5A9ZG memory map)                                */
/* ======================================================================== */

/* AHB/APB peripheral base addresses */
#define PERIPH_BASE             0x40000000UL
#define AHB1PERIPH_BASE         (PERIPH_BASE + 0x00020000UL)
#define APB1PERIPH_BASE         PERIPH_BASE
#define APB2PERIPH_BASE         (PERIPH_BASE + 0x00010000UL)

/* GPIO base addresses */
#define GPIOA_BASE              (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE              (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE              (AHB1PERIPH_BASE + 0x0800UL)

/* RCC (Reset and Clock Control) */
#define RCC_BASE                (AHB1PERIPH_BASE + 0x4400UL)

/* I²C1 */
#define I2C1_BASE               (APB1PERIPH_BASE + 0x5400UL)

/* SPI1 */
#define SPI1_BASE               (APB2PERIPH_BASE + 0x3000UL)

/* SPI2 */
#define SPI2_BASE               (APB1PERIPH_BASE + 0x3800UL)

/* USART2 */
#define USART2_BASE             (APB1PERIPH_BASE + 0x4400UL)

/* TIM2 */
#define TIM2_BASE               (APB1PERIPH_BASE + 0x0000UL)

/* ADC1 */
#define ADC1_BASE               (APB2PERIPH_BASE + 0x5000UL)

/* SYSCFG */
#define SYSCFG_BASE             (APB2PERIPH_BASE + 0x0000UL)

/* EXTI */
#define EXTI_BASE               (APB2PERIPH_BASE + 0x0400UL)

/* CRC */
#define CRC_BASE                (AHB1PERIPH_BASE + 0x3000UL)

/* PWR (Power Control) */
#define PWR_BASE                (PERIPH_BASE + 0x7000UL)

/* IWDG */
#define IWDG_BASE               (PERIPH_BASE + 0x8000UL)

/* WWDG */
#define WWDG_BASE               (PERIPH_BASE + 0x8100UL)

/* ======================================================================== */
/*  GPIO REGISTER STRUCT                                                     */
/* ======================================================================== */

typedef struct {
    volatile uint32_t MODER;        /* 0x00 — Mode register              */
    volatile uint32_t OTYPER;       /* 0x04 — Output type register       */
    volatile uint32_t OSPEEDR;      /* 0x08 — Output speed register      */
    volatile uint32_t PUPDR;        /* 0x0C — Pull-up/pull-down register */
    volatile uint32_t IDR;          /* 0x10 — Input data register        */
    volatile uint32_t ODR;          /* 0x14 — Output data register       */
    volatile uint32_t BSRR;         /* 0x18 — Bit set/reset register     */
    volatile uint32_t LCKR;         /* 0x1C — Lock register              */
    volatile uint32_t AFRL;         /* 0x20 — Alternate function low     */
    volatile uint32_t AFRH;         /* 0x24 — Alternate function high    */
    volatile uint32_t BRR;          /* 0x28 — Bit reset register         */
} GPIO_TypeDef;

/* GPIO mode bits */
#define GPIO_MODE_INPUT         0
#define GPIO_MODE_OUTPUT        1
#define GPIO_MODE_AF            2
#define GPIO_MODE_ANALOG        3

/* GPIO output type */
#define GPIO_OTYPE_PP           0   /* Push-pull */
#define GPIO_OTYPE_OD           1   /* Open-drain */

/* GPIO speed */
#define GPIO_SPEED_LOW          0
#define GPIO_SPEED_MEDIUM       1
#define GPIO_SPEED_HIGH         2
#define GPIO_SPEED_VERY_HIGH    3

/* GPIO pull-up/pull-down */
#define GPIO_PUPD_NONE          0
#define GPIO_PUPD_PULLUP        1
#define GPIO_PUPD_PULLDOWN      2

/* Convenience pin macros */
#define GPIO_PA0                ((GPIO_TypeDef *)GPIOA_BASE), 0
#define GPIO_PA1                ((GPIO_TypeDef *)GPIOA_BASE), 1
#define GPIO_PA2                ((GPIO_TypeDef *)GPIOA_BASE), 2
#define GPIO_PA3                ((GPIO_TypeDef *)GPIOA_BASE), 3
#define GPIO_PA4                ((GPIO_TypeDef *)GPIOA_BASE), 4
#define GPIO_PA5                ((GPIO_TypeDef *)GPIOA_BASE), 5
#define GPIO_PA6                ((GPIO_TypeDef *)GPIOA_BASE), 6
#define GPIO_PA7                ((GPIO_TypeDef *)GPIOA_BASE), 7
#define GPIO_PA8                ((GPIO_TypeDef *)GPIOA_BASE), 8
#define GPIO_PA9                ((GPIO_TypeDef *)GPIOA_BASE), 9
#define GPIO_PA10               ((GPIO_TypeDef *)GPIOA_BASE), 10
#define GPIO_PA11               ((GPIO_TypeDef *)GPIOA_BASE), 11
#define GPIO_PA12               ((GPIO_TypeDef *)GPIOA_BASE), 12
#define GPIO_PA13               ((GPIO_TypeDef *)GPIOA_BASE), 13
#define GPIO_PA14               ((GPIO_TypeDef *)GPIOA_BASE), 14
#define GPIO_PA15               ((GPIO_TypeDef *)GPIOA_BASE), 15
#define GPIO_PB0                ((GPIO_TypeDef *)GPIOB_BASE), 0
#define GPIO_PB1                ((GPIO_TypeDef *)GPIOB_BASE), 1
#define GPIO_PB2                ((GPIO_TypeDef *)GPIOB_BASE), 2
#define GPIO_PB3                ((GPIO_TypeDef *)GPIOB_BASE), 3
#define GPIO_PB4                ((GPIO_TypeDef *)GPIOB_BASE), 4
#define GPIO_PB5                ((GPIO_TypeDef *)GPIOB_BASE), 5
#define GPIO_PB6                ((GPIO_TypeDef *)GPIOB_BASE), 6
#define GPIO_PB7                ((GPIO_TypeDef *)GPIOB_BASE), 7
#define GPIO_PB8                ((GPIO_TypeDef *)GPIOB_BASE), 8
#define GPIO_PB9                ((GPIO_TypeDef *)GPIOB_BASE), 9
#define GPIO_PC0                ((GPIO_TypeDef *)GPIOC_BASE), 0
#define GPIO_PC1                ((GPIO_TypeDef *)GPIOC_BASE), 1
#define GPIO_PC2                ((GPIO_TypeDef *)GPIOC_BASE), 2
#define GPIO_PC3                ((GPIO_TypeDef *)GPIOC_BASE), 3
#define GPIO_PC4                ((GPIO_TypeDef *)GPIOC_BASE), 4
#define GPIO_PC5                ((GPIO_TypeDef *)GPIOC_BASE), 5
#define GPIO_PC6                ((GPIO_TypeDef *)GPIOC_BASE), 6
#define GPIO_PC7                ((GPIO_TypeDef *)GPIOC_BASE), 7
#define GPIO_PC8                ((GPIO_TypeDef *)GPIOC_BASE), 8
#define GPIO_PC9                ((GPIO_TypeDef *)GPIOC_BASE), 9
#define GPIO_PC10               ((GPIO_TypeDef *)GPIOC_BASE), 10
#define GPIO_PC11               ((GPIO_TypeDef *)GPIOC_BASE), 11

/* GPIO bit set/reset macros */
#define GPIO_SET_PIN(gpio, pin)     ((gpio)->BSRR = (1UL << (pin)))
#define GPIO_RESET_PIN(gpio, pin)   ((gpio)->BRR  = (1UL << (pin)))
#define GPIO_READ_PIN(gpio, pin)    (((gpio)->IDR >> (pin)) & 1UL)

/* ======================================================================== */
/*  RCC REGISTER STRUCT                                                      */
/* ======================================================================== */

typedef struct {
    volatile uint32_t CR;            /* 0x00 — Clock control register      */
    volatile uint32_t ICSCR;         /* 0x04 — Internal clock sources cal  */
    uint32_t         RESERVED0[1];
    volatile uint32_t CFGR;          /* 0x0C — Clock configuration reg     */
    uint32_t         RESERVED1[2];
    volatile uint32_t PLL1CFGR;      /* 0x18 — PLL1 configuration          */
    uint32_t         RESERVED2[1];
    volatile uint32_t PLL1DIVR;      /* 0x20 — PLL1 dividers               */
    volatile uint32_t PLL1FRACR;     /* 0x24 — PLL1 fractional control     */
    uint32_t         RESERVED3[6];
    volatile uint32_t CRRCR;         /* 0x40 — Clock recovery RC register  */
    uint32_t         RESERVED4[7];
    volatile uint32_t CIER;          /* 0x60 — Clock interrupt enable reg  */
    volatile uint32_t CIFR;          /* 0x64 — Clock interrupt flag reg    */
    volatile uint32_t CICR;          /* 0x68 — Clock interrupt clear reg   */
    uint32_t         RESERVED5[21];
    volatile uint32_t AHB1ENR;       /* 0xE0 — AHB1 peripheral clock enable*/
    volatile uint32_t AHB2ENR;       /* 0xE4 — AHB2 peripheral clock enable*/
    volatile uint32_t AHB3ENR;       /* 0xE8 — AHB3 peripheral clock enable*/
    uint32_t         RESERVED6[1];
    volatile uint32_t APB1ENR1;      /* 0xF0 — APB1 peripheral clock en 1  */
    volatile uint32_t APB1ENR2;      /* 0xF4 — APB1 peripheral clock en 2  */
    volatile uint32_t APB2ENR;       /* 0xF8 — APB2 peripheral clock en    */
    uint32_t         RESERVED7[1];
    volatile uint32_t AHB1SMENR;     /* 0x100 — AHB1 sleep mode enable     */
    volatile uint32_t AHB2SMENR;     /* 0x104 — AHB2 sleep mode enable     */
    uint32_t         RESERVED8[2];
    volatile uint32_t APB1SMENR1;    /* 0x110 — APB1 sleep mode enable 1   */
    volatile uint32_t APB1SMENR2;    /* 0x114 — APB1 sleep mode enable 2   */
    volatile uint32_t APB2SMENR;     /* 0x118 — APB2 sleep mode enable     */
    uint32_t         RESERVED9[1];
    volatile uint32_t AHB1RSTR;      /* 0x120 — AHB1 peripheral reset      */
    uint32_t         RESERVED10[2];
    volatile uint32_t APB1RSTR1;     /* 0x12C — APB1 peripheral reset 1    */
    volatile uint32_t APB1RSTR2;     /* 0x130 — APB1 peripheral reset 2    */
    volatile uint32_t APB2RSTR;      /* 0x134 — APB2 peripheral reset      */
} RCC_TypeDef;

/* RCC register bit definitions */
#define RCC_CR_HSI16ON           (1U << 0)
#define RCC_CR_HSI16RDYF        (1U << 1)
#define RCC_CR_MSION            (1U << 8)
#define RCC_CR_MSIRDYF          (1U << 9)
#define RCC_CR_HSEON            (1U << 16)
#define RCC_CR_HSERDYF          (1U << 17)
#define RCC_CR_PLL1ON           (1U << 24)
#define RCC_CR_PLL1RDYF         (1U << 25)
#define RCC_CR_LSEON            (1U << 9)  /* In BDCR, not CR */
#define RCC_CR_LSERDYF          (1U << 10) /* In BDCR */

/* PLL1CFGR bits */
#define RCC_PLL1CFGR_PLL1SRC_HSI16  (0UL << 0)
#define RCC_PLL1CFGR_PLL1SRC_HSE    (1UL << 0)
#define RCC_PLL1CFGR_PLL1P_EN      (1U << 16)
#define RCC_PLL1CFGR_PLL1Q_EN      (1U << 20)
#define RCC_PLL1CFGR_PLL1R_EN      (1U << 24)

/* ======================================================================== */
/*  I²C REGISTER STRUCT                                                     */
/* ======================================================================== */

typedef struct {
    volatile uint32_t CR1;           /* 0x00 — Control register 1          */
    volatile uint32_t CR2;           /* 0x04 — Control register 2          */
    volatile uint32_t OAR1;          /* 0x08 — Own address register 1      */
    volatile uint32_t OAR2;          /* 0x0C — Own address register 2      */
    volatile uint32_t TIMINGR;       /* 0x10 — Timing register             */
    volatile uint32_t TIMEOUTR;      /* 0x14 — Timeout register            */
    volatile uint32_t ISR;           /* 0x18 — Interrupt and status reg    */
    volatile uint32_t ICR;           /* 0x1C — Interrupt clear register    */
    volatile uint32_t PECR;          /* 0x20 — PEC register                */
    volatile uint32_t RXDR;          /* 0x24 — Receive data register       */
    volatile uint32_t TXDR;          /* 0x28 — Transmit data register      */
} I2C_TypeDef;

/* I²C register bit definitions */
#define I2C_CR1_PE               (1U << 0)   /* Peripheral enable           */
#define I2C_CR1_TXIE            (1U << 1)   /* TX interrupt enable          */
#define I2C_CR1_RXIE            (1U << 2)   /* RX interrupt enable          */
#define I2C_CR1_NACKIE          (1U << 8)   /* NACK interrupt enable        */
#define I2C_CR1_STOPIE          (1U << 9)   /* STOP detection IE            */
#define I2C_CR1_ERRIE           (1U << 10)  /* Error interrupt enable       */
#define I2C_CR1_TCIE            (1U << 11)  /* Transfer complete IE         */
#define I2C_CR1_ANFOFF          (1U << 12)  /* Analog noise filter off      */
#define I2C_CR1_DNF             (3U << 8)   /* Digital noise filter         */

#define I2C_CR2_SADD_Pos        (0)
#define I2C_CR2_SADD_Msk        (0x3FFUL << I2C_CR2_SADD_Pos)
#define I2C_CR2_RD_WRN          (1U << 10)  /* Read/write direction         */
#define I2C_CR2_NBYTES_Pos      (16)
#define I2C_CR2_NBYTES_Msk      (0xFFUL << I2C_CR2_NBYTES_Pos)
#define I2C_CR2_START           (1U << 13)  /* Generate START condition     */
#define I2C_CR2_STOP            (1U << 14)  /* Generate STOP condition      */
#define I2C_CR2_AUTOEND         (1U << 25)  /* Automatic STOP mode          */
#define I2C_CR2_RELOAD          (1U << 24)  /* NBYTES reload mode           */

#define I2C_ISR_TXE             (1U << 0)   /* TX data register empty       */
#define I2C_ISR_RXNE            (1U << 2)   /* RX data register not empty   */
#define I2C_ISR_STOPF           (1U << 5)   /* STOP detection flag          */
#define I2C_ISR_NACKF           (1U << 6)   /* NACK received flag           */
#define I2C_ISR_BUSY            (1U << 14)  /* Bus busy                     */
#define I2C_ISR_TC              (1U << 6)   /* Transfer complete (rel=0)    */
#define I2C_ISR_TCR             (1U << 7)   /* Transfer complete (rel=1)    */

/* ======================================================================== */
/*  SPI REGISTER STRUCT                                                     */
/* ======================================================================== */

typedef struct {
    volatile uint32_t CR1;           /* 0x00 — Control register 1          */
    volatile uint32_t CR2;           /* 0x04 — Control register 2          */
    volatile uint32_t CFG1;          /* 0x08 — Configuration register 1    */
    volatile uint32_t CFG2;          /* 0x0C — Configuration register 2    */
    volatile uint32_t IER;           /* 0x10 — Interrupt enable register   */
    volatile uint32_t SR;            /* 0x14 — Status register             */
    volatile uint32_t IFCR;          /* 0x18 — Interrupt flag clear reg    */
    volatile uint32_t TXDR;          /* 0x20 — Transmit data register      */
    volatile uint32_t RXDR;          /* 0x30 — Receive data register       */
    volatile uint32_t CRCPOLY;       /* 0x34 — CRC polynomial register     */
    volatile uint32_t TXCRC;         /* 0x38 — TX CRC register             */
    volatile uint32_t RXCRC;         /* 0x3C — RX CRC register             */
    volatile uint32_t UDRDR;         /* 0x40 — Underrun data register      */
    volatile uint32_t I2SCFGR;       /* 0x50 — I2S configuration register  */
    volatile uint32_t I2SCFGR2;      /* 0x54 — I2S configuration 2         */
} SPI_TypeDef;

/* SPI register bit definitions */
#define SPI_CR1_SPE             (1U << 0)   /* SPI enable                  */
#define SPI_CR1_MSTR            (1U << 2)   /* Master selection            */
#define SPI_CR1_SSI             (1U << 8)   /* Internal SS control         */
#define SPI_CR1_SSM             (1U << 9)   /* Software SS management      */

#define SPI_CR2_TSIZE_Pos       (0)
#define SPI_CR2_TSIZE_Msk       (0xFFFFUL << SPI_CR2_TSIZE_Pos)

#define SPI_CFG1_MBR_Pos        (28)        /* Master baud rate divisor    */
#define SPI_CFG1_MBR_Msk        (0x7UL << SPI_CFG1_MBR_Pos)
#define SPI_CFG1_DSIZE_Pos      (0)         /* Data size                  */
#define SPI_CFG1_DSIZE_Msk      (0x1FUL << SPI_CFG1_DSIZE_Pos)

#define SPI_CFG2_CPOL           (1U << 2)   /* Clock polarity             */
#define SPI_CFG2_CPHA           (1U << 3)   /* Clock phase                */
#define SPI_CFG2_SSM            (1U << 6)   /* Software slave management  */
#define SPI_CFG2_SSIOP          (1U << 4)   /* SSI output enable          */

#define SPI_SR_TXE              (1U << 0)   /* TX buffer empty            */
#define SPI_SR_RXP              (1U << 1)   /* RX buffer not empty (packed)*/
#define SPI_SR_RXWNE            (1U << 2)   /* RX word not empty          */
#define SPI_SR_EOT              (1U << 3)   /* End of transfer            */
#define SPI_SR_TXTF             (1U << 5)   /* TX transfer finished       */
#define SPI_SR_BSY              (1U << 6)   /* Busy flag                  */
#define SPI_SR_FRLVL_Pos        (8)         /* FIFO RX level              */
#define SPI_SR_FRLVL_Msk        (3UL << SPI_SR_FRLVL_Pos)

/* ======================================================================== */
/*  USART REGISTER STRUCT                                                   */
/* ======================================================================== */

typedef struct {
    volatile uint32_t CR1;           /* 0x00 — Control register 1          */
    volatile uint32_t CR2;           /* 0x04 — Control register 2          */
    volatile uint32_t CR3;           /* 0x08 — Control register 3          */
    volatile uint32_t BRR;           /* 0x0C — Baud rate register          */
    volatile uint32_t RTOR;          /* 0x10 — Receiver timeout register   */
    volatile uint32_t RQR;           /* 0x18 — Request register            */
    volatile uint32_t ISR;           /* 0x1C — Interrupt and status reg    */
    volatile uint32_t ICR;           /* 0x20 — Interrupt flag clear reg    */
    volatile uint32_t RDR;           /* 0x24 — Receive data register       */
    volatile uint32_t TDR;           /* 0x28 — Transmit data register      */
} USART_TypeDef;

/* USART register bit definitions */
#define USART_CR1_UE            (1U << 0)   /* USART enable                */
#define USART_CR1_TE            (1U << 3)   /* Transmitter enable          */
#define USART_CR1_RE            (1U << 2)   /* Receiver enable             */
#define USART_CR1_M0            (1U << 12)  /* Word length bit 0          */
#define USART_CR1_OVER8         (1U << 15)  /* Oversampling by 8          */
#define USART_CR1_FIFOEN        (1U << 29)  /* FIFO enable                */

#define USART_CR2_STOP1         (1U << 12)  /* Stop bits: 1 stop bit      */

#define USART_ISR_TXE           (1U << 7)   /* TX data register empty     */
#define USART_ISR_TC            (1U << 6)   /* Transmission complete      */
#define USART_ISR_RXNE          (1U << 5)   /* RX data register not empty */
#define USART_ISR_ORE           (1U << 3)   /* Overrun error              */
#define USART_ISR_FE            (1U << 1)   /* Framing error              */

/* ======================================================================== */
/*  TIMER (TIM2) REGISTER STRUCT                                            */
/* ======================================================================== */

typedef struct {
    volatile uint32_t CR1;           /* 0x00 — Control register 1          */
    volatile uint32_t CR2;           /* 0x04 — Control register 2          */
    volatile uint32_t SMCR;          /* 0x08 — Slave mode control reg      */
    volatile uint32_t DIER;          /* 0x0C — DMA/Interrupt enable reg    */
    volatile uint32_t SR;            /* 0x10 — Status register             */
    volatile uint32_t EGR;           /* 0x14 — Event generation register   */
    volatile uint32_t CCMR1;         /* 0x18 — Capture/compare mode 1      */
    volatile uint32_t CCMR2;         /* 0x1C — Capture/compare mode 2      */
    volatile uint32_t CCER;          /* 0x20 — Capture/compare enable reg  */
    volatile uint32_t CNT;           /* 0x24 — Counter                     */
    volatile uint32_t PSC;           /* 0x28 — Prescaler                   */
    volatile uint32_t ARR;           /* 0x2C — Auto-reload register        */
    volatile uint32_t CCR1;          /* 0x34 — Capture/compare register 1  */
    volatile uint32_t CCR2;          /* 0x38 — Capture/compare register 2  */
    volatile uint32_t CCR3;          /* 0x3C — Capture/compare register 3  */
    volatile uint32_t CCR4;          /* 0x40 — Capture/compare register 4  */
    volatile uint32_t BDTR;          /* 0x44 — Break and dead-time reg     */
    volatile uint32_t CCMR3;         /* 0x48 — Capture/compare mode reg 3  */
    volatile uint32_t DCR;           /* 0x5C — DMA control register        */
    volatile uint32_t DMAR;          /* 0x60 — DMA address for burst       */
} TIM_TypeDef;

/* Timer register bit definitions */
#define TIM_CR1_CEN             (1U << 0)   /* Counter enable              */
#define TIM_CR1_UDIS            (1U << 1)   /* Update disable              */
#define TIM_CR1_URS             (1U << 2)   /* Update request source       */
#define TIM_CR1_OPM             (1U << 3)   /* One-pulse mode              */
#define TIM_CR1_ARPE            (1U << 7)   /* Auto-reload preload enable  */

#define TIM_CCMR1_OC1M_Pos      (4)         /* OC1 mode                    */
#define TIM_CCMR1_OC1M_Msk      (7UL << TIM_CCMR1_OC1M_Pos)
#define TIM_CCMR1_OC1PE         (1U << 3)   /* OC1 preload enable          */

#define TIM_CCER_CC1E           (1U << 0)   /* CC1 output enable           */
#define TIM_CCER_CC1P           (1U << 1)   /* CC1 output polarity         */

/* PWM mode 1: OC1M = 110 (PWM mode 1) — CNT < CCR1 → active */
#define TIM_OCM_PWM1            6

/* ======================================================================== */
/*  ADC REGISTER STRUCT                                                     */
/* ======================================================================== */

typedef struct {
    volatile uint32_t ISR;           /* 0x00 — Interrupt and status reg    */
    volatile uint32_t IER;           /* 0x04 — Interrupt enable register   */
    volatile uint32_t CR;            /* 0x08 — Control register            */
    volatile uint32_t CFGR;          /* 0x0C — Configuration register      */
    volatile uint32_t CFGR2;         /* 0x10 — Configuration register 2    */
    volatile uint32_t SMPR1;         /* 0x14 — Sampling time register 1    */
    volatile uint32_t SMPR2;         /* 0x18 — Sampling time register 2    */
    volatile uint32_t TR1;           /* 0x20 — Watchdog threshold 1        */
    volatile uint32_t TR2;           /* 0x24 — Watchdog threshold 2        */
    volatile uint32_t TR3;           /* 0x28 — Watchdog threshold 3        */
    volatile uint32_t SQR1;          /* 0x30 — Regular sequence register 1 */
    volatile uint32_t SQR2;          /* 0x34 — Regular sequence register 2 */
    volatile uint32_t SQR3;          /* 0x38 — Regular sequence register 3 */
    volatile uint32_t SQR4;          /* 0x3C — Regular sequence register 4 */
    volatile uint32_t DR;            /* 0x40 — Regular data register       */
    volatile uint32_t JSQR;          /* 0x4C — Injected sequence reg       */
    volatile uint32_t OFR1;          /* 0x60 — Offset correction reg 1     */
    volatile uint32_t OFR2;          /* 0x64 — Offset correction reg 2     */
    volatile uint32_t OFR3;          /* 0x68 — Offset correction reg 3     */
    volatile uint32_t OFR4;          /* 0x6C — Offset correction reg 4     */
    volatile uint32_t JDR1;          /* 0x80 — Injected data register 1    */
    volatile uint32_t JDR2;          /* 0x84 — Injected data register 2    */
    volatile uint32_t JDR3;          /* 0x88 — Injected data register 3    */
    volatile uint32_t JDR4;          /* 0x8C — Injected data register 4    */
    volatile uint32_t AWD2CR;        /* 0xA0 — Analog watchdog 2 config    */
    volatile uint32_t AWD3CR;        /* 0xA4 — Analog watchdog 3 config    */
    volatile uint32_t DIFSEL;        /* 0xC0 — Differential mode select    */
    volatile uint32_t CALFACT;       /* 0xC8 — Calibration factor          */
} ADC_TypeDef;

/* ======================================================================== */
/*  PWR REGISTER STRUCT                                                     */
/* ======================================================================== */

typedef struct {
    volatile uint32_t CR1;           /* 0x00 — Power control register 1    */
    volatile uint32_t CR2;           /* 0x04 — Power control register 2    */
    volatile uint32_t CR3;           /* 0x08 — Power control register 3    */
    volatile uint32_t CR4;           /* 0x0C — Power control register 4    */
    volatile uint32_t SR1;           /* 0x10 — Power status register 1     */
    volatile uint32_t SR2;           /* 0x14 — Power status register 2     */
    volatile uint32_t SCR;           /* 0x18 — Status clear register       */
    volatile uint32_t PUCRA;         /* 0x20 — Pull-up control register A  */
    volatile uint32_t PDCRA;         /* 0x24 — Pull-down control reg A     */
} PWR_TypeDef;

/* PWR register bit definitions */
#define PWR_CR1_LPR             (1U << 14)  /* Low-power run mode          */
#define PWR_CR1_SBP             (1U << 8)   /* SRAM2 parity enable         */
#define PWR_CR1_FLPS            (1U << 9)   /* Flash low-power mode        */

#define PWR_CR2_STOP2           (1U << 1)   /* Stop 2 entry request        */
#define PWR_CR2_PVME1           (1U << 4)   /* Programmable voltage mon 1  */
#define PWR_CR2_PVME2           (1U << 5)   /* Programmable voltage mon 2  */
#define PWR_CR2_PVM_1V5         (3U << 6)   /* 1.5V PVM threshold          */

#define PWR_SR1_SBF             (1U << 0)   /* Standby flag                */
#define PWR_SR1_STOPF           (1U << 1)   /* Stop flag                   */
#define PWR_SR1_REGLPF          (1U << 2)   /* Regulator low-power flag    */

/* ======================================================================== */
/*  IWDG (Independent Watchdog)                                             */
/* ======================================================================== */

typedef struct {
    volatile uint32_t KR;            /* 0x00 — Key register               */
    volatile uint32_t PR;            /* 0x04 — Prescaler register         */
    volatile uint32_t RLR;           /* 0x08 — Reload register            */
    volatile uint32_t SR;            /* 0x0C — Status register            */
    volatile uint32_t WINR;          /* 0x10 — Window register            */
} IWDG_TypeDef;

#define IWDG_KEY_RELOAD         0xAAAAUL
#define IWDG_KEY_UNLOCK         0x5555UL
#define IWDG_KEY_START          0xCCCCUL

/* ======================================================================== */
/*  CRC REGISTER STRUCT                                                     */
/* ======================================================================== */

typedef struct {
    volatile uint32_t DR;            /* 0x00 — Data register               */
    volatile uint32_t IDR;           /* 0x04 — Independent data reg        */
    volatile uint32_t CR;            /* 0x08 — Control register            */
    volatile uint32_t INIT;          /* 0x10 — Initial CRC value           */
    volatile uint32_t POL;           /* 0x14 — CRC polynomial              */
} CRC_TypeDef;

#define CRC_CR_RESET            (1U << 0)
#define CRC_CR_REV_IN_Pos       (5)         /* Reverse input data         */
#define CRC_CR_REV_OUT          (1U << 7)   /* Reverse output data        */

/* ======================================================================== */
/*  PERIPHERAL INSTANCE POINTERS                                            */
/* ======================================================================== */

#define GPIOA                   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB                   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC                   ((GPIO_TypeDef *)GPIOC_BASE)
#define RCC                     ((RCC_TypeDef *)RCC_BASE)
#define I2C1                    ((I2C_TypeDef *)I2C1_BASE)
#define SPI1                    ((SPI_TypeDef *)SPI1_BASE)
#define SPI2                    ((SPI_TypeDef *)SPI2_BASE)
#define USART2                  ((USART_TypeDef *)USART2_BASE)
#define TIM2                    ((TIM_TypeDef *)TIM2_BASE)
#define ADC1                    ((ADC_TypeDef *)ADC1_BASE)
#define PWR                     ((PWR_TypeDef *)PWR_BASE)
#define IWDG                    ((IWDG_TypeDef *)IWDG_BASE)
#define CRC                     ((CRC_TypeDef *)CRC_BASE)

/* ======================================================================== */
/*  BITBAND MACROS (for atomic bit operations)                              */
/* ======================================================================== */

/** @brief Bit-band address calculation for AHB/APB peripherals. */
#define BITBAND_PERIPH(reg, bit)    (*((volatile uint32_t *) \
    (0x42000000UL + ((uint32_t)&(reg) - PERIPH_BASE) * 32 + (bit) * 4)))

/** @brief Bit-band address calculation for SRAM. */
#define BITBAND_SRAM(reg, bit)      (*((volatile uint32_t *) \
    (0x22000000UL + ((uint32_t)&(reg) - 0x20000000UL) * 32 + (bit) * 4)))

#ifdef __cplusplus
}
#endif

#endif /* REGISTERS_H */