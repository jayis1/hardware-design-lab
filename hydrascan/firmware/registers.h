/*
 * registers.h — STM32H733 register base addresses and bit definitions
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Only the peripherals HydraScan touches are defined here. Keeping the
 * register map minimal and explicit makes the firmware hermetic and
 * readable without pulling in the full CMSIS device header (which is a
 * fine dependency, but we declare what we use to keep the build simple).
 * Field layout follows RM0433 (STM32H7 reference manual).
 */
#ifndef HYDRASCAN_REGISTERS_H
#define HYDRASCAN_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ------------------------------------------------- */
#define PERIPH_BASE      0x40000000u
#define APB1PERIPH_BASE  (PERIPH_BASE)
#define APB2PERIPH_BASE  (PERIPH_BASE + 0x00010000u)
#define AHB1PERIPH_BASE  0x40020000u
#define AHB4PERIPH_BASE  0x58020000u

#define RCC_BASE         0x58024400u
#define PWR_BASE         0x58024800u
#define FLASH_REG_BASE  0x52002000u

/* GPIO banks (AHB4) */
#define GPIOA_BASE       (AHB4PERIPH_BASE + 0x0000u)
#define GPIOB_BASE       (AHB4PERIPH_BASE + 0x0100u)
#define GPIOC_BASE       (AHB4PERIPH_BASE + 0x0200u)
#define GPIOD_BASE       (AHB4PERIPH_BASE + 0x0300u)
#define GPIOE_BASE       (AHB4PERIPH_BASE + 0x0400u)
#define GPIOH_BASE       (AHB4PERIPH_BASE + 0x0700u)

#define GPIOA (*(gpio_t *)GPIOA_BASE)
#define GPIOB (*(gpio_t *)GPIOB_BASE)
#define GPIOC (*(gpio_t *)GPIOC_BASE)
#define GPIOD (*(gpio_t *)GPIOD_BASE)
#define GPIOE (*(gpio_t *)GPIOE_BASE)
#define GPIOH (*(gpio_t *)GPIOH_BASE)

/* RCC */
#define RCC (*(rcc_t *)RCC_BASE)
/* SPI2/3, I2C1, UART4 on APB1; SPI1, ADC1, USB on APB2 */

/* Peripheral enable bits in RCC AHB/APB registers */
#define RCC_AHB4ENR_GPIOAEN  (1u << 0)
#define RCC_AHB4ENR_GPIOBEN  (1u << 1)
#define RCC_AHB4ENR_GPIOCEN  (1u << 2)
#define RCC_AHB4ENR_GPIODEN  (1u << 3)
#define RCC_AHB4ENR_GPIOEEN  (1u << 4)
#define RCC_AHB4ENR_GPIOHEN  (1u << 7)

#define RCC_APB1LENR_SPI2EN  (1u << 14)
#define RCC_APB1LENR_SPI3EN  (1u << 15)
#define RCC_APB1LENR_I2C1EN (1u << 21)
#define RCC_APB1LENR_UART4EN (1u << 24)
#define RCC_APB2ENR_SPI1EN   (1u << 12)
#define RCC_AHB2ENR_ADC12EN  (1u << 24)
#define RCC_AHB1ENR_QSPI1EN  (1u << 14)   /* QUADSPI on AHB3 in H7; keep alias */

/* ---- GPIO register layout (RM0433 §11.4) --------------------------- */
typedef struct {
    volatile uint32_t MODER;      /* 0x00 2 bits/pin                       */
    volatile uint32_t OTYPER;     /* 0x04 1 bit/pin                        */
    volatile uint32_t OSPEEDR;    /* 0x08 2 bits/pin                       */
    volatile uint32_t PUPDR;      /* 0x0C 2 bits/pin                       */
    volatile uint32_t IDR;        /* 0x10 input (RO)                       */
    volatile uint32_t ODR;        /* 0x14 output                           */
    volatile uint32_t BSRR;       /* 0x18 set/reset (WO)                   */
    volatile uint32_t LCKR;       /* 0x1C                                  */
    volatile uint32_t AFRL;      /* 0x20 alt func 0..7                    */
    volatile uint32_t AFRH;      /* 0x24 alt func 8..15                   */
} gpio_t;

#define GPIO_MODE_INPUT   0u
#define GPIO_MODE_OUTPUT  1u
#define GPIO_MODE_AF       2u
#define GPIO_MODE_ANALOG  3u

#define GPIO_OTYPE_PP      0u
#define GPIO_OTYPE_OD      1u

#define GPIO_OSPEED_LOW    0u
#define GPIO_OSPEED_HIGH   2u

#define GPIO_PUPD_NONE      0u
#define GPIO_PUPD_PU        1u
#define GPIO_PUPD_PD        2u

