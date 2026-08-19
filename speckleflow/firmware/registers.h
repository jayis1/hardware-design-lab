/*
 * registers.h — STM32H733 register definitions for SpeckleFlow
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Minimal hand-written register map for the peripherals used by
 * SpeckleFlow.  We deliberately avoid pulling in the full ST HAL
 * (which is ~100 kLOC) and instead define only the registers we touch,
 * keeping the firmware small, auditable, and buildable with bare-metal
 * arm-none-eabi-gcc.
 */

#ifndef SPECKLEFLOW_REGISTERS_H
#define SPECKLEFLOW_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ----------------------------------------------------- */

#define PERIPH_BASE      0x40000000u
#define PERIPH_AHB1_BASE 0x40020000u
#define PERIPH_AHB2_BASE 0x48000000u
#define PERIPH_AHB3_BASE 0x51000000u
#define PERIPH_AHB4_BASE 0x58020000u
#define PERIPH_APB1_BASE 0x40000000u
#define PERIPH_APB2_BASE 0x40010000u
#define PERIPH_APB4_BASE 0x58000000u

/* ---- AHB1 / AHB4 GPIO --------------------------------------------------- */

#define GPIOA_BASE  (PERIPH_AHB4_BASE + 0x0000)
#define GPIOB_BASE  (PERIPH_AHB4_BASE + 0x0400)
#define GPIOC_BASE  (PERIPH_AHB4_BASE + 0x0800)
#define GPIOD_BASE  (PERIPH_AHB4_BASE + 0x0C00)
#define GPIOE_BASE  (PERIPH_AHB4_BASE + 0x1000)
#define GPIOH_BASE  (PERIPH_AHB4_BASE + 0x1C00)

typedef struct {
    volatile uint32_t MODER;    /* 0x00 mode */
    volatile uint32_t OTYPER;   /* 0x04 output type */
    volatile uint32_t OSPEEDR;  /* 0x08 output speed */
    volatile uint32_t PUPDR;    /* 0x0C pull-up/pull-down */
    volatile uint32_t IDR;      /* 0x10 input data */
    volatile uint32_t ODR;      /* 0x14 output data */
    volatile uint32_t BSRR;     /* 0x18 bit set/reset */
    volatile uint32_t LCKR;     /* 0x1C lock */
    volatile uint32_t AFRL;     /* 0x20 alt function low */
    volatile uint32_t AFRH;     /* 0x24 alt function high */
    volatile uint32_t RESERVED; /* 0x28 */
    volatile uint32_t SECCFGR;  /* 0x2C security */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOH ((GPIO_TypeDef *)GPIOH_BASE)

/* ---- RCC (Reset & Clock Control) ---------------------------------------- */

#define RCC_BASE (PERIPH_AHB1_BASE + 0x4400)

typedef struct {
    volatile uint32_t CR;        /* 0x00 clock control */
    volatile uint32_t HSICFGR;   /* 0x04 HSI config */
    volatile uint32_t CRRCR;     /* 0x08 CRS control */
    volatile uint32_t CSICFGR;   /* 0x0C CSI config */
    volatile uint32_t CFGR;      /* 0x10 clock config */
    volatile uint32_t RESERVED0; /* 0x14 */
    volatile uint32_t PLLCFGR;   /* 0x18 PLL config */
    volatile uint32_t RESERVED1; /* 0x1C */
    volatile uint32_t CIER;      /* 0x20 clock interrupt enable */
    volatile uint32_t CIFR;      /* 0x24 clock interrupt flag */
    volatile uint32_t CICR;      /* 0x28 clock interrupt clear */
    volatile uint32_t RESERVED2; /* 0x2C */
    volatile uint32_t BDCR;      /* 0x30 backup domain */
    volatile uint32_t CSR;       /* 0x34 control/status */
    volatile uint32_t RESERVED3[2]; /* 0x38-0x3C */
    volatile uint32_t AHB3RSTR;  /* 0x40 AHB3 reset */
    volatile uint32_t AHB1RSTR;  /* 0x44 AHB1 reset */
    volatile uint32_t AHB2RSTR;  /* 0x48 AHB2 reset */
    volatile uint32_t AHB4RSTR;  /* 0x4C AHB4 reset */
    volatile uint32_t APB3RSTR;  /* 0x50 APB3 reset */
    volatile uint32_t APB1LRSTR; /* 0x54 APB1L reset */
    volatile uint32_t APB1HRSTR; /* 0x58 APB1H reset */
    volatile uint32_t APB2RSTR;  /* 0x5C APB2 reset */
    volatile uint32_t APB4RSTR;  /* 0x60 APB4 reset */
    volatile uint32_t GCR;       /* 0x64 global control */
    volatile uint32_t RESERVED4[8]; /* 0x68-0x84 */
    volatile uint32_t AHB3ENR;   /* 0x88 AHB3 enable */
    volatile uint32_t AHB1ENR;   /* 0x8C AHB1 enable */
    volatile uint32_t AHB2ENR;   /* 0x90 AHB2 enable */
    volatile uint32_t AHB4ENR;   /* 0x94 AHB4 enable */
    volatile uint32_t APB3ENR;   /* 0x98 APB3 enable */
    volatile uint32_t APB1LENR;  /* 0x9C APB1L enable */
    volatile uint32_t APB1HENR;  /* 0xA0 APB1H enable */
    volatile uint32_t APB2ENR;   /* 0xA4 APB2 enable */
    volatile uint32_t APB4ENR;  /* 0xA8 APB4 enable */
    /* ... (we only use enable/reset/config registers) */
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)RCC_BASE)

