/*
 * registers.h — STM32G474RET6 register definitions (lightweight, no HAL).
 *                Only the peripherals used by LithoCore are defined.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_REGISTERS_H
#define LITHOCORE_REGISTERS_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * Base addresses
 * ------------------------------------------------------------------------- */
#define PERIPH_BASE         0x40000000U

#define GPIOA_BASE          (PERIPH_BASE + 0x00020000U)
#define GPIOB_BASE          (PERIPH_BASE + 0x00020400U)
#define GPIOC_BASE          (PERIPH_BASE + 0x00020800U)
#define GPIOD_BASE          (PERIPH_BASE + 0x00020C00U)
#define GPIOF_BASE          (PERIPH_BASE + 0x00022000U)

#define RCC_BASE            (PERIPH_BASE + 0x00021000U)
#define PWR_BASE            (PERIPH_BASE + 0x00000000U)
#define FLASH_REG_BASE      (PERIPH_BASE + 0x00022000U)
#define SYSCFG_BASE         (PERIPH_BASE + 0x00010000U)
#define EXTI_BASE           (PERIPH_BASE + 0x00010400U)
#define DMA1_BASE           (PERIPH_BASE + 0x00020000U + 0x8000U)
#define DMAMUX1_BASE        (PERIPH_BASE + 0x00020800U)

#define ADC1_BASE           (PERIPH_BASE + 0x00012400U)
#define ADC12_BASE          (PERIPH_BASE + 0x00012400U)
#define SPI1_BASE           (PERIPH_BASE + 0x00013000U)
#define SPI3_BASE           (PERIPH_BASE + 0x00003C00U)
#define USART1_BASE         (PERIPH_BASE + 0x00013800U)
#define USART3_BASE         (PERIPH_BASE + 0x00004400U)
#define TIM1_BASE           (PERIPH_BASE + 0x00012C00U)
#define TIM2_BASE           (PERIPH_BASE + 0x00000000U)
#define TIM6_BASE           (PERIPH_BASE + 0x00001000U)
#define TIM7_BASE           (PERIPH_BASE + 0x00001400U)
#define USB_BASE            (PERIPH_BASE + 0x00006800U)
#define CORDIC_BASE         (PERIPH_BASE + 0x00020C00U)
#define FMAC_BASE           (PERIPH_BASE + 0x00021000U)
#define RTC_BASE            (PERIPH_BASE + 0x00002800U)
#define WWDG_BASE           (PERIPH_BASE + 0x00002C00U)
#define IWDG_BASE           (PERIPH_BASE + 0x00003000U)
#define NVIC_BASE           (0xE000E100U)
#define SYSTICK_BASE        (0xE000E010U)
#define SCB_BASE            (0xE000ED00U)

/* -------------------------------------------------------------------------
 * GPIO register layout
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
    volatile uint32_t OTYPER;   /* 0x04 */
    volatile uint32_t OSPEEDR;  /* 0x08 */
    volatile uint32_t PUPDR;    /* 0x0C */
    volatile uint32_t IDR;      /* 0x10 */
    volatile uint32_t ODR;      /* 0x14 */
    volatile uint32_t BSRR;     /* 0x18 — set: bits[15:0], reset: bits[31:16] */
    volatile uint32_t LCKR;     /* 0x1C */
    volatile uint32_t AFRL;     /* 0x20 */
    volatile uint32_t AFRH;     /* 0x24 */
    volatile uint32_t BRR;      /* 0x28 */
    uint32_t reserved[3];
} GPIO_TypeDef;

#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOF   ((GPIO_TypeDef *)GPIOF_BASE)

/* GPIO MODER values */
#define GPIO_MODER_INPUT      0x00U
#define GPIO_MODER_OUTPUT     0x01U
#define GPIO_MODER_AF         0x02U
#define GPIO_MODER_ANALOG     0x03U

