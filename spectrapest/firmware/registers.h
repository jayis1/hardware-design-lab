/*
 * registers.h — STM32H753 register definitions for SpectraPest
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Low-level peripheral register definitions. This header avoids the full
 * ST HAL and provides direct register access for time-critical operations
 * (DMA setup, interrupt configuration, clock tree programming).
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------- *
 *  Base addresses
 * ----------------------------------------------------------------- */
#define PERIPH_BASE         0x40000000UL
#define PERIPH_APB1_BASE    0x40000000UL
#define PERIPH_APB2_BASE    0x40010000UL
#define PERIPH_APB3_BASE    0x50000000UL
#define PERIPH_AHB1_BASE    0x40020000UL
#define PERIPH_AHB2_BASE    0x48000000UL
#define PERIPH_AHB3_BASE    0x50200000UL
#define PERIPH_AHB4_BASE    0x58000000UL

/* ----------------------------------------------------------------- *
 *  RCC (Reset and Clock Control)
 * ----------------------------------------------------------------- */
#define RCC_BASE            (PERIPH_AHB1_BASE + 0x0000)
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_PLLCFGR          (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_AHB1ENR          (*(volatile uint32_t *)(RCC_BASE + 0xD0))
#define RCC_AHB2ENR          (*(volatile uint32_t *)(RCC_BASE + 0xD4))
#define RCC_AHB3ENR          (*(volatile uint32_t *)(RCC_BASE + 0xD8))
#define RCC_AHB4ENR          (*(volatile uint32_t *)(RCC_BASE + 0xDC))
#define RCC_APB1ENR          (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_APB1ENR1         (*(volatile uint32_t *)(RCC_BASE + 0xE0))
#define RCC_APB1ENR2         (*(volatile uint32_t *)(RCC_BASE + 0xE4))
#define RCC_APB2ENR          (*(volatile uint32_t *)(RCC_BASE + 0xE8))
#define RCC_APB3ENR          (*(volatile uint32_t *)(RCC_BASE + 0xEC))
#define RCC_AHB1LPENR        (*(volatile uint32_t *)(RCC_BASE + 0xE0))

/* RCC_CR bit definitions */
#define RCC_CR_HSION         (1 << 5)
#define RCC_CR_HSIRDY        (1 << 4)
#define RCC_CR_HSEON         (1 << 16)
#define RCC_CR_HSERDY        (1 << 17)
#define RCC_CR_HSEBYP        (1 << 18)
#define RCC_CR_PLL1ON        (1 << 24)
#define RCC_CR_PLL1RDY       (1 << 25)

/* RCC_CFGR SW bits */
#define RCC_CFGR_SW_HSI      0
#define RCC_CFGR_SW_HSE      1
#define RCC_CFGR_SW_PLL      3
#define RCC_CFGR_SWS_MASK    (0x3 << 3)
#define RCC_CFGR_SWS_PLL     (0x3 << 3)

/* Enable bits */
#define RCC_AHB1ENR_DMA1EN   (1 << 0)
#define RCC_AHB1ENR_DMA2EN   (1 << 1)
#define RCC_AHB1ENR_FLASHEN  (1 << 8)
#define RCC_AHB1ENR_CRCEN    (1 << 19)
#define RCC_AHB3ENR_MDMAEN    (1 << 0)
#define RCC_AHB4ENR_GPIOAEN  (1 << 0)
#define RCC_AHB4ENR_GPIOBEN  (1 << 1)
#define RCC_AHB4ENR_GPIOCEN  (1 << 2)
#define RCC_AHB4ENR_GPIODEN  (1 << 3)
#define RCC_AHB4ENR_GPIOEEN  (1 << 4)
#define RCC_APB1ENR1_USART2EN (1 << 17)
#define RCC_APB1ENR1_UART4EN  (1 << 19)
#define RCC_APB1ENR1_I2C1EN  (1 << 21)
#define RCC_APB1ENR1_SPI2EN  (1 << 14)
#define RCC_APB1ENR1_TIM2EN   (1 << 0)
#define RCC_APB1ENR2_LPUART1EN (1 << 0)
#define RCC_APB2ENR_SPI1EN  (1 << 12)
#define RCC_APB2ENR_USART1EN (1 << 4)
#define RCC_APB3ENR_LTDCEN    (1 << 3)

/* ----------------------------------------------------------------- *
 *  GPIO
 * ----------------------------------------------------------------- */
#define GPIOA_BASE          (PERIPH_AHB4_BASE + 0x0000)
#define GPIOB_BASE          (PERIPH_AHB4_BASE + 0x0400)
#define GPIOC_BASE          (PERIPH_AHB4_BASE + 0x0800)
#define GPIOD_BASE          (PERIPH_AHB4_BASE + 0x0C00)
#define GPIOE_BASE          (PERIPH_AHB4_BASE + 0x1000)

typedef struct {
    volatile uint32_t MODER;    /* Offset 0x00 */
    volatile uint32_t OTYPER;   /* Offset 0x04 */
    volatile uint32_t OSPEEDR;   /* Offset 0x08 */
    volatile uint32_t PUPDR;    /* Offset 0x0C */
    volatile uint32_t IDR;      /* Offset 0x10 */
    volatile uint32_t ODR;      /* Offset 0x14 */
    volatile uint32_t BSRR;     /* Offset 0x18 */
    volatile uint32_t LCKR;     /* Offset 0x1C */
    volatile uint32_t AFRL;     /* Offset 0x20 */
    volatile uint32_t AFRH;     /* Offset 0x24 */
} gpio_regs_t;

#define GPIOA ((gpio_regs_t *)GPIOA_BASE)
#define GPIOB ((gpio_regs_t *)GPIOB_BASE)
#define GPIOC ((gpio_regs_t *)GPIOC_BASE)
#define GPIOD ((gpio_regs_t *)GPIOD_BASE)
#define GPIOE ((gpio_regs_t *)GPIOE_BASE)

/* GPIO modes */
#define GPIO_MODE_INPUT      0
#define GPIO_MODE_OUTPUT      1
#define GPIO_MODE_AF          2
#define GPIO_MODE_ANALOG      3

/* GPIO output types */
#define GPIO_OTYPE_PP        0
#define GPIO_OTYPE_OD        1

/* GPIO speeds */
#define GPIO_SPEED_LOW       0
#define GPIO_SPEED_MED       1
#define GPIO_SPEED_HIGH      2
#define GPIO_SPEED_VHIGH     3

/* GPIO pull */
#define GPIO_PUPD_NONE       0
#define GPIO_PUPD_UP         1
#define GPIO_PUPD_DOWN       2

/* AF (alternate function) numbers */
#define AF_USART2_TX         7
#define AF_USART2_RX         7
#define AF_SPI1_SCK          5
#define AF_SPI1_MISO          5
#define AF_SPI1_MOSI          5
#define AF_I2C1_SDA          4
#define AF_I2C1_SCL          4
#define AF_UART4_TX          6
#define AF_UART4_RX          6
#define AF_I2S2_CK           5
#define AF_I2S2_WS           5
#define AF_I2S2_SD           5

static inline void gpio_set_mode(gpio_regs_t *port, uint8_t pin, uint8_t mode) {
    port->MODER = (port->MODER & ~(3 << (pin * 2))) | ((uint32_t)mode << (pin * 2));
}
static inline void gpio_set_af(gpio_regs_t *port, uint8_t pin, uint8_t af) {
    if (pin < 8) {
        port->AFRL = (port->AFRL & ~(0xF << (pin * 4))) | ((uint32_t)af << (pin * 4));
    } else {
        port->AFRH = (port->AFRH & ~(0xF << ((pin - 8) * 4))) | ((uint32_t)af << ((pin - 8) * 4));
    }
}
static inline void gpio_set_otype(gpio_regs_t *port, uint8_t pin, uint8_t otype) {
    port->OTYPER = (port->OTYPER & ~(1 << pin)) | ((uint32_t)otype << pin);
}
static inline void gpio_set_ospeed(gpio_regs_t *port, uint8_t pin, uint8_t speed) {
    port->OSPEEDR = (port->OSPEEDR & ~(3 << (pin * 2))) | ((uint32_t)speed << (pin * 2));
}
static inline void gpio_set_pupd(gpio_regs_t *port, uint8_t pin, uint8_t pupd) {
    port->PUPDR = (port->PUPDR & ~(3 << (pin * 2))) | ((uint32_t)pupd << (pin * 2));
}
static inline void gpio_write(gpio_regs_t *port, uint8_t pin, uint8_t val) {
    if (val)
        port->BSRR = 1 << pin;
    else
        port->BSRR = 1 << (pin + 16);
}
static inline uint8_t gpio_read(gpio_regs_t *port, uint8_t pin) {
    return (port->IDR >> pin) & 1;
}

/* ----------------------------------------------------------------- *
 *  USART (for GNSS and ESP32-C3)
 * ----------------------------------------------------------------- */
#define USART2_BASE         (PERIPH_APB1_BASE + 0x4400)
#define UART4_BASE          (PERIPH_APB1_BASE + 0x4C00)

typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t CR3;    /* 0x08 */
    volatile uint32_t BRR;    /* 0x0C */
    volatile uint32_t GTPR;   /* 0x10 */
    volatile uint32_t RTOR;   /* 0x14 */
    volatile uint32_t RQR;    /* 0x18 */
    volatile uint32_t ISR;    /* 0x1C */
    volatile uint32_t ICR;    /* 0x20 */
    volatile uint32_t RDR;    /* 0x24 */
    volatile uint32_t TDR;    /* 0x28 */
} usart_regs_t;