/* RCC enable bits we use */
#define RCC_AHB4ENR_GPIOAEN  (1u << 0)
#define RCC_AHB4ENR_GPIOBEN  (1u << 1)
#define RCC_AHB4ENR_GPIOCEN  (1u << 2)
#define RCC_AHB4ENR_GPIODEN  (1u << 3)
#define RCC_AHB4ENR_GPIOEEN  (1u << 4)
#define RCC_AHB4ENR_GPIOHEN  (1u << 7)
#define RCC_AHB3ENR_SDMMC1EN (1u << 16)
#define RCC_AHB1ENR_USB1EN   (1u << 12)
#define RCC_AHB1ENR_DMA1EN   (1u << 0)
#define RCC_AHB1ENR_DMA2EN   (1u << 1)
#define RCC_APB1LENR_USART3EN (1u << 18)
#define RCC_APB1LENR_UART4EN  (1u << 19)
#define RCC_APB1LENR_I2C1EN   (1u << 21)
#define RCC_APB1LENR_DAC1EN   (1u << 29)
#define RCC_APB2ENR_SPI1EN   (1u << 12)
#define RCC_APB2ENR_SPI4EN   (1u << 13)
#define RCC_APB2ENR_TIM1EN   (1u << 0)
#define RCC_APB2ENR_TIM8EN   (1u << 1)
#define RCC_APB2ENR_USART1EN (1u << 4)
#define RCC_APB4ENR_I2C4EN   (1u << 6)
#define RCC_APB4ENR_SYSCFGEN (1u << 1)

/* ---- PWR (Power Control) ------------------------------------------------ */

#define PWR_BASE (PERIPH_APB1_BASE + 0x7000)

typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t CR3;   /* 0x08 */
    volatile uint32_t CPUCR; /* 0x0C */
    volatile uint32_t RESERVED[13]; /* 0x10-0x40 */
    volatile uint32_t SR1;   /* 0x44 */
    volatile uint32_t SR2;   /* 0x48 */
    volatile uint32_t SCR;   /* 0x4C */
    volatile uint32_t CR4;   /* 0x50 */
    /* ... */
} PWR_TypeDef;

#define PWR ((PWR_TypeDef *)PWR_BASE)
#define PWR_CR3_SCUEN (1u << 2)

/* ---- Flash controller --------------------------------------------------- */

#define FLASH_BASE (PERIPH_AHB1_BASE + 0x2000)

