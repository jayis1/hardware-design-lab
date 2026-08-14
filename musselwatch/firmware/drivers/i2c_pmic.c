/*
 * drivers/i2c_pmic.c — I2C1 driver for BQ25870 charger + FM24C64 FRAM
 *
 * Implements blocking I2C1 read/write for the BQ25870 single-cell Li-ion
 * charger (0x6B) and the FM24C64 64 Kbit ferroelectric RAM (0x50) used
 * as a non-volatile event log.  I2C1 runs at 100 kHz on PB8 (SCL) / PB9
 * (SDA), AF4.
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "../board.h"
#include "../registers.h"
#include "i2c_pmic.h"

/* Timing register value for 100 kHz I2C, PCLK = 80 MHz, analog filter on */
#define I2C1_TIMING_100K 0x00101D2Du

void i2c1_init(void)
{
    RCC_APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
    RCC_AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;
    (void)RCC_APB1ENR1;

    /* PB8 (SCL) and PB9 (SDA) -> AF4, open-drain, pull-up */
    GPIO_MODER(GPIOB_BASE) &= ~((3u << (8u * 2u)) | (3u << (9u * 2u)));
    GPIO_MODER(GPIOB_BASE) |=  ((2u << (8u * 2u)) | (2u << (9u * 2u)));
    GPIO_OTYPER(GPIOB_BASE) |=  (1u << 8u) | (1u << 9u); /* open-drain */
    GPIO_PUPDR(GPIOB_BASE)  |=  (1u << (8u * 2u)) | (1u << (9u * 2u)); /* pull-up */
    GPIO_AFRL(GPIOB_BASE) &= ~(0xFu << (8u * 4u));
    GPIO_AFRL(GPIOB_BASE) &= ~(0xFu << (9u * 4u));
    /* AF4 in the low 32-bit register for pins 0-7, but for pin 8/9 it's AFRH */
    GPIO_AFRH(GPIOB_BASE) &= ~(0xFu << ((8u - 8u) * 4u));
    GPIO_AFRH(GPIOB_BASE) &= ~(0xFu << ((9u - 8u) * 4u));
    GPIO_AFRH(GPIOB_BASE) |=  (4u << ((8u - 8u) * 4u));
    GPIO_AFRH(GPIOB_BASE) |=  (4u << ((9u - 8u) * 4u));

    I2C1_TIMINGR = I2C1_TIMING_100K;
    I2C1_CR1 = I2C_CR1_PE;
}

static bool i2c_wait_flag(uint32_t mask, bool set, uint32_t timeout)
{
    while (timeout--) {
        bool active = (I2C1_ISR & mask) != 0u;
        if (active == set) return true;
    }
    return false;
}

bool i2c1_write(uint8_t addr, uint8_t reg, uint8_t val)
{
    I2C1_CR2 = ((uint32_t)(addr << 1)) | (2u << 16) | I2C_CR2_START;
    if (!i2c_wait_flag(I2C_ISR_TXIS, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
    I2C1_TXDR = reg;
    if (!i2c_wait_flag(I2C_ISR_TXIS, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
    I2C1_TXDR = val;
    if (!i2c_wait_flag(I2C_ISR_TC, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
    I2C1_CR2 |= I2C_CR2_STOP;
    return true;
}

bool i2c1_read(uint8_t addr, uint8_t reg, uint8_t *val)
{
    /* Write register address */
    I2C1_CR2 = ((uint32_t)(addr << 1)) | (1u << 16) | I2C_CR2_START;
    if (!i2c_wait_flag(I2C_ISR_TXIS, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
    I2C1_TXDR = reg;
    if (!i2c_wait_flag(I2C_ISR_TC, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }

    /* Repeated start, read 1 byte */
    I2C1_CR2 = ((uint32_t)(addr << 1)) | (1u << 16) | I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    if (!i2c_wait_flag(I2C_ISR_RXNE, true, 1000u)) return false;
    *val = (uint8_t)I2C1_RXDR;
    i2c_wait_flag(I2C_ISR_BUSY, false, 1000u);
    return true;
}

bool i2c1_read_burst(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    if (len == 0u) return true;

    I2C1_CR2 = ((uint32_t)(addr << 1)) | (1u << 16) | I2C_CR2_START;
    if (!i2c_wait_flag(I2C_ISR_TXIS, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
    I2C1_TXDR = reg;
    if (!i2c_wait_flag(I2C_ISR_TC, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }

    I2C1_CR2 = ((uint32_t)(addr << 1)) | ((uint32_t)len << 16) | I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    for (uint8_t i = 0; i < len; i++) {
        if (!i2c_wait_flag(I2C_ISR_RXNE, true, 2000u)) return false;
        buf[i] = (uint8_t)I2C1_RXDR;
    }
    i2c_wait_flag(I2C_ISR_BUSY, false, 1000u);
    return true;
}

/* ---- BQ25870 PMIC operations ------------------------------------ */

bool pmic_set_charge_current_ma(uint16_t current_ma)
{
    /* REG04: ICHG[6:0] = (current_ma - 64) / 64 mA, 64 mA steps, 64..8192 mA */
    if (current_ma < 64u) current_ma = 64u;
    uint8_t ichg = (uint8_t)(((current_ma - 64u) / 64u) & 0x7Fu);
    return i2c1_write(PMIC_I2C_ADDR, 0x04u, ichg);
}

bool pmic_get_charger_state(uint8_t *state)
{
    uint8_t reg08;
    if (!i2c1_read(PMIC_I2C_ADDR, 0x0Bu, &reg08)) return false;
    /* REG0B bits[4:3] = charge state: 0=no input, 1=charging, 2=charge done, 3=fault */
    *state = (reg08 >> 3) & 0x03u;
    return true;
}

bool pmic_enable_shipping_mode(void)
{
    /* REG09: disable buck, enable shipping (bit7=1) */
    return i2c1_write(PMIC_I2C_ADDR, 0x09u, 0x80u);
}

/* ---- FM24C64 FRAM operations ------------------------------------ */

/*
 * Write event data to FM24C64 (64 Kbit = 8192 bytes).
 * Address is byte offset (0..8191).  Page write of up to 8 bytes.
 */
bool fram_write_event(uint32_t addr, const uint8_t *data, uint8_t len)
{
    if (addr + len > 8192u) return false;
    /* Device addr 0x50, with 16-bit memory address (2 bytes) */
    I2C1_CR2 = ((uint32_t)(FRAM_I2C_ADDR << 1)) | ((uint32_t)(len + 2u) << 16) | I2C_CR2_START;
    if (!i2c_wait_flag(I2C_ISR_TXIS, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
    I2C1_TXDR = (uint8_t)(addr >> 8);
    if (!i2c_wait_flag(I2C_ISR_TXIS, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
    I2C1_TXDR = (uint8_t)(addr & 0xFFu);
    for (uint8_t i = 0; i < len; i++) {
        if (!i2c_wait_flag(I2C_ISR_TXIS, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
        I2C1_TXDR = data[i];
    }
    if (!i2c_wait_flag(I2C_ISR_TC, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
    I2C1_CR2 |= I2C_CR2_STOP;
    return true;
}

bool fram_read_event(uint32_t addr, uint8_t *buf, uint8_t len)
{
    if (addr + len > 8192u) return false;
    /* Set memory address with 2-byte write (no data) */
    I2C1_CR2 = ((uint32_t)(FRAM_I2C_ADDR << 1)) | (2u << 16) | I2C_CR2_START;
    if (!i2c_wait_flag(I2C_ISR_TXIS, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
    I2C1_TXDR = (uint8_t)(addr >> 8);
    if (!i2c_wait_flag(I2C_ISR_TXIS, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }
    I2C1_TXDR = (uint8_t)(addr & 0xFFu);
    if (!i2c_wait_flag(I2C_ISR_TC, true, 1000u)) { I2C1_CR2 |= I2C_CR2_STOP; return false; }

    /* Repeated start, read len bytes */
    I2C1_CR2 = ((uint32_t)(FRAM_I2C_ADDR << 1)) | ((uint32_t)len << 16) | I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    for (uint8_t i = 0; i < len; i++) {
        if (!i2c_wait_flag(I2C_ISR_RXNE, true, 2000u)) return false;
        buf[i] = (uint8_t)I2C1_RXDR;
    }
    i2c_wait_flag(I2C_ISR_BUSY, false, 1000u);
    return true;
}