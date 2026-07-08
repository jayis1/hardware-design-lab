/*
 * registers.h — Minimal STM32L476 register definitions for Lichenwatch.
 *
 * Only the registers/bits we actually touch are defined. This keeps the
 * firmware self-contained without dragging in a vendor header pack.
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef LICHENWATCH_REGISTERS_H
#define LICHENWATCH_REGISTERS_H

#include <stdint.h>

/* ------------------------------------------------------------------------ */
/* Base addresses                                                            */
/* ------------------------------------------------------------------------ */
#define PERIPH_BASE           0x40000000UL
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x18000000UL)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x08000000UL)
#define APB1PERIPH_BASE       (PERIPH_BASE + 0x00000000UL)
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000UL)

/* DMA controllers */
#define DMA1_BASE             (AHB1PERIPH_BASE + 0x00000000UL)
#define DMA2_BASE             (AHB1PERIPH_BASE + 0x00000400UL)

/* Clock system */
#define RCC_BASE              (AHB1PERIPH_BASE + 0x00001000UL)
#define PWR_BASE              (APB1PERIPH_BASE + 0x00007000UL)
#define FLASH_REGS_BASE       (AHB1PERIPH_BASE + 0x00002000UL)

/* GPIO ports */
#define GPIOA_BASE            (AHB2PERIPH_BASE + 0x00000000UL)
#define GPIOB_BASE            (AHB2PERIPH_BASE + 0x00000400UL)
#define GPIOC_BASE            (AHB2PERIPH_BASE + 0x00000800UL)
#define GPIOD_BASE            (AHB2PERIPH_BASE + 0x00000C00UL)
#define GPIOH_BASE            (AHB2PERIPH_BASE + 0x00001C00UL)

/* Timers */
#define TIM2_BASE             (APB1PERIPH_BASE + 0x00000000UL)
#define TIM3_BASE             (APB1PERIPH_BASE + 0x00000400UL)
#define TIM4_BASE             (APB1PERIPH_BASE + 0x00000800UL)

/* ADC */
#define ADC1_BASE             (AHB2PERIPH_BASE + 0x00002500UL)
#define ADC1_COMMON_BASE      (AHB2PERIPH_BASE + 0x00002600UL)

/* SPI */
#define SPI1_BASE             (APB2PERIPH_BASE + 0x00003000UL)
#define SPI2_BASE             (APB1PERIPH_BASE + 0x00003800UL)

/* I2C */
#define I2C1_BASE             (APB1PERIPH_BASE + 0x00005400UL)

/* USART */
#define USART1_BASE           (APB2PERIPH_BASE + 0x00003800UL)
#define USART3_BASE           (APB1PERIPH_BASE + 0x00004800UL)

/* LPUART1 */
#define LPUART1_BASE          (APB1PERIPH_BASE + 0x00004C00UL)

/* RTC */
#define RTC_BASE              (APB1PERIPH_BASE + 0x00002800UL)

/* EXTI */
#define EXTI_BASE             (APB2PERIPH_BASE + 0x00000000UL)

/* SysTick */
#define SYSTICK_BASE          0xE000E010UL

/* NVIC */
#define NVIC_BASE             0xE000E100UL
#define SCB_BASE               0xE000ED00UL

/* ------------------------------------------------------------------------ */
/* Register access macros                                                     */
/* ------------------------------------------------------------------------ */
#define REG32(addr)   (*(volatile uint32_t *)(addr))
#define REG16(addr)   (*(volatile uint16_t *)(addr))
#define REG8(addr)    (*(volatile uint8_t  *)(addr))

