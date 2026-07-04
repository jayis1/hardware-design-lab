/*
 * i2c.c — Minimal I²C1 driver (400 kHz, blocking, bare-metal)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Used by the DRV2605L haptic driver and MAX17048 fuel gauge.
 * PB0 = SCL, PB1 = SDA, alt fn 4. The bus runs at 400 kHz (PCLK1=80 MHz,
 * timing value 0x10909). No DMA — blocking only.
 */
#include "i2c.h"
#include "../registers.h"
#include "../board.h"

static void delay_us(uint32_t us)
{
    /* Approximate at 80 MHz: each iteration ~4 cycles → 20 iter/ms. */
    for (volatile uint32_t i = 0; i < us * 20; i++) { }
}

int i2c_init(void)
{
    RCC_AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;
    RCC_APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    /* PB0 = SCL, PB1 = SDA, alt fn 4, open-drain, pull-up (external 4.7 k). */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (0*2))) | (GPIO_MODE_AF << (0*2));
    GPIOB->MODER = (GPIOB->MODER & ~(3U << (1*2))) | (GPIO_MODE_AF << (1*2));
    GPIOB->OTYPER |= (1U << 0) | (1U << 1);  /* open-drain */
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFU << (0*4))) | (4U << (0*4));
    GPIOB->AFRL = (GPIOB->AFRL & ~(0xFU << (1*4))) | (4U << (1*4));

    /* Configure I2C1 timing for 400 kHz at 80 MHz PCLK. */
    I2C1->TIMINGR = 0x00109B28U;   /* standard 400 kHz timing */
    I2C1->CR1 = I2C_CR1_PE;
    return 0;
}

int i2c_write_reg(uint8_t dev_addr_7bit, uint8_t reg, uint8_t val)
{
    uint8_t addr = dev_addr_7bit << 1;

    /* Write 2 bytes (reg + val): NBYTES=2, AUTOEND. */
    I2C1->CR2 = (uint32_t)(addr) | (2U << 16) | I2C_CR2_START | I2C_CR2_AUTOEND;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & I2C_ISR_NACKF) return -1;
    }
    I2C1->TXDR = reg;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & I2C_ISR_NACKF) return -1;
    }
    I2C1->TXDR = val;
    while (!(I2C1->ISR & I2C_ISR_TC) && !(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}

int i2c_read_reg(uint8_t dev_addr_7bit, uint8_t reg, uint8_t *val)
{
    uint8_t addr = dev_addr_7bit << 1;

    /* Write reg pointer (NBYTES=1, no AUTOEND, reload). */
    I2C1->CR2 = (uint32_t)(addr) | (1U << 16) | I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & I2C_ISR_NACKF) return -1;
    }
    I2C1->TXDR = reg;
    while (!(I2C1->ISR & I2C_ISR_TC)) { }

    /* Read 1 byte with repeated start, AUTOEND. */
    I2C1->CR2 = (uint32_t)(addr | 1U) | (1U << 16) | I2C_CR2_START | I2C_CR2_AUTOEND;
    while (!(I2C1->ISR & I2C_ISR_RXNE)) { }
    *val = (uint8_t)I2C1->RXDR;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}

int i2c_read_burst(uint8_t dev_addr_7bit, uint8_t reg, uint8_t *buf, uint8_t n)
{
    uint8_t addr = dev_addr_7bit << 1;

    /* Write reg pointer (NBYTES=1, no AUTOEND). */
    I2C1->CR2 = (uint32_t)(addr) | (1U << 16) | I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & I2C_ISR_NACKF) return -1;
    }
    I2C1->TXDR = reg;
    while (!(I2C1->ISR & I2C_ISR_TC)) { }

    /* Read N bytes with repeated start, AUTOEND. */
    I2C1->CR2 = (uint32_t)(addr | 1U) | ((uint32_t)n << 16) | I2C_CR2_START | I2C_CR2_AUTOEND;
    for (uint8_t i = 0; i < n; i++) {
        while (!(I2C1->ISR & I2C_ISR_RXNE)) { }
        buf[i] = (uint8_t)I2C1->RXDR;
    }
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}