/*
 * registers.h — STM32H733 Register Definitions and Bit Masks
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_REGISTERS_H
#define LIGNOSCAN_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ---- */
#define PERIPH_BASE         (0x40000000UL)
#define AHB1_BASE           (PERIPH_BASE + 0x00020000UL)
#define AHB4_BASE           (PERIPH_BASE + 0x00500000UL)
#define APB1_BASE           (PERIPH_BASE + 0x00010000UL)
#define APB2_BASE           (PERIPH_BASE + 0x00040000UL)

/* ---- RCC ---- */
#define RCC_BASE            (AHB1_BASE + 0x1000UL)
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x48))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_AHB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0x50))
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0x54))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB1LENR        (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB1HENR        (*(volatile uint32_t *)(RCC_BASE + 0x5C))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_AHB4RSTR        (*(volatile uint32_t *)(RCC_BASE + 0x80))

/* RCC CR bits */
#define RCC_CR_HSION        (1U << 0)
#define RCC_CR_HSIRDY       (1U << 1)
#define RCC_CR_HSEON        (1U << 16)
#define RCC_CR_HSERDY       (1U << 17)
#define RCC_CR_PLL1ON       (1U << 24)
#define RCC_CR_PLL1RDY      (1U << 25)

/* RCC PLLCFGR bits */
#define RCC_PLLCFGR_PLLSRC_HSE  (1U << 0)
#define RCC_PLLCFGR_PLL1M_SHIFT 4
#define RCC_PLLCFGR_PLL1N_SHIFT 8
#define RCC_PLLCFGR_PLL1P_SHIFT 17

/* RCC AHB4ENR bits (GPIO) */
#define RCC_AHB4ENR_GPIOAEN (1U << 0)
#define RCC_AHB4ENR_GPIOBEN (1U << 1)
#define RCC_AHB4ENR_GPIOCEN (1U << 2)
#define RCC_AHB4ENR_GPIODEN (1U << 3)
#define RCC_AHB4ENR_GPIOEEN (1U << 4)
#define RCC_AHB4ENR_GPIOFEN (5U << 5)
#define RCC_AHB4ENR_GPIOGEN (1U << 6)
#define RCC_AHB4ENR_GPIOHEN (1U << 7)

/* RCC APB1ENR bits */
#define RCC_APB1LENR_SPI2EN (1U << 14)
#define RCC_APB1LENR_SPI3EN (1U << 15)
#define RCC_APB1LENR_USART2EN (1U << 17)
#define RCC_APB1LENR_USART3EN (1U << 18)
#define RCC_APB1LENR_UART4EN (1U << 19)
#define RCC_APB1LENR_UART5EN (1U << 20)
#define RCC_APB1LENR_I2C1EN (1U << 21)
#define RCC_APB1LENR_TIM2EN (1U << 0)
#define RCC_APB1LENR_TIM3EN (1U << 1)
#define RCC_APB1LENR_TIM4EN (1U << 2)

/* RCC APB2ENR bits */
#define RCC_APB2ENR_SPI1EN  (1U << 12)
#define RCC_APB2ENR_USART1EN (1U << 14)
#define RCC_APB2ENR_TIM1EN  (1U << 0)
#define RCC_APB2ENR_TIM8EN  (1U << 1)
#define RCC_APB2ENR_ADC1EN  (1U << 8)
#define RCC_APB2ENR_SYSCFGEN (1U << 0)

/* ---- GPIO ---- */
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
    volatile uint32_t BRR;       /* 0x28 */
} GPIO_TypeDef;

#define GPIOA_BASE         (AHB4_BASE + 0x0000UL)
#define GPIOB_BASE         (AHB4_BASE + 0x0400UL)
#define GPIOC_BASE         (AHB4_BASE + 0x0800UL)
#define GPIOD_BASE         (AHB4_BASE + 0x0C00UL)
#define GPIOE_BASE         (AHB4_BASE + 0x1000UL)
#define GPIOF_BASE         (AHB4_BASE + 0x1400UL)
#define GPIOG_BASE         (AHB4_BASE + 0x1800UL)
#define GPIOH_BASE         (AHB4_BASE + 0x1C00UL)

#define GPIOA              ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB              ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC              ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD              ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE              ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOH              ((GPIO_TypeDef *)GPIOH_BASE)

/* GPIO modes */
#define GPIO_MODE_INPUT     0x00
#define GPIO_MODE_OUTPUT    0x01
#define GPIO_MODE_AF        0x02
#define GPIO_MODE_ANALOG    0x03

#define GPIO_OTYPE_PP       0x00
#define GPIO_OTYPE_OD       0x01

#define GPIO_OSPEED_LOW     0x00
#define GPIO_OSPEED_MID     0x01
#define GPIO_OSPEED_HIGH    0x02
#define GPIO_OSPEED_VHIGH   0x03