/* ------------------------------------------------------------------------ */
/* RCC — Reset & Clock Control                                               */
/* ------------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR;       /* 0x00 clock control         */
    volatile uint32_t ICSCR;    /* 0x04 internal clock sel    */
    volatile uint32_t CFGR;      /* 0x08 clock config           */
    volatile uint32_t PLLCFGR;   /* 0x0C PLL config            */
    volatile uint32_t PLLSAI1CFGR; /* 0x10 PLLSAI1             */
    volatile uint32_t PLLSAI2CFGR; /* 0x14 PLLSAI2             */
    volatile uint32_t CIER;      /* 0x18 clock int enable       */
    volatile uint32_t CIFR;      /* 0x1C clock int flag         */
    volatile uint32_t CICR;      /* 0x20 clock int clear        */
    volatile uint32_t RESERVED0; /* 0x24 */
    volatile uint32_t CCIPR;     /* 0x28 clock config per-periph */
    volatile uint32_t RESERVED1[2];
    volatile uint32_t BDCR;      /* 0x34 backup domain control */
    volatile uint32_t CSR;      /* 0x38 control/status         */
    volatile uint32_t RESERVED2;
    volatile uint32_t AHBSMENR;  /* 0x40 AHB1 periph clock en in sleep */
    volatile uint32_t AHB2SMENR; /* 0x44 AHB2 */
    volatile uint32_t AHB3SMENR; /* 0x48 AHB3 */
    volatile uint32_t RESERVED3;
    volatile uint32_t APB1SMENR; /* 0x50 APB1 */
    volatile uint32_t APB2SMENR; /* 0x54 APB2 */
    volatile uint32_t RESERVED4;
    volatile uint32_t AHB1ENR;   /* 0x5C AHB1 periph clock enable  */
    volatile uint32_t AHB2ENR;   /* 0x60 AHB2 */
    volatile uint32_t AHB3ENR;   /* 0x64 AHB3 */
    volatile uint32_t RESERVED5;
    volatile uint32_t APB1ENR1;  /* 0x6C APB1 enable register 1  */
    volatile uint32_t APB1ENR2;  /* 0x70 APB1 enable register 2  */
    volatile uint32_t APB2ENR;   /* 0x74 APB2 enable            */
} RCC_TypeDef;

#define RCC   ((RCC_TypeDef *)RCC_BASE)

/* RCC bits we use */
#define RCC_CR_HSION        (1U << 8)
#define RCC_CR_HSIRDY       (1U << 10)
#define RCC_CR_HSEON        (1U << 16)
#define RCC_CR_HSERDY       (1U << 17)
#define RCC_CR_HSEBYP       (1U << 18)
#define RCC_CR_PLLON        (1U << 24)
#define RCC_CR_PLLRDY       (1U << 25)

#define RCC_CFGR_SW_MSK     0x3U
#define RCC_CFGR_SWS_MSK    (0x3U << 3)
#define RCC_CFGR_SW_HSI     0
#define RCC_CFGR_SW_HSE     2
#define RCC_CFGR_SW_PLL     3

#define RCC_PLLCFGR_PLLSRC_HSE  (2U << 0)
#define RCC_PLLCFGR_PLLM_MSK    0x7U
#define RCC_PLLCFGR_PLLN_MSK     0x1FFU
#define RCC_PLLCFGR_PLLREN      (1U << 24)

/* Peripheral clock enable bits */
#define RCC_AHB2ENR_GPIOAEN   (1U << 0)
#define RCC_AHB2ENR_GPIOBEN   (1U << 1)
#define RCC_AHB2ENR_GPIOCEN   (1U << 2)
#define RCC_AHB2ENR_GPIOHEN   (1U << 7)
#define RCC_AHB2ENR_ADCEN     (1U << 13)

#define RCC_AHB1ENR_DMA1EN    (1U << 0)
#define RCC_AHB1ENR_DMA2EN   (1U << 1)
#define RCC_AHB1ENR_FLASHEN   (1U << 8)

