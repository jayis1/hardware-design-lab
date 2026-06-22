/*
 * registers.h — STM32H733 register definitions and bit masks
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* ========================================================================
 * Base Addresses — STM32H733 (Cortex-M7)
 * ======================================================================== */

#define PERIPH_BASE         (0x40000000UL)
#define PERIPH_APB1_BASE    (PERIPH_BASE)
#define PERIPH_APB2_BASE    (PERIPH_BASE + 0x00010000UL)
#define PERIPH_APB3_BASE    (PERIPH_BASE + 0x00020000UL)
#define PERIPH_AHB1_BASE    (PERIPH_BASE + 0x00020000UL)
#define PERIPH_AHB2_BASE    (PERIPH_BASE + 0x08000000UL)
#define PERIPH_AHB3_BASE    (PERIPH_BASE + 0x10000000UL)

/* AHB1 */
#define GPIOA_BASE          (PERIPH_AHB1_BASE + 0x0000UL)
#define GPIOB_BASE          (PERIPH_AHB1_BASE + 0x0400UL)
#define GPIOC_BASE          (PERIPH_AHB1_BASE + 0x0800UL)
#define GPIOH_BASE          (PERIPH_AHB1_BASE + 0x1C00UL)

/* AHB2 */
#define RNG_BASE            (PERIPH_AHB2_BASE + 0x0800UL)
#define AES_BASE            (PERIPH_AHB2_BASE + 0x1800UL)
#define SRAM2_BASE          (PERIPH_AHB2_BASE + 0x3000UL)

/* APB1 */
#define TIM2_BASE           (PERIPH_APB1_BASE + 0x0000UL)
#define TIM3_BASE           (PERIPH_APB1_BASE + 0x0400UL)
#define TIM6_BASE           (PERIPH_APB1_BASE + 0x1000UL)
#define TIM7_BASE           (PERIPH_APB1_BASE + 0x1400UL)
#define SPI2_BASE           (PERIPH_APB1_BASE + 0x3800UL)
#define USART2_BASE         (PERIPH_APB1_BASE + 0x4400UL)
#define USART3_BASE         (PERIPH_APB1_BASE + 0x4800UL)
#define I2C1_BASE           (PERIPH_APB1_BASE + 0x5400UL)
#define I2C2_BASE           (PERIPH_APB1_BASE + 0x5800UL)
#define PWR_BASE            (PERIPH_APB1_BASE + 0x7000UL)

/* APB2 */
#define TIM1_BASE           (PERIPH_APB2_BASE + 0x0000UL)
#define TIM8_BASE           (PERIPH_APB2_BASE + 0x0400UL)
#define USART1_BASE         (PERIPH_APB2_BASE + 0x1000UL)
#define SPI1_BASE           (PERIPH_APB2_BASE + 0x3000UL)
#define SPI4_BASE           (PERIPH_APB2_BASE + 0x3400UL)
#define ADC1_BASE           (PERIPH_APB2_BASE + 0x4400UL)
#define ADC2_BASE           (PERIPH_APB2_BASE + 0x4800UL)
#define SYSCFG_BASE         (PERIPH_APB2_BASE + 0x1400UL)

/* AHB3 */
#define FMC_BASE            (PERIPH_AHB3_BASE + 0x0000UL)
#define QSPI_BASE           (PERIPH_AHB3_BASE + 0x1000UL)

/* ========================================================================
 * Register Access Macros
 * ======================================================================== */

#define REG32(addr)         (*(volatile uint32_t *)(addr))
#define REG16(addr)         (*(volatile uint16_t *)(addr))
#define REG8(addr)          (*(volatile uint8_t *)(addr))

#define SET_BIT(reg, bit)       ((reg) |= (bit))
#define CLEAR_BIT(reg, bit)     ((reg) &= ~(bit))
#define READ_BIT(reg, bit)      ((reg) & (bit))
#define WRITE_REG(reg, val)     ((reg) = (val))
#define MODIFY_REG(reg, clrmask, setmask) \
    ((reg) = (((reg) & ~(clrmask)) | (setmask)))

/* ========================================================================
 * RCC — Reset and Clock Control
 * ======================================================================== */

