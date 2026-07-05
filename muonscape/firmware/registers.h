/*
 * registers.h — STM32H723 register-level definitions used by MûonScape
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Minimal subset of the STM32H723 register map referenced by the
 * drivers. Not a complete CMSIS replacement — only what this firmware
 * touches. Where the ST HAL would normally be used we prefer direct
 * register access for determinism and auditability.
 */
#ifndef MUONSCAPE_REGISTERS_H
#define MUONSCAPE_REGISTERS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Base addresses (STM32H723, RM0468 r1.x) ---- */
#define PERIPH_BASE       0x40000000UL
#define AHB1PERIPH_BASE   0x40020000UL
#define AHB2PERIPH_BASE   0x48000000UL
#define AHB3PERIPH_BASE   0x50000000UL
#define AHB4PERIPH_BASE   0x58000000UL
#define APB1PERIPH_BASE   0x40000000UL
#define APB2PERIPH_BASE   0x48010000UL
#define APB4PERIPH_BASE   0x58020000UL

/* GPIO ports (AHB4) */
#define GPIOA_BASE        (AHB4PERIPH_BASE + 0x0000)
#define GPIOB_BASE        (AHB4PERIPH_BASE + 0x0400)
#define GPIOC_BASE        (AHB4PERIPH_BASE + 0x0800)
#define GPIOD_BASE        (AHB4PERIPH_BASE + 0x0C00)
#define GPIOE_BASE        (AHB4PERIPH_BASE + 0x1000)
#define GPIOF_BASE        (AHB4PERIPH_BASE + 0x1400)
#define GPIOG_BASE        (AHB4PERIPH_BASE + 0x1800)
#define GPIOH_BASE        (AHB4PERIPH_BASE + 0x1C00)

/* Convenience port handle macros (resolving to base addresses) */
#define GPIOA   GPIOA_BASE
#define GPIOB   GPIOB_BASE
#define GPIOC   GPIOC_BASE
#define GPIOD   GPIOD_BASE
#define GPIOE   GPIOE_BASE
#define GPIOF   GPIOF_BASE
#define GPIOG   GPIOG_BASE
#define GPIOH   GPIOH_BASE

/* RCC (AHB4 r1) */
#define RCC_BASE          (AHB1PERIPH_BASE + 0x4400)
/* PWR (AHB4) */
#define PWR_BASE           (AHB1PERIPH_BASE + 0x4800)

/* Flash controller */
#define FLASH_REGS_BASE   (AHB1PERIPH_BASE + 0x2000)

/* SPI peripherals (APB2 / AHB1) */
#define SPI1_BASE          (APB2PERIPH_BASE + 0x3000)
#define SPI3_BASE          (APB2PERIPH_BASE + 0x3C00)

/* I2C (APB1) */
#define I2C1_BASE          (APB1PERIPH_BASE + 0x5400)

/* USART (APB1 / APB2) */
#define USART2_BASE        (APB1PERIPH_BASE + 0x4400)
#define USART3_BASE        (APB1PERIPH_BASE + 0x4800)

/* SDMMC (AHB3) */
#define SDMMC1_BASE        (AHB3PERIPH_BASE + 0x1000)

/* DMA stream (DMA1, AHB1 r1) */
#define DMA1_BASE          (AHB1PERIPH_BASE + 0x0000)
#define DMA1_STREAM0_BASE  (DMA1_BASE + 0x0010)
#define DMA1_STREAM1_BASE  (DMA1_BASE + 0x0028)
#define DMA1_STREAM2_BASE  (DMA1_BASE + 0x0040)
#define DMA1_STREAM3_BASE  (DMA1_BASE + 0x0058)

/* DMAMUX (AHB1 r1) */
#define DMAMUX1_BASE       (AHB1PERIPH_BASE + 0x0800)

/* ADC (AHB2) */
#define ADC1_BASE          (AHB2PERIPH_BASE + 0x0000)
#define ADC2_BASE          (AHB2PERIPH_BASE + 0x0100)
#define ADC12_COMMON_BASE  (AHB2PERIPH_BASE + 0x0300)

/* TIM (APB1 / APB2) */
#define TIM1_BASE          (APB2PERIPH_BASE + 0x0000)
#define TIM8_BASE          (APB2PERIPH_BASE + 0x0400)

/* USB OTG (AHB1) */
#define OTG1_BASE          (AHB1PERIPH_BASE + 0x8000)