typedef struct {
    volatile uint32_t ACR;      /* 0x00 access control */
    volatile uint32_t KEYR;     /* 0x04 key */
    volatile uint32_t OPTKEYR;  /* 0x08 option key */
    volatile uint32_t CR;       /* 0x0C control */
    volatile uint32_t SR;       /* 0x10 status */
    volatile uint32_t CCR;      /* 0x14 clear */
    volatile uint32_t PRAR_CUR; /* 0x18 */
    volatile uint32_t PRAR_PRG; /* 0x1C */
    volatile uint32_t SCAR_CUR; /* 0x20 */
    volatile uint32_t SCAR_PRG; /* 0x24 */
    volatile uint32_t WPSN_CUR; /* 0x28 */
    volatile uint32_t WPSN_PRG; /* 0x2C */
    volatile uint32_t BOOT_CUR; /* 0x30 */
    volatile uint32_t BOOT_PRG; /* 0x34 */
    volatile uint32_t OPTCR;    /* 0x18 */
    volatile uint32_t OPTSR;    /* 0x3C */
    volatile uint32_t OPTCCR;   /* 0x40 */
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef *)FLASH_BASE)
#define FLASH_ACR_LATENCY_MASK 0xFu
#define FLASH_ACR_PRFTEN       (1u << 8)
#define FLASH_ACR_WRHIGHFREQ_MASK 0x3u

/* ---- SPI ---------------------------------------------------------------- */

#define SPI1_BASE (PERIPH_APB2_BASE + 0x3000)
#define SPI4_BASE (PERIPH_APB2_BASE + 0x4000)

typedef struct {
    volatile uint32_t CR1;     /* 0x00 control 1 */
    volatile uint32_t CR2;     /* 0x04 control 2 */
    volatile uint32_t CFG1;    /* 0x08 config 1 */
    volatile uint32_t CFG2;    /* 0x0C config 2 */
    volatile uint32_t IER;     /* 0x10 interrupt enable */
    volatile uint32_t SR;      /* 0x14 status */
    volatile uint32_t IFCR;    /* 0x18 interrupt flag clear */
    volatile uint32_t TXDR;    /* 0x1C TX data (8/16/32 bit) */
    volatile uint32_t RXDR;    /* 0x20 RX data */
    volatile uint32_t RESERVED[2]; /* 0x24-0x28 */
    volatile uint32_t I2SCFGR; /* 0x2C I2S config */
    volatile uint32_t I2SPR;   /* 0x30 I2S prescaler */
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)
#define SPI4 ((SPI_TypeDef *)SPI4_BASE)

#define SPI_CR1_CSTART    (1u << 22)
#define SPI_CR1_SPE       (1u << 0)
#define SPI_CFG1_MASTER   (1u << 2)
#define SPI_CFG1_RXDMAEN  (1u << 15)
#define SPI_CFG1_TXDMAEN  (1u << 12)
#define SPI_CFG1_MBR_DIV4 (2u << 28)
#define SPI_CFG1_DSIZE_8  (7u << 0)
#define SPI_CFG1_DSIZE_16 (15u << 0)
#define SPI_CFG2_CPOL     (1u << 0)
#define SPI_CFG2_CPHA     (1u << 1)
#define SPI_CFG2_MASTER   (1u << 2)
#define SPI_CFG2_SSOE     (1u << 6)
#define SPI_CFG2_AFCNTR   (1u << 31)
#define SPI_SR_RXP        (1u << 0)
#define SPI_SR_TXP        (1u << 1)
#define SPI_SR_EOT        (1u << 3)
#define SPI_SR_OVR        (1u << 6)
#define SPI_SR_TXTF       (1u << 5)
#define SPI_IFCR_CLEAR    0xFFFFFFFFu

/* ---- USART --------------------------------------------------------------- */

#define USART1_BASE (PERIPH_APB2_BASE + 0x1000)
#define USART3_BASE (PERIPH_APB1_BASE + 0x4800)
#define UART4_BASE  (PERIPH_APB1_BASE + 0x4C00)

typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t CR3;   /* 0x08 */
    volatile uint32_t BRR;   /* 0x0C baudrate */
    volatile uint32_t GTDR;  /* 0x10 guard time / data */
    volatile uint32_t RTOR;  /* 0x14 receiver timeout */
    volatile uint32_t RQR;   /* 0x18 request */
    volatile uint32_t ISR;   /* 0x1C interrupt status */
    volatile uint32_t ICR;   /* 0x20 interrupt clear */
    volatile uint32_t RDR;   /* 0x24 receive data */
    volatile uint32_t TDR;   /* 0x28 transmit data */
} USART_TypeDef;

#define USART1 ((USART_TypeDef *)USART1_BASE)
#define USART3 ((USART_TypeDef *)USART3_BASE)
#define UART4  ((USART_TypeDef *)UART4_BASE)

#define USART_CR1_UE     (1u << 0)
#define USART_CR1_RE     (1u << 2)
#define USART_CR1_TE     (1u << 3)
#define USART_CR1_RXNEIE (1u << 5)
#define USART_CR1_TCIE   (1u << 6)
#define USART_CR1_DMAR   (1u << 7)
#define USART_CR1_DMAT   (1u << 7)
#define USART_CR1_M      (1u << 12)
#define USART_CR1_OVER8  (1u << 15)
#define USART_CR3_DMAT   (1u << 7)
#define USART_CR3_DMAR   (1u << 6)
#define USART_CR3_CTSE   (1u << 9)
#define USART_CR3_RTSE   (1u << 8)
#define USART_ISR_RXNE   (1u << 5)
#define USART_ISR_TXE    (1u << 7)
#define USART_ISR_TC     (1u << 6)
#define USART_ISR_BUSY   (1u << 16)

/* ---- I2C ---------------------------------------------------------------- */

#define I2C1_BASE (PERIPH_APB1_BASE + 0x5400)
#define I2C4_BASE (PERIPH_APB4_BASE + 0x5800)

typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t OAR1;    /* 0x08 own address 1 */
    volatile uint32_t OAR2;    /* 0x0C own address 2 */
    volatile uint32_t TIMINGR; /* 0x10 timing */
    volatile uint32_t TIMEOUTR;/* 0x14 timeout */
    volatile uint32_t ISR;     /* 0x18 interrupt status */
    volatile uint32_t ICR;     /* 0x1C interrupt clear */
    volatile uint32_t PECR;    /* 0x20 PEC */
    volatile uint32_t RXDR;   /* 0x24 RX data */
    volatile uint32_t TXDR;   /* 0x28 TX data */
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)
#define I2C4 ((I2C_TypeDef *)I2C4_BASE)

#define I2C_CR1_PE          (1u << 0)
#define I2C_CR2_START       (1u << 13)
#define I2C_CR2_STOP        (1u << 14)
#define I2C_CR2_NACK        (1u << 15)
#define I2C_CR2_AUTOEND     (1u << 25)
#define I2C_CR2_RELOAD      (1u << 24)
#define I2C_CR2_NBYTES_SHIFT 16
#define I2C_CR2_ADD10       (1u << 11)
#define I2C_CR2_RD_WRN      (1u << 10)
#define I2C_ISR_TXE         (1u << 0)
#define I2C_ISR_RXNE        (1u << 2)
#define I2C_ISR_TC          (1u << 6)
#define I2C_ISR_STOPF       (1u << 5)
#define I2C_ISR_NACKF       (1u << 4)
#define I2C_ISR_BUSY        (1u << 15)

/* ---- DMA (DMA1, DMA2) --------------------------------------------------- */

#define DMA1_BASE (PERIPH_AHB1_BASE + 0x0000)
#define DMA2_BASE (PERIPH_AHB1_BASE + 0x0400)

typedef struct {
    volatile uint32_t ISR;    /* 0x00 int status (low) */
    volatile uint32_t RESERVED0;
    volatile uint32_t ISRHI;  /* 0x08 int status (high) */
    volatile uint32_t RESERVED1;
    volatile uint32_t IFCR;   /* 0x10 int flag clear (low) */
    volatile uint32_t RESERVED2;
    volatile uint32_t IFCRHI; /* 0x18 int flag clear (high) */
} DMA_Common_TypeDef;