#define RCC_BASE            (PERIPH_AHB1_BASE + 0x4400UL)
#define RCC_CR              REG32(RCC_BASE + 0x00)
#define RCC_PLLCFGR         REG32(RCC_BASE + 0x0C)
#define RCC_CFGR            REG32(RCC_BASE + 0x10)
#define RCC_AHB1ENR         REG32(RCC_BASE + 0xD0)
#define RCC_AHB2ENR         REG32(RCC_BASE + 0xD4)
#define RCC_AHB3ENR         REG32(RCC_BASE + 0xD8)
#define RCC_APB1ENR         REG32(RCC_BASE + 0xE0)
#define RCC_APB1LENR        REG32(RCC_BASE + 0xE0)
#define RCC_APB1HENR        REG32(RCC_BASE + 0xE4)
#define RCC_APB2ENR         REG32(RCC_BASE + 0xE8)
#define RCC_AHB1RSTR        REG32(RCC_BASE + 0x28)
#define RCC_APB1RSTR        REG32(RCC_BASE + 0x20)
#define RCC_APB2RSTR        REG32(RCC_BASE + 0x24)
#define RCC_D1CFGR          REG32(RCC_BASE + 0x18)
#define RCC_D2CFGR          REG32(RCC_BASE + 0x1C)
#define RCC_D3CFGR          REG32(RCC_BASE + 0x20)

/* RCC CR bits */
#define RCC_CR_HSION        (1U << 10)
#define RCC_CR_HSIRDY       (1U << 11)
#define RCC_CR_HSEON        (1U << 16)
#define RCC_CR_HSERDY       (1U << 17)
#define RCC_CR_HSEBYP       (1U << 18)
#define RCC_CR_PLL1ON       (1U << 24)
#define RCC_CR_PLL1RDY      (1U << 25)

/* RCC PLLCFGR fields */
#define RCC_PLLCFGR_PLLSRC_HSE  (2U << 0)
#define RCC_PLLCFGR_PLLM_SHIFT   4
#define RCC_PLLCFGR_PLLN_SHIFT   8
#define RCC_PLLCFGR_PLLP_SHIFT   17
#define RCC_PLLCFGR_PLLQ_SHIFT   21
#define RCC_PLLCFGR_PLLR_SHIFT   25

/* RCC AHB1ENR bits */
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_AHB1ENR_GPIOCEN (1U << 2)
#define RCC_AHB1ENR_GPIOHEN (1U << 7)
#define RCC_AHB1ENR_DMA1EN  (1U << 0)  /* Placeholder, DMA1 on AHB1 */
#define RCC_AHB1ENR_CRCEN   (1U << 19)

/* RCC AHB2ENR bits */
#define RCC_AHB2ENR_RNGEN   (1U << 6)
#define RCC_AHB2ENR_AESEN   (1U << 16)
#define RCC_AHB2ENR_SRAM2EN (1U << 24)

/* RCC AHB3ENR bits */
#define RCC_AHB3ENR_FMCEN   (1U << 0)
#define RCC_AHB3ENR_QSPIEN  (1U << 1)

/* RCC APB1ENR bits */
#define RCC_APB1ENR_TIM2EN  (1U << 0)
#define RCC_APB1ENR_TIM3EN  (1U << 1)
#define RCC_APB1ENR_TIM6EN  (1U << 4)
#define RCC_APB1ENR_TIM7EN  (1U << 5)
#define RCC_APB1ENR_SPI2EN  (1U << 14)
#define RCC_APB1ENR_USART2EN (1U << 17)
#define RCC_APB1ENR_USART3EN (1U << 18)
#define RCC_APB1ENR_I2C1EN  (1U << 21)
#define RCC_APB1ENR_I2C2EN  (1U << 22)
#define RCC_APB1ENR_PWREN   (1U << 28)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_TIM1EN  (1U << 0)
#define RCC_APB2ENR_SPI1EN  (1U << 12)
#define RCC_APB2ENR_SPI4EN  (1U << 13)
#define RCC_APB2ENR_USART1EN (1U << 14)
#define RCC_APB2ENR_ADC1EN  (1U << 24)
#define RCC_APB2ENR_SYSCFGEN (1U << 0)

/* ========================================================================
 * GPIO — General Purpose I/O
 * ======================================================================== */

typedef struct {
    volatile uint32_t MODER;     /* 0x00 */
    volatile uint32_t OTYPER;    /* 0x04 */
    volatile uint32_t OSPEEDR;   /* 0x08 */
    volatile uint32_t PUPDR;     /* 0x0C */
    volatile uint32_t IDR;       /* 0x10 */
    volatile uint32_t ODR;       /* 0x14 */
    volatile uint32_t BSRR;      /* 0x18 */
    volatile uint32_t LCKR;      /* 0x1C */
    volatile uint32_t AFRL;      /* 0x20 */
    volatile uint32_t AFRH;      /* 0x24 */
} gpio_reg_t;