#define GPIO_PUPD_NONE      0x00
#define GPIO_PUPD_PU        0x01
#define GPIO_PUPD_PD        0x02

/* GPIO pin helpers */
#define GPIO_PIN(n)         (1U << (n))
#define GPIO_SET(port, pin) ((port)->BSRR = (1U << (pin)))
#define GPIO_CLR(port, pin) ((port)->BSRR = (1U << ((pin) + 16)))
#define GPIO_GET(port, pin) (((port)->IDR >> (pin)) & 1U)

/* ---- SPI ---- */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t CFG1;      /* 0x08 */
    volatile uint32_t CFG2;      /* 0x0C */
    volatile uint32_t IER;       /* 0x10 */
    volatile uint32_t SR;        /* 0x14 */
    volatile uint32_t IFCR;      /* 0x18 */
    volatile uint32_t TXDR;      /* 0x1C */
    volatile uint32_t RXDR;      /* 0x20 */
    volatile uint32_t I2SCFGR;   /* 0x24 */
} SPI_TypeDef;

#define SPI1_BASE          (APB2_BASE + 0x3000UL)
#define SPI2_BASE          (APB1_BASE + 0x3800UL)
#define SPI3_BASE          (APB1_BASE + 0x3C00UL)
#define SPI1               ((SPI_TypeDef *)SPI1_BASE)
#define SPI2               ((SPI_TypeDef *)SPI2_BASE)
#define SPI3               ((SPI_TypeDef *)SPI3_BASE)

/* SPI CR1 bits */
#define SPI_CR1_SPE         (1U << 0)
#define SPI_CR1_CSTART      (1U << 9)
#define SPI_CR1_HALT        (1U << 18)

/* SPI CFG1 bits */
#define SPI_CFG1_MBR_SHIFT 28
#define SPI_CFG1_DSIZE_SHIFT 0
#define SPI_CFG1_FTHLV_SHIFT 5
#define SPI_CFG1_MASTER     (1U << 2)
#define SPI_CFG1_RXDMAEN    (1U << 14)
#define SPI_CFG1_TXDMAEN    (1U << 15)

/* SPI SR bits */
#define SPI_SR_TXP          (1U << 1)
#define SPI_SR_RXP          (1U << 0)
#define SPI_SR_EOT          (1U << 3)
#define SPI_SR_OVR          (1U << 6)
#define SPI_SR_TSER         (1U << 10)

/* ---- USART/UART ---- */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t CR3;       /* 0x08 */
    volatile uint32_t BRR;       /* 0x0C */
    volatile uint32_t GTPR;      /* 0x10 */
    volatile uint32_t RTOR;      /* 0x14 */
    volatile uint32_t RQR;       /* 0x18 */
    volatile uint32_t ISR;       /* 0x1C */
    volatile uint32_t ICR;       /* 0x20 */
    volatile uint32_t RDR;       /* 0x24 */
    volatile uint32_t TDR;       /* 0x28 */
} USART_TypeDef;

#define USART1_BASE        (APB2_BASE + 0x1000UL)
#define USART2_BASE        (APB1_BASE + 0x4000UL)
#define USART3_BASE        (APB1_BASE + 0x4800UL)
#define UART4_BASE         (APB1_BASE + 0x5000UL)
#define USART1             ((USART_TypeDef *)USART1_BASE)
#define USART2             ((USART_TypeDef *)USART2_BASE)
#define USART3             ((USART_TypeDef *)USART3_BASE)
#define UART4              ((USART_TypeDef *)UART4_BASE)

/* USART CR1 bits */
#define USART_CR1_UE        (1U << 0)
#define USART_CR1_RE        (1U << 2)
#define USART_CR1_TE        (1U << 3)
#define USART_CR1_RXNEIE    (1U << 5)
#define USART_CR1_TCIE      (1U << 6)

/* USART ISR bits */
#define USART_ISR_TXE       (1U << 7)
#define USART_ISR_RXNE      (1U << 5)
#define USART_ISR_TC        (1U << 6)

/* ---- I2C ---- */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t OAR1;      /* 0x08 */
    volatile uint32_t OAR2;      /* 0x0C */
    volatile uint32_t TIMINGR;   /* 0x10 */
    volatile uint32_t TIMEOUTR;  /* 0x14 */
    volatile uint32_t ISR;       /* 0x18 */
    volatile uint32_t ICR;       /* 0x1C */
    volatile uint32_t PECR;      /* 0x20 */
    volatile uint32_t RXDR;      /* 0x24 */
    volatile uint32_t TXDR;      /* 0x28 */
} I2C_TypeDef;

#define I2C1_BASE          (APB1_BASE + 0x5400UL)
#define I2C1               ((I2C_TypeDef *)I2C1_BASE)