/* -------------------------------------------------------------------------
 * RCC register layout (simplified — key registers only)
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR;       /* 0x00 */
    volatile uint32_t ICSCR;    /* 0x04 */
    volatile uint32_t CRRCR;    /* 0x08 */
    volatile uint32_t CFGR;     /* 0x0C */
    volatile uint32_t PLLCFGR;  /* 0x10 */
    volatile uint32_t RESERVED0;/* 0x14 */
    volatile uint32_t CIFR;     /* 0x18 */
    volatile uint32_t CICR;     /* 0x1C */
    volatile uint32_t RESERVED1;/* 0x20 */
    volatile uint32_t BDCR;     /* 0x24 */
    volatile uint32_t CSR;      /* 0x28 */
    volatile uint32_t CRRCR2;   /* 0x2C */
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)RCC_BASE)

/* RCC CR bits */
#define RCC_CR_HSION      (1U << 8)
#define RCC_CR_HSIRDY     (1U << 10)
#define RCC_CR_HSEON      (1U << 16)
#define RCC_CR_HSERDY     (1U << 17)
#define RCC_CR_HSEBYP     (1U << 18)
#define RCC_CR_PLLON      (1U << 24)
#define RCC_CR_PLLRDY     (1U << 25)

/* RCC CFGR bits */
#define RCC_CFGR_SW_HSI   0x0U
#define RCC_CFGR_SW_HSE   0x1U
#define RCC_CFGR_SW_PLL   0x3U
#define RCC_CFGR_SWS_MASK (0x3U << 2)
#define RCC_CFGR_SWS_PLL  (0x3U << 2)

/* -------------------------------------------------------------------------
 * PWR registers
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t CR3;      /* 0x08 */
    volatile uint32_t CR4;      /* 0x0C */
    volatile uint32_t SR1;      /* 0x10 */
    volatile uint32_t SR2;      /* 0x14 */
    volatile uint32_t SCR;      /* 0x18 */
} PWR_TypeDef;

#define PWR ((PWR_TypeDef *)PWR_BASE)

#define PWR_CR1_LPMS_STOP2  (0x2U << 0)

/* -------------------------------------------------------------------------
 * SPI register layout
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t SR;       /* 0x08 */
    volatile uint32_t DR;       /* 0x0C */
    volatile uint32_t CRCPR;    /* 0x10 */
    volatile uint32_t RXCRCR;   /* 0x14 */
    volatile uint32_t TXCRCR;   /* 0x18 */
    volatile uint32_t I2SCFGR;  /* 0x1C */
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)
#define SPI3 ((SPI_TypeDef *)SPI3_BASE)

#define SPI_CR1_SPE        (1U << 6)
#define SPI_CR1_MSTR       (1U << 2)
#define SPI_CR1_CPOL       (1U << 1)
#define SPI_CR1_CPHA       (1U << 0)
#define SPI_CR1_BR_DIV2    (0x0U << 3)
#define SPI_CR1_BR_DIV4    (0x1U << 3)
#define SPI_CR1_BR_DIV8    (0x2U << 3)
#define SPI_CR1_BR_DIV16   (0x3U << 3)
#define SPI_CR1_BR_DIV32   (0x4U << 3)
#define SPI_CR1_BR_DIV64   (0x5U << 3)
#define SPI_CR1_BR_DIV128  (0x6U << 3)
#define SPI_CR1_BR_DIV256  (0x7U << 3)
#define SPI_CR1_SSM        (1U << 9)
#define SPI_CR1_SSI        (1U << 8)
#define SPI_CR1_LSBFIRST   (1U << 7)

#define SPI_CR2_DS_8BIT    (0x7U << 8)
#define SPI_CR2_DS_16BIT   (0xFU << 8)
#define SPI_CR2_RXNEIE     (1U << 6)
#define SPI_CR2_TXEIE      (1U << 7)
#define SPI_CR2_FRXTH      (1U << 2)

#define SPI_SR_RXNE        (1U << 0)
#define SPI_SR_TXE         (1U << 1)
#define SPI_SR_BSY         (1U << 7)
#define SPI_SR_MODF        (1U << 4)
#define SPI_SR_OVR         (1U << 6)

/* -------------------------------------------------------------------------
 * USART register layout
 * ------------------------------------------------------------------------- */
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

#define USART1 ((USART_TypeDef *)USART1_BASE)
#define USART3 ((USART_TypeDef *)USART3_BASE)