#define GPIOA               ((gpio_reg_t *)GPIOA_BASE)
#define GPIOB               ((gpio_reg_t *)GPIOB_BASE)
#define GPIOC               ((gpio_reg_t *)GPIOC_BASE)
#define GPIOH               ((gpio_reg_t *)GPIOH_BASE)

/* GPIO mode values */
#define GPIO_MODE_INPUT     0x0
#define GPIO_MODE_OUTPUT    0x1
#define GPIO_MODE_AF        0x2
#define GPIO_MODE_ANALOG   0x3

/* GPIO output type */
#define GPIO_OTYPE_PP       0x0
#define GPIO_OTYPE_OD       0x1

/* GPIO speed */
#define GPIO_SPEED_LOW      0x0
#define GPIO_SPEED_MEDIUM   0x1
#define GPIO_SPEED_HIGH     0x2
#define GPIO_SPEED_VERY_HIGH 0x3

/* GPIO pull */
#define GPIO_PUPD_NONE      0x0
#define GPIO_PUPD_PULLUP    0x1
#define GPIO_PUPD_PULLDOWN  0x2

/* GPIO pin helpers */
#define GPIO_PIN(n)         (1U << (n))
#define GPIO_BSRR_SET(n)    (1U << (n))
#define GPIO_BSRR_RESET(n)  (1U << ((n) + 16))

/* AF function codes */
#define AF4_I2C             4
#define AF5_SPI             5
#define AF6_SPI             6
#define AF7_USART           7
#define AF10_USB            10
#define AF12_OTG            12

/* ========================================================================
 * I2C — Inter-Integrated Circuit
 * ======================================================================== */

typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t OAR1;       /* 0x08 */
    volatile uint32_t OAR2;       /* 0x0C */
    volatile uint32_t TIMINGR;    /* 0x10 */
    volatile uint32_t TIMEOUTR;   /* 0x14 */
    volatile uint32_t ISR;        /* 0x18 */
    volatile uint32_t ICR;        /* 0x1C */
    volatile uint32_t PECR;       /* 0x20 */
    volatile uint32_t RXDR;       /* 0x24 */
    volatile uint32_t TXDR;       /* 0x28 */
} i2c_reg_t;

#define I2C1                ((i2c_reg_t *)I2C1_BASE)
#define I2C2                ((i2c_reg_t *)I2C2_BASE)

/* I2C CR1 bits */
#define I2C_CR1_PE          (1U << 0)
#define I2C_CR1_TXIE        (1U << 1)
#define I2C_CR1_RXIE        (1U << 2)
#define I2C_CR1_ADDRIE      (1U << 3)
#define I2C_CR1_NACKIE      (1U << 4)
#define I2C_CR1_STOPIE      (1U << 5)
#define I2C_CR1_TCIE        (1U << 6)
#define I2C_CR1_ERRIE       (1U << 7)
#define I2C_CR1_DNF_SHIFT   8
#define I2C_CR1_ANFOFF      (1U << 12)
#define I2C_CR1_SBC         (1U << 16)

/* I2C CR2 bits */
#define I2C_CR2_SADD_SHIFT  0
#define I2C_CR2_RD_WRN      (1U << 10)
#define I2C_CR2_ADD10       (1U << 11)
#define I2C_CR2_HEAD10R     (1U << 12)
#define I2C_CR2_START       (1U << 13)
#define I2C_CR2_STOP        (1U << 14)
#define I2C_CR2_NACK        (1U << 15)
#define I2C_CR2_NBYTES_SHIFT 16
#define I2C_CR2_RELOAD      (1U << 24)
#define I2C_CR2_AUTOEND     (1U << 25)
#define I2C_CR2_PECBYTE     (1U << 26)

/* I2C ISR bits */
#define I2C_ISR_TXE         (1U << 0)
#define I2C_ISR_TXIS        (1U << 1)
#define I2C_ISR_RXNE        (1U << 2)
#define I2C_ISR_ADDR        (1U << 3)
#define I2C_ISR_NACKF       (1U << 4)
#define I2C_ISR_STOPF       (1U << 5)
#define I2C_ISR_TC          (1U << 6)
#define I2C_ISR_TCR         (1U << 7)
#define I2C_ISR_BERR        (1U << 8)
#define I2C_ISR_ARLO        (1U << 9)
#define I2C_ISR_OVR         (1U << 10)
#define I2C_ISR_PECERR      (1U << 11)
#define I2C_ISR_TIMEOUT     (1U << 12)
#define I2C_ISR_ALERT       (1U << 13)
#define I2C_ISR_BUSY        (1U << 15)