typedef struct {
    volatile uint32_t CR;     /* 0x00 channel config */
    volatile uint32_t NDTR;   /* 0x04 data count */
    volatile uint32_t PAR;    /* 0x08 peripheral address */
    volatile uint32_t M0AR;   /* 0x0C memory address 0 */
    volatile uint32_t M1AR;   /* 0x10 memory address 1 */
    volatile uint32_t FCR;    /* 0x14 FIFO control */
} DMA_Stream_TypeDef;

typedef struct {
    DMA_Common_TypeDef Common;
    DMA_Stream_TypeDef Stream[8];
} DMA_TypeDef;

#define DMA1 ((DMA_TypeDef *)DMA1_BASE)
#define DMA2 ((DMA_TypeDef *)DMA2_BASE)

#define DMA_CR_EN         (1u << 0)
#define DMA_CR_DMEIE      (1u << 1)
#define DMA_CR_TEIE       (1u << 2)
#define DMA_CR_HTIE       (1u << 3)
#define DMA_CR_TCIE       (1u << 4)
#define DMA_CR_PFCTRL     (1u << 5)
#define DMA_CR_DIR_P2M    (0u << 6)
#define DMA_CR_DIR_M2P    (1u << 6)
#define DMA_CR_DIR_M2M    (2u << 6)
#define DMA_CR_CIRC       (1u << 8)
#define DMA_CR_PINC       (1u << 9)
#define DMA_CR_MINC       (1u << 10)
#define DMA_CR_PSIZE_8    (0u << 11)
#define DMA_CR_PSIZE_16   (1u << 11)
#define DMA_CR_PSIZE_32   (2u << 11)
#define DMA_CR_MSIZE_8    (0u << 13)
#define DMA_CR_MSIZE_16   (1u << 13)
#define DMA_CR_MSIZE_32   (2u << 13)
#define DMA_CR_PINCOS     (1u << 15)
#define DMA_CR_PL_LOW     (0u << 16)
#define DMA_CR_PL_MED     (1u << 16)
#define DMA_CR_PL_HIGH    (2u << 16)
#define DMA_CR_PL_VHIGH   (3u << 16)
#define DMA_CR_DBM        (1u << 19)
#define DMA_CR_CT         (1u << 19)
#define DMA_CR_PBURST_4   (1u << 21)
#define DMA_CR_MBURST_4   (1u << 23)

/* ---- TIM (timers) ------------------------------------------------------- */

#define TIM1_BASE  (PERIPH_APB2_BASE + 0x0000)
#define TIM6_BASE  (PERIPH_APB1_BASE + 0x1000)
#define TIM8_BASE  (PERIPH_APB2_BASE + 0x0400)

typedef struct {
    volatile uint32_t CR1;   /* 0x00 */
    volatile uint32_t CR2;   /* 0x04 */
    volatile uint32_t SMCR;  /* 0x08 */
    volatile uint32_t DIER;  /* 0x0C DMA/IE */
    volatile uint32_t SR;    /* 0x10 status */
    volatile uint32_t EGR;   /* 0x14 event gen */
    volatile uint32_t CCMR1; /* 0x18 */
    volatile uint32_t CCMR2; /* 0x1C */
    volatile uint32_t CCER;  /* 0x20 */
    volatile uint32_t CNT;   /* 0x24 counter */
    volatile uint32_t PSC;   /* 0x28 prescaler */
    volatile uint32_t ARR;   /* 0x2C auto-reload */
    volatile uint32_t RCR;   /* 0x30 repetition */
    volatile uint32_t CCR1;  /* 0x34 */
    volatile uint32_t CCR2;  /* 0x38 */
    volatile uint32_t CCR3;  /* 0x3C */
    volatile uint32_t CCR4;  /* 0x40 */
    volatile uint32_t BDTR;  /* 0x44 break & deadtime */
    volatile uint32_t DCR;   /* 0x48 DMA control */
    volatile uint32_t DMAR;  /* 0x4C DMA address */
    volatile uint32_t OR;    /* 0x50 option */
} TIM_TypeDef;