#define USART_CR1_UE       (1U << 0)
#define USART_CR1_RE       (1U << 2)
#define USART_CR1_TE       (1U << 3)
#define USART_CR1_RXNEIE   (1U << 5)
#define USART_CR1_TCIE     (1U << 6)
#define USART_CR1_IDLEIE   (1U << 4)

#define USART_ISR_RXNE     (1U << 5)
#define USART_ISR_TC       (1U << 6)
#define USART_ISR_IDLE     (1U << 4)
#define USART_ISR_ORE      (1U << 3)
#define USART_ISR_FE       (1U << 1)
#define USART_ISR_NE       (1U << 2)

/* -------------------------------------------------------------------------
 * Timer register layout (simplified — TIM1, TIM6)
 * ------------------------------------------------------------------------- */
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
    volatile uint32_t RCR;      /* 0x30 */
    volatile uint32_t CCR1;     /* 0x34 */
    volatile uint32_t CCR2;     /* 0x38 */
    volatile uint32_t CCR3;     /* 0x3C */
    volatile uint32_t CCR4;     /* 0x40 */
    volatile uint32_t BDTR;     /* 0x44 */
    volatile uint32_t DCR;      /* 0x48 */
    volatile uint32_t DMAR;     /* 0x4C */
    volatile uint32_t OR;       /* 0x50 */
} TIM_TypeDef;

#define TIM1 ((TIM_TypeDef *)TIM1_BASE)
#define TIM2 ((TIM_TypeDef *)TIM2_BASE)
#define TIM6 ((TIM_TypeDef *)TIM6_BASE)
#define TIM7 ((TIM_TypeDef *)TIM7_BASE)

#define TIM_CR1_CEN        (1U << 0)
#define TIM_CR1_ARPE       (1U << 7)
#define TIM_DIER_UIE       (1U << 0)
#define TIM_DIER_CC1IE     (1U << 1)
#define TIM_SR_UIF         (1U << 0)
#define TIM_SR_CC1IF       (1U << 1)
#define TIM_BDTR_MOE       (1U << 15)

/* TIM CCMR — output compare PWM mode 1 */
#define TIM_CCMR1_OC1M_PWM1  (0x6U << 4)
#define TIM_CCMR1_OC1PE      (1U << 3)
#define TIM_CCER_CC1E        (1U << 0)

/* -------------------------------------------------------------------------
 * ADC register layout (STM32G4 ADC — simplified)
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t ISR;      /* 0x00 */
    volatile uint32_t IER;      /* 0x04 */
    volatile uint32_t CR;       /* 0x08 */
    volatile uint32_t CFGR;     /* 0x0C */
    volatile uint32_t CFGR2;    /* 0x10 */
    volatile uint32_t SMPR1;    /* 0x14 */
    volatile uint32_t SMPR2;    /* 0x18 */
    volatile uint32_t TR1;      /* 0x1C */
    volatile uint32_t TR2;      /* 0x20 */
    volatile uint32_t TR3;      /* 0x24 */
    volatile uint32_t RESERVED0;/* 0x28 */
    volatile uint32_t SQR1;     /* 0x30 */
    volatile uint32_t SQR2;     /* 0x34 */
    volatile uint32_t SQR3;     /* 0x38 */
    volatile uint32_t SQR4;     /* 0x3C */
    volatile uint32_t DR;       /* 0x40 */
    volatile uint32_t RESERVED1[8];
    volatile uint32_t AWD2CR;   /* 0xA0 */
    volatile uint32_t AWD3CR;   /* 0xA4 */
} ADC_TypeDef;

#define ADC1 ((ADC_TypeDef *)ADC1_BASE)

#define ADC_ISR_ADRDY      (1U << 0)
#define ADC_ISR_EOC        (1U << 2)
#define ADC_ISR_EOS        (1U << 3)
#define ADC_ISR_AWD1       (1U << 7)
#define ADC_CR_ADEN        (1U << 0)
#define ADC_CR_ADSTART     (1U << 2)
#define ADC_CR_ADDIS       (1U << 1)
#define ADC_CR_ADCAL       (1U << 31)
#define ADC_IER_EOCIE      (1U << 2)
#define ADC_IER_EOSIE      (1U << 3)