#define USART2 ((usart_regs_t *)USART2_BASE)
#define UART4  ((usart_regs_t *)UART4_BASE)

/* USART CR1 bits */
#define USART_CR1_UE         (1 << 0)
#define USART_CR1_RE         (1 << 2)
#define USART_CR1_TE         (1 << 3)
#define USART_CR1_RXNEIE    (1 << 5)
#define USART_CR1_TCIE      (1 << 6)

/* USART ISR bits */
#define USART_ISR_RXNE      (1 << 5)
#define USART_ISR_TC         (1 << 6)
#define USART_ISR_BUSY      (1 << 15)

/* ----------------------------------------------------------------- *
 *  I2C (for environmental sensors)
 * ----------------------------------------------------------------- */
#define I2C1_BASE           (PERIPH_APB1_BASE + 0x5400)

typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t OAR1;   /* 0x08 */
    volatile uint32_t OAR2;   /* 0x0C */
    volatile uint32_t TIMINGR; /* 0x10 */
    volatile uint32_t TIMEOUTR; /* 0x14 */
    volatile uint32_t ISR;    /* 0x18 */
    volatile uint32_t ICR;    /* 0x1C */
    volatile uint32_t PECR;   /* 0x20 */
    volatile uint32_t RXDR;   /* 0x24 */
    volatile uint32_t TXDR;   /* 0x28 */
} i2c_regs_t;

