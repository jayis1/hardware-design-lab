/*
 * registers.h — STM32L4R9 register-level bit definitions
 * Author: jayis1
 * License: MIT
 *
 * Minimal hand-rolled register definitions used by the firmware drivers in
 * place of the full STM32Cube HAL where direct register access is clearer
 * (TIM1 PWM, SPI for ADS1256, I2C for depth/RTC, EXTI for DRDY/DIO).
 * Base addresses follow ST RM0432 reference manual.
 */
#ifndef HYDROFLUOR_REGISTERS_H
#define HYDROFLUOR_REGISTERS_H

#include <stdint.h>

/* ---- Peripheral base addresses (RM0432 §2.3) ---- */
#define PERIPH_BASE         0x40000000UL
#define PERIPH_BASE_APB1    0x40000000UL
#define PERIPH_BASE_APB2    0x48000400UL   /* TIM1 etc. on APB2 in L4R9 */
#define AHB1PERIPH_BASE     0x40018000UL
#define AHB2PERIPH_BASE     0x48000000UL

/* RCC */
#define RCC_BASE            (AHB1PERIPH_BASE + 0x0000)
#define RCC_CR              (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR            (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_PLLCFGR         (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x48))  /* GPIOA-K */
#define RCC_AHB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x4C))
#define RCC_APB1ENR1        (*(volatile uint32_t *)(RCC_BASE + 0x58))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x78))
#define RCC_BDCR            (*(volatile uint32_t *)(RCC_BASE + 0x90))  /* LSE */

#define RCC_CR_HSIRDY       (1U << 10)
#define RCC_CR_HSION        (1U << 8)
#define RCC_CR_PLLRDY       (1U << 25)
#define RCC_CR_PLLON        (1U << 24)

#define RCC_CFGR_SW_HSI     0U
#define RCC_CFGR_SW_PLL     3U
#define RCC_CFGR_SWS_MSK    0xCU

#define RCC_PLLCFGR_PLLSRC_HSI16  (1U << 0)
#define RCC_PLLCFGR_PLLM_MSK     (0x3FU)
#define RCC_PLLCFGR_PLLN_MSK     (0x1FFU << 8)
#define RCC_PLLCFGR_PLLP_POS     17
#define RCC_PLLCFGR_PLLQ_POS     21
#define RCC_PLLCFGR_PLLR_POS     25

/* GPIO (RM0432 §8.4) */
#define GPIOA_BASE          (AHB2PERIPH_BASE + 0x0000)
#define GPIOB_BASE          (AHB2PERIPH_BASE + 0x0400)
#define GPIOC_BASE          (AHB2PERIPH_BASE + 0x0800)
#define GPIOD_BASE          (AHB2PERIPH_BASE + 0x0C00)

typedef struct {
    volatile uint32_t MODER;    /* 0x00 */
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
} gpio_t;

#define GPIOA ((gpio_t *)GPIOA_BASE)
#define GPIOB ((gpio_t *)GPIOB_BASE)
#define GPIOC ((gpio_t *)GPIOC_BASE)
#define GPIOD ((gpio_t *)GPIOD_BASE)

#define GPIO_MODE_INPUT      0
#define GPIO_MODE_OUTPUT      1
#define GPIO_MODE_AF           2
#define GPIO_MODE_ANALOG       3
#define GPIO_PUPD_NONE         0
#define GPIO_PUPD_PULLUP       1
#define GPIO_PUPD_PULLDOWN     2
#define GPIO_OTYPE_PP          0
#define GPIO_OTYPE_OD          1
#define GPIO_OSPEED_LOW         0
#define GPIO_OSPEED_HIGH       3

static inline void gpio_mode(gpio_t *g, uint8_t pin, uint8_t mode) {
    g->MODER = (g->MODER & ~(3U << (pin*2))) | ((uint32_t)mode << (pin*2));
}
static inline void gpio_pupd(gpio_t *g, uint8_t pin, uint8_t pupd) {
    g->PUPDR = (g->PUPDR & ~(3U << (pin*2))) | ((uint32_t)pupd << (pin*2));
}
static inline void gpio_af(gpio_t *g, uint8_t pin, uint8_t af) {
    if (pin < 8) g->AFRL = (g->AFRL & ~(0xFU << (pin*4))) | ((uint32_t)af << (pin*4));
    else         g->AFRH = (g->AFRH & ~(0xFU << ((pin-8)*4))) | ((uint32_t)af << ((pin-8)*4));
}
static inline void gpio_set(gpio_t *g, uint8_t pin) { g->BSRR = 1U << pin; }
static inline void gpio_clr(gpio_t *g, uint8_t pin) { g->BSRR = 1U << (pin+16); }
static inline uint8_t gpio_read(gpio_t *g, uint8_t pin) { return (g->IDR >> pin) & 1U; }

