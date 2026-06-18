/*
 * registers.h — STM32H7A3 register helpers for AeroCast
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Lightweight, header-only register map for the peripherals used by AeroCast.
 * No CMSIS dependency — keeps the build self-contained for review.
 */
#ifndef AEROCAST_REGISTERS_H
#define AEROCAST_REGISTERS_H

#include <stdint.h>

/* ---- Base addresses (from STM32H7A3 reference manual RM0466) ---- */
#define PERIPH_BASE       0x40000000UL
#define AHB1_BASE         (PERIPH_BASE + 0x00020000UL)
#define AHB2_BASE         (PERIPH_BASE + 0x08020000UL)
#define APB1_BASE         (PERIPH_BASE + 0x00000000UL)
#define APB2_BASE         (PERIPH_BASE + 0x00010000UL)

#define GPIOA_BASE        (AHB4_BASE + 0x0000UL)
#define GPIOB_BASE        (AHB4_BASE + 0x0400UL)
#define GPIOC_BASE        (AHB4_BASE + 0x0800UL)
#define GPIOD_BASE        (AHB4_BASE + 0x0C00UL)
#define GPIOE_BASE        (AHB4_BASE + 0x1000UL)
#define GPIOH_BASE        (AHB4_BASE + 0x1C00UL)
#define AHB4_BASE         (PERIPH_BASE + 0x10000000UL)

#define SPI1_BASE         (APB2_BASE + 0x3000UL)
#define SPI2_BASE         (APB1_BASE + 0x3800UL)
#define SPI3_BASE         (APB1_BASE + 0x3C00UL)

#define USART2_BASE       (APB1_BASE + 0x2400UL)
#define USART3_BASE       (APB1_BASE + 0x2800UL)

#define I2C1_BASE         (APB1_BASE + 0x4000UL)
#define I2C4_BASE         (APB1_BASE + 0x5000UL)

#define TIM2_BASE         (APB1_BASE + 0x0000UL)
#define TIM3_BASE         (APB1_BASE + 0x0400UL)

#define DAC1_BASE         (AHB2_BASE + 0x0000UL)
#define ADC1_BASE         (AHB2_BASE + 0x2000UL)
#define ADC2_BASE         (AHB2_BASE + 0x2100UL)

#define SDMMC1_BASE       (AHB2_BASE + 0x2000UL)

/* RCC for clock-gating */
#define RCC_BASE          (AHB1_BASE + 0x4400UL)
#define RCC_AHB4ENR       (*(volatile uint32_t *)(RCC_BASE + 0x0E0UL))
#define RCC_APB1ENR       (*(volatile uint32_t *)(RCC_BASE + 0x058UL))
#define RCC_APB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x060UL))
#define RCC_AHB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x0C0UL))

/* PWR for voltage scaling */
#define PWR_BASE          (APB1_BASE + 0x5000UL)
#define PWR_D3CR          (*(volatile uint32_t *)(PWR_BASE + 0x0CUL))

/* ---- GPIO helpers ---- */
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
} gpio_t;

#define GPIO(port) ((gpio_t *)(port##_BASE))

#define GPIO_MODE_INPUT   0
#define GPIO_MODE_OUTPUT  1
#define GPIO_MODE_AF      2
#define GPIO_MODE_ANALOG  3

static inline void gpio_set_mode(volatile uint32_t *port, uint8_t pin, uint8_t mode)
{
    gpio_t *g = (gpio_t *)port;
    g->MODER = (g->MODER & ~(3u << (2u * pin))) | ((uint32_t)mode << (2u * pin));
}

static inline void gpio_set_af(volatile uint32_t *port, uint8_t pin, uint8_t af)
{
    gpio_t *g = (gpio_t *)port;
    if (pin < 8)
        g->AFRL = (g->AFRL & ~(0xFu << (4u * pin))) | ((uint32_t)af << (4u * pin));
    else
        g->AFRH = (g->AFRH & ~(0xFu << (4u * (pin - 8u)))) |
                  ((uint32_t)af << (4u * (pin - 8u)));
}

static inline void gpio_set_pupd(volatile uint32_t *port, uint8_t pin, uint8_t pupd)
{
    gpio_t *g = (gpio_t *)port;
    g->PUPDR = (g->PUPDR & ~(3u << (2u * pin))) | ((uint32_t)pupd << (2u * pin));
}

static inline void gpio_set_otype(volatile uint32_t *port, uint8_t pin, uint8_t otype)
{
    gpio_t *g = (gpio_t *)port;
    g->OTYPER = (g->OTYPER & ~(1u << pin)) | ((uint32_t)otype << pin);
}

static inline void gpio_set(volatile uint32_t *port, uint8_t pin)
{
    gpio_t *g = (gpio_t *)port;
    g->BSRR = 1u << pin;
}

static inline void gpio_clr(volatile uint32_t *port, uint8_t pin)
{
    gpio_t *g = (gpio_t *)port;
    g->BSRR = 1u << (pin + 16u);
}

static inline uint8_t gpio_read(volatile uint32_t *port, uint8_t pin)
{
    gpio_t *g = (gpio_t *)port;
    return (g->IDR >> pin) & 1u;
}

static inline void gpio_toggle(volatile uint32_t *port, uint8_t pin)
{
    gpio_t *g = (gpio_t *)port;
    if ((g->ODR >> pin) & 1u) gpio_clr(port, pin); else gpio_set(port, pin);
}