/* ---- RCC (minimal) --------------------------------------------------- */
typedef struct {
    volatile uint32_t CR;         /* 0x00 */
    volatile uint32_t HSICFGR;    /* 0x04 */
    volatile uint32_t CRRCR;      /* 0x08 */
    volatile uint32_t CSICFGR;    /* 0x0C */
    uint32_t reserved[4];
    volatile uint32_t D1CFGR;     /* 0x28 */
    volatile uint32_t D2CFGR;     /* 0x2C */
    volatile uint32_t D3CFGR;     /* 0x30 */
    uint32_t reserved2[2];
    volatile uint32_t PLLCKSELR;  /* 0x38 */
    volatile uint32_t PLLCFGR;   /* 0x3C */
    volatile uint32_t PLL1DIVR;   /* 0x40 */
    uint32_t reserved3;
    volatile uint32_t PLL1FRACR;  /* 0x48 */
    uint32_t reserved4[4];
    volatile uint32_t CIER;       /* 0x60 */
    volatile uint32_t CIFR;       /* 0x64 */
    volatile uint32_t CICR;       /* 0x68 */
    uint32_t reserved5[3];
    volatile uint32_t BDCR;       /* 0x78 */
    volatile uint32_t CSR;        /* 0x7C */
    /* AHB, APB enables ... */
    volatile uint32_t AHB4ENR;    /* 0x0E0 (actual offset in RM0433) */
} rcc_t;

/* Real offsets for RCC enable registers on STM32H7 */
#define RCC_AHB4ENR_OF   0x0E0u
#define RCC_AHB3ENR_OF   0x0DCu
#define RCC_AHB1ENR_OF   0x0D8u
#define RCC_AHB2ENR_OF   0x0E4u
#define RCC_APB1LENR_OF  0x0E8u
#define RCC_APB1HENR_OF  0x0ECu
#define RCC_APB2ENR_OF   0x0F0u
#define RCC_APB3ENR_OF   0x0F4u

#define RCC_REG32(off) (*(volatile uint32_t *)(RCC_BASE + (off)))

/* ---- SPI (minimal common layout) ----------------------------------- */
typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t SR;         /* 0x08 */
    volatile uint32_t DR;        /* 0x0C */
    volatile uint32_t CRCPR;     /* 0x10 */
    volatile uint32_t RXCRCR;    /* 0x14 */
    volatile uint32_t TXCRCR;    /* 0x18 */
    volatile uint32_t I2SCFGR;   /* 0x1C */
    volatile uint32_t I2SPR;     /* 0x20 */
} spi_t;

#define SPI1 (*(spi_t *)0x40013000u)
#define SPI2 (*(spi_t *)0x40003800u)
#define SPI3 (*(spi_t *)0x40003C00u)

#define SPI_CR1_SPE     (1u << 6)
#define SPI_CR1_MSTR    (1u << 2)
#define SPI_CR1_CPHA    (1u << 0)
#define SPI_CR1_CPOL    (1u << 1)
#define SPI_CR1_BR_DIV4 (1u << 3)
#define SPI_CR1_SSM     (1u << 9)
#define SPI_CR1_SSI     (1u << 8)
#define SPI_CR1_LSBFIRST (1u << 7)
#define SPI_CR2_DS_8BIT (7u << 0)   /* 0111 = 8-bit */
#define SPI_CR2_FRXTH   (1u << 12)
#define SPI_CR2_SSOE    (1u << 2)
#define SPI_SR_RXNE     (1u << 0)
#define SPI_SR_TXE      (1u << 1)
#define SPI_SR_BSY      (1u << 7)

/* ---- I2C (minimal) -------------------------------------------------- */
typedef struct {
    volatile uint32_t CR1;        /* 0x00 */
    volatile uint32_t CR2;        /* 0x04 */
    volatile uint32_t OAR1;       /* 0x08 */
    volatile uint32_t OAR2;       /* 0x0C */
    volatile uint32_t TIMINGR;    /* 0x10 */
    volatile uint32_t TIMEOUTr;   /* 0x14 */
    uint32_t reserved;
    volatile uint32_t ISR;        /* 0x18 */
    volatile uint32_t ICR;        /* 0x1C */
    volatile uint32_t PECR;       /* 0x20 */
    volatile uint32_t RXDR;       /* 0x24 */
    volatile uint32_t TXDR;       /* 0x28 */
} i2c_t;

#define I2C1 (*(i2c_t *)0x40005400u)