#define RCC_APB1ENR1_TIM2EN   (1U << 0)
#define RCC_APB1ENR1_TIM3EN   (1U << 1)
#define RCC_APB1ENR1_TIM4EN   (1U << 2)
#define RCC_APB1ENR1_SPI2EN  (1U << 14)
#define RCC_APB1ENR1_USART3EN (1U << 18)
#define RCC_APB1ENR1_I2C1EN  (1U << 21)
#define RCC_APB1ENR1_RTCAPBEN (1U << 10)
#define RCC_APB1ENR1_LPUART1EN (1U << 31)
#define RCC_APB1ENR2_LPUART1EN (1U << 0)

#define RCC_APB2ENR_USART1EN  (1U << 14)
#define RCC_APB2ENR_SPI1EN   (1U << 12)
#define RCC_APB2ENR_TIM1EN   (1U << 11)
#define RCC_APB2ENR_SYSCFGEN  (1U << 0)

/* RTC enable in BDCR */
#define RCC_BDCR_RTCEN      (1U << 15)
#define RCC_BDCR_LSEON      (1U << 0)
#define RCC_BDCR_LSERDY     (1U << 1)

/* ------------------------------------------------------------------------ */
/* GPIO                                                                       */
/* ------------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t MODER;   /* 0x00 mode              */
    volatile uint32_t OTYPER;  /* 0x04 output type        */
    volatile uint32_t OSPEEDR; /* 0x08 output speed       */
    volatile uint32_t PUPDR;   /* 0x0C pull-up/pull-down  */
    volatile uint32_t IDR;     /* 0x10 input data          */
    volatile uint32_t ODR;     /* 0x14 output data         */
    volatile uint32_t BSRR;    /* 0x18 bit set/reset       */
    volatile uint32_t LCKR;    /* 0x1C lock                */
    volatile uint32_t AFRL;    /* 0x20 alt func low        */
    volatile uint32_t AFRH;    /* 0x24 alt func high       */
    volatile uint32_t BRR;     /* 0x28 bit reset           */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOH ((GPIO_TypeDef *)GPIOH_BASE)

#define GPIO_MODE_INPUT   0x0
#define GPIO_MODE_OUTPUT  0x1
#define GPIO_MODE_AF      0x2
#define GPIO_MODE_ANALOG  0x3

#define GPIO_PUPD_NONE    0x0
#define GPIO_PUPD_PULLUP  0x1
#define GPIO_PUPD_PULLDN  0x2

#define GPIO_OTYPE_PP     0x0
#define GPIO_OTYPE_OD     0x1

#define GPIO_OSPEED_LOW    0x0
#define GPIO_OSPEED_MED    0x1
#define GPIO_OSPEED_HIGH   0x2
#define GPIO_OSPEED_VHIGH  0x3

/* Helper to compute MODER/PUPDR/AFR bit positions */
#define GPIO_BIT(n, width) ((n) * (width))

/* ------------------------------------------------------------------------ */
/* ADC1 (simplified — enough for single-channel software-triggered reads)    */
/* ------------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t ISR;      /* 0x00 */
    volatile uint32_t IER;       /* 0x04 */
    volatile uint32_t CR;       /* 0x08 */
    volatile uint32_t CFGR;     /* 0x0C */
    volatile uint32_t CFGR2;    /* 0x10 */
    volatile uint32_t SMPR1;    /* 0x14 */
    volatile uint32_t SMPR2;    /* 0x18 */
    volatile uint32_t RESERVED0[2];
    volatile uint32_t TR1;      /* 0x20 */
    volatile uint32_t RESERVED1[5];
    volatile uint32_t SQR1;     /* 0x30 */
    volatile uint32_t SQR2;     /* 0x34 */
    volatile uint32_t SQR3;     /* 0x38 */
    volatile uint32_t SQR4;     /* 0x3C */
    volatile uint32_t RESERVED2[3];
    volatile uint32_t DR;       /* 0x40 (common) */
    volatile uint32_t RESERVED3[9];
    volatile uint32_t AWD2CR;   /* 0xA0 */
} ADC_TypeDef;

#define ADC1 ((ADC_TypeDef *)ADC1_BASE)