/* ---- Timer helpers (minimal TIM2/TIM3 register layout) ---- */
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
} tim_t;

#define TIM(p) ((tim_t *)(p##_BASE))

static inline void tim_init_pwm(uint32_t base, uint32_t psc, uint32_t arr, uint32_t ccr1)
{
    tim_t *t = (tim_t *)base;
    t->CR1 = 0;
    t->PSC = psc;
    t->ARR = arr;
    t->CCR1 = ccr1;
    /* PWM mode 1 on CH1, preload enabled */
    t->CCMR1 = (6u << 4u) | (1u << 3u);
    t->CCER = 1u;       /* CC1E */
    t->BDTR = (1u << 15u); /* MOE */
    t->EGR = 1u;
    t->CR1 = 1u;        /* CEN */
}

static inline void tim_set_duty(uint32_t base, uint32_t ccr)
{
    tim_t *t = (tim_t *)base;
    t->CCR1 = ccr;
}

/* ---- SPI helpers (minimal) ---- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CFG1;
    volatile uint32_t CFG2;
    volatile uint32_t IER;
    volatile uint32_t SR;
    volatile uint32_t IFCR;
    volatile uint32_t TXDR;
    volatile uint32_t RXDR;
    volatile uint32_t CRCPR;
} spi_t;

#define SPI(p) ((spi_t *)(p##_BASE))

static inline void spi_init_master(uint32_t base, uint32_t br_div, uint32_t word_sz)
{
    spi_t *s = (spi_t *)base;
    s->CR1 = 0;
    s->CFG1 = (word_sz << 0u) | (br_div << 28u) | (1u << 22u) /* MSTR */;
    s->CFG2 = (1u << 22u) /* SSOE */ | (1u << 2u) /* CPOL */ | (1u << 1u) /* CPHA */;
    s->CR2 = 0;
    s->CR1 = (1u << 0u) /* SPE */ | (1u << 9u) /* IOLOCK */;
}

static inline void spi_write_u32(uint32_t base, uint32_t val)
{
    spi_t *s = (spi_t *)base;
    while (!(s->SR & (1u << 1u))) ; /* wait TXP */
    s->TXDR = val;
    while (s->SR & (1u << 0u)) ; /* wait RXWNE clear-ish */
    (void)s->RXDR;
}

/* ---- USART helpers ---- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t BRR;
    volatile uint32_t GTPR;
    volatile uint32_t RTOR;
    volatile uint32_t RQR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t RDR;
    volatile uint32_t TDR;
} usart_t;

#define UART(p) ((usart_t *)(p##_BASE))

static inline void uart_init(uint32_t base, uint32_t baud)
{
    usart_t *u = (usart_t *)base;
    u->CR1 = 0;
    u->BRR = (280000000UL + baud / 2u) / baud; /* ~oversample 16 */
    u->CR2 = 0;
    u->CR3 = 0;
    u->CR1 = (1u << 0u) /* UE */ | (1u << 3u) /* TE */ | (1u << 2u) /* RE */;
}

static inline void uart_putc(uint32_t base, uint8_t c)
{
    usart_t *u = (usart_t *)base;
    while (!(u->ISR & (1u << 7u))) ; /* TXE */
    u->TDR = c;
}

static inline void uart_puts(uint32_t base, const char *s)
{
    while (*s) uart_putc(base, (uint8_t)*s++);
}

static inline int uart_getc_nonblocking(uint32_t base, uint8_t *c)
{
    usart_t *u = (usart_t *)base;
    if (u->ISR & (1u << 5u)) { /* RXNE */
        *c = (uint8_t)u->RDR;
        return 1;
    }
    return 0;
}

/* ---- I2C helpers ---- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t TIMINGR;
    volatile uint32_t TIMEOUTR;
    volatile uint32_t ISR;
    volatile uint32_t ICR;
    volatile uint32_t PECR;
    volatile uint32_t RXDR;
    volatile uint32_t TXDR;
} i2c_t;

#define I2C(p) ((i2c_t *)(p##_BASE))

static inline void i2c_init(uint32_t base, uint32_t timingr)
{
    i2c_t *i = (i2c_t *)base;
    i->CR1 = 0;
    i->TIMINGR = timingr;
    i->CR1 = (1u << 0u); /* PE */
}

/* ---- DAC helpers ---- */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t SWTRGR;
    volatile uint32_t DHR12R1;
    volatile uint32_t DHR12R2;
    volatile uint32_t DOR1;
    volatile uint32_t DOR2;
} dac_t;

#define DAC(p) ((dac_t *)(p##_BASE))

static inline void dac_init_ch(uint32_t base, uint8_t ch)
{
    dac_t *d = (dac_t *)base;
    d->CR |= (1u << (16u + ch));  /* EN ch */
    for (volatile int i = 0; i < 100; ++i) ;
    d->CR |= (1u << (0u + ch * 16u)); /* TEN trigger-enable */
}

static inline void dac_write(uint32_t base, uint8_t ch, uint16_t val)
{
    dac_t *d = (dac_t *)base;
    if (ch == 0) d->DHR12R1 = val & 0xFFFu;
    else         d->DHR12R2 = val & 0xFFFu;
}

/* ---- Simple busy-wait ---- */
static inline void delay_ms(uint32_t ms)
{
    /* roughly 280 MHz / 3 cycles per loop ≈ 93 333 loops/ms */
    volatile uint32_t n = ms * 93000u;
    while (n--) ;
}

#endif /* AEROCAST_REGISTERS_H */