/* FMC (AHB3) — for pSRAM */
#define FMC_BASE           (AHB3PERIPH_BASE + 0x0000)

/* ---- Generic register accessor macros ---- */
#define REG32(addr)        (*(volatile uint32_t *)(addr))
#define REG16(addr)        (*(volatile uint16_t *)(addr))
#define REG8(addr)         (*(volatile uint8_t  *)(addr))
#define BIT(n)             (1UL << (n))

/* ---- GPIO register offsets ---- */
#define GPIO_MODER_OFF     0x00
#define GPIO_OTYPER_OFF    0x04
#define GPIO_OSPEEDR_OFF   0x08
#define GPIO_PUPDR_OFF     0x0C
#define GPIO_IDR_OFF       0x10
#define GPIO_ODR_OFF       0x14
#define GPIO_BSRR_OFF      0x18
#define GPIO_LCKR_OFF      0x1C
#define GPIO_AFRL_OFF      0x20
#define GPIO_AFRH_OFF      0x24

/* GPIO helpers */
#define GPIO_MODER(p)      REG32((p) + GPIO_MODER_OFF)
#define GPIO_OTYPER(p)     REG32((p) + GPIO_OTYPER_OFF)
#define GPIO_OSPEEDR(p)    REG32((p) + GPIO_OSPEEDR_OFF)
#define GPIO_PUPDR(p)      REG32((p) + GPIO_PUPDR_OFF)
#define GPIO_IDR(p)        REG32((p) + GPIO_IDR_OFF)
#define GPIO_ODR(p)        REG32((p) + GPIO_ODR_OFF)
#define GPIO_BSRR(p)       REG32((p) + GPIO_BSRR_OFF)
#define GPIO_AFRL(p)       REG32((p) + GPIO_AFRL_OFF)
#define GPIO_AFRH(p)       REG32((p) + GPIO_AFRH_OFF)

/* ---- RCC bits we touch ---- */
#define RCC_CR             REG32(RCC_BASE + 0x00)
#define RCC_CFGR           REG32(RCC_BASE + 0x10)
#define RCC_PLL1CFGR       REG32(RCC_BASE + 0x28)
#define RCC_AHB1ENR        REG32(RCC_BASE + 0xD8)
#define RCC_AHB2ENR        REG32(RCC_BASE + 0xDC)
#define RCC_AHB3ENR        REG32(RCC_BASE + 0xE0)
#define RCC_AHB4ENR        REG32(RCC_BASE + 0xE4)
#define RCC_APB1ENR        REG32(RCC_BASE + 0xE8)
#define RCC_APB1ENR1       REG32(RCC_BASE + 0xE8)
#define RCC_APB1ENR2       REG32(RCC_BASE + 0xEC)
#define RCC_APB2ENR        REG32(RCC_BASE + 0xF0)
#define RCC_APB4ENR        REG32(RCC_BASE + 0xF4)

/* RCC enable bits */
#define RCC_AHB1ENR_DMA1EN  BIT(0)
#define RCC_AHB1ENR_USB1OTGEN BIT(24)
#define RCC_AHB3ENR_FMEN    BIT(0)
#define RCC_AHB3ENR_SDMMC1EN BIT(16)
#define RCC_AHB4ENR_GPIOAEN BIT(0)
#define RCC_AHB4ENR_GPIOBEN BIT(1)
#define RCC_AHB4ENR_GPIOCEN BIT(2)
#define RCC_AHB4ENR_GPIODEN BIT(3)
#define RCC_AHB4ENR_GPIOEEN BIT(4)
#define RCC_APB1ENR1_I2C1EN BIT(21)
#define RCC_APB1ENR1_USART2EN BIT(17)
#define RCC_APB1ENR1_USART3EN BIT(18)
#define RCC_APB2ENR_SPI1EN  BIT(12)
#define RCC_APB2ENR_SPI3EN  BIT(15)
#define RCC_APB2ENR_TIM1EN  BIT(0)
#define RCC_APB2ENR_TIM8EN  BIT(1)
#define RCC_APB2ENR_ADC12EN BIT(5)