#define I2C_CR1_PE       (1u << 0)
#define I2C_CR2_START    (1u << 13)
#define I2C_CR2_STOP     (1u << 14)
#define I2C_CR2_NBYTES(m) ((m) << 16)
#define I2C_CR2_ADD10    (1u << 11)
#define I2C_CR2_RD_WRN   (1u << 10)
#define I2C_CR2_AUTOEND  (1u << 15)
#define I2C_ISR_TXE      (1u << 0)
#define I2C_ISR_RXNE     (1u << 2)
#define I2C_ISR_TC       (1u << 6)
#define I2C_ISR_NACKF    (1u << 4)
#define I2C_ISR_BUSY     (1u << 15)

/* ---- USART (minimal) ----------------------------------------------- */
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
} usart_t;

#define UART4 (*(usart_t *)0x40004400u)
#define USART_CR1_UE     (1u << 0)
#define USART_CR1_TE     (1u << 3)
#define USART_CR1_RE     (1u << 2)
#define USART_CR1_RXNEIE (1u << 5)
#define USART_ISR_RXNE   (1u << 5)
#define USART_ISR_TXE    (1u << 7)
#define USART_ISR_TC     (1u << 6)

/* ---- ADC1 (minimal) ------------------------------------------------- */
#define ADC1_BASE       0x40022000u
typedef struct {
    volatile uint32_t ISR;        /* 0x00 */
    volatile uint32_t IER;        /* 0x04 */
    volatile uint32_t CR;         /* 0x08 */
    volatile uint32_t CFGR;       /* 0x0C */
    volatile uint32_t CFGR2;      /* 0x10 */
    volatile uint32_t SMPR1;     /* 0x14 */
    volatile uint32_t SMPR2;     /* 0x18 */
    uint32_t reserved1[2];
    volatile uint32_t TR1;        /* 0x20 */
    volatile uint32_t TR2;        /* 0x24 */
    volatile uint32_t TR3;        /* 0x28 */
    uint32_t reserved2[4];
    volatile uint32_t SQR1;       /* 0x30 */
    volatile uint32_t SQR2;       /* 0x34 */
    volatile uint32_t SQR3;       /* 0x38 */
    volatile uint32_t SQR4;       /* 0x3C */
    uint32_t reserved3[4];
    volatile uint32_t DR;         /* 0x4C (regular) */
} adc_t;
#define ADC1 (*(adc_t *)ADC1_BASE)

#define ADC_ISR_EOC      (1u << 2)
#define ADC_CR_ADEN      (1u << 0)
#define ADC_CR_ADSTART   (1u << 2)
#define ADC_CR_ADVREGEN  (1u << 28)

/* ---- QSPI (minimal) ------------------------------------------------- */
typedef struct {
    volatile uint32_t CR;         /* 0x00 */
    volatile uint32_t DCR;        /* 0x04 */
    volatile uint32_t SR;         /* 0x08 */
    volatile uint32_t FCR;        /* 0x0C */
    volatile uint32_t DLR;        /* 0x10 */
    volatile uint32_t CCR;        /* 0x14 */
    volatile uint32_t AR;         /* 0x18 */
    volatile uint32_t ABR;        /* 0x1C */
    volatile uint32_t PSMAR;      /* 0x20 */
    volatile uint32_t PSMKR;      /* 0x24 */
    volatile uint32_t PIR;        /* 0x28 */
    volatile uint32_t LPTR;       /* 0x2C */
} qspi_t;
#define QSPI (*(qspi_t *)0x90000000u)

#define QSPI_SR_BUSY     (1u << 5)
#define QSPI_SR_TCF      (1u << 1)
#define QSPI_CR_EN       (1u << 0)
#define QSPI_CCR_FMODE_INDW  (0u << 26)
#define QSPI_CCR_FMODE_INDR  (1u << 26)
#define QSPI_CCR_FMODE_INDA  (3u << 26)

/* ---- SysTick (Cortex-M7 core) -------------------------------------- */
#define SYSTICK_BASE     0xE000E010u
typedef struct {
    volatile uint32_t CSR;        /* 0x00 */
    volatile uint32_t LOAD;       /* 0x04 */
    volatile uint32_t VAL;        /* 0x08 */
    volatile uint32_t CALIB;      /* 0x0C */
} systick_t;
#define SYSTICK (*(systick_t *)SYSTICK_BASE)
#define SYSTICK_CSR_ENABLE (1u << 0)
#define SYSTICK_CSR_TICKINT (1u << 1)
#define SYSTICK_CSR_CLKSOURCE (1u << 2)

/* NVIC (Cortex-M) */
#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100u)
#define NVIC_ICPR0 (*(volatile uint32_t *)0xE000E280u)

/* ---- Misc helper macros -------------------------------------------- */
#define REG32(addr) (*(volatile uint32_t *)(addr))
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#endif /* HYDRASCAN_REGISTERS_H */