#define I2C_CR1_PE          (1U << 0)
#define I2C_ISR_TXE         (1U << 0)
#define I2C_ISR_RXNE        (1U << 2)
#define I2C_ISR_TC          (1U << 6)
#define I2C_ISR_NACKF       (1U << 12)

/* ---- TIM ---- */
typedef struct {
    volatile uint32_t CR1;       /* 0x00 */
    volatile uint32_t CR2;       /* 0x04 */
    volatile uint32_t SMCR;      /* 0x08 */
    volatile uint32_t DIER;      /* 0x0C */
    volatile uint32_t SR;        /* 0x10 */
    volatile uint32_t EGR;       /* 0x14 */
    volatile uint32_t CCMR1;     /* 0x18 */
    volatile uint32_t CCMR2;     /* 0x1C */
    volatile uint32_t CCER;      /* 0x20 */
    volatile uint32_t CNT;       /* 0x24 */
    volatile uint32_t PSC;       /* 0x28 */
    volatile uint32_t ARR;       /* 0x2C */
    volatile uint32_t CCR1;      /* 0x30 */
    volatile uint32_t CCR2;      /* 0x34 */
    volatile uint32_t CCR3;      /* 0x38 */
    volatile uint32_t CCR4;      /* 0x3C */
} TIM_TypeDef;

#define TIM1_BASE          (APB2_BASE + 0x0000UL)
#define TIM2_BASE          (APB1_BASE + 0x0000UL)
#define TIM3_BASE          (APB1_BASE + 0x0400UL)
#define TIM4_BASE          (APB1_BASE + 0x0800UL)
#define TIM1               ((TIM_TypeDef *)TIM1_BASE)
#define TIM2               ((TIM_TypeDef *)TIM2_BASE)
#define TIM3               ((TIM_TypeDef *)TIM3_BASE)

#define TIM_CR1_CEN         (1U << 0)
#define TIM_CR1_ARPE        (1U << 7)
#define TIM_DIER_UIE        (1U << 0)
#define TIM_SR_UIF          (1U << 0)

/* ---- DMA ---- */
#define DMA1_BASE          (AHB1_BASE + 0x0000UL)
#define DMA1_STREAM0_BASE  (DMA1_BASE + 0x010UL)
#define DMA1_STREAM1_BASE  (DMA1_BASE + 0x028UL)

typedef struct {
    volatile uint32_t CR;        /* 0x00 */
    volatile uint32_t NDTR;      /* 0x04 */
    volatile uint32_t PAR;       /* 0x08 */
    volatile uint32_t M0AR;      /* 0x0C */
    volatile uint32_t M1AR;      /* 0x10 */
    volatile uint32_t FCR;       /* 0x14 */
} DMA_Stream_TypeDef;

/* ---- Flash (for calibration storage) ---- */
#define FLASH_BASE         (AHB1_BASE + 0x2000UL)
#define FLASH_KEYR         (*(volatile uint32_t *)(FLASH_BASE + 0x04))
#define FLASH_SR           (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_CR           (*(volatile uint32_t *)(FLASH_BASE + 0x14))

/* ---- NVIC ---- */
#define NVIC_BASE          (0xE000E100UL)
#define NVIC_ISER0         (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ICPR0         (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_IPR_BASE      ((volatile uint8_t *)(NVIC_BASE + 0x300))

/* ---- SysTick ---- */
#define SYSTICK_BASE       (0xE000E010UL)
#define SYSTICK_CSR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_RVR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_CVR        (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYSTICK_CSR_ENABLE (1U << 0)
#define SYSTICK_CSR_CLKSRC (1U << 2)

/* ---- Watchdog (IWDG) ---- */
#define IWDG_BASE          (0x40003000UL)
#define IWDG_KR            (*(volatile uint32_t *)(IWDG_BASE + 0x00))
#define IWDG_PR            (*(volatile uint32_t *)(IWDG_BASE + 0x04))
#define IWDG_RLR           (*(volatile uint32_t *)(IWDG_BASE + 0x08))
#define IWDG_SR            (*(volatile uint32_t *)(IWDG_BASE + 0x0C))

/* ---- System control ---- */
#define SCB_BASE           (0xE000ED00UL)
#define SCB_VTOR           (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_AIRCR          (*(volatile uint32_t *)(SCB_BASE + 0x0C))

/* ---- Interrupt numbers ---- */
#define IRQ_SPI1           35
#define IRQ_SPI2           36
#define IRQ_USART1         37
#define IRQ_USART2         38
#define IRQ_USART3         39
#define IRQ_TIM2           28
#define IRQ_I2C1_EV        31
#define IRQ_EXTI0          6
#define IRQ_EXTI1          7
#define IRQ_EXTI2          8

#endif /* LIGNOSCAN_REGISTERS_H */