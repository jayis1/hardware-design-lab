/*
 * drivers/thermal.c — TMP117 16-bit temperature sensor
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * TMP117 has a fixed I2C address 0x48 and a single 16-bit temperature
 * register at 0x00, with 7.8125 m°C/LSB resolution and a ±0.1 °C
 * accuracy. We poll it via the minimal I2C1 register driver below.
 */
#include "thermal.h"
#include "../registers.h"

static void i2c1_init(void)
{
    RCC_REG32(RCC_APB1LENR_OF) |= RCC_APB1LENR_I2C1EN;
    (void)RCC_REG32(RCC_APB1LENR_OF);
    /* Configure PB8/PB9 as AF4 (I2C1), pull-up, open-drain. */
    hgpio_t pins[2] = { PIN_I2C1_SCL, PIN_I2C1_SDA };
    for (int i = 0; i < 2; ++i) {
        pins[i].port->MODER  &= ~(3u << (2u * pins[i].pin));
        pins[i].port->MODER  |= (GPIO_MODE_AF << (2u * pins[i].pin));
        pins[i].port->OTYPER |= (1u << pins[i].pin);   /* open-drain      */
        pins[i].port->PUPDR |= (GPIO_PUPD_PU << (2u * pins[i].pin));
        if (pins[i].pin < 8)
            pins[i].port->AFRL |= (4u << (4u * pins[i].pin));
        else
            pins[i].port->AFRH |= (4u << (4u * (pins[i].pin - 8)));
    }
    /* 100 kHz I2C from 120 MHz PCLK; TIMINGR value from RM0433. */
    I2C1.TIMINGR = 0x10909CECu;
    I2C1.CR1 = I2C_CR1_PE;
}

static hydra_err_t i2c1_read_reg(uint8_t addr7, uint8_t reg, uint8_t *buf, uint8_t n)
{
    uint32_t t0 = board_millis();
    while (I2C1.ISR & I2C_ISR_BUSY) {
        if ((board_millis() - t0) > 50) return HYDRA_ERR_TIMEOUT;
    }
    /* Write the register pointer */
    I2C1.CR2 = ((uint32_t)addr7 << 1) | I2C_CR2_NBYTES(1) | I2C_CR2_AUTOEND;
    I2C1.CR2 |= I2C_CR2_START;
    while (!(I2C1.ISR & I2C_ISR_TXE)) { }
    I2C1.TXDR = reg;
    t0 = board_millis();
    while (!(I2C1.ISR & I2C_ISR_TC)) {
        if (I2C1.ISR & I2C_ISR_NACKF) { I2C1.ICR = 0x3FFu; return HYDRA_ERR_IO; }
        if ((board_millis() - t0) > 50) return HYDRA_ERR_TIMEOUT;
    }
    /* Read back n bytes */
    I2C1.CR2 = ((uint32_t)addr7 << 1) | I2C_CR2_NBYTES(n) | I2C_CR2_AUTOEND
             | I2C_CR2_RD_WRN;
    I2C1.CR2 |= I2C_CR2_START;
    for (uint8_t i = 0; i < n; ++i) {
        t0 = board_millis();
        while (!(I2C1.ISR & I2C_ISR_RXNE)) {
            if ((board_millis() - t0) > 50) return HYDRA_ERR_TIMEOUT;
        }
        buf[i] = (uint8_t)I2C1.RXDR;
    }
    I2C1.ICR = 0x3FFu;
    return HYDRA_OK;
}

hydra_err_t thermal_init(void)
{
    i2c1_init();
    /* TMP117 powers up in continuous mode by default; nothing more to do.
     * We could set config register 0x01 to 1 Hz conversion, but the
     * default 1 Hz is fine for our sweep timing. */
    return HYDRA_OK;
}

hydra_err_t thermal_read(float *out_celsius)
{
    if (!out_celsius) return HYDRA_ERR_IO;
    uint8_t buf[2] = {0};
    hydra_err_t e = i2c1_read_reg(TMP117_I2C_ADDR, 0x00, buf, 2);
    if (e != HYDRA_OK) return e;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *out_celsius = (float)raw * 0.0078125f;   /* 7.8125 m°C/LSB         */
    return HYDRA_OK;
}