#define ADC_ISR_ADRDY    (1U << 0)
#define ADC_ISR_EOC      (1U << 2)
#define ADC_CR_ADEN      (1U << 0)
#define ADC_CR_ADDIS     (1U << 1)
#define ADC_CR_ADSTART   (1U << 2)
#define ADC_CR_ADVREG    (1U << 28)
#define ADC_CR_DEEPPWD   (1U << 29)
#define ADC_CR_ADCAL     (1U << 31)

#define ADC_CFGR_CONT    (1U << 1)
#define ADC_CFGR_OVRMOD   (1U << 12)

/* ------------------------------------------------------------------------ */
/* TIM (general purpose)                                                    */
/* ------------------------------------------------------------------------ */
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

#define TIM2 ((TIM_TypeDef *)TIM2_BASE)
#define TIM3 ((TIM_TypeDef *)TIM3_BASE)
#define TIM4 ((TIM_TypeDef *)TIM4_BASE)

#define TIM_CR1_CEN      (1U << 0)
#define TIM_SR_UIF        (1U << 0)
#define TIM_SR_CC1IF      (1U << 1)

#define TIM_CCMR1_OC1M_PWM1  (0x6 << 4)
#define TIM_CCMR1_OC2M_PWM1  (0x6 << 12)
#define TIM_CCMR2_OC3M_PWM1  (0x6 << 4)
#define TIM_CCER_CC1E        (1U << 0)
#define TIM_CCER_CC2E        (1U << 4)
#define TIM_CCER_CC3E        (1U << 8)

/* ------------------------------------------------------------------------ */
/* I2C                                                                        */
/* ------------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t OAR1;     /* 0x08 */
    volatile uint32_t OAR2;     /* 0x0C */
    volatile uint32_t TIMINGR;  /* 0x10 */
    volatile uint32_t TIMEOUTR; /* 0x14 */
    volatile uint32_t ISR;       /* 0x18 */
    volatile uint32_t ICR;       /* 0x1C */
    volatile uint32_t PECR;      /* 0x20 */
    volatile uint32_t RXDR;      /* 0x24 */
    volatile uint32_t TXDR;      /* 0x28 */
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)I2C1_BASE)

#define I2C_CR1_PE         (1U << 0)
#define I2C_CR2_START      (1U << 13)
#define I2C_CR2_STOP       (1U << 14)
#define I2C_CR2_RD_WRN     (1U << 10)
#define I2C_CR2_NBYTES_MSK  0xFFU
#define I2C_CR2_NBYTES(n)   ((n) << 16)
#define I2C_CR2_AUTOEND    (1U << 20)
#define I2C_ISR_TXIS       (1U << 1)
#define I2C_ISR_RXNE       (1U << 2)
#define I2C_ISR_NACKF      (1U << 4)
#define I2C_ISR_STOPF      (1U << 5)
#define I2C_ISR_BUSY       (1U << 15)

/* ------------------------------------------------------------------------ */
/* SPI                                                                        */
/* ------------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t SR;       /* 0x08 */
    volatile uint32_t DR;       /* 0x0C */
    volatile uint32_t CRCPR;    /* 0x10 */
    volatile uint32_t RXCRCR;   /* 0x14 */
    volatile uint32_t TXCRCR;   /* 0x18 */
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)
#define SPI2 ((SPI_TypeDef *)SPI2_BASE)

#define SPI_CR1_CPHA       (1U << 0)
#define SPI_CR1_CPOL       (1U << 1)
#define SPI_CR1_MSTR       (1U << 2)
#define SPI_CR1_SSI         (1U << 8)
#define SPI_CR1_SSM         (1U << 9)
#define SPI_CR1_BR_DIV8     (1U << 3)
#define SPI_CR1_SPE         (1U << 6)
#define SPI_CR2_DS_8BIT     (0x7 << 8)
#define SPI_CR2_FRXTH      (1U << 12)
#define SPI_SR_RXNE         (1U << 0)
#define SPI_SR_TXE          (1U << 1)
#define SPI_SR_BSY          (1U << 7)