#define I2C1   ((i2c_regs_t *)I2C1_BASE)

/* I2C CR1 bits */
#define I2C_CR1_PE          (1 << 0)
#define I2C_CR1_TXIE        (1 << 1)
#define I2C_CR1_RXIE        (1 << 2)

/* I2C CR2 bits */
#define I2C_CR2_START       (1 << 13)
#define I2C_CR2_STOP         (1 << 14)
#define I2C_CR2_NBYTES_SHIFT 16
#define I2C_CR2_AUTOEND     (1 << 20)
#define I2C_CR2_RD_WRN      (1 << 10)

/* I2C ISR bits */
#define I2C_ISR_TXIS        (1 << 1)
#define I2C_ISR_RXNE        (1 << 2)
#define I2C_ISR_TC          (1 << 6)
#define I2C_ISR_NACKF       (1 << 4)
#define I2C_ISR_BUSY        (1 << 15)

/* ----------------------------------------------------------------- *
 *  SPI (for LoRa, Flash, FRAM)
 * ----------------------------------------------------------------- */
#define SPI1_BASE           (PERIPH_APB2_BASE + 0x3000)
#define SPI2_BASE           (PERIPH_APB1_BASE + 0x3800)

typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t CFG1;   /* 0x08 */
    volatile uint32_t CFG2;   /* 0x0C */
    volatile uint32_t IER;    /* 0x10 */
    volatile uint32_t SR;     /* 0x14 */
    volatile uint32_t IFR;    /* 0x18 */
    volatile uint32_t IFCR;   /* 0x1C */
    volatile uint32_t TXDR;   /* 0x20 */
    volatile uint32_t RXDR;   /* 0x24 */
    volatile uint32_t CRC1;   /* 0x28 */
    volatile uint32_t UDRDR;  /* 0x30 */
} spi_regs_t;