#define TIM1 ((TIM_TypeDef *)TIM1_BASE)
#define TIM6 ((TIM_TypeDef *)TIM6_BASE)
#define TIM8 ((TIM_TypeDef *)TIM8_BASE)

#define TIM_CR1_CEN  (1u << 0)
#define TIM_CR1_ARPE (1u << 7)
#define TIM_DIER_UIE (1u << 0)
#define TIM_SR_UIF   (1u << 0)

/* ---- DAC ---------------------------------------------------------------- */

#define DAC_BASE (PERIPH_APB1_BASE + 0x7800)

typedef struct {
    volatile uint32_t CR;      /* 0x00 */
    volatile uint32_t SWTRIGR; /* 0x04 software trigger */
    volatile uint32_t DHR12R1; /* 0x08 data holding 12-bit right ch1 */
    volatile uint32_t DHR12L1; /* 0x0C data holding 12-bit left ch1 */
    volatile uint32_t DHR8R1;  /* 0x10 data holding 8-bit ch1 */
    volatile uint32_t DHR12R2; /* 0x14 */
    volatile uint32_t DHR12L2; /* 0x18 */
    volatile uint32_t DHR8R2;  /* 0x1C */
    volatile uint32_t DHR12RD; /* 0x20 dual */
    volatile uint32_t DHR12LD; /* 0x24 */
    volatile uint32_t DHR8RD;  /* 0x28 */
    volatile uint32_t DOR1;    /* 0x2C data output ch1 */
    volatile uint32_t DOR2;    /* 0x30 */
    volatile uint32_t SR;      /* 0x34 status */
} DAC_TypeDef;

#define DAC1 ((DAC_TypeDef *)DAC_BASE)
#define DAC_CR_EN1    (1u << 0)
#define DAC_CR_DMAEN1 (1u << 12)
#define DAC_CR_TEN1   (1u << 2)
#define DAC_CR_TSEL1_SW (0u << 3)

/* ---- ADC ---------------------------------------------------------------- */

#define ADC1_BASE (PERIPH_AHB1_BASE + 0x0000) /* placeholder, real ADC on APB2 */
#define ADC_BASE  (PERIPH_AHB4_BASE + 0x0000) /* ADC common */

typedef struct {
    volatile uint32_t ISR;   /* 0x00 */
    volatile uint32_t IER;   /* 0x04 */
    volatile uint32_t CR;    /* 0x08 */
    volatile uint32_t CFGR;  /* 0x0C */
    volatile uint32_t SQR1;  /* 0x10 */
    volatile uint32_t SQR2;  /* 0x14 */
    volatile uint32_t SQR3;  /* 0x18 */
    volatile uint32_t SQR4;  /* 0x1C */
    volatile uint32_t DR;    /* 0x20 */
    volatile uint32_t RESERVED[8]; /* 0x24-0x40 */
    volatile uint32_t SMPR1; /* 0x44 */
    volatile uint32_t SMPR2; /* 0x48 */
} ADC_TypeDef;

#define ADC1 ((ADC_TypeDef *)(PERIPH_AHB4_BASE + 0x0000))
#define ADC_CR_ADEN  (1u << 0)
#define ADC_CR_ADSTART (1u << 2)
#define ADC_ISR_ADRDY (1u << 0)
#define ADC_ISR_EOC   (1u << 2)

/* ---- SDMMC -------------------------------------------------------------- */

#define SDMMC1_BASE (PERIPH_AHB3_BASE + 0x0000)