/* -------------------------------------------------------------------------
 * CORDIC register layout (hardware math accelerator)
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CSR;      /* 0x00 — control/status */
    volatile uint32_t RESERVED0;
    volatile uint32_t RDATA;    /* 0x08 — read result */
    volatile uint32_t RDATA32;  /* 0x0C — 32-bit result */
    volatile uint32_t WDATA;    /* 0x10 — write argument */
    volatile uint32_t WDATA32;  /* 0x14 — 32-bit argument */
} CORDIC_TypeDef;

#define CORDIC ((CORDIC_TypeDef *)CORDIC_BASE)

/* CORDIC CSR fields */
#define CORDIC_CSR_FUNC_COSINE   0x00U
#define CORDIC_CSR_FUNC_SINE     0x01U
#define CORDIC_CSR_FUNC_PHASE    0x02U
#define CORDIC_CSR_FUNC_MODULUS  0x03U
#define CORDIC_CSR_FUNC_ATAN     0x04U
#define CORDIC_CSR_FUNC_COSHyper 0x05U
#define CORDIC_CSR_FUNC_SINHyper 0x06U
#define CORDIC_CSR_FUNC_ATANHyper 0x07U
#define CORDIC_CSR_FUNC_ROTATION 0x08U
#define CORDIC_CSR_FUNC_ADD      0x09U

#define CORDIC_CSR_SCALE_SHIFT   16
#define CORDIC_CSR_PRECISION_SHIFT 8
#define CORDIC_CSR_NRES         (1U << 4)
#define CORDIC_CSR_NARGS        (1U << 1)
#define CORDIC_CSR_IREADY       (1U << 23)
#define CORDIC_CSR_IWRITE       (1U << 22)
#define CORDIC_CSR_RRDY         (1U << 20)

/* -------------------------------------------------------------------------
 * FMAC register layout (filter math accelerator)
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t X1BUFCFG;  /* 0x00 */
    volatile uint32_t X2BUFCFG;  /* 0x04 */
    volatile uint32_t YBUFCFG;   /* 0x08 */
    volatile uint32_t RESERVED0;
    volatile uint32_t PARAM;     /* 0x10 */
    volatile uint32_t YSIZE;     /* 0x14 */
    volatile uint32_t XSIZE;     /* 0x18 */
    volatile uint32_t YFULL;     /* 0x1C — actually YSIZE access port */
    volatile uint32_t RESERVED1;
    volatile uint32_t X1CFG;     /* 0x24 */
    volatile uint32_t X2CFG;     /* 0x28 */
    volatile uint32_t RESERVED2[2];
    volatile uint32_t Y;         /* 0x34 — output read */
    volatile uint32_t X1;        /* 0x38 — X1 write */
    volatile uint32_t X2;        /* 0x3C — X2 write */
    volatile uint32_t RESERVED3[2];
    volatile uint32_t SR;        /* 0x48 */
    volatile uint32_t IER;       /* 0x4C */
} FMAC_TypeDef;

#define FMAC ((FMAC_TypeDef *)FMAC_BASE)

#define FMAC_SR_YEMPTY    (1U << 0)
#define FMAC_SR_YFULL     (1U << 1)
#define FMAC_SR_X1EMPTY   (1U << 2)
#define FMAC_SR_X1FULL    (1U << 3)
#define FMAC_SR_X2EMPTY   (1U << 4)
#define FMAC_SR_X2FULL    (1U << 5)

/* -------------------------------------------------------------------------
 * SysTick / NVIC / SCB
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CTRL;     /* 0x00 */
    volatile uint32_t LOAD;     /* 0x04 */
    volatile uint32_t VAL;      /* 0x08 */
    volatile uint32_t CALIB;    /* 0x0C */
} SysTick_TypeDef;

#define SysTick ((SysTick_TypeDef *)SYSTICK_BASE)

#define SysTick_CTRL_ENABLE  (1U << 0)
#define SysTick_CTRL_TICKINT (1U << 1)
#define SysTick_CTRL_CLKSOURCE (1U << 2)

