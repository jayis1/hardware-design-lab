/*
 * drivers/fuel_gauge.c — MAX17048 Li-ion fuel gauge driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "../board.h"
#include "../registers.h"
#include "fuel_gauge.h"

static int i2c1_read_u16_be(uint8_t addr, uint8_t reg, uint16_t *out)
{
    I2C1->CR2 = ((uint32_t)addr << 1) | (1u << 16) | I2C_CR2_START;
    uint32_t timeout = 100000u;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) { if (--timeout == 0) return -1; }
    I2C1->TXDR = reg;
    timeout = 100000u;
    while (!(I2C1->ISR & I2C_ISR_TC)) { if (--timeout == 0) return -1; }
    I2C1->CR2 = ((uint32_t)addr << 1) | (1u << 1) | (2u << 16) | I2C_CR2_START;
    timeout = 100000u;
    while (!(I2C1->ISR & I2C_ISR_RXNE)) { if (--timeout == 0) return -1; }
    uint8_t hi = I2C1->RXDR;
    timeout = 100000u;
    while (!(I2C1->ISR & I2C_ISR_RXNE)) { if (--timeout == 0) return -1; }
    uint8_t lo = I2C1->RXDR;
    timeout = 100000u;
    while (!(I2C1->ISR & I2C_ISR_TC)) { if (--timeout == 0) return -1; }
    I2C1->CR2 = I2C_CR2_STOP;
    *out = ((uint16_t)hi << 8) | lo;
    return 0;
}

void fuel_gauge_init(void)
{
    /* Quick-start command to VCELL register */
    uint16_t qs = 0x0040;
    I2C1->CR2 = ((uint32_t)FUEL_ADDR << 1) | (1u << 16) | I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) ;
    I2C1->TXDR = 0x26;   /* MODE register */
    while (!(I2C1->ISR & I2C_ISR_TXIS)) ;
    I2C1->TXDR = (uint8_t)(qs >> 8);
    while (!(I2C1->ISR & I2C_ISR_TXIS)) ;
    I2C1->TXDR = (uint8_t)(qs & 0xFF);
    while (!(I2C1->ISR & I2C_ISR_TC)) ;
    I2C1->CR2 = I2C_CR2_STOP;
}

uint8_t fuel_gauge_get_percent(void)
{
    uint16_t soc;
    if (i2c1_read_u16_be(FUEL_ADDR, 0x06, &soc) != 0) return 0;
    /* SOC is in 1/256 % units */
    return (uint8_t)(soc >> 8);
}

float fuel_gauge_get_voltage(void)
{
    uint16_t v;
    if (i2c1_read_u16_be(FUEL_ADDR, 0x02, &v) != 0) return 0.0f;
    /* VCELL: 12-bit, 78.125 µV/LSB → divide by 8000 for V (approx) */
    uint16_t raw = v >> 4;
    return (float)raw * 0.000078125f * 16.0f;
}