typedef struct {
    volatile uint32_t POWER;  /* 0x00 */
    volatile uint32_t CLKCR;  /* 0x04 clock control */
    volatile uint32_t ARG;    /* 0x08 argument */
    volatile uint32_t CMD;    /* 0x0C command */
    volatile uint32_t RESPCMD;/* 0x10 response command */
    volatile uint32_t RESP1;  /* 0x14 response 1 */
    volatile uint32_t RESP2;  /* 0x18 */
    volatile uint32_t RESP3;  /* 0x1C */
    volatile uint32_t RESP4;  /* 0x20 */
    volatile uint32_t DTIMER; /* 0x24 data timer */
    volatile uint32_t DLEN;   /* 0x28 data length */
    volatile uint32_t DCTRL;  /* 0x2C data control */
    volatile uint32_t DCOUNT; /* 0x30 data count */
    volatile uint32_t STA;    /* 0x34 status */
    volatile uint32_t ICR;    /* 0x38 interrupt clear */
    volatile uint32_t MASK;   /* 0x3C interrupt mask */
    volatile uint32_t RESERVED[2]; /* 0x40-0x44 */
    volatile uint32_t FIFOCNT;/* 0x48 FIFO count */
    volatile uint32_t FIFO;   /* 0x4C FIFO */
} SDMMC_TypeDef;

#define SDMMC1 ((SDMMC_TypeDef *)SDMMC1_BASE)
#define SDMMC_POWER_PWRCTRL_ON 0x3u
#define SDMMC_CLKCR_CLKEN      (1u << 8)
#define SDMMC_CMD_CPSMEN       (1u << 10)
#define SDMMC_STA_RXFIFOE      (1u << 19)
#define SDMMC_STA_RXFIFOHF     (1u << 15)
#define SDMMC_STA_TXFIFOE      (1u << 18)
#define SDMMC_STA_DATAEND      (1u << 8)
#define SDMMC_STA_CMDREND      (1u << 6)
#define SDMMC_ICR_CLEAR_ALL    0xFFFFFFFFu

/* ---- USB (USB OTG-HS) --------------------------------------------------- */

#define USB1_BASE (PERIPH_AHB1_BASE + 0x0000) /* OTG1 */

/* Minimal — we only reference the base for clock enable here. */

/* ---- SysTick ------------------------------------------------------------ */

#define SCB_VTOR  (*(volatile uint32_t *)0xE000ED08u)
#define SysTick_BASE 0xE000E010u

typedef struct {
    volatile uint32_t CTRL;   /* 0x00 */
    volatile uint32_t LOAD;   /* 0x04 */
    volatile uint32_t VAL;    /* 0x08 */
    volatile uint32_t CALIB;  /* 0x0C */
} SysTick_TypeDef;

#define SysTick ((SysTick_TypeDef *)SysTick_BASE)
#define SysTick_CTRL_CLKSOURCE (1u << 2)
#define SysTick_CTRL_TICKINT   (1u << 1)
#define SysTick_CTRL_ENABLE    (1u << 0)

/* ---- NVIC --------------------------------------------------------------- */

#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100u)
#define NVIC_ICER0 (*(volatile uint32_t *)0xE000E180u)
#define NVIC_IPR_BASE ((volatile uint8_t *)0xE000E400u)

static inline void nvic_enable(uint32_t irq) {
    NVIC_ISER0 = (1u << (irq & 31));
}
static inline void nvic_set_priority(uint32_t irq, uint8_t prio) {
    NVIC_IPR_BASE[irq] = prio << 4;
}

/* ---- IRQ numbers (STM32H7) ---------------------------------------------- */

#define SDMMC1_IRQn        49
#define SPI1_IRQn          35
#define USART1_IRQn        37
#define USART3_IRQn        39
#define DMA1_Stream0_IRQn  11
#define DMA1_Stream1_IRQn  12
#define DMA1_Stream2_IRQn  13
#define DMA1_Stream3_IRQn  14
#define DMA1_Stream4_IRQn  15
#define DMA2_Stream0_IRQn  56
#define DMA2_Stream1_IRQn  57
#define DMA2_Stream2_IRQn  58
#define DMA2_Stream3_IRQn  59
#define TIM6_DAC_IRQn      54
#define TIM1_UP_IRQn       25

#endif /* SPECKLEFLOW_REGISTERS_H */