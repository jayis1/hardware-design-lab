/*
 * SkyPilot — registers.h — MMIO Register Map for STM32H743VIT6
 * Canonical copy extracted from phase4_software_stack.md
 */

#ifndef SKYPILOT_REGISTERS_H
#define SKYPILOT_REGISTERS_H

#include <stdint.h>

/* Base addresses */
#define RCC_BASE            0x58024400UL
#define GPIOA_BASE          0x58020000UL
#define GPIOB_BASE          0x58020400UL
#define GPIOC_BASE          0x58020800UL
#define GPIOD_BASE          0x58020C00UL
#define GPIOE_BASE          0x58021000UL
#define SPI1_BASE           0x40013000UL
#define SPI2_BASE           0x40003800UL
#define SPI4_BASE           0x40013400UL
#define I2C1_BASE           0x40005400UL
#define USART1_BASE         0x40011000UL
#define USART4_BASE         0x40004400UL
#define USART8_BASE         0x40007C00UL
#define TIM1_BASE           0x40012C00UL
#define TIM2_BASE           0x40000000UL
#define TIM3_BASE           0x40000400UL
#define TIM8_BASE           0x40013400UL
#define DMA1_BASE           0x40020000UL
#define DMA2_BASE           0x40020400UL
#define ADC1_BASE           0x40022000UL
#define FDCAN1_BASE         0x40006000UL
#define PWR_BASE            0x58004000UL
#define FLASH_BASE          0x52002000UL
#define SYSCFG_BASE         0x58001400UL
#define DBGMCU_BASE         0x58005400UL
#define NVIC_BASE           0xE000E100UL
#define SCB_BASE            0xE000ED00UL
#define IWDG_BASE           0x58004C00UL

/* Register types */
typedef volatile uint32_t reg32_t;
typedef volatile uint16_t reg16_t;
typedef volatile uint8_t  reg8_t;

/* RCC registers */
typedef struct {
    reg32_t CR;             /* 0x00 */
    reg32_t HSICFGR;        /* 0x04 */
    reg32_t CRRCR;          /* 0x08 */
    reg32_t CSICFGR;        /* 0x0C */
    reg32_t CFGR;           /* 0x10 */
    reg32_t D1CFGR;         /* 0x14 */
    reg32_t D2CFGR;         /* 0x18 */
    reg32_t D3CFGR;         /* 0x1C */
    reg32_t reserved0[2];
    reg32_t PLLCKSELR;      /* 0x28 */
    reg32_t PLLCFGR;        /* 0x2C */
    reg32_t PLL1DIVR;       /* 0x30 */
    reg32_t PLL1FRACR;      /* 0x34 */
    reg32_t PLL2DIVR;       /* 0x38 */
    reg32_t PLL2FRACR;      /* 0x3C */
    reg32_t PLL3DIVR;       /* 0x40 */
    reg32_t PLL3FRACR;      /* 0x44 */
    reg32_t reserved1;
    reg32_t D1CCIPR;        /* 0x4C */
    reg32_t D2CCIP1R;       /* 0x50 */
    reg32_t D2CCIP2R;       /* 0x54 */
    reg32_t D3CCIPR;        /* 0x58 */
    reg32_t reserved2;
    reg32_t CIER;           /* 0x60 */
    reg32_t CIFR;           /* 0x64 */
    reg32_t CICR;           /* 0x68 */
    reg32_t reserved3;
    reg32_t BDCR;           /* 0x70 */
    reg32_t CSR;            /* 0x74 */
    reg32_t reserved4[2];
    reg32_t AHB3ENR;        /* 0x80 */
    reg32_t AHB1ENR;        /* 0x84 */
    reg32_t AHB2ENR;        /* 0x88 */
    reg32_t AHB4ENR;        /* 0x8C */
    reg32_t APB1LENR;       /* 0x90 */
    reg32_t APB1HENR;       /* 0x94 */
    reg32_t APB2ENR;        /* 0x98 */
    reg32_t APB3ENR;        /* 0x9C */
    reg32_t APB4ENR;        /* 0xA0 */
} RCC_Type;

#define RCC ((RCC_Type *)RCC_BASE)

/* GPIO registers */
typedef struct {
    reg32_t MODER;     /* 0x00 */
    reg32_t OTYPER;    /* 0x04 */
    reg32_t OSPEEDR;   /* 0x08 */
    reg32_t PUPDR;     /* 0x0C */
    reg32_t IDR;       /* 0x10 */
    reg32_t ODR;       /* 0x14 */
    reg32_t BSRR;      /* 0x18 */
    reg32_t LCKR;      /* 0x1C */
    reg32_t AFR[2];    /* 0x20, 0x24 */
} GPIO_Type;