/* ------------------------------------------------------------------------ */
/* USART                                                                      */
/* ------------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR1;      /* 0x00 */
    volatile uint32_t CR2;      /* 0x04 */
    volatile uint32_t CR3;      /* 0x08 */
    volatile uint32_t BRR;      /* 0x0C */
    volatile uint32_t GTPR;     /* 0x10 */
    volatile uint32_t RTOR;     /* 0x14 */
    volatile uint32_t RQR;      /* 0x18 */
    volatile uint32_t ISR;       /* 0x1C */
    volatile uint32_t ICR;       /* 0x20 */
    volatile uint32_t RDR;      /* 0x24 */
    volatile uint32_t TDR;      /* 0x28 */
} USART_TypeDef;

#define USART1 ((USART_TypeDef *)USART1_BASE)
#define USART3 ((USART_TypeDef *)USART3_BASE)
#define LPUART1 ((USART_TypeDef *)LPUART1_BASE)

#define USART_CR1_UE        (1U << 0)
#define USART_CR1_TE        (1U << 3)
#define USART_CR1_RE        (1U << 2)
#define USART_CR1_RXNEIE    (1U << 5)
#define USART_ISR_TXE       (1U << 7)
#define USART_ISR_RXNE      (1U << 5)
#define USART_ISR_TC        (1U << 6)

/* ------------------------------------------------------------------------ */
/* RTC                                                                        */
/* ------------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t TR;       /* 0x00 time      */
    volatile uint32_t DR;       /* 0x04 date      */
    volatile uint32_t SSR;      /* 0x08 subsec     */
    volatile uint32_t ICR;      /* 0x0C init & ctrl */
    volatile uint32_t PRER;     /* 0x10 prescaler  */
} RTC_TypeDef;

#define RTC ((RTC_TypeDef *)RTC_BASE)

#define RTC_ICR_INIT        (1U << 7)
#define RTC_ICR_INITF       (1U << 6)

/* ------------------------------------------------------------------------ */
/* NVIC / SCB (minimal)                                                       */
/* ------------------------------------------------------------------------ */
#define NVIC_ISER0   REG32(NVIC_BASE + 0x000)
#define NVIC_ICER0   REG32(NVIC_BASE + 0x080)
#define NVIC_IPR(n)  REG8(NVIC_BASE + 0x300 + (n))

#define SCB_SCR      REG32(SCB_BASE + 0x10)
#define SCB_SCR_SLEEPDEEP  (1U << 2)

/* ------------------------------------------------------------------------ */
/* SysTick                                                                    */
/* ------------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CSR;     /* 0x00 */
    volatile uint32_t LOAD;    /* 0x04 */
    volatile uint32_t VAL;     /* 0x08 */
    volatile uint32_t CALIB;   /* 0x0C */
} SysTick_TypeDef;

#define SYSTICK ((SysTick_TypeDef *)SYSTICK_BASE)
#define SYSTICK_CSR_ENABLE  (1U << 0)
#define SYSTICK_CSR_CLKSRC  (1U << 2)
#define SYSTICK_CSR_TICKINT (1U << 1)

/* ------------------------------------------------------------------------ */
/* PWR (power control — minimal)                                              */
/* ------------------------------------------------------------------------ */
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t CR3;     /* 0x08 */
    volatile uint32_t CR4;     /* 0x0C */
    volatile uint32_t SR1;     /* 0x10 */
    volatile uint32_t SR2;     /* 0x14 */
    volatile uint32_t SCR;     /* 0x18 */
} PWR_TypeDef;

#define PWR ((PWR_TypeDef *)PWR_BASE)

#define PWR_CR1_LPMS_STOP2  (0x2U << 0)

#endif /* LICHENWATCH_REGISTERS_H */