#define SPI1   ((spi_regs_t *)SPI1_BASE)
#define SPI2   ((spi_regs_t *)SPI2_BASE)

/* SPI CFG1 bits */
#define SPI_CFG1_MASTER     (1 << 0)
#define SPI_CFG1_CSIZE_8BIT  (0x7 << 0)  /* 8-bit data */
#define SPI_CFG1_CRCEN      (1 << 8)

/* SPI CFG2 bits */
#define SPI_CFG2_CPOL       (1 << 0)
#define SPI_CFG2_CPHA       (1 << 1)
#define SPI_CFG2_LSBFIRST   (1 << 2)
#define SPI_CFG2_COMM_TX    (1 << 17)
#define SPI_CFG2_COMM_RX    (2 << 17)
#define SPI_CFG2_COMM_FULL  (3 << 17)

/* SPI CR bits */
#define SPI_CR1_SPE          (1 << 0)
#define SPI_CR1_CSTART       (1 << 1)
#define SPI_CR1_CSUSP       (1 << 2)

/* SPI SR bits */
#define SPI_SR_TXP          (1 << 0)
#define SPI_SR_RXP          (1 << 1)
#define SPI_SR_EOT          (1 << 3)
#define SPI_SR_TCF          (1 << 5)

/* ----------------------------------------------------------------- *
 *  DMA (Direct Memory Access)
 * ----------------------------------------------------------------- */
#define DMA1_BASE           (PERIPH_AHB1_BASE + 0x6000)

typedef struct {
    volatile uint32_t PAR;    /* Peripheral address */
    volatile uint32_t M0AR;   /* Memory address 0 */
    volatile uint32_t M1AR;   /* Memory address 1 */
    volatile uint32_t NDTR;   /* Number of data transfers */
    volatile uint32_t CR;     /* Configuration register */
    volatile uint32_t FCR;    /* FIFO control */
} dma_stream_t;

#define DMA1_STREAM(n)  ((dma_stream_t *)(DMA1_BASE + n * 0x20))
#define DMA1_STR3       DMA1_STREAM(3)
#define DMA1_STR4       DMA1_STREAM(4)
#define DMA1_STR5       DMA1_STREAM(5)
#define DMA1_STR6       DMA1_STREAM(6)
#define DMA1_STR7       DMA1_STREAM(7)

/* DMA stream register (high-level) */
typedef struct {
    volatile uint32_t LISR;   /* Low interrupt status */
    volatile uint32_t HISR;   /* High interrupt status */
    volatile uint32_t LIFCR;  /* Low interrupt flag clear */
    volatile uint32_t HIFCR;  /* High interrupt flag clear */
} dma_controller_t;

#define DMA1_CTRL ((dma_controller_t *)(DMA1_BASE + 0x00))

/* DMA CR bits */
#define DMA_CR_EN          (1 << 0)
#define DMA_CR_TCIE        (1 << 4)
#define DMA_CR_HTIE        (1 << 3)
#define DMA_CR_MINC        (1 << 10)  /* Memory increment */
#define DMA_CR_PINC        (1 << 9)   /* Peripheral increment */
#define DMA_CR_DBM         (1 << 14)  /* Double buffer mode */
#define DMA_CR_CT          (1 << 19)  /* Current target (double buffer) */
#define DMA_CR_PL_MED      (1 << 16)
#define DMA_CR_PL_HIGH     (2 << 16)
#define DMA_CR_PL_VHIGH    (3 << 16)
#define DMA_CR_PSIZE_16    (1 << 11)  /* 16-bit peripheral */
#define DMA_CR_MSIZE_16    (1 << 13)  /* 16-bit memory */
#define DMA_CR_DIR_P2M     (0 << 6)   /* Peripheral-to-memory */
#define DMA_CR_DIR_M2P     (1 << 6)  /* Memory-to-peripheral */
#define DMA_CR_CIRC        (1 << 8)   /* Circular mode */

