/*
 * registers.h — STM32L4R9ZI register definitions
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Minimal register-level definitions for the STM32L4R9ZI Cortex-M4F
 * microcontroller.  Only the peripherals used by MycoMesh are defined.
 * This is a hand-written subset — the full ST HAL is not used to keep the
 * firmware self-contained and buildable with bare arm-none-eabi-gcc.
 */

#ifndef MYCOMESH_REGISTERS_H
#define MYCOMESH_REGISTERS_H

#include <stdint.h>

/* ===================================================================== */
/*  Cortex-M4 core peripherals                                            */
/* ===================================================================== */

#define SCB_BASE            0xE000ED00UL
#define SCB_VTOR            (*(volatile uint32_t *)(SCB_BASE + 0x08))
#define SCB_CPACR           (*(volatile uint32_t *)(SCB_BASE + 0x88))
#define SCB_SCR_SLEEPDEEP   (1U << 2)
#define SCB_SCR_SEVONPEND   (1U << 4)

#define NVIC_BASE           0xE000E100UL
#define NVIC_ISER(n)        (*(volatile uint32_t *)(NVIC_BASE + 0x000 + (n)*4))
#define NVIC_ICPR(n)        (*(volatile uint32_t *)(NVIC_BASE + 0x180 + (n)*4))
#define NVIC_IP(n)          (*(volatile uint8_t *)(0xE000E400UL + n))

#define SysTick_BASE        0xE000E010UL
#define SysTick_CSR         (*(volatile uint32_t *)(SysTick_BASE + 0x00))
#define SysTick_LOAD        (*(volatile uint32_t *)(SysTick_BASE + 0x04))
#define SysTick_VAL         (*(volatile uint32_t *)(SysTick_BASE + 0x08))

#define SysTick_ENABLE      (1U << 0)
#define SysTick_TICKINT     (1U << 1)
#define SysTick_CLKSOURCE   (1U << 2)

/* ===================================================================== */
/*  Base addresses                                                        */
/* ===================================================================== */

#define PERIPH_BASE         0x40000000UL
#define AHB1PERI_BASE       (PERIPH_BASE + 0x00020000UL)
#define AHB2PERI_BASE       (PERIPH_BASE + 0x00000000UL)
#define APB1PERI_BASE       (PERIPH_BASE + 0x00010000UL)
#define APB2PERI_BASE       (PERIPH_BASE + 0x0001C000UL)

/* ===================================================================== */
/*  RCC — Reset and Clock Control                                         */
/* ===================================================================== */

