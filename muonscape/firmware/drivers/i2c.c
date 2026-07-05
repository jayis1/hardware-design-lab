/*
 * i2c.c — I²C bus driver (STM32H723 I2C1)
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Polling-mode master driver. 400 kHz. Handles 7-bit addressing,
 * write-then-read (register access), and NACK timeout.
 */
#include "i2c.h"
#include "registers.h"
#include "board.h"

muon_status_t i2c_init(void)
{
    /* Enable I2C1 clock */
    RCC_APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
    /* GPIO PB8/PB9 AF4, open-drain, high speed */
    uint32_t portb = GPIOB_BASE;
    /* Reset MODER for pins 8,9 */
    GPIO_MODER(portb) &= ~(3U << (8 * 2)) & ~(3U << (9 * 2));
    GPIO_MODER(portb) |= (2U << (8 * 2)) | (2U << (9 * 2));   /* AF */
    GPIO_OTYPER(portb) |= (1U << 8) | (1U << 9);            /* open-drain */
    GPIO_OSPEEDR(portb) |= (3U << (8 * 2)) | (3U << (9 * 2));
    /* AFRL for pins 8,9: AF4 */
    GPIO_AFRL(portb) &= ~(0xFU << (8 * 4)) & ~(0xFU << (9 * 4));
    GPIO_AFRL(portb) |= (4U << (8 * 4)) | (4U << (9 * 4));
    /* Pull-ups on SDA/SCL (external 4k7 also present) */
    GPIO_PUPDR(portb) |= (1U << (8 * 2)) | (1U << (9 * 2));

    i2c_regs_t *i2c = (i2c_regs_t *)I2C1_BASE;
    i2c->CR1 = 0;
    /* TIMINGR for 400 kHz at 137.5 MHz PCLK: PRESC=1, SCLL=0x13, SCLH=0xF,
     * SDADEL=0x2, SCLDEL=0x4 — value from ST's I2C configurator. */
    i2c->TIMINGR = 0x30B0E824;
    i2c->CR1 = I2C_CR1_PE;
    return MUON_OK;
}

static muon_status_t i2c_wait_flag(uint32_t mask, int set, uint32_t timeout_ms)
{
    uint32_t t0 = muon_millis();
    while (1) {
        uint32_t isr = ((i2c_regs_t *)I2C1_BASE)->ISR;
        int cond = (isr & mask) ? 1 : 0;
        if (cond == set) return MUON_OK;
        if ((muon_millis() - t0) > timeout_ms) return MUON_ERR_TIMEOUT;
    }
}

muon_status_t i2c_write(uint8_t addr, const uint8_t *data, uint16_t n)
{
    if (!data || n == 0) return MUON_ERR_INVALID_ARG;
    i2c_regs_t *i2c = (i2c_regs_t *)I2C1_BASE;
    /* Wait bus idle */
    MUON_RETURN_ON_ERR(i2c_wait_flag(I2C_ISR_BUSY, 0, I2C_TIMEOUT_MS));

    /* Configure transfer: 7-bit addr, NBYTES=n, write, auto-end */
    i2c->CR2 = ((uint32_t)(addr & 0x7F) << 1)
             | ((uint32_t)n << 16)
             | I2C_CR2_AUTOEND;
    /* START */
    i2c->CR2 |= I2C_CR2_START;

    for (uint16_t i = 0; i < n; ++i) {
        MUON_RETURN_ON_ERR(i2c_wait_flag(I2C_ISR_TXIS, 1, I2C_TIMEOUT_MS));
        i2c->TXDR = data[i];
    }
    /* Wait for auto-END (STOPF) */
    MUON_RETURN_ON_ERR(i2c_wait_flag(I2C_ISR_STOPF, 1, I2C_TIMEOUT_MS));
    i2c->ICR = I2C_ISR_STOPF;
    /* Check NACK */
    if (i2c->ISR & I2C_ISR_NACKF) {
        i2c->ICR = I2C_ISR_NACKF;
        return MUON_ERR_I2C;
    }
    return MUON_OK;
}

muon_status_t i2c_read(uint8_t addr, uint8_t *data, uint16_t n)
{
    if (!data || n == 0) return MUON_ERR_INVALID_ARG;
    i2c_regs_t *i2c = (i2c_regs_t *)I2C1_BASE;
    MUON_RETURN_ON_ERR(i2c_wait_flag(I2C_ISR_BUSY, 0, I2C_TIMEOUT_MS));
    i2c->CR2 = ((uint32_t)(addr & 0x7F) << 1) | I2C_CR2_AUTOEND
            | ((uint32_t)n << 16) | I2C_CR2_START | (1U << 10); /* READ = bit10 */
    for (uint16_t i = 0; i < n; ++i) {
        MUON_RETURN_ON_ERR(i2c_wait_flag(I2C_ISR_RXNE, 1, I2C_TIMEOUT_MS));
        data[i] = (uint8_t)i2c->RXDR;
    }
    MUON_RETURN_ON_ERR(i2c_wait_flag(I2C_ISR_STOPF, 1, I2C_TIMEOUT_MS));
    i2c->ICR = I2C_ISR_STOPF;
    return MUON_OK;
}

muon_status_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_write(addr, buf, 2);
}

muon_status_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *val, uint16_t n)
{
    if (!val) return MUON_ERR_INVALID_ARG;
    MUON_RETURN_ON_ERR(i2c_write(addr, &reg, 1));
    return i2c_read(addr, val, n);
}