/* DMA channel select (in CR, bits 25-27 = CHSEL) */
#define DMA_CHSEL_SPI1      3
#define DMA_CHSEL_I2S2      4
#define DMA_CHSEL_DCMI      5

/* ----------------------------------------------------------------- *
 *  DCMI (Camera interface)
 * ----------------------------------------------------------------- */
#define DCMI_BASE           (PERIPH_AHB2_BASE + 0x5000)

typedef struct {
    volatile uint32_t CR;    /* 0x00 Control */
    volatile uint32_t SR;    /* 0x04 Status */
    volatile uint32_t RISR;   /* 0x08 Raw interrupt */
    volatile uint32_t IER;   /* 0x0C Interrupt enable */
    volatile uint32_t CR1;   /* 0x10 */
    volatile uint32_t CWSZ;  /* 0x14 Crop window size */
    volatile uint32_t CWSTRTR; /* 0x18 Crop window start */
    volatile uint32_t DATA;  /* 0x1C Data */
    volatile uint32_t SCR;   /* 0x20 Synchronization codes */
    volatile uint32_t ESCR;  /* 0x24 Embedded synchronization */
} dcmi_regs_t;

#define DCMI   ((dcmi_regs_t *)DCMI_BASE)

#define DCMI_CR_ENABLE      (1 << 0)
#define DCMI_CR_CAPTURE     (1 << 1)
#define DCMI_CR_JPEG        (1 << 3)
#define DCMI_CR_HSYNC       (1 << 4)
#define DCMI_CR_VSYNC       (1 << 5)
#define DCMI_CR_EDM_8BIT     (0 << 8)
#define DCMI_CR_FCRC_0      (0 << 10)
#define DCMI_CR_PCKPOL      (1 << 14)
#define DCMI_CR_CM          (1 << 15)

/* ----------------------------------------------------------------- *
 *  RTC (Real-time clock)
 * ----------------------------------------------------------------- */
#define RTC_BASE            (PERIPH_APB1_BASE + 0x2800)

typedef struct {
    volatile uint32_t TR;    /* Time register */
    volatile uint32_t DR;    /* Date register */
    volatile uint32_t CR;    /* Control */
    volatile uint32_t ISR;   /* Init/status */
    volatile uint32_t PRER;  /* Prescaler */
    volatile uint32_t WUTR;  /* Wakeup timer */
    volatile uint32_t CALR;  /* Calibration */
    volatile uint32_t SSR;   /* Sub-second */
    volatile uint32_t SHIFTR; /* Shift control */
    volatile uint32_t TSTR;  /* Timestamp time */
    volatile uint32_t TSDR;  /* Timestamp date */
    volatile uint32_t TSSSR; /* Timestamp subsecond */
    volatile uint32_t CALR2;
    volatile uint32_t ALRMAR;
    volatile uint32_t ALRMASSR;
    volatile uint32_t ALRMBR;
    volatile uint32_t ALRMBSSR;
} rtc_regs_t;

#define RTC ((rtc_regs_t *)RTC_BASE)

#define RTC_ISR_INIT         (1 << 7)
#define RTC_ISR_INITF        (1 << 6)
#define RTC_ISR_RSF          (1 << 5)
#define RTC_CR_WUTE          (1 << 10)
#define RTC_CR_WUTIE         (1 << 14)

/* ----------------------------------------------------------------- *
 *  TIM (Timer for scheduler tick)
 * ----------------------------------------------------------------- */
#define TIM2_BASE           (PERIPH_APB1_BASE + 0x0000)

typedef struct {
    volatile uint32_t CR1;    /* 0x00 */
    volatile uint32_t CR2;    /* 0x04 */
    volatile uint32_t SMCR;   /* 0x08 */
    volatile uint32_t DIER;   /* 0x0C */
    volatile uint32_t SR;     /* 0x10 */
    volatile uint32_t EGR;    /* 0x14 */
    volatile uint32_t CCMR1;   /* 0x18 */
    volatile uint32_t CCMR2;  /* 0x1C */
    volatile uint32_t CCER;   /* 0x20 */
    volatile uint32_t CNT;    /* 0x24 */
    volatile uint32_t PSC;    /* 0x28 */
    volatile uint32_t ARR;    /* 0x2C */
    volatile uint32_t CCR1;   /* 0x34 */
    volatile uint32_t CCR2;   /* 0x38 */
    volatile uint32_t CCR3;   /* 0x3C */
    volatile uint32_t CCR4;   /* 0x40 */
} tim_regs_t;