/* ---- SPI register layout ---- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t CFG1;      /* 0x08 */
    volatile uint32_t CFG2;      /* 0x0C */
    volatile uint32_t IER;      /* 0x10 */
    volatile uint32_t SR;       /* 0x14 */
    volatile uint32_t IFCR;     /* 0x18 */
    volatile uint32_t TXDR;     /* 0x1C — actually 8/16-bit accessed */
    volatile uint32_t RXDR;     /* 0x20 */
    volatile uint32_t CRCL;     /* 0x24 */
    volatile uint32_t CRCH;     /* 0x28 */
    volatile uint32_t RXCRC;    /* 0x2C */
    volatile uint32_t TXCRC;    /* 0x30 */
} spi_regs_t;

#define SPI_CR1_SPE        BIT(0)
#define SPI_CR1_CSTART     BIT(9)
#define SPI_CR1_SPE_EN     BIT(0)
#define SPI_CFG1_MASTER    BIT(2)
#define SPI_CFG1_DSIZE_8   (7U)            /* 8-bit data */
#define SPI_CFG1_DSIZE_16  (15U)           /* 16-bit data */
#define SPI_CFG2_CPOL      BIT(2)
#define SPI_CFG2_CPHA      BIT(3)
#define SPI_CFG2_SSOE      BIT(8)
#define SPI_CFG2_MASTER    BIT(22)
#define SPI_SR_TXP         BIT(0)
#define SPI_SR_RXP         BIT(1)
#define SPI_SR_EOT         BIT(3)
#define SPI_SR_OVR         BIT(6)
#define SPI_SR_MODF        BIT(8)

/* ---- I2C register layout ---- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t OAR1;     /* 0x08 */
    volatile uint32_t OAR2;     /* 0x0C */
    volatile uint32_t TIMINGR;  /* 0x10 */
    volatile uint32_t TIMEOUTR; /* 0x14 */
    volatile uint32_t ISR;      /* 0x18 */
    volatile uint32_t ICR;      /* 0x1C */
    volatile uint32_t PECR;     /* 0x20 */
    volatile uint32_t RXDR;     /* 0x24 */
    volatile uint32_t TXDR;     /* 0x28 */
} i2c_regs_t;

#define I2C_CR1_PE         BIT(0)
#define I2C_CR1_TXIE       BIT(1)
#define I2C_CR1_RXIE       BIT(2)
#define I2C_CR2_START      BIT(13)
#define I2C_CR2_STOP       BIT(14)
#define I2C_CR2_NACK       BIT(15)
#define I2C_CR2_AUTOEND    BIT(10)
#define I2C_ISR_TXIS       BIT(1)
#define I2C_ISR_RXNE       BIT(2)
#define I2C_ISR_TCR        BIT(7)
#define I2C_ISR_TC         BIT(6)
#define I2C_ISR_STOPF      BIT(5)
#define I2C_ISR_NACKF      BIT(4)
#define I2C_ISR_BUSY       BIT(15)

/* ---- USART register layout (subset) ---- */
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
} usart_regs_t;

#define USART_CR1_UE       BIT(0)
#define USART_CR1_RE        BIT(2)
#define USART_CR1_TE        BIT(3)
#define USART_CR1_RXNEIE    BIT(5)
#define USART_ISR_RXNE      BIT(5)
#define USART_ISR_TXE       BIT(7)
#define USART_ISR_TC        BIT(6)

/* ---- FMC (pSRAM) ---- */
#define FMC_BCR1           REG32(FMC_BASE + 0x00)
#define FMC_BTR1           REG32(FMC_BASE + 0x04)
#define FMC_BWTR1          REG32(FMC_BASE + 0x104)
#define FMC_PCR            REG32(FMC_BASE + 0x80)

/* ---- RCC PLL / clock helpers ---- */
void muon_clock_init(void);   /* set up 25 MHz HSE -> 550 MHz SYSCLK */
void muon_gpio_init(void);     /* configure all GPIO per board.h */
void muon_psram_init(void);    /* FMC bank 1 for 8 MB pSRAM */

/* ---- NVIC helpers ---- */
#define NVIC_ISER0         REG32(0xE000E100)
#define NVIC_ICER0         REG32(0xE000E180)
#define NVIC_IPR_BASE      0xE000E400

/* ---- SysTick / delay ---- */
void muon_systick_init(uint32_t ticks_per_ms);
void muon_delay_ms(uint32_t ms);
uint32_t muon_millis(void);

/* ---- Cache control ---- */
void muon_scache_enable(void);
void muon_dcache_enable(void);
void muon_dcache_invalidate_range(uint32_t addr, uint32_t size);
void muon_dcache_clean_range(uint32_t addr, uint32_t size);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_REGISTERS_H */