#define GPIOA ((GPIO_Type *)GPIOA_BASE)
#define GPIOB ((GPIO_Type *)GPIOB_BASE)
#define GPIOC ((GPIO_Type *)GPIOC_BASE)
#define GPIOD ((GPIO_Type *)GPIOD_BASE)
#define GPIOE ((GPIO_Type *)GPIOE_BASE)

/* SPI registers (H7 variant with CFG1/CFG2) */
typedef struct {
    reg32_t CR1;       /* 0x00 */
    reg32_t CR2;       /* 0x04 */
    reg32_t SR;        /* 0x08 (legacy) */
    reg32_t DR;        /* 0x0C (legacy) */
    reg32_t CRCPR;     /* 0x10 */
    reg32_t RXCRCR;    /* 0x14 */
    reg32_t TXCRCR;    /* 0x18 */
    reg32_t I2SCFGR;   /* 0x1C */
    reg32_t I2SPR;     /* 0x20 */
    reg32_t CFG1;      /* 0x28 */
    reg32_t CFG2;      /* 0x2C */
    reg32_t IER;       /* 0x30 */
    reg32_t SR_H7;     /* 0x34 */
    reg32_t IFCR;      /* 0x38 */
    reg32_t TXDR;      /* 0x3C */
    reg32_t RXDR;      /* 0x40 */
} SPI_Type;

#define SPI1 ((SPI_Type *)SPI1_BASE)
#define SPI2 ((SPI_Type *)SPI2_BASE)
#define SPI4 ((SPI_Type *)SPI4_BASE)

/* USART registers */
typedef struct {
    reg32_t CR1;       /* 0x00 */
    reg32_t CR2;       /* 0x04 */
    reg32_t CR3;       /* 0x08 */
    reg32_t BRR;       /* 0x0C */
    reg32_t GTPR;      /* 0x10 */
    reg32_t RTOR;      /* 0x14 */
    reg32_t RQR;       /* 0x18 */
    reg32_t ISR;       /* 0x1C */
    reg32_t ICR;       /* 0x20 */
    reg32_t RDR;       /* 0x24 */
    reg32_t TDR;       /* 0x28 */
} USART_Type;

#define USART1 ((USART_Type *)USART1_BASE)
#define USART4 ((USART_Type *)USART4_BASE)
#define USART8 ((USART_Type *)USART8_BASE)

/* Timer registers */
typedef struct {
    reg32_t CR1;       /* 0x00 */
    reg32_t CR2;       /* 0x04 */
    reg32_t SMCR;      /* 0x08 */
    reg32_t DIER;      /* 0x0C */
    reg32_t SR;        /* 0x10 */
    reg32_t EGR;       /* 0x14 */
    reg32_t CCMR1;     /* 0x18 */
    reg32_t CCMR2;     /* 0x1C */
    reg32_t CCER;      /* 0x20 */
    reg32_t CNT;       /* 0x24 */
    reg32_t PSC;       /* 0x28 */
    reg32_t ARR;       /* 0x2C */
    reg32_t RCR;       /* 0x30 */
    reg32_t CCR[4];    /* 0x34-0x40 */
    reg32_t BDTR;      /* 0x44 */
    reg32_t DCR;       /* 0x48 */
    reg32_t DMAR;      /* 0x4C */
} TIM_Type;

#define TIM1 ((TIM_Type *)TIM1_BASE)
#define TIM2 ((TIM_Type *)TIM2_BASE)

/* I2C registers */
typedef struct {
    reg32_t CR1;       /* 0x00 */
    reg32_t CR2;       /* 0x04 */
    reg32_t OAR1;      /* 0x08 */
    reg32_t OAR2;      /* 0x0C */
    reg32_t TIMINGR;   /* 0x10 */
    reg32_t TIMEOUTR;  /* 0x14 */
    reg32_t ISR;       /* 0x18 */
    reg32_t ICR;       /* 0x1C */
    reg32_t PECR;      /* 0x20 */
    reg32_t RXDR;      /* 0x24 */
    reg32_t TXDR;      /* 0x28 */
} I2C_Type;

#define I2C1 ((I2C_Type *)I2C1_BASE)

/* DMA stream registers (H7) */
typedef struct {
    reg32_t CR;        /* 0x00 */
    reg32_t NDTR;      /* 0x04 */
    reg32_t PAR;       /* 0x08 */
    reg32_t M0AR;      /* 0x0C */
    reg32_t M1AR;      /* 0x10 */
    reg32_t FCR;       /* 0x14 */
} DMA_Stream_Type;

/* IWDG registers */
typedef struct {
    reg32_t KR;        /* 0x00 */
    reg32_t PR;        /* 0x04 */
    reg32_t RLR;       /* 0x08 */
    reg32_t SR;        /* 0x0C */
    reg32_t WINR;      /* 0x10 */
} IWDG_Type;

#define IWDG ((IWDG_Type *)IWDG_BASE)

#endif /* SKYPILOT_REGISTERS_H */