#define TIM2   ((tim_regs_t *)TIM2_BASE)

#define TIM_CR1_CEN          (1 << 0)
#define TIM_DIER_UIE        (1 << 0)
#define TIM_SR_UIF          (1 << 0)

/* ----------------------------------------------------------------- *
 *  Flash controller (for reading model weights)
 * ----------------------------------------------------------------- */
#define FLASH_BASE          (PERIPH_AHB1_BASE + 0x2000)

typedef struct {
    volatile uint32_t ACR;    /* Access control */
    volatile uint32_t KEYR1;  /* Key register 1 */
    volatile uint32_t OPTKEYR;
    volatile uint32_t CR1;    /* Control 1 */
    volatile uint32_t SR1;    /* Status 1 */
    volatile uint32_t CCR1;   /* Clear flag 1 */
} flash_regs_t;

#define FLASH  ((flash_regs_t *)FLASH_BASE)

#define FLASH_ACR_LATENCY_MASK  0x0F
#define FLASH_ACR_PRFTEN        (1 << 8)
#define FLASH_ACR_ICEN          (1 << 9)
#define FLASH_ACR_DCEN          (1 << 10)

/* ----------------------------------------------------------------- *
 *  NVIC (Nested Vectored Interrupt Controller)
 * ----------------------------------------------------------------- */
#define NVIC_BASE           0xE000E100UL

#define NVIC_ISER0          (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ISER1          (*(volatile uint32_t *)(NVIC_BASE + 0x004))
#define NVIC_ICER0          (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ICER1          (*(volatile uint32_t *)(NVIC_BASE + 0x084))
#define NVIC_IPR(n)         (*(volatile uint8_t *)(NVIC_BASE + 0x300 + n))

/* IRQ numbers */
#define IRQ_TIM2             28
#define IRQ_USART2           38
#define IRQ_SPI1             35
#define IRQ_DMA1_STR3        58
#define IRQ_DMA1_STR4        59
#define IRQ_DMA1_STR5        60
#define IRQ_DMA1_STR6        61
#define IRQ_EXTI0            6
#define IRQ_EXTI1            7
#define IRQ_EXTI2            8

/* ----------------------------------------------------------------- *
 *  SysTick (for scheduler tick backup)
 * ----------------------------------------------------------------- */
#define SYSTICK_BASE        0xE000E010UL
#define SYSTICK_CSR         (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYSTICK_CALIB       (*(volatile uint32_t *)(SYSTICK_BASE + 0x0C))

#define SYSTICK_CSR_ENABLE  (1 << 0)
#define SYSTICK_CSR_TICKINT (1 << 1)
#define SYSTICK_CSR_CLKSRC   (1 << 2)

/* ----------------------------------------------------------------- *
 *  ADC (for battery/solar voltage reading)
 * ----------------------------------------------------------------- */
#define ADC1_BASE           (PERIPH_AHB2_BASE + 0x0000)

typedef struct {
    volatile uint32_t ISR;    /* 0x00 */
    volatile uint32_t IER;    /* 0x04 */
    volatile uint32_t CR;     /* 0x08 */
    volatile uint32_t CFGR;   /* 0x0C */
    volatile uint32_t SQR1;   /* 0x30 */
    volatile uint32_t SQR2;   /* 0x34 */
    volatile uint32_t SQR3;   /* 0x38 */
    volatile uint32_t DR1;    /* 0x40 */
    volatile uint32_t DR2;    /* 0x44 */
    volatile uint32_t SMPR1;  /* 0x14 */
    volatile uint32_t SMPR2;  /* 0x18 */
} adc_regs_t;

#define ADC1   ((adc_regs_t *)ADC1_BASE)

#define ADC_ISR_ADRDY       (1 << 0)
#define ADC_ISR_EOC         (1 << 2)
#define ADC_CR_ADEN          (1 << 0)
#define ADC_CR_ADSTART      (1 << 2)
#define RCC_AHB2ENR_ADC12EN  (1 << 5)

#ifdef __cplusplus
}
#endif

#endif /* REGISTERS_H */