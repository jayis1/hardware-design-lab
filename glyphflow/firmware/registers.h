/*
 * registers.h — STM32L432KC peripheral register map (bare-metal)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Minimal direct-register definitions for the peripherals GlyphFlow uses.
 * No ST HAL dependency. Addresses from RM0394 reference manual.
 */
#ifndef GLYPHFLOW_REGISTERS_H
#define GLYPHFLOW_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses ------------------------------------------------- */
#define PERIPH_BASE        0x40000000UL
#define AHB1_BASE          0x48000000UL
#define AHB2_BASE          0x48010000UL
#define APB1_BASE          0x40000000UL
#define APB2_BASE          0x48000000UL

/* Cortex-M4 system registers */
#define SCB_BASE           0xE000ED00UL
#define SYSTICK_BASE       0xE000E010UL
#define NVIC_BASE          0xE000E100UL
#define EXTI_BASE          0xE000EA00UL
#define DWT_BASE           0xE0001000UL

/* ---- RCC (clock control) ------------------------------------------- */
#define RCC_BASE           (AHB1_BASE + 0x1000)
#define RCC_CR             (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_ICSCR          (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_CFGR           (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_PLLCFGR        (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_AHB2ENR        (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_APB1ENR1       (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB1ENR2       (*(volatile uint32_t *)(RCC_BASE + 0x5C))
#define RCC_APB2ENR        (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_CCIPR          (*(volatile uint32_t *)(RCC_BASE + 0x88))
#define RCC_BDCR           (*(volatile uint32_t *)(RCC_BASE + 0x90))
#define RCC_CSR            (*(volatile uint32_t *)(RCC_BASE + 0x94))

/* RCC enable bits */
#define RCC_AHB2ENR_GPIOAEN  (1U << 0)
#define RCC_AHB2ENR_GPIOBEN  (1U << 1)
#define RCC_AHB2ENR_GPIOCEN  (1U << 2)
#define RCC_APB1ENR1_I2C1EN  (1U << 21)
#define RCC_APB1ENR1_USART2EN (1U << 17)
#define RCC_APB1ENR2_LPUART1EN (1U << 0)
#define RCC_APB2ENR_SPI1EN   (1U << 12)
#define RCC_APB2ENR_USART1EN (1U << 14)
#define RCC_APB1ENR1_TIM2EN  (1U << 0)
#define RCC_APB1ENR1_TIM6EN  (1U << 4)

/* ---- PWR ----------------------------------------------------------- */
#define PWR_BASE            (APB1_BASE + 0x7000)
#define PWR_CR1             (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CR2             (*(volatile uint32_t *)(PWR_BASE + 0x04))
#define PWR_CR3             (*(volatile uint32_t *)(PWR_BASE + 0x08))
#define PWR_CR4             (*(volatile uint32_t *)(PWR_BASE + 0x0C))
#define PWR_SR1             (*(volatile uint32_t *)(PWR_BASE + 0x10))
#define PWR_SR2             (*(volatile uint32_t *)(PWR_BASE + 0x14))
#define PWR_SCR             (*(volatile uint32_t *)(PWR_BASE + 0x18)

/* ---- GPIO ---------------------------------------------------------- */
#define GPIOA_BASE          (AHB2_BASE + 0x0000)
#define GPIOB_BASE          (AHB2_BASE + 0x0400)
#define GPIOC_BASE          (AHB2_BASE + 0x0800)
#define GPIOH_BASE          (AHB2_BASE + 0x1C00)

typedef struct {
    volatile uint32_t MODER;   /* 0x00 */
    volatile uint32_t OTYPER;  /* 0x04 */
    volatile uint32_t OSPEEDR; /* 0x08 */
    volatile uint32_t PUPDR;   /* 0x0C */
    volatile uint32_t IDR;     /* 0x10 */
    volatile uint32_t ODR;     /* 0x14 */
    volatile uint32_t BSRR;    /* 0x18 */
    volatile uint32_t LCKR;    /* 0x1C */
    volatile uint32_t AFRL;    /* 0x20 */
    volatile uint32_t AFRH;    /* 0x24 */
    volatile uint32_t BRR;     /* 0x28 */
} GPIO_TypeDef;

#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *)GPIOC_BASE)

/* GPIO mode bits */
#define GPIO_MODE_IN    0x00
#define GPIO_MODE_OUT   0x01
#define GPIO_MODE_AF    0x02
#define GPIO_MODE_AN    0x03

/* ---- SPI1 ---------------------------------------------------------- */
#define SPI1_BASE           (APB2_BASE + 0x3000)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t SR;      /* 0x08 */
    volatile uint32_t DR;      /* 0x0C */
    volatile uint32_t CRCPR;    /* 0x10 */
    volatile uint32_t RXCRCR;  /* 0x14 */
    volatile uint32_t TXCRCR;  /* 0x18 */
} SPI_TypeDef;

