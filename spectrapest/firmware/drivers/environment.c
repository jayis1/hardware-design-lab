/*
 * drivers/environment.c — Environmental sensor driver for SpectraPest
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Reads temperature/humidity (SHT45), barometric pressure (LPS22HB),
 * ambient light (VEML7700), and CO2 (SCD41) via I2C1.
 *
 * These environmental readings serve two purposes:
 *   1. Determine if conditions are suitable for insect activity (temp > 10°C)
 *   2. Provide context for detection events (pest activity correlates with
 *      temperature, humidity, and CO2 levels)
 *
 * I2C1 is configured at 400 kHz (fast mode). All sensors use 7-bit
 * addressing. The I2C driver implements blocking reads/writes with
 * timeout handling — no DMA is used since transactions are short
 * (1-9 bytes per sensor read).
 */

#include "environment.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

/* ----------------------------------------------------------------- *
 *  I2C1 initialization
 * ----------------------------------------------------------------- */
int env_init(void) {
    /* Enable I2C1 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    /* Configure I2C1 pins: SDA=PB8, SCL=PB9 */
    gpio_set_mode(GPIOB, 8, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 8, AF_I2C1_SDA);
    gpio_set_otype(GPIOB, 8, GPIO_OTYPE_OD);
    gpio_set_pupd(GPIOB, 8, GPIO_PUPD_UP);
    gpio_set_ospeed(GPIOB, 8, GPIO_SPEED_HIGH);

    gpio_set_mode(GPIOB, 9, GPIO_MODE_AF);
    gpio_set_af(GPIOB, 9, AF_I2C1_SCL);
    gpio_set_otype(GPIOB, 9, GPIO_OTYPE_OD);
    gpio_set_pupd(GPIOB, 9, GPIO_PUPD_UP);
    gpio_set_ospeed(GPIOB, 9, GPIO_SPEED_HIGH);

    /* Configure I2C1 timing for 400 kHz at 120 MHz PCLK
     * TIMINGR = PRESC(4) | SCLL(8) | SCLH(8) | SDADEL(4) | SCLDEL(4)
     * For 400 kHz: PRESC=0, SCLL=30, SCLH=30, SDADEL=0, SCLDEL=0x4 */
    I2C1->TIMINGR = (0x0 << 28) | (30 << 16) | (30 << 8) | (0 << 4) | 0x4;
    I2C1->CR1 = I2C_CR1_PE;  /* Enable peripheral */

    board_delay_ms(10);

    /* Initialize SCD41 CO2 sensor: stop periodic measurement first,
     * then start periodic measurement (1 reading per 5 seconds) */
    uint8_t cmd[2] = { 0x01, 0x04 };  /* Stop_periodic_measurement */
    i2c_write(I2C_ADDR_SCD41, cmd, 2);
    board_delay_ms(500);

    cmd[0] = 0x21;  /* start_periodic_measurement */
    cmd[1] = 0xB1;
    i2c_write(I2C_ADDR_SCD41, cmd, 2);

    return 0;
}

/* ----------------------------------------------------------------- *
 *  I2C write (blocking, with timeout)
 * ----------------------------------------------------------------- */
int i2c_write(uint8_t addr, uint8_t *data, uint8_t len) {
    uint32_t timeout = board_get_tick_ms() + 100;

    /* Configure transfer: N bytes, write, auto-end */
    I2C1->CR2 = ((uint32_t)addr << 1) | ((uint32_t)len << I2C_CR2_NBYTES_SHIFT) |
                I2C_CR2_AUTOEND;

    /* Generate START */
    I2C1->CR2 |= I2C_CR2_START;

    /* Write each byte */
    for (uint8_t i = 0; i < len; i++) {
        while (!(I2C1->ISR & I2C_ISR_TXIS)) {
            if (board_get_tick_ms() > timeout) return -1;
            if (I2C1->ISR & I2C_ISR_NACKF) return -1;
        }
        I2C1->TXDR = data[i];
    }

    /* Wait for STOP (auto-end) */
    while (I2C1->ISR & I2C_ISR_BUSY) {
        if (board_get_tick_ms() > timeout) return -1;
    }

    return 0;
}

/* ----------------------------------------------------------------- *
 *  I2C read (blocking, with timeout)
 * ----------------------------------------------------------------- */
int i2c_read(uint8_t addr, uint8_t *data, uint8_t len) {
    uint32_t timeout = board_get_tick_ms() + 100;

    /* Configure transfer: N bytes, read, auto-end */
    I2C1->CR2 = ((uint32_t)addr << 1) | I2C_CR2_RD_WRN |
                ((uint32_t)len << I2C_CR2_NBYTES_SHIFT) | I2C_CR2_AUTOEND;

    /* Generate START */
    I2C1->CR2 |= I2C_CR2_START;

    /* Read each byte */
    for (uint8_t i = 0; i < len; i++) {
        while (!(I2C1->ISR & I2C_ISR_RXNE)) {
            if (board_get_tick_ms() > timeout) return -1;
            if (I2C1->ISR & I2C_ISR_NACKF) return -1;
        }
        data[i] = I2C1->RXDR;
    }

    /* Wait for STOP */
    while (I2C1->ISR & I2C_ISR_BUSY) {
        if (board_get_tick_ms() > timeout) return -1;
    }

    return 0;
}