/* Helper to write a single bit on any GPIO port (defined in drivers). */
void gpio_write_bit(gpio_t *g, uint8_t pin, uint8_t val);

/* TIM1 (RM0432 §29) — APB2 timer, base 0x48012C00 on L4R9 */
#define TIM1_BASE            0x40012C00UL   /* legacy alias; see RM */
typedef struct {
    volatile uint32_t CR1, CR2, SMCR, DIER, SR, EGR, CCMR1, CCMR2, CCER, CNT,
                      PSC, ARR, RCR, CCR1, CCR2, CCR3, CCR4, reserved, BDTR,
                      DCR, DMAR;
} tim_t;
#define TIM1 ((tim_t *)TIM1_BASE)

#define TIM_CR1_CEN          (1U << 0)
#define TIM_CR1_ARPE         (1U << 7)
#define TIM_BDTR_MOE         (1U << 15)
#define TIM_DIER_CC1IE       (1U << 1)
#define TIM_DIER_UIE         (1U << 0)
#define TIM_CCER_CC1E        (1U << 0)
#define TIM_CCER_CC1P        (1U << 1)
#define TIM_CCMR1_OC1M_PWM1  (6U << 4)
#define TIM_CCMR1_OC1PE      (1U << 3)
#define TIM_SR_UIF           (1U << 0)
#define TIM_SR_CC1IF         (1U << 1)

/* SPI1 (RM0432 §41) */
#define SPI1_BASE            0x40013000UL
typedef struct {
    volatile uint32_t CR1, CR2, SR, DR, CRCPR, RXCRC, TXCRC, I2SCFGR;
} spi_t;
#define SPI1 ((spi_t *)SPI1_BASE)
#define SPI2 ((spi_t *)(0x40003800UL))

#define SPI_CR1_SPE          (1U << 6)
#define SPI_CR1_MSTR         (1U << 2)
#define SPI_CR1_BR_DIV8      (1U << 3)
#define SPI_CR1_CPHA         (1U << 0)
#define SPI_CR1_CPOL         (1U << 1)
#define SPI_CR1_LSBFIRST     (1U << 7)
#define SPI_CR2_FRXTH        (1U << 12)
#define SPI_CR2_DS_8BIT      (7U << 8)
#define SPI_SR_RXNE          (1U << 0)
#define SPI_SR_TXE           (1U << 1)
#define SPI_SR_BSY           (1U << 7)

static inline void spi_wait_tx(spi_t *s) { while (!(s->SR & SPI_SR_TXE)); }
static inline void spi_wait_rx(spi_t *s) { while (!(s->SR & SPI_SR_RXNE)); }
static inline void spi_wait_busy(spi_t *s) { while (s->SR & SPI_SR_BSY); }
static inline uint8_t spi_xfer(spi_t *s, uint8_t b) {
    spi_wait_tx(s);
    *(volatile uint8_t *)&s->DR = b;
    spi_wait_rx(s);
    return *(volatile uint8_t *)&s->DR;
}

/* I2C1 (RM0432 §37) */
#define I2C1_BASE            0x40005400UL
typedef struct {
    volatile uint32_t CR1, CR2, OAR1, OAR2, TIMINGR, TIMEOUTR, ISR, ICRR,
                      PECR, RXDR, TXDR;
} i2c_t;
#define I2C1 ((i2c_t *)I2C1_BASE)

#define I2C_CR1_PE           (1U << 0)
#define I2C_CR2_START        (1U << 13)
#define I2C_CR2_STOP         (1U << 14)
#define I2C_CR2_NBYTES(n)    ((uint32_t)(n) << 16)
#define I2C_CR2_AUTOEND       (1U << 25)
#define I2C_CR2_RD_WRN        (1U << 10)
#define I2C_ISR_BUSY          (1U << 15)
#define I2C_ISR_TXE          (1U << 0)
#define I2C_ISR_RXNE         (1U << 2)
#define I2C_ISR_TC           (1U << 6)
#define I2C_ISR_NACKF        (1U << 4)

/* USART1 (RM0432 §38) for BLE */
#define USART1_BASE          0x40013800UL
typedef struct {
    volatile uint32_t CR1, CR2, CR3, BRR, GTPR, RTOR, RQR, ISR, ICR, RDR, TDR;
} usart_t;
#define USART1 ((usart_t *)USART1_BASE)