/* I2C ICR bits */
#define I2C_ICR_ADDRCF      (1U << 3)
#define I2C_ICR_NACKCF      (1U << 4)
#define I2C_ICR_STOPCF      (1U << 5)
#define I2C_ICR_BERRCF      (1U << 8)
#define I2C_ICR_ARLOCF      (1U << 9)
#define I2C_ICR_OVRCF       (1U << 10)
#define I2C_ICR_PECCF       (1U << 11)
#define I2C_ICR_TIMOUTCF    (1U << 12)
#define I2C_ICR_ALERTCF     (1U << 13)

/* I2C timing for 400 kHz at 96 MHz PCLK */
#define I2C_TIMING_400K     0x10A13E56U

/* ========================================================================
 * ADC — Analog-to-Digital Converter
 * ======================================================================== */

typedef struct {
    volatile uint32_t ISR;        /* 0x00 */
    volatile uint32_t IER;        /* 0x04 */
    volatile uint32_t CR;         /* 0x08 */
    volatile uint32_t CFGR;       /* 0x0C */
    volatile uint32_t CFGR2;      /* 0x10 */
    volatile uint32_t SMPR1;      /* 0x14 */
    volatile uint32_t SMPR2;      /* 0x18 */
    volatile uint32_t PCSEL;      /* 0x1C */
    volatile uint32_t LFSR;       /* 0x20 */
    volatile uint32_t AWD2CR;     /* 0x24 */
    volatile uint32_t AWD3CR;     /* 0x28 */
    volatile uint32_t RESERVED0;  /* 0x2C */
    volatile uint32_t DIFSEL;     /* 0x30 */
    volatile uint32_t CALFACT;    /* 0x34 */
    volatile uint32_t TR1;        /* 0x38-0x3C */
    volatile uint32_t TR2;        /* 0x40 */
    volatile uint32_t TR3;        /* 0x44 */
    volatile uint32_t SQR1;       /* 0x30 in H7, offset varies */
    volatile uint32_t SQR2;       /* 0x34 */
    volatile uint32_t SQR3;       /* 0x38 */
    volatile uint32_t SQR4;       /* 0x3C */
    volatile uint32_t DR;         /* 0x4C */
    volatile uint32_t RESERVED1[2];
    volatile uint32_t OFR[4];
    /* ... more registers ... */
} adc_reg_t;

#define ADC1                ((adc_reg_t *)ADC1_BASE)

/* ADC ISR bits */
#define ADC_ISR_ADRDY       (1U << 0)
#define ADC_ISR_EOSMP       (1U << 1)
#define ADC_ISR_EOC         (1U << 2)
#define ADC_ISR_EOS         (1U << 3)
#define ADC_ISR_OVR         (1U << 4)

/* ADC CR bits */
#define ADC_CR_ADEN         (1U << 0)
#define ADC_CR_ADDIS        (1U << 1)
#define ADC_CR_ADSTART      (1U << 2)
#define ADC_CR_ADSTP        (1U << 4)
#define ADC_CR_ADCAL        (1U << 8)
#define ADC_CR_ADVREGEN     (1U << 28)
#define ADC_CR_DEEPPWD      (1U << 29)

/* ADC CFGR bits */
#define ADC_CFGR_RES_16BIT  (0x0U << 3)
#define ADC_CFGR_RES_12BIT  (0x2U << 3)
#define ADC_CFGR_ALIGN_RIGHT (0U << 15)
#define ADC_CFGR_DMAEN      (1U << 0)
#define ADC_CFGR_DMACFG     (1U << 1)
#define ADC_CFGR_CONT       (1U << 13)
#define ADC_CFGR_OVRMOD     (1U << 12)
#define ADC_CFGR_EXTEN_RISING (0x1U << 10)

/* ADC sample time */
#define ADC_SMPR_64CYC      0x4
#define ADC_SMPR_387CYC     0x6
#define ADC_SMPR_810CYC     0x7

/* ========================================================================
 * SPI — Serial Peripheral Interface
 * ======================================================================== */

typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t CFG1;       /* 0x08 */
    volatile uint32_t CFG2;       /* 0x0C */
    volatile uint32_t IER;        /* 0x10 */
    volatile uint32_t SR;         /* 0x14 */
    volatile uint32_t IFCR;       /* 0x18 */
    volatile uint32_t TXDR;       /* 0x20 */
    volatile uint32_t RXDR;       /* 0x24 */
    volatile uint32_t CRCPR;      /* 0x28 */
    volatile uint32_t UDRCR;      /* 0x2C */
} spi_reg_t;

#define SPI1                ((spi_reg_t *)SPI1_BASE)
#define SPI2                ((spi_reg_t *)SPI2_BASE)
#define SPI4                ((spi_reg_t *)SPI4_BASE)

/* SPI CR1 bits */
#define SPI_CR1_SPE         (1U << 0)
#define SPI_CR1_CSTART      (1U << 9)
#define SPI_CR1_MASRX       (1U << 8)
#define SPI_CR1_CSUSP       (1U << 10)
#define SPI_CR1_HALT        (1U << 1)
#define SPI_CR1_MASTER      (1U << 2)

/* SPI CFG1 bits */
#define SPI_CFG1_DSIZE_SHIFT 0
#define SPI_CFG1_FTHLV_SHIFT 5
#define SPI_CFG1_TXDMAEN    (1U << 15)
#define SPI_CFG1_RXDMAEN    (1U << 14)

/* SPI CFG2 bits */
#define SPI_CFG2_MASTER     (1U << 22)
#define SPI_CFG2_SSM        (1U << 26)
#define SPI_CFG2_SSI        (1U << 25)
#define SPI_CFG2_MIDI_SHIFT 4
#define SPI_CFG2_MBR_DIV2   (0x0U)
#define SPI_CFG2_MBR_DIV4   (0x1U)
#define SPI_CFG2_MBR_DIV8   (0x2U)
#define SPI_CFG2_MBR_DIV16  (0x3U)
#define SPI_CFG2_MBR_DIV32  (0x4U)
#define SPI_CFG2_MBR_DIV64  (0x5U)
#define SPI_CFG2_MBR_DIV128 (0x6U)
#define SPI_CFG2_MBR_DIV256 (0x7U)
#define SPI_CFG2_CPOL_HIGH  (1U << 1)
#define SPI_CFG2_CPHA_2EDGE (1U << 0)
#define SPI_CFG2_LSBFRST    (1U << 23)

/* SPI SR bits */
#define SPI_SR_RXP          (1U << 0)
#define SPI_SR_TXP          (1U << 1)
#define SPI_SR_EOT          (1U << 3)
#define SPI_SR_TXTF         (1U << 4)
#define SPI_SR_UDR          (1U << 5)
#define SPI_SR_OVR          (1U << 6)
#define SPI_SR_CRCE         (1U << 7)
#define SPI_SR_TIFRE        (1U << 8)
#define SPI_SR_SUSP         (1U << 11)
#define SPI_SR_TXC          (1U << 12)
#define SPI_SR_RXPLVL_SHIFT 13

/* ========================================================================
 * USART
 * ======================================================================== */

typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t CR3;        /* 0x08 */
    volatile uint32_t BRR;        /* 0x0C */
    volatile uint32_t GTPR;       /* 0x10 */
    volatile uint32_t RTOR;       /* 0x14 */
    volatile uint32_t RQR;        /* 0x18 */
    volatile uint32_t ISR;        /* 0x1C */
    volatile uint32_t ICR;        /* 0x20 */
    volatile uint32_t RDR;        /* 0x24 */
    volatile uint32_t TDR;        /* 0x28 */
} usart_reg_t;

#define USART1               ((usart_reg_t *)USART1_BASE)
#define USART2               ((usart_reg_t *)USART2_BASE)
#define USART3               ((usart_reg_t *)USART3_BASE)

/* USART CR1 bits */
#define USART_CR1_UE         (1U << 0)
#define USART_CR1_RE         (1U << 2)
#define USART_CR1_TE         (1U << 3)
#define USART_CR1_IDLEIE     (1U << 4)
#define USART_CR1_RXNEIE     (1U << 5)
#define USART_CR1_TCIE       (1U << 6)
#define USART_CR1_TXEIE      (1U << 7)
#define USART_CR1_PCE        (1U << 10)
#define USART_CR1_PS         (1U << 9)
#define USART_CR1_M0         (1U << 12)
#define USART_CR1_OVER8      (1U << 15)