/* NVIC — ISER/ICER at offset 0x000/0x080 */
#define NVIC_ISER0   (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ICER0   (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ISPR0   (*(volatile uint32_t *)(NVIC_BASE + 0x100))
#define NVIC_ICPR0   (*(volatile uint32_t *)(NVIC_BASE + 0x180))
#define NVIC_IP_BASE (NVIC_BASE + 0x300)

/* SCB */
#define SCB_SCR      (*(volatile uint32_t *)(SCB_BASE + 0x10))
#define SCB_SCR_SLEEPDEEP  (1U << 2)
#define SCB_AIRCR    (*(volatile uint32_t *)(SCB_BASE + 0x0C))
#define SCB_VTOR     (*(volatile uint32_t *)(SCB_BASE + 0x08))

/* -------------------------------------------------------------------------
 * IRQ numbers (STM32G4 — used for NVIC enable/clear)
 * ------------------------------------------------------------------------- */
#define IRQ_USART1        53
#define IRQ_USART3        39
#define IRQ_SPI1          35
#define IRQ_SPI3          51
#define IRQ_TIM1_UP       24
#define IRQ_TIM6_DAC      54
#define IRQ_ADC1_2        18
#define IRQ_EXTI9_5       23
#define IRQ_EXTI15_10     40
#define IRQ_USB_HP        74
#define IRQ_USB_LP        75

/* -------------------------------------------------------------------------
 * Flash (for storage.c and DFU)
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t ACR;      /* 0x00 */
    volatile uint32_t KEYR;     /* 0x04 */
    volatile uint32_t OPTKEYR;  /* 0x08 */
    volatile uint32_t SR;       /* 0x0C */
    volatile uint32_t CR;       /* 0x10 */
    volatile uint32_t ECCR;     /* 0x14 */
    volatile uint32_t RESERVED0;
    volatile uint32_t OPTR;     /* 0x1C */
    volatile uint32_t PCROP1SR; /* 0x20 */
    volatile uint32_t PCROP1ER; /* 0x24 */
    volatile uint32_t WRP1AR;   /* 0x28 */
    volatile uint32_t WRP1BR;   /* 0x2C */
} FLASH_TypeDef;

#define FLASH_REG ((FLASH_TypeDef *)FLASH_REG_BASE)

#define FLASH_SR_BSY       (1U << 16)
#define FLASH_SR_EOP       (1U << 0)
#define FLASH_CR_LOCK      (1U << 31)
#define FLASH_CR_PG        (1U << 0)
#define FLASH_CR_PER       (1U << 1)
#define FLASH_CR_MER1      (1U << 2)
#define FLASH_CR_STRT      (1U << 16)
#define FLASH_CR_PNB_SHIFT 3

#define FLASH_PAGE_SIZE    2048U   /* STM32G4: 2 KB per page */
#define FLASH_PAGE_COUNT   256U    /* 512 KB / 2 KB */

/* -------------------------------------------------------------------------
 * RTC (for timestamping and wake-from-STOP2)
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t TR;       /* 0x00 — time register */
    volatile uint32_t DR;       /* 0x04 — date register */
    volatile uint32_t CR;       /* 0x08 */
    volatile uint32_t ISR;      /* 0x0C */
    volatile uint32_t PRER;     /* 0x10 */
    volatile uint32_t WUTR;     /* 0x14 */
    volatile uint32_t RESERVED0[2];
    volatile uint32_t ALRMAR;   /* 0x1C */
    volatile uint32_t ALRMBR;   /* 0x20 */
} RTC_TypeDef;

#define RTC ((RTC_TypeDef *)RTC_BASE)

#define RTC_CR_WUTE        (1U << 10)
#define RTC_CR_WUTIE       (1U << 14)
#define RTC_ISR_WUTF       (1U << 10)
#define RTC_ISR_RSF        (1U << 5)
#define RTC_ISR_INIT       (1U << 7)
#define RTC_ISR_INITF      (1U << 6)

#endif /* LITHOCORE_REGISTERS_H */