#define RCC_BASE            (AHB1PERI_BASE + 0x1000UL)
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_ICSCR           (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_CIIR            (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_CICR            (*(volatile uint32_t *)(RCC_BASE + 0x14))

#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x4C + 0x04))
#define RCC_AHB3ENR         (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB1ENR1        (*(volatile uint32_t *)(RCC_BASE + 0x58 + 0x04))
#define RCC_APB1ENR2        (*(volatile uint32_t *)(RCC_BASE + 0x60))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x60 + 0x04))

/* RCC_CR bits */
#define RCC_CR_HSION        (1U << 8)
#define RCC_CR_HSIRDY       (1U << 10)
#define RCC_CR_HSEON        (1U << 16)
#define RCC_CR_HSERDY       (1U << 17)
#define RCC_CR_HSEBYP       (1U << 18)
#define RCC_CR_PLLON        (1U << 24)
#define RCC_CR_PLLRDY       (1U << 25)

/* RCC_CFGR bits */
#define RCC_CFGR_SW_MASK    0x3U
#define RCC_CFGR_SW_HSI     0x1U
#define RCC_CFGR_SW_HSE     0x2U
#define RCC_CFGR_SW_PLL     0x3U
#define RCC_CFGR_SWS_SHIFT  2
#define RCC_CFGR_SWS_MASK   (0x3U << 2)
#define RCC_CFGR_HPRE_SHIFT 4
#define RCC_CFGR_PPRE1_SHIFT 8
#define RCC_CFGR_PPRE2_SHIFT 11

/* RCC_PLLCFGR bits */
#define RCC_PLLCFGR_PLLM_Pos    4
#define RCC_PLLCFGR_PLLN_Pos    8
#define RCC_PLLCFGR_PLLP_Pos    17
#define RCC_PLLCFGR_PLLR_Pos    25
#define RCC_PLLCFGR_PLLREN      (1U << 24)
#define RCC_PLLCFGR_PLLSRC_HSE  (1U << 0)

/* RCC AHB1 enable bits */
#define RCC_AHB1ENR_DMA1EN     (1U << 0)
#define RCC_AHB1ENR_DMA2EN     (1U << 1)
#define RCC_AHB1ENR_CRCEN      (1U << 12)

/* RCC AHB2 enable bits */
#define RCC_AHB2ENR_GPIOAEN    (1U << 0)
#define RCC_AHB2ENR_GPIOBEN    (1U << 1)
#define RCC_AHB2ENR_GPIOCEN    (1U << 2)
#define RCC_AHB2ENR_GPIODEN    (1U << 3)
#define RCC_AHB2ENR_GPIOHEN    (1U << 7)
#define RCC_AHB2ENR_ADCEN      (1U << 13)

/* RCC APB1 enable bits */
#define RCC_APB1ENR1_TIM2EN    (1U << 0)
#define RCC_APB1ENR1_SPI2EN    (1U << 14)
#define RCC_APB1ENR1_USART3EN  (1U << 18)
#define RCC_APB1ENR1_UART4EN   (1U << 19)
#define RCC_APB1ENR1_I2C1EN    (1U << 21)
#define RCC_APB1ENR1_RTCAPBEN  (1U << 10)
#define RCC_APB1ENR1_LPUART1EN (1U << 0)

/* RCC APB2 enable bits */
#define RCC_APB2ENR_TIM1EN     (1U << 11)
#define RCC_APB2ENR_TIM16EN    (1U << 17)
#define RCC_APB2ENR_TIM17EN    (1U << 18)
#define RCC_APB2ENR_SPI1EN     (1U << 12)
#define RCC_APB2ENR_SPI3EN     (1U << 15)
#define RCC_APB2ENR_SYSCFGEN   (1U << 0)
#define RCC_APB2ENR_USART1EN   (1U << 14)

/* PWR */
#define RCC_APB1ENR1_PWREN     (1U << 28)
#define PWR_BASE               (APB1PERI_BASE + 0x7000UL)
#define PWR_CR1                (*(volatile uint32_t *)(PWR_BASE + 0x00))
#define PWR_CR2                (*(volatile uint32_t *)(PWR_BASE + 0x04))
#define PWR_CR3                (*(volatile uint32_t *)(PWR_BASE + 0x08))
#define PWR_CR4                (*(volatile uint32_t *)(PWR_BASE + 0x0C))
#define PWR_SR1                (*(volatile uint32_t *)(PWR_BASE + 0x10))
#define PWR_SR2                (*(volatile uint32_t *)(PWR_BASE + 0x14))
#define PWR_SCR                (*(volatile uint32_t *)(PWR_BASE + 0x18))

#define PWR_CR1_LPMS_STOP2     (0x2U << 0)
#define PWR_CR3_EIWUL          (1U << 1)

/* ===================================================================== */
/*  FLASH controller                                                      */
/* ===================================================================== */

#define FLASH_BASE          (AHB1PERI_BASE + 0x2000UL)
#define FLASH_ACR           (*(volatile uint32_t *)(FLASH_BASE + 0x00))
#define FLASH_ACR_LATENCY(n) ((n) << 0)
#define FLASH_ACR_PRFTEN    (1U << 8)
#define FLASH_ACR_ICEN      (1U << 9)
#define FLASH_ACR_DCEN      (1U << 10)
#define FLASH_ACR_RUN_PD    (1U << 13)
#define FLASH_ACR_SLEEP_PD  (1U << 14)

/* ===================================================================== */
/*  GPIO — General Purpose I/O                                            */
/* ===================================================================== */

#define GPIOA_BASE          (AHB2PERI_BASE + 0x0000UL)
#define GPIOB_BASE          (AHB2PERI_BASE + 0x0400UL)
#define GPIOC_BASE          (AHB2PERI_BASE + 0x0800UL)
#define GPIOD_BASE          (AHB2PERI_BASE + 0x0C00UL)
#define GPIOH_BASE          (AHB2PERI_BASE + 0x1C00UL)

typedef struct {
    volatile uint32_t MODER;     /* 0x00 Mode register          */
    volatile uint32_t OTYPER;    /* 0x04 Output type register   */
    volatile uint32_t OSPEEDR;   /* 0x08 Output speed register  */
    volatile uint32_t PUPDR;     /* 0x0C Pull-up/pull-down      */
    volatile uint32_t IDR;       /* 0x10 Input data register    */
    volatile uint32_t ODR;       /* 0x14 Output data register   */
    volatile uint32_t BSRR;      /* 0x18 Bit set/reset register */
    volatile uint32_t LCKR;      /* 0x1C Lock register          */
    volatile uint32_t AFRL;      /* 0x20 Alternate function low */
    volatile uint32_t AFRH;      /* 0x24 Alternate function high*/
    volatile uint32_t BRR;       /* 0x28 Bit reset register     */
} GPIO_TypeDef;

#define GPIOA               ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOH               ((GPIO_TypeDef *)GPIOH_BASE)

/* GPIO mode values */
#define GPIO_MODE_INPUT     0x0U
#define GPIO_MODE_OUTPUT    0x1U
#define GPIO_MODE_AF        0x2U
#define GPIO_MODE_ANALOG   0x3U

/* GPIO pull-up/pull-down values */
#define GPIO_PUPD_NONE      0x0U
#define GPIO_PUPD_PU        0x1U
#define GPIO_PUPD_PD        0x2U

/* GPIO output speed */
#define GPIO_SPEED_LOW      0x0U
#define GPIO_SPEED_MEDIUM   0x1U
#define GPIO_SPEED_HIGH     0x2U
#define GPIO_SPEED_VHIGH    0x3U

/* AF selections */
#define GPIO_AF5_SPI1       5
#define GPIO_AF6_SPI3       6
#define GPIO_AF4_I2C1       4
#define GPIO_AF7_USART      7
#define GPIO_AF10_USB        10

/* Helper macros */
#define GPIO_SET_MODE(gpio, pin, mode) \
    do { (gpio)->MODER = ((gpio)->MODER & ~(3U << ((pin)*2))) | ((mode) << ((pin)*2)); } while(0)
#define GPIO_SET_PUPD(gpio, pin, pupd) \
    do { (gpio)->PUPDR = ((gpio)->PUPDR & ~(3U << ((pin)*2))) | ((pupd) << ((pin)*2)); } while(0)
#define GPIO_SET_SPEED(gpio, pin, speed) \
    do { (gpio)->OSPEEDR = ((gpio)->OSPEEDR & ~(3U << ((pin)*2))) | ((speed) << ((pin)*2)); } while(0)
#define GPIO_SET_AF(gpio, pin, af) \
    do { \
        if ((pin) < 8) \
            (gpio)->AFRL = ((gpio)->AFRL & ~(0xFU << ((pin)*4))) | ((af) << ((pin)*4)); \
        else \
            (gpio)->AFRH = ((gpio)->AFRH & ~(0xFU << (((pin)-8)*4))) | ((af) << (((pin)-8)*4)); \
    } while(0)

#define GPIO_SET(gpio, pin)   ((gpio)->BSRR = (1U << (pin)))
#define GPIO_RESET(gpio, pin) ((gpio)->BSRR = (1U << ((pin) + 16)))
#define GPIO_READ(gpio, pin)  (((gpio)->IDR >> (pin)) & 1U)

/* ===================================================================== */
/*  SPI — Serial Peripheral Interface                                     */
/* ===================================================================== */

#define SPI1_BASE           (APB2PERI_BASE + 0x3000UL)
#define SPI2_BASE           (APB1PERI_BASE + 0x3800UL)
#define SPI3_BASE           (APB1PERI_BASE + 0x3C00UL)

typedef struct {
    volatile uint32_t CR1;      /* 0x00 Control register 1     */
    volatile uint32_t CR2;      /* 0x04 Control register 2     */
    volatile uint32_t SR;       /* 0x08 Status register        */
    volatile uint32_t DR;       /* 0x0C Data register          */
    volatile uint32_t CRCPR;    /* 0x10 CRC polynomial         */
    volatile uint32_t RXCRCR;   /* 0x14 RX CRC                 */
    volatile uint32_t TXCRCR;   /* 0x18 TX CRC                 */
} SPI_TypeDef;

#define SPI1                ((SPI_TypeDef *)SPI1_BASE)
#define SPI2                ((SPI_TypeDef *)SPI2_BASE)
#define SPI3                ((SPI_TypeDef *)SPI3_BASE)

/* SPI_CR1 bits */
#define SPI_CR1_CPHA        (1U << 0)
#define SPI_CR1_CPOL        (1U << 1)
#define SPI_CR1_MSTR        (1U << 2)
#define SPI_CR1_BR_SHIFT    3
#define SPI_CR1_BR_MASK     (0x7U << 3)
#define SPI_CR1_SPE         (1U << 6)
#define SPI_CR1_LSBFIRST    (1U << 7)
#define SPI_CR1_SSI         (1U << 8)
#define SPI_CR1_SSM         (1U << 9)
#define SPI_CR1_RXONLY      (1U << 10)
#define SPI_CR1_CRCL        (1U << 11)
#define SPI_CR1_CPOL_0      0
#define SPI_CR1_CPHA_0      0

/* SPI_CR2 bits */
#define SPI_CR2_DS_SHIFT    8
#define SPI_CR2_DS_8BIT     (7U << 8)
#define SPI_CR2_DS_16BIT    (15U << 8)
#define SPI_CR2_FRXTH       (1U << 0)  /* RXNE threshold: 8-bit        */
#define SPI_CR2_SSOE        (1U << 2)
#define SPI_CR2_NSSP        (1U << 3)
#define SPI_CR2_TXEIE       (1U << 1)
#define SPI_CR2_RXNEIE      (1U << 0)

/* SPI_SR bits */
#define SPI_SR_RXNE         (1U << 0)
#define SPI_SR_TXE          (1U << 1)
#define SPI_SR_BSY          (1U << 7)
#define SPI_SR_FRE          (1U << 8)
#define SPI_SR_OVR          (1U << 6)
#define SPI_SR_MODF         (1U << 5)
#define SPI_SR_CRCERR       (1U << 4)

/* SPI baud rate prescalers */
#define SPI_BR_DIV2         0
#define SPI_BR_DIV4         1
#define SPI_BR_DIV8         2
#define SPI_BR_DIV16        3
#define SPI_BR_DIV32        4
#define SPI_BR_DIV64        5
#define SPI_BR_DIV128       6
#define SPI_BR_DIV256       7

/* ===================================================================== */
/*  USART / UART                                                          */
/* ===================================================================== */

#define USART1_BASE         (APB2PERI_BASE + 0x3800UL)
#define USART3_BASE         (APB1PERI_BASE + 0x4000UL)
#define UART4_BASE          (APB1PERI_BASE + 0x4400UL)

typedef struct {
    volatile uint32_t CR1;      /* 0x00 Control register 1     */
    volatile uint32_t CR2;      /* 0x04 Control register 2     */
    volatile uint32_t CR3;      /* 0x08 Control register 3     */
    volatile uint32_t BRR;      /* 0x0C Baud rate              */
    volatile uint32_t GTPR;     /* 0x10 Guard time / prescaler */
    volatile uint32_t RTOR;     /* 0x14 Receiver timeout       */
    volatile uint32_t RQR;      /* 0x18 Request register       */
    volatile uint32_t ISR;      /* 0x1C Interrupt status       */
    volatile uint32_t ICR;      /* 0x20 Interrupt flag clear   */
    volatile uint32_t RDR;      /* 0x24 Receive data           */
    volatile uint32_t TDR;      /* 0x28 Transmit data          */
} USART_TypeDef;

#define USART1              ((USART_TypeDef *)USART1_BASE)
#define USART3              ((USART_TypeDef *)USART3_BASE)
#define UART4               ((USART_TypeDef *)UART4_BASE)

#define USART_CR1_UE        (1U << 0)
#define USART_CR1_RE        (1U << 2)
#define USART_CR1_TE        (1U << 3)
#define USART_CR1_RXNEIE    (1U << 5)
#define USART_CR1_TCIE      (1U << 6)
#define USART_CR1_M0        (1U << 12)
#define USART_CR1_OVER8     (1U << 15)

#define USART_ISR_RXNE      (1U << 5)
#define USART_ISR_TXE       (1U << 7)
#define USART_ISR_TC        (1U << 6)

/* ===================================================================== */
/*  I2C — Inter-Integrated Circuit                                        */
/* ===================================================================== */

#define I2C1_BASE           (APB1PERI_BASE + 0x5400UL)

typedef struct {
    volatile uint32_t CR1;      /* 0x00 Control register 1     */
    volatile uint32_t CR2;      /* 0x04 Control register 2     */
    volatile uint32_t OAR1;     /* 0x08 Own address register 1 */
    volatile uint32_t OAR2;     /* 0x0C Own address register 2 */
    volatile uint32_t TIMINGR;  /* 0x10 Timing register        */
    volatile uint32_t TIMEOUTR; /* 0x14 Timeout register       */
    volatile uint32_t ISR;      /* 0x18 Interrupt status       */
    volatile uint32_t ICR;      /* 0x1C Interrupt flag clear   */
    volatile uint32_t PECR;     /* 0x20 PEC register           */
    volatile uint32_t RXDR;     /* 0x24 Receive data           */
    volatile uint32_t TXDR;     /* 0x28 Transmit data          */
} I2C_TypeDef;

#define I2C1                ((I2C_TypeDef *)I2C1_BASE)

#define I2C_CR1_PE          (1U << 0)
#define I2C_CR1_TXIE        (1U << 1)
#define I2C_CR1_RXIE        (1U << 2)
#define I2C_CR1_NACKIE      (1U << 4)
#define I2C_CR1_STOPIE      (1U << 5)

#define I2C_ISR_TXE         (1U << 0)
#define I2C_ISR_RXNE        (1U << 2)
#define I2C_ISR_NACKF       (1U << 3)
#define I2C_ISR_STOPF       (1U << 5)
#define I2C_ISR_TC          (1U << 6)
#define I2C_ISR_BUSY        (1U << 15)

#define I2C_CR2_NBYTES_SHIFT 16
#define I2C_CR2_RD_WRN      (1U << 10)
#define I2C_CR2_START       (1U << 13)
#define I2C_CR2_STOP        (1U << 14)
#define I2C_CR2_AUTOEND     (1U << 25)

/* ===================================================================== */
/*  DMA — Direct Memory Access                                            */
/* ===================================================================== */

#define DMA1_BASE           (AHB1PERI_BASE + 0x2000UL)
#define DMA1_CHANNEL_BASE(n) (DMA1_BASE + 0x08 + (n) * 0x20UL)

typedef struct {
    volatile uint32_t CCR;      /* 0x00 Channel config         */
    volatile uint32_t CNDTR;    /* 0x04 Transfer count         */
    volatile uint32_t CPAR;     /* 0x08 Peripheral address     */
    volatile uint32_t CMAR;     /* 0x0C Memory address         */
    volatile uint32_t RESERVED;
} DMA_Channel_TypeDef;

#define DMA1_CH1            ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x08))
#define DMA1_CH2            ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x28))
#define DMA1_CH3            ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x48))

#define DMA1_ISR            (*(volatile uint32_t *)(DMA1_BASE + 0x00))
#define DMA1_IFCR           (*(volatile uint32_t *)(DMA1_BASE + 0x04))

/* DMA CCR bits */
#define DMA_CCR_EN          (1U << 0)
#define DMA_CCR_TCIE        (1U << 1)
#define DMA_CCR_HTIE        (1U << 2)
#define DMA_CCR_TEIE        (1U << 3)
#define DMA_CCR_DIR         (1U << 4)   /* 0: periph→mem, 1: mem→periph */
#define DMA_CCR_CIRC        (1U << 5)
#define DMA_CCR_PINC        (1U << 6)
#define DMA_CCR_MINC        (1U << 7)
#define DMA_CCR_PSIZE_8     (0U << 8)
#define DMA_CCR_PSIZE_16    (1U << 8)
#define DMA_CCR_PSIZE_32    (2U << 8)
#define DMA_CCR_MSIZE_8     (0U << 10)
#define DMA_CCR_MSIZE_16    (1U << 10)
#define DMA_CCR_MSIZE_32    (2U << 10)
#define DMA_CCR_PRIO_LOW    (0U << 12)
#define DMA_CCR_PRIO_MED    (1U << 12)
#define DMA_CCR_PRIO_HIGH   (2U << 12)
#define DMA_CCR_PRIO_VHIGH  (3U << 12)

/* ===================================================================== */
/*  RTC — Real-Time Clock                                                 */
/* ===================================================================== */

#define RTC_BASE            (APB1PERI_BASE + 0x2800UL)
#define RTC_TR              (*(volatile uint32_t *)(RTC_BASE + 0x00))
#define RTC_DR              (*(volatile uint32_t *)(RTC_BASE + 0x04))
#define RTC_CR              (*(volatile uint32_t *)(RTC_BASE + 0x08))
#define RTC_ISR             (*(volatile uint32_t *)(RTC_BASE + 0x0C))
#define RTC_PRER            (*(volatile uint32_t *)(RTC_BASE + 0x10))
#define RTC_WUTR            (*(volatile uint32_t *)(RTC_BASE + 0x14))
#define RTC_ALRMAR          (*(volatile uint32_t *)(RTC_BASE + 0x1C))
#define RTC_WPR             (*(volatile uint32_t *)(RTC_BASE + 0x24))

#define RTC_CR_WUTE         (1U << 10)
#define RTC_CR_WUTIE        (1U << 14)
#define RTC_ISR_WUTF        (1U << 10)
#define RTC_ISR_INIT        (1U << 7)
#define RTC_ISR_INITF       (1U << 6)

/* ===================================================================== */
/*  TIM — Timers                                                          */
/* ===================================================================== */

#define TIM1_BASE           (APB2PERI_BASE + 0x0000UL)
#define TIM2_BASE           (APB1PERI_BASE + 0x0000UL)

typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t BDTR;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} TIM_TypeDef;

#define TIM1                ((TIM_TypeDef *)TIM1_BASE)
#define TIM2                ((TIM_TypeDef *)TIM2_BASE)

#define TIM_CR1_CEN         (1U << 0)
#define TIM_DIER_UIE        (1U << 0)
#define TIM_SR_UIF          (1U << 0)

/* ===================================================================== */
/*  USB — Universal Serial Bus (device)                                   */
/* ===================================================================== */

#define USB_BASE            (APB1PERI_BASE + 0x5C00UL)
#define USB_CNTR            (*(volatile uint32_t *)(USB_BASE + 0x00))
#define USB_ISTR            (*(volatile uint32_t *)(USB_BASE + 0x04))
#define USB_FNR             (*(volatile uint32_t *)(USB_BASE + 0x08))
#define USB_DADDR           (*(volatile uint32_t *)(USB_BASE + 0x0C))
#define USB_BTABLE          (*(volatile uint32_t *)(USB_BASE + 0x10))

#define USB_CNTR_USBRST     (1U << 0)
#define USB_CNTR_RESETM     (1U << 1)
#define USB_CNTR_CTRM       (1U << 15)

/* ===================================================================== */
/*  EXTI — External Interrupts                                            */
/* ===================================================================== */

#define EXTI_BASE           (APB2PERI_BASE + 0x3C00UL)
#define EXTI_IMR1           (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_EMR1           (*(volatile uint32_t *)(EXTI_BASE + 0x04))
#define EXTI_RTSR1          (*(volatile uint32_t *)(EXTI_BASE + 0x08))
#define EXTI_FTSR1          (*(volatile uint32_t *)(EXTI_BASE + 0x0C))
#define EXTI_SWIER1         (*(volatile uint32_t *)(EXTI_BASE + 0x10))
#define EXTI_PR1            (*(volatile uint32_t *)(EXTI_BASE + 0x14))

#define EXTI_IMR2           (*(volatile uint32_t *)(EXTI_BASE + 0x20))
#define EXTI_PR2            (*(volatile uint32_t *)(EXTI_BASE + 0x34))

/* ===================================================================== */
/*  IRQ numbers                                                           */
/* ===================================================================== */

#define IRQ_EXTI4           16      /* ADS1298 DRDY on PC4              */
#define IRQ_DMA1_CH1        17
#define IRQ_DMA1_CH2        18
#define IRQ_DMA1_CH3        19
#define IRQ_SPI1            35
#define IRQ_SPI2            36
#define IRQ_SPI3            51
#define IRQ_USART3          39
#define IRQ_UART4           52
#define IRQ_I2C1_EV         31
#define IRQ_RTC_WKUP        3
#define IRQ_TIM2            28
#define IRQ_USB_FS          82

/* ===================================================================== */
/*  Interrupt enable/disable helpers                                      */
/* ===================================================================== */

static inline void nvic_enable(uint32_t irq)
{
    NVIC_ISER(irq / 32) = (1U << (irq % 32));
}

static inline void nvic_clear_pending(uint32_t irq)
{
    NVIC_ICPR(irq / 32) = (1U << (irq % 32));
}

static inline void nvic_set_priority(uint32_t irq, uint8_t prio)
{
    NVIC_IP(irq) = prio;
}

/* ===================================================================== */
/*  CRC (for SD log checksums)                                            */
/* ===================================================================== */

#define CRC_BASE            (AHB1PERI_BASE + 0x4000UL)
#define CRC_DR              (*(volatile uint32_t *)(CRC_BASE + 0x00))
#define CRC_CR              (*(volatile uint32_t *)(CRC_BASE + 0x08))
#define CRC_CR_RESET        (1U << 0)

#endif /* MYCOMESH_REGISTERS_H */