#define SPI1    ((SPI_TypeDef *)SPI1_BASE)

#define SPI_CR1_CPHA        (1U << 0)
#define SPI_CR1_CPOL        (1U << 1)
#define SPI_CR1_MSTR         (1U << 2)
#define SPI_CR1_BR_SHIFT    3
#define SPI_CR1_SPE         (1U << 6)
#define SPI_CR1_LSBFIRST    (1U << 7)
#define SPI_CR1_SSI         (1U << 8)
#define SPI_CR1_SSM         (1U << 9)
#define SPI_CR1_RXONLY      (1U << 10)
#define SPI_CR1_CRCL        (1U << 11)
#define SPI_CR1_CRCEN       (1U << 13)
#define SPI_CR1_BIDIOE      (1U << 14)
#define SPI_CR1_BIDIMODE    (1U << 15)
#define SPI_CR2_DS_SHIFT    8
#define SPI_CR2_FRXTH      (1U << 12)
#define SPI_CR2_NSSP       (1U << 3)
#define SPI_CR2_SSOE       (1U << 2)
#define SPI_CR2_TXEIE      (1U << 7)
#define SPI_CR2_RXNEIE     (1U << 6)

#define SPI_SR_BSY          (1U << 7)
#define SPI_SR_OVR          (1U << 6)
#define SPI_SR_TXE         (1U << 1)
#define SPI_SR_RXNE        (1U << 0)
#define SPI_SR_FRE         (1U << 8)

/* ---- I2C1 ---------------------------------------------------------- */
#define I2C1_BASE           (APB1_BASE + 0x5400)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t OAR1;    /* 0x08 */
    volatile uint32_t OAR2;    /* 0x0C */
    volatile uint32_t TIMINGR; /* 0x10 */
    volatile uint32_t TIMEOUTR; /* 0x14 */
    volatile uint32_t ISR;     /* 0x18 */
    volatile uint32_t ICR;     /* 0x1C */
    volatile uint32_t PECR;    /* 0x20 */
    volatile uint32_t RXDR;    /* 0x24 */
    volatile uint32_t TXDR;    /* 0x28 */
} I2C_TypeDef;

#define I2C1    ((I2C_TypeDef *)I2C1_BASE)

#define I2C_CR1_PE          (1U << 0)
#define I2C_CR1_TXIE        (1U << 1)
#define I2C_CR1_RXIE        (1U << 2)
#define I2C_CR1_STOPIE      (1U << 5)
#define I2C_CR2_NBYTES_SHIFT 16
#define I2C_CR2_START       (1U << 13)
#define I2C_CR2_STOP        (1U << 14)
#define I2C_CR2_NACK        (1U << 15)
#define I2C_CR2_RELOAD       (1U << 24)
#define I2C_CR2_AUTOEND      (1U << 25)
#define I2C_ISR_BUSY        (1U << 15)
#define I2C_ISR_TXIS        (1U << 1)
#define I2C_ISR_RXNE        (1U << 2)
#define I2C_ISR_TCR         (1U << 4)
#define I2C_ISR_TC          (1U << 6)
#define I2C_ISR_NACKF       (1U << 12)
#define I2C_ISR_STOPF      (1U << 5)

/* ---- USART1 (for BLE module UART transport) ------------------------ */
#define USART1_BASE         (APB2_BASE + 0x3800)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t CR3;     /* 0x08 */
    volatile uint32_t BRR;     /* 0x0C */
    volatile uint32_t GTPR;    /* 0x10 */
    volatile uint32_t RTOR;    /* 0x14 */
    volatile uint32_t RQR;     /* 0x18 */
    volatile uint32_t ISR;     /* 0x1C */
    volatile uint32_t ICR;     /* 0x20 */
    volatile uint32_t RDR;     /* 0x24 */
    volatile uint32_t TDR;     /* 0x28 */
} USART_TypeDef;

#define USART1   ((USART_TypeDef *)USART1_BASE)

#define USART_CR1_UE        (1U << 0)
#define USART_CR1_TE        (1U << 3)
#define USART_CR1_RE        (1U << 2)
#define USART_CR1_TXEIE     (1U << 7)
#define USART_CR1_RXNEIE   (1U << 5)
#define USART_ISR_TXE      (1U << 7)
#define USART_ISR_RXNE     (1U << 5)
#define USART_ISR_TC        (1U << 6)