/* USART ISR bits */
#define USART_ISR_PE         (1U << 0)
#define USART_ISR_FE         (1U << 1)
#define USART_ISR_NE         (1U << 2)
#define USART_ISR_ORE        (1U << 3)
#define USART_ISR_IDLE       (1U << 4)
#define USART_ISR_RXNE       (1U << 5)
#define USART_ISR_TC         (1U << 6)
#define USART_ISR_TXE        (1U << 7)
#define USART_ISR_BUSY       (1U << 16)

/* ========================================================================
 * Timer
 * ======================================================================== */

typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t SMCR;       /* 0x08 */
    volatile uint32_t DIER;       /* 0x0C */
    volatile uint32_t SR;         /* 0x10 */
    volatile uint32_t EGR;        /* 0x14 */
    volatile uint32_t CCMR1;      /* 0x18 */
    volatile uint32_t CCMR2;      /* 0x1C */
    volatile uint32_t CCER;       /* 0x20 */
    volatile uint32_t CNT;        /* 0x24 */
    volatile uint32_t PSC;        /* 0x28 */
    volatile uint32_t ARR;        /* 0x2C */
    volatile uint32_t CCR1;       /* 0x30 */
    volatile uint32_t CCR2;       /* 0x34 */
} tim_reg_t;

#define TIM1                ((tim_reg_t *)TIM1_BASE)
#define TIM2                ((tim_reg_t *)TIM2_BASE)
#define TIM3                ((tim_reg_t *)TIM3_BASE)
#define TIM6                ((tim_reg_t *)TIM6_BASE)
#define TIM7                ((tim_reg_t *)TIM7_BASE)

/* TIM CR1 bits */
#define TIM_CR1_CEN         (1U << 0)
#define TIM_CR1_UDIS        (1U << 1)
#define TIM_CR1_URS         (1U << 2)
#define TIM_CR1_ARPE        (1U << 7)

/* TIM DIER bits */
#define TIM_DIER_UIE        (1U << 0)
#define TIM_DIER_CC1IE      (1U << 1)

/* TIM SR bits */
#define TIM_SR_UIF          (1U << 0)
#define TIM_SR_CC1IF        (1U << 1)

/* ========================================================================
 * DMA — Direct Memory Access (Stream-based for H7)
 * ======================================================================== */

#define DMA1_BASE           (PERIPH_AHB1_BASE + 0x6000UL)

typedef struct {
    volatile uint32_t LISR;       /* 0x00 Low interrupt status */
    volatile uint32_t HISR;       /* 0x04 High interrupt status */
    volatile uint32_t LIFCR;      /* 0x08 Low int flag clear */
    volatile uint32_t HIFCR;      /* 0x0C High int flag clear */
} dma_common_reg_t;

typedef struct {
    volatile uint32_t CR;         /* 0x00 */
    volatile uint32_t NDTR;       /* 0x04 */
    volatile uint32_t PAR;        /* 0x08 */
    volatile uint32_t M0AR;       /* 0x0C */
    volatile uint32_t M1AR;       /* 0x10 */
    volatile uint32_t FCR;        /* 0x14 */
} dma_stream_reg_t;

#define DMA1                ((dma_common_reg_t *)DMA1_BASE)
#define DMA1_STR0           ((dma_stream_reg_t *)(DMA1_BASE + 0x10))
#define DMA1_STR1           ((dma_stream_reg_t *)(DMA1_BASE + 0x28))
#define DMA1_STR2           ((dma_stream_reg_t *)(DMA1_BASE + 0x40))
#define DMA1_STR3           ((dma_stream_reg_t *)(DMA1_BASE + 0x58))

