/*
 * drivers/i2c.c — I2C3 driver for SHT45 (temp/humidity) and VEML7700 (ambient light)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#include "i2c.h"
#include "../registers.h"
#include "../board.h"
#include <stdint.h>

#define I2C3_TIMING_80MHZ_100K  0x30420F1FUL  /* RM0394 table for 80 MHz / 100 kHz */

void i2c3_init(void)
{
    /* Enable I2C3 clock */
    RCC_APB1ENR1 |= RCC_APB1ENR1_I2C3EN;

    /* Reset peripheral then enable */
    I2C3->CR1 = 0;
    I2C3->TIMINGR = I2C3_TIMING_80MHZ_100K;
    I2C3->CR1 = I2C_CR1_PE;

    /* Configure PA8 (SCL) and PA9 (SDA) as AF6, open-drain, pull-up */
    GPIOA->MODER &= ~((3U << (8*2)) | (3U << (9*2)));
    GPIOA->MODER |=  (GPIO_MODE_AF << (8*2)) | (GPIO_MODE_AF << (9*2));
    GPIOA->OTYPER |= (1U << 8) | (1U << 9);   /* open-drain */
    GPIOA->PUPDR |=  (1U << (8*2)) | (1U << (9*2)); /* pull-up */
    GPIOA->AFRL &= ~((0xFU << (8*4)) | (0xFU << (9*4)));
    GPIOA->AFRL |=  (7U << (8*4)) | (7U << (9*4));   /* AF7 = I2C3 */
}

/* Write N bytes to a given I2C device address (no register addr variant). */
int i2c3_write(uint8_t addr7, const uint8_t *data, uint8_t n)
{
    I2C3->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
    I2C3->CR2 = ((addr7 & 0x7F) << 1) | ((uint32_t)n << I2C_CR2_NBYTES_SHIFT) | I2C_CR2_AUTOEND;
    I2C3->CR2 |= I2C_CR2_START;

    for (uint8_t i = 0; i < n; i++) {
        uint32_t to = 100000;
        while (!(I2C3->ISR & I2C_ISR_TXE) && to--) { }
        if (!to) return -1;
        I2C3->TXDR = data[i];
    }
    uint32_t to = 100000;
    while ((I2C3->ISR & I2C_ISR_STOPF) == 0 && to--) { }
    return (I2C3->ISR & I2C_ISR_NACKF) ? -1 : 0;
}

/* Read N bytes from a given I2C device address. */
int i2c3_read(uint8_t addr7, uint8_t *buf, uint8_t n)
{
    I2C3->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
    I2C3->CR2 = ((addr7 & 0x7F) << 1) | ((uint32_t)n << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND | I2C_CR2_START;

    for (uint8_t i = 0; i < n; i++) {
        uint32_t to = 100000;
        while (!(I2C3->ISR & I2C_ISR_RXNE) && to--) { }
        if (!to) return -1;
        buf[i] = (uint8_t)I2C3->RXDR;
    }
    uint32_t to = 100000;
    while ((I2C3->ISR & I2C_ISR_STOPF) == 0 && to--) { }
    return (I2C3->ISR & I2C_ISR_NACKF) ? -1 : 0;
}

/* ---- SHT45 temperature/humidity sensor ---------------------------- */
/* SHT4x default I2C address = 0x44 */
#define SHT45_ADDR  0x44

int sht45_read(int16_t *temp_centi, uint16_t *rh_centi)
{
    uint8_t cmd = 0xFD;   /* measure: high repeatability, clock stretching */
    if (i2c3_write(SHT45_ADDR, &cmd, 1) < 0) return -1;

    /* Max measurement time ~8.5 ms for high repeatability */
    delay_ms(10);

    uint8_t raw[6];
    if (i2c3_read(SHT45_ADDR, raw, 6) < 0) return -1;

    /* CRC-8 check (poly 0x31, init 0xFF) on each pair */
    for (int g = 0; g < 2; g++) {
        uint8_t crc = 0xFF;
        for (int j = 0; j < 2; j++) {
            crc ^= raw[g*3 + j];
            for (int k = 0; k < 8; k++)
                crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
        }
        if (crc != raw[g*3 + 2]) return -2;
    }

    uint16_t t_ticks = (raw[0] << 8) | raw[1];
    uint16_t rh_ticks = (raw[3] << 8) | raw[4];

    /* Conversion per SHT4x datasheet */
    *temp_centi = (int16_t)(-4500 + 17500UL * t_ticks / 65535UL);
    uint32_t rh = -600UL + 12500UL * rh_ticks / 65535UL;
    if (rh > 10000) rh = 10000;
    *rh_centi = (uint16_t)rh;
    return 0;
}

/* ---- VEML7700 ambient light sensor --------------------------------- */
#define VEML7700_ADDR  0x10

int veml7700_init(void)
{
    uint8_t buf[3] = {0x00, 0x00, 0x18}; /* config reg, integration 100ms, gain 1/8 */
    return i2c3_write(VEML7700_ADDR, buf, 3);
}

int veml7700_read_lux(uint16_t *lux)
{
    uint8_t cmd = 0x04;  /* ALS result register address */
    if (i2c3_write(VEML7700_ADDR, &cmd, 1) < 0) return -1;
    uint8_t raw[2];
    if (i2c3_read(VEML7700_ADDR, raw, 2) < 0) return -1;
    uint16_t als = (raw[1] << 8) | raw[0];
    /* Approximate lux: scale by resolution factor (0.2304 for 100ms/g1/8) */
    *lux = (uint16_t)(als * 2304UL / 10000UL);
    return 0;
}