int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    return i2c_write(addr, buf, 2);
}

int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len) {
    /* Write register address */
    if (i2c_write(addr, &reg, 1) < 0) return -1;
    /* Read data */
    return i2c_read(addr, buf, len);
}

/* ----------------------------------------------------------------- *
 *  Read SHT45 temperature/humidity (single-shot, no clock stretching)
 *  Command 0xFD = measure T/RH with clock stretching, returns 6 bytes:
 *    [T_MSB, T_CRC, T_LSB, RH_MSB, RH_CRC, RH_LSB]
 *  T (°C) = -45 + 175 * (raw / 65535)
 *  RH (%) = 0 + 100 * (raw / 65535)
 * ----------------------------------------------------------------- */
int env_read_temp_humidity(float *temp_c, float *humidity_pct) {
    uint8_t cmd = 0xFD;  /* Single-shot, clock stretching */
    if (i2c_write(I2C_ADDR_SHT45, &cmd, 1) < 0) return -1;

    board_delay_ms(10);  /* Measurement takes ~8.5 ms */

    uint8_t data[6];
    if (i2c_read(I2C_ADDR_SHT45, data, 6) < 0) return -1;

    uint16_t raw_t = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_rh = ((uint16_t)data[3] << 8) | data[4];

    *temp_c = -45.0f + 175.0f * (float)raw_t / 65535.0f;
    *humidity_pct = 100.0f * (float)raw_rh / 65535.0f;

    return 0;
}

/* ----------------------------------------------------------------- *
 *  Read LPS22HB pressure (one-shot mode)
 *  Register 0x10 = CTRL_REG1 (set ODR=75Hz, block data update)
 *  Read 0x28-0x2A for pressure (3 bytes, LSB first)
 *  P (hPa) = raw / 4096
 * ----------------------------------------------------------------- */
int env_read_pressure(float *pressure_hpa) {
    /* Set ODR to one-shot and trigger */
    i2c_write_reg(I2C_ADDR_LPS22HB, 0x10, 0x02);  /* ODR = one-shot */
    i2c_write_reg(I2C_ADDR_LPS22HB, 0x21, 0x01);  /* One-shot trigger */

    board_delay_ms(10);

    uint8_t data[3];
    if (i2c_read_reg(I2C_ADDR_LPS22HB, 0x28, data, 3) < 0) return -1;

    int32_t raw = ((int32_t)data[2] << 16) | ((int32_t)data[1] << 8) | data[0];
    if (raw & 0x800000) raw |= 0xFF000000;  /* Sign extend */
    *pressure_hpa = (float)raw / 4096.0f;

    return 0;
}

/* ----------------------------------------------------------------- *
 *  Read VEML7700 ambient light
 *  Register 0x04 = ALS (16-bit, raw lux count)
 *  Lux = raw * 0.25 * gain_resolution * integration_time_factor
 *  For default config (gain=1, 100ms): lux = raw * 0.0036
 * ----------------------------------------------------------------- */
int env_read_light(uint16_t *lux) {
    uint8_t data[2];
    if (i2c_read_reg(I2C_ADDR_VEML7700, 0x04, data, 2) < 0) return -1;

    uint16_t raw = ((uint16_t)data[1] << 8) | data[0];
    /* Default config: gain=1/8, 100ms → resolution = 0.25 lux/lsb × gain factor */
    /* For simplicity, use approximate conversion */
    *lux = (uint16_t)((float)raw * 0.36f);

    return 0;
}

/* ----------------------------------------------------------------- *
 *  Read SCD41 CO2 concentration
 *  Command 0x02 0x0E = read_measurement, returns 9 bytes:
 *    [CO2_MSB, CO2_CRC, CO2_LSB, T_MSB, T_CRC, T_LSB, RH_MSB, RH_CRC, RH_LSB]
 *  CO2 (ppm) = raw (16-bit)
 *  Wait at least 5 seconds after start_periodic_measurement before reading
 * ----------------------------------------------------------------- */
int env_read_co2(uint16_t *co2_ppm) {
    uint8_t cmd[2] = { 0x02, 0x0E };  /* Read measurement */
    if (i2c_write(I2C_ADDR_SCD41, cmd, 2) < 0) return -1;

    board_delay_ms(5);

    uint8_t data[9];
    if (i2c_read(I2C_ADDR_SCD41, data, 9) < 0) return -1;

    *co2_ppm = ((uint16_t)data[0] << 8) | data[1];
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Read all environmental sensors
 * ----------------------------------------------------------------- */
int env_read_all(env_data_t *out) {
    if (!out) return -1;

    int result = 0;
    result |= env_read_temp_humidity(&out->temperature_c, &out->humidity_pct);
    result |= env_read_pressure(&out->pressure_hpa);
    result |= env_read_light(&out->light_lux);
    result |= env_read_co2(&out->co2_ppm);

    return result;
}