/* ---- TIM2 (sample timer) ------------------------------------------- */
#define TIM2_BASE           (APB1_BASE + 0x0000)
typedef struct {
    volatile uint32_t CR1;     /* 0x00 */
    volatile uint32_t CR2;     /* 0x04 */
    volatile uint32_t SMCR;    /* 0x08 */
    volatile uint32_t DIER;    /* 0x0C */
    volatile uint32_t SR;      /* 0x10 */
    volatile uint32_t EGR;    /* 0x14 */
    volatile uint32_t CCMR1;   /* 0x18 */
    volatile uint32_t CCMR2;   /* 0x1C */
    volatile uint32_t CCER;    /* 0x20 */
    volatile uint32_t CNT;     /* 0x24 */
    volatile uint32_t PSC;     /* 0x28 */
    volatile uint32_t ARR;     /* 0x2C */
    volatile uint32_t CCR1;    /* 0x34 */
    volatile uint32_t CCR2;    /* 0x38 */
    volatile uint32_t CCR3;    /* 0x3C */
    volatile uint32_t CCR4;    /* 0x40 */
} TIM_TypeDef;

#define TIM2    ((TIM_TypeDef *)TIM2_BASE)

#define TIM_CR1_CEN         (1U << 0)
#define TIM_DIER_UIE        (1U << 0)

/* ---- RTC ----------------------------------------------------------- */
#define RTC_BASE            (APB1_BASE + 0x2800)
#define RTC_TR              (*(volatile uint32_t *)(RTC_BASE + 0x00))
#define RTC_DR              (*(volatile uint32_t *)(RTC_BASE + 0x04))
#define RTC_CR              (*(volatile uint32_t *)(RTC_BASE + 0x08))
#define RTC_ISR             (*(volatile uint32_t *)(RTC_BASE + 0x0C))
#define RTC_PRER            (*(volatile uint32_t *)(RTC_BASE + 0x10))
#define RTC_WUTR            (*(volatile uint32_t *)(RTC_BASE + 0x14))
#define RTC_WPR             (*(volatile uint32_t *)(RTC_BASE + 0x24))

#define RTC_CR_WUTE         (1U << 10)
#define RTC_CR_WUTIE        (1U << 14)
#define RTC_ISR_WUTF        (1U << 10)
#define RTC_ISR_WUTWF       (1U << 2)
#define RTC_ISR_INIT        (1U << 7)
#define RTC_CR_WUCKSEL_SH   0

/* ---- EXTI ---------------------------------------------------------- */
#define EXTI_IMR1           (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_EMR1          (*(volatile uint32_t *)(EXTI_BASE + 0x04))
#define EXTI_RTSR1          (*(volatile uint32_t *)(EXTI_BASE + 0x08))
#define EXTI_FTSR1          (*(volatile uint32_t *)(EXTI_BASE + 0x0C))
#define EXTI_SWIER1         (*(volatile uint32_t *)(EXTI_BASE + 0x10))
#define EXTI_PR1           (*(volatile uint32_t *)(EXTI_BASE + 0x14))

/* ---- NVIC ---------------------------------------------------------- */
#define NVIC_ISER0         (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ICER0         (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ISPR0         (*(volatile uint32_t *)(NVIC_BASE + 0x100))
#define NVIC_ICPR0         (*(volatile uint32_t *)(NVIC_BASE + 0x180))

#define NVIC_IPR(n)         (*(volatile uint8_t *)(NVIC_BASE + 0x300 + (n)))

/* ---- SCB ----------------------------------------------------------- */
#define SCB_SCR             (*(volatile uint32_t *)(SCB_BASE + 0x10))
#define SCB_SCR_SLEEPDEEP   (1U << 2)
#define SCB_VTOR           (*(volatile uint32_t *)(SCB_BASE + 0x08))

/* ---- SysTick ------------------------------------------------------- */
#define SYSTICK_CSR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_RVR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_CVR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYSTICK_CSR_ENABLE  (1U << 0)
#define SYSTICK_CSR_TICKINT (1U << 1)
#define SYSTICK_CSR_CLKSOURCE (1U << 2)

/* ---- DWT (cycle counter) ------------------------------------------- */
#define DWT_CTRL           (*(volatile uint32_t *)(DWT_BASE + 0x00))
#define DWT_CYCCNT         (*(volatile uint32_t *)(DWT_BASE + 0x04))
#define DWT_CTRL_CYCCNTENA (1U << 0)

/* ---- Flash controller ---------------------------------------------- */
#define FLASH_BASE         (AHB1_BASE + 0x2000)
#define FLASH_ACR          (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_KEYR         (*(volatile uint32_t *)(FLASH_BASE + 0x08))
#define FLASH_SR           (*(volatile uint32_t *)(FLASH_BASE + 0x10))
#define FLASH_CR           (*(volatile uint32_t *)(FLASH_BASE + 0x14))
#define FLASH_AR           (*(volatile uint32_t *)(FLASH_BASE + 0x18))
#define FLASH_SR_BSY       (1U << 16)
#define FLASH_CR_LOCK      (1U << 31)
#define FLASH_CR_PG         (1U << 0)
#define FLASH_CR_PER       (1U << 1)
#define FLASH_CR_STRT      (1U << 16)
#define FLASH_ACR_LATENCY_MASK 0x07

#endif /* GLYPHFLOW_REGISTERS_H */