#define USART_CR1_UE         (1U << 0)
#define USART_CR1_RE         (1U << 2)
#define USART_CR1_TE         (1U << 3)
#define USART_CR1_RXNEIE     (1U << 5)
#define USART_ISR_RXNE       (1U << 5)
#define USART_ISR_TXE        (1U << 7)

static inline void uart_wait_tx(usart_t *u) { while (!(u->ISR & USART_ISR_TXE)); }
static inline void uart_putc(usart_t *u, uint8_t c) { u->TDR = c; uart_wait_tx(u); }

/* EXTI (RM0432 §13) */
#define EXTI_BASE            (AHB1PERIPH_BASE + 0x1000)
#define EXTI_IMR1            (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_RTSR1           (*(volatile uint32_t *)(EXTI_BASE + 0x08))
#define EXTI_FTSR1           (*(volatile uint32_t *)(EXTI_BASE + 0x0C))
#define EXTI_PR1             (*(volatile uint32_t *)(EXTI_BASE + 0x14))  /* pending clear */

/* NVIC */
#define NVIC_ISER0          (*(volatile uint32_t *)0xE000E100)
#define NVIC_ICPR0          (*(volatile uint32_t *)0xE000E280)
static inline void nvic_enable(uint8_t irq) {
    NVIC_ISER0 = 1U << (irq & 31);
}
static inline void nvic_clear(uint8_t irq) {
    NVIC_ICPR0 = 1U << (irq & 31);
}

/* SysTick */
#define SYST_CSR            (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR            (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR            (*(volatile uint32_t *)0xE000E018)
#define SYSTICK_ENABLE      (1U << 0)
#define SYSTICK_CLKSRC      (1U << 2)
#define SYSTICK_TICKINT     (1U << 1)

/* ADC1 (internal, for battery) */
#define ADC1_BASE            0x50040000UL
typedef struct {
    volatile uint32_t ISR, IER, CR, CFGR, SQR1, SQR2, SQR3, DR;
} adc_t;
#define ADC1 ((adc_t *)ADC1_BASE)

#define ADC_ISR_ADRDY        (1U << 0)
#define ADC_CR_ADEN         (1U << 0)
#define ADC_CR_ADSTART      (1U << 2)
#define ADC_ISR_EOC         (1U << 2)

/* DMA (RM0432 §15) — DMA1 channel used for ADC SPI receive */
#define DMA1_BASE            0x40026000UL
typedef struct {
    volatile uint32_t CCR, CNDTR, CPAR, CMAR, CCR2;
} dma_ch_t;
#define DMA1_CH1 ((dma_ch_t *)(DMA1_BASE + 0x08))   /* channel 1 */

#define DMA_CCR_EN           (1U << 0)
#define DMA_CCR_MINC         (1U << 10)
#define DMA_CCR_PINC         (1U << 9)
#define DMA_CCR_TCIE         (1U << 1)
#define DMA_CCR_DIR_P2M      (0U << 4)
#define DMA_CCR_PSIZE_8BIT   (0U << 8)
#define DMA_CCR_MSIZE_8BIT   (0U << 10)

/* Flash / option bytes for DFU boot */
#define FLASH_BASE_ADDR      0x08000000UL
#define FLASH_PAGE_SIZE       2048

/* QSPI (RM0432 §14) — QuadSPI peripheral */
#define QSPI_BASE            0x48090000UL
typedef struct {
    volatile uint32_t CR, DCR, SR, FCR, DLR, CCR, AR, ABR, PIR, PSMAR, PSMKR,
                      WCCR, IR, WTCR;
} qspi_t;
#define QSPI ((qspi_t *)QSPI_BASE)

#define QSPI_CR_EN            (1U << 0)
#define QSPI_CR_DMAEN         (1U << 2)
#define QSPI_CR_TOIE          (1U << 11)
#define QSPI_SR_TCF           (1U << 5)
#define QSPI_CCR_FMODE_IND_READ  (3U << 26)
#define QSPI_CCR_FMODE_IND_WRITE (0U << 26)
#define QSPI_CCR_DMODE_QSPI    (3U << 24)
#define QSPI_CCR_IMODE_QSPI    (3U << 8)
#define QSPI_CCR_ADSIZE_24BIT  (2U << 18)

/* RTC (backup-domain PCF85263A is external; the internal RTC is unused) */
#define LSE_ENABLE_DELAY_MS  200

#endif /* HYDROFLUOR_REGISTERS_H */