/* DMA stream CR bits */
#define DMA_CR_EN           (1U << 0)
#define DMA_CR_DMEIE        (1U << 2)
#define DMA_CR_TEIE         (1U << 3)
#define DMA_CR_HTIE         (1U << 4)
#define DMA_CR_TCIE         (1U << 5)
#define DMA_CR_PFCTRL       (1U << 5)
#define DMA_CR_DIR_P2M      (0x0U << 6)
#define DMA_CR_DIR_M2P      (0x1U << 6)
#define DMA_CR_DIR_M2M      (0x2U << 6)
#define DMA_CR_CIRC         (1U << 8)
#define DMA_CR_PINC         (1U << 9)
#define DMA_CR_MINC         (1U << 10)
#define DMA_CR_PSIZE_8BIT   (0x0U << 11)
#define DMA_CR_PSIZE_16BIT  (0x1U << 11)
#define DMA_CR_PSIZE_32BIT  (0x2U << 11)
#define DMA_CR_MSIZE_8BIT   (0x0U << 13)
#define DMA_CR_MSIZE_16BIT  (0x1U << 13)
#define DMA_CR_MSIZE_32BIT  (0x2U << 13)
#define DMA_CR_PRIO_LOW     (0x0U << 16)
#define DMA_CR_PRIO_MED     (0x1U << 16)
#define DMA_CR_PRIO_HIGH    (0x2U << 16)
#define DMA_CR_PRIO_VHIGH   (0x3U << 16)

/* ========================================================================
 * PWR — Power Control
 * ======================================================================== */

#define PWR_CR1             REG32(PWR_BASE + 0x00)
#define PWR_CSR1            REG32(PWR_BASE + 0x04)
#define PWR_CR2             REG32(PWR_BASE + 0x08)
#define PWR_CR3             REG32(PWR_BASE + 0x0C)

#define PWR_CR1_LPDS        (1U << 0)
#define PWR_CR1_VOS_SHIFT   3
#define PWR_CR3_SDEN        (1U << 4)

/* ========================================================================
 * FLASH — Flash memory controller
 * ======================================================================== */

#define FLASH_BASE          (PERIPH_AHB1_BASE + 0x2000UL)
#define FLASH_ACR           REG32(FLASH_BASE + 0x00)
#define FLASH_KEYR          REG32(FLASH_BASE + 0x04)
#define FLASH_SR            REG32(FLASH_BASE + 0x10)
#define FLASH_CR            REG32(FLASH_BASE + 0x14)
#define FLASH_OPTCR         REG32(FLASH_BASE + 0x18)

#define FLASH_ACR_LATENCY_MASK  (0xFU)
#define FLASH_ACR_PRFTEN    (1U << 8)
#define FLASH_ACR_ICEN      (1U << 9)
#define FLASH_ACR_DCEN      (1U << 10)
#define FLASH_SR_BSY        (1U << 0)
#define FLASH_SR_EOP        (1U << 1)
#define FLASH_CR_LOCK       (1U << 31)
#define FLASH_CR_PG         (1U << 0)
#define FLASH_CR_SER        (1U << 1)
#define FLASH_CR_STRT       (1U << 3)

/* ========================================================================
 * RNG — Random Number Generator
 * ======================================================================== */

#define RNG_CR              REG32(RNG_BASE + 0x00)
#define RNG_SR              REG32(RNG_BASE + 0x04)
#define RNG_DR              REG32(RNG_BASE + 0x08)

#define RNG_CR_RNGEN        (1U << 2)
#define RNG_SR_DRDY         (1U << 0)
#define RNG_SR_CECS         (1U << 1)
#define RNG_SR_SECS         (1U << 2)

/* ========================================================================
 * SCB — System Control Block (Cortex-M7 core)
 * ======================================================================== */

#define SCB_BASE            (0xE000E100UL)
#define SCB_CPACR           REG32(0xE000ED88UL)
#define SCB_SCR             REG32(0xE000ED10UL)
#define NVIC_ISER0          REG32(0xE000E100UL)
#define NVIC_ICER0          REG32(0xE000E180UL)
#define NVIC_ISPR0          REG32(0xE000E200UL)
#define NVIC_ICPR0          REG32(0xE000E280UL)
#define NVIC_IPR_BASE       (0xE000E400UL)

#define SCB_SCR_SLEEPDEEP   (1U << 2)
#define SCB_SCR_SEVONPEND   (1U << 4)

/* SysTick */
#define SYST_CSR            REG32(0xE000E010UL)
#define SYST_RVR            REG32(0xE000E014UL)
#define SYST_CVR            REG32(0xE000E018UL)
#define SYST_CSR_ENABLE     (1U << 0)
#define SYST_CSR_TICKINT    (1U << 1)
#define SYST_CSR_CLKSOURCE  (1U << 2)

/* ========================================================================
 * Interrupt numbers (IRQn)
 * ======================================================================== */

#define IRQn_WWDG           0
#define IRQn_DMA1_STR0      11
#define IRQn_DMA1_STR1      12
#define IRQn_DMA1_STR2      13
#define IRQn_DMA1_STR3      14
#define IRQn_TIM2           28
#define IRQn_TIM3           29
#define IRQn_I2C1_EV        31
#define IRQn_I2C1_ER        32
#define IRQn_USART2         38
#define IRQn_USART3         39
#define IRQn_TIM6_DAC       54
#define IRQn_TIM7           55
#define IRQn_I2C2_EV        56
#define IRQn_I2C2_ER        57
#define IRQn_SPI1           60
#define IRQn_SPI2           61
#define IRQn_USART1         37
#define IRQn_ADC1           18
#define IRQn_EXTI0          6
#define IRQn_EXTI1          7
#define IRQn_EXTI2          8

/* NVIC helper */
#define NVIC_ENABLE(irq)    (NVIC_ISER0 = (1U << ((irq) & 31)))
#define NVIC_DISABLE(irq)   (NVIC_ICER0 = (1U << ((irq) & 31)))
#define NVIC_SET_PRIO(irq, prio) \
    REG32(NVIC_IPR_BASE + ((irq) / 4) * 4) = \
    (prio << (((irq) % 4) * 8))

/* ========================================================================
 * External Flash (W25Q64) — SPI commands
 * ======================================================================== */

#define W25Q_CMD_READ       0x03
#define W25Q_CMD_FAST_READ  0x0B
#define W25Q_CMD_PAGE_PROG  0x02
#define W25Q_CMD_SECTOR_ER  0x20
#define W25Q_CMD_CHIP_ER    0xC7
#define W25Q_CMD_RDID       0x9F
#define W25Q_CMD_RDSR       0x05
#define W25Q_CMD_WREN       0x06
#define W25Q_CMD_WRDI       0x04
#define W25Q_CMD_PWR_DN     0xB9
#define W25Q_CMD_PWR_UP     0xAB

#define W25Q_SR_WIP         0x01
#define W25Q_SR_WEL         0x02

/* ========================================================================
 * BLE Module (SPBTLE-RC) — HCI interface constants
 * ======================================================================== */

#define BLE_HCI_RESET       0x0C03
#define BLE_HCI_READ_BD_ADDR 0x1009
#define BLE_HCI_LE_SET_ADV  0x2006
#define BLE_HCI_LE_SET_DATA 0x2008
#define BLE_HCI_LE_ADV_EN   0x200A

#define BLE_ADV_TYPE_IND    0x00
#define BLE_ADV_TYPE_SCAN   0x01
#define BLE_ADV_TYPE_NONCONN 0x02

/* ========================================================================
 * Sensor I²C addresses (7-bit)
 * ======================================================================== */

#define BME688_ADDR_1       0x77    /* BME688 sensor #1 (SDO=GND) */
#define BME688_ADDR_2       0x76    /* BME688 sensor #2 (SDO=VDD) */
#define SCD41_ADDR          0x62    /* Sensirion SCD41 CO2 */
#define DGS_ETHANOL_ADDR    0x48    /* Spec DGS-Ethanol */
#define DGS_NH3_ADDR        0x49    /* Spec DGS-NH3 */
#define DGS_H2S_ADDR        0x4A    /* Spec DGS-H2S */
#define BMP390_ADDR         0x77    /* BMP390 pressure */
#define SGP40_ADDR          0x59    /* SGP40 VOC index (auxiliary) */

/* BME688 register map */
#define BME688_REG_CTRL     0x74
#define BME688_REG_CTRL_GAS 0x71
#define BME688_REG_CONFIG   0x75
#define BME688_REG_CTRL_HUM 0x72
#define BME688_REG_STATUS   0x1D
#define BME688_REG_DATA     0x1D
#define BME688_REG_RES_HEAT 0x5A
#define BME688_REG_IDAC     0x50
#define BME688_REG_GAS_WAIT 0x64
#define BME688_REG_CTRL_GAS2 0x73

/* SCD41 register map */
#define SCD41_CMD_START     0x21B1
#define SCD41_CMD_READ      0xEC05
#define SCD41_CMD_STOP      0x3F86
#define SCD41_CMD_REINIT    0x21BE
#define SCD41_CMD_SERIAL    0x3682

/* BMP390 register map */
#define BMP390_REG_CHIP_ID  0xD0
#define BMP390_REG_DATA     0x04
#define BMP390_REG_CONFIG   0x1F
#define BMP390_REG_CTRL     0x1B
#define BMP390_REG_OSR      0x1C
#define BMP390_REG_CALIB    0x00

#endif /* REGISTERS_H */