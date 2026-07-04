/*
 * fuel.c — MAX17048 LiPo fuel gauge driver (I²C)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * The MAX17048 reports state-of-charge directly in percent and cell voltage
 * with 78.125 µV resolution; no characterization table is needed. GlyphFlow
 * reads it once per minute and on USB insert.
 */
#include "fuel.h"
#include "i2c.h"
#include "../registers.h"
#include "../board.h"

int fuel_init(void)
{
    uint8_t ver;
    if (i2c_read_reg(MAX17048_ADDR, MAX17048_VERSION, &ver) < 0) return -1;
    /* On a properly wired MAX17048, VERSION reads 0x12 (rev 1). */
    return (ver == 0x12 || ver == 0x13) ? 0 : -1;
}

uint8_t fuel_percent(void)
{
    uint8_t hi, lo;
    if (i2c_read_reg(MAX17048_ADDR, MAX17048_SOC, &hi) < 0) return 0;
    if (i2c_read_reg(MAX17048_ADDR, MAX17048_SOC + 1, &lo) < 0) return 0;
    /* SOC register is a 16-bit fixed-point with 1/256 % LSB in the high byte.
     * The high byte alone is the integer percent. */
    return hi;
}

uint16_t fuel_voltage_mv(void)
{
    uint8_t hi, lo;
    if (i2c_read_reg(MAX17048_ADDR, MAX17048_VCELL, &hi) < 0) return 0;
    if (i2c_read_reg(MAX17048_ADDR, MAX17048_VCELL + 1, &lo) < 0) return 0;
    uint16_t raw = ((uint16_t)hi << 8) | lo;
    /* raw * 78.125 µV → mV: raw * 0.078125 */
    return (uint16_t)((raw * 78U) / 1000U);
}

int fuel_quick_start(void)
{
    /* Writing 0x4000 to MODE triggers a quick-start of the fuel gauge model. */
    uint8_t cmd[2] = { 0x40, 0x00 };
    /* Use raw I²C write: send the MODE register address + 2 payload bytes. */
    uint8_t addr = MAX17048_ADDR << 1;
    I2C1->CR2 = (uint32_t)addr | (3U << 16) | I2C_CR2_START | I2C_CR2_AUTOEND;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) { if (I2C1->ISR & I2C_ISR_NACKF) return -1; }
    I2C1->TXDR = 0x06;  /* MODE register */
    while (!(I2C1->ISR & I2C_ISR_TXIS)) { }
    I2C1->TXDR = cmd[0];
    while (!(I2C1->ISR & I2C_ISR_TXIS)) { }
    I2C1->TXDR = cmd[1];
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}