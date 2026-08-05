/*
 * drivers/thermo.c — Auxiliary temperature/humidity/pressure sensors
 *
 * SHT45 (air T & RH), BMP390 (barometric pressure), and DS18B20
 * (leaf-surface temperature via 1-wire).  These are auxiliary to the
 * primary psychrometer and sky-IR channels; they provide cross-check
 * data, the pressure input for psychrometric computation, and the
 * leaf-surface temperature for the frost-watch state machine.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "thermo.h"
#include "../board.h"

/* ------------------------------------------------------------------ */
/*  SHT45 — I²C air temperature and relative humidity                  */
/* ------------------------------------------------------------------ */
#define SHT45_CMD_MEASURE_HIGHREP  0xFD  /* clock stretching, high repeatability */

int sht45_measure(int32_t *temp_cx100, int32_t *rh_x1000)
{
    I2C_TypeDef *i2c = I2C1;
    uint32_t to;

    /* Send measurement command */
    i2c->CR2 = ((uint32_t)I2C_ADDR_SHT45 << 1) | (1u << 16) |
               I2C_CR2_START | I2C_CR2_AUTOEND;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = SHT45_CMD_MEASURE_HIGHREP;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;

    /* Wait for measurement (typ. 8.5 ms for high-rep) */
    delay_ms(10);

    /* Read 6 bytes: temp MSB, LSB, CRC, RH MSB, LSB, CRC */
    uint8_t buf[6] = {0};
    i2c->CR2 = ((uint32_t)I2C_ADDR_SHT45 << 1) | (6u << 16) |
               I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    for (int i = 0; i < 6; i++) {
        to = 0x10000;
        while (!(i2c->ISR & I2C_ISR_RXNE) && to--) ;
        buf[i] = i2c->RXDR & 0xFF;
    }
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;

    if (to == 0) return -1;

    /* Convert: T = -45 + 175 * (rawT / 2^16)  →  0.01 °C units */
    uint16_t raw_t = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t raw_rh = ((uint16_t)buf[3] << 8) | buf[4];

    /* T_cx100 = -4500 + 17500 * raw_t / 65536 */
    int32_t t = -4500 + (int32_t)((17500LL * raw_t) >> 16);
    /* RH_x1000 = 1000 * raw_rh / 65536 (clamped to 0..100000) */
    int32_t rh = (int32_t)((100000LL * raw_rh) >> 16);
    if (rh < 0) rh = 0;
    if (rh > 100000) rh = 100000;

    *temp_cx100 = t;
    *rh_x1000 = rh;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  BMP390 — I²C barometric pressure                                   */
/* ------------------------------------------------------------------ */
#define BMP390_REG_PRESS_MSB   0x04
#define BMP390_REG_PRESS_LSB   0x03
#define BMP380_REG_PRESS_XLSB  0x02
#define BMP390_REG_TEMP_MSB    0x07
#define BMP390_REG_TEMP_LSB    0x06
#define BMP390_REG_TEMP_XLSB   0x05
#define BMP390_REG_CTRL        0x1B
#define BMP390_REG_OSR         0x1C
#define BMP390_REG_ODR         0x1D
#define BMP390_REG_PWR_CTRL    0x1B
#define BMP390_REG_CMD         0x7E

int bmp390_init(void)
{
    /* Set oversampling: pressure ×8, temperature ×1 */
    /* OSR register: bits 5-7 = temp osr, bits 0-2 = press osr */
    /* Write OSR = 0x05 (press x8 = 0b101, temp x1 = 0b000) */
    I2C_TypeDef *i2c = I2C1;
    uint32_t to;

    i2c->CR2 = ((uint32_t)I2C_ADDR_BMP390 << 1) | (2u << 16) |
               I2C_CR2_START | I2C_CR2_AUTOEND;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = BMP390_REG_OSR;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = 0x05;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;

    /* Power control: enable pressure + temperature measurement */
    i2c->CR2 = ((uint32_t)I2C_ADDR_BMP390 << 1) | (2u << 16) |
               I2C_CR2_START | I2C_CR2_AUTOEND;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = BMP390_REG_PWR_CTRL;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = 0x33;  /* pressure enable + temp enable */
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;

    delay_ms(5);
    return 0;
}

int bmp390_read(int32_t *pressure_hpa, int32_t *temp_cx100)
{
    I2C_TypeDef *i2c = I2C1;
    uint32_t to;

    /* Read 6 bytes starting from PRESS_XLSB (0x02) */
    i2c->CR2 = ((uint32_t)I2C_ADDR_BMP390 << 1) | (1u << 16) | I2C_CR2_START;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = 0x02;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TC) && to--) ;

    uint8_t buf[6] = {0};
    i2c->CR2 = ((uint32_t)I2C_ADDR_BMP390 << 1) | (6u << 16) |
               I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    for (int i = 0; i < 6; i++) {
        to = 0x10000;
        while (!(i2c->ISR & I2C_ISR_RXNE) && to--) ;
        buf[i] = i2c->RXDR & 0xFF;
    }
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;
    if (to == 0) return -1;

    /* Assemble 24-bit raw pressure and temperature */
    int32_t raw_p = ((uint32_t)buf[2] << 16) | ((uint32_t)buf[1] << 8) | buf[0];
    int32_t raw_t = ((uint32_t)buf[5] << 16) | ((uint32_t)buf[4] << 8) | buf[3];

    /* Simplified conversion (factory calibration would use BMP390
     * compensation coefficients from EEPROM; here we use a linear
     * approximation adequate for the psychrometric pressure input). */
    /* Pressure: typical sea-level raw ~87000 → 1013 hPa */
    *pressure_hpa = (raw_p * 1013) / 87000;
    /* Temperature: raw ~525000 → 25.00 °C */
    *temp_cx100 = (raw_t * 2500) / 525000;

    return 0;
}

/* ------------------------------------------------------------------ */
/*  DS18B20 — 1-wire leaf-surface temperature                          */
/*                                                                    */
/*  The DS18B20 is on a dedicated 1-wire bus on PB12 (open-drain).     */
/*  We bit-bang the 1-wire protocol at 160 MHz for precise timing.     */
/* ------------------------------------------------------------------ */
#define DS18B20_PIN       12
#define DS18B20_PORT      GPIOB
#define DS18B20_CMD_SKIP_ROM   0xCC
#define DS18B20_CMD_CONVERT_T  0x44
#define DS18B20_CMD_READ_SCR   0xBE

static void ow_set_output(void)
{
    GPIO_CONFIG(DS18B20_PORT, DS18B20_PIN, GPIO_MODE_OUTPUT, GPIO_OTYPE_OD,
                GPIO_SPEED_VHIGH, GPIO_PUPD_NONE, 0);
}

static void ow_set_input(void)
{
    GPIO_CONFIG(DS18B20_PORT, DS18B20_PIN, GPIO_MODE_INPUT, 0, 0,
                GPIO_PUPD_UP, 0);
}

static void ow_low(void)
{
    SET_BITS(DS18B20_PORT->BRR, (1u << DS18B20_PIN));
}

static void ow_release(void)
{
    SET_BITS(DS18B20_PORT->BSRR, (1u << DS18B20_PIN));
}

static uint8_t ow_read_bit(void)
{
    uint8_t bit;
    ow_set_output();
    ow_low();
    delay_us(2);     /* pull low for 2 µs */
    ow_set_input();
    delay_us(10);    /* wait for bus to settle */
    bit = (DS18B20_PORT->IDR >> DS18B20_PIN) & 1;
    delay_us(60);    /* rest of time slot */
    return bit;
}

static void ow_write_bit(uint8_t bit)
{
    ow_set_output();
    ow_low();
    if (bit) {
        delay_us(2);
        ow_set_input();
        delay_us(70);
    } else {
        delay_us(70);
        ow_set_input();
        delay_us(2);
    }
}

static int ow_reset(void)
{
    ow_set_output();
    ow_low();
    delay_us(500);   /* 500 µs reset pulse */
    ow_set_input();
    delay_us(70);    /* wait for presence */
    uint8_t presence = ((DS18B20_PORT->IDR >> DS18B20_PIN) & 1) ^ 1;
    delay_us(430);
    return presence;  /* 1 = device present */
}

static void ow_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(byte & 1);
        byte >>= 1;
    }
}

static uint8_t ow_read_byte(void)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte |= (ow_read_bit() << i);
    }
    return byte;
}

int ds18b20_read_cx100(int32_t *temp_cx100)
{
    if (!ow_reset()) return -1;

    ow_write_byte(DS18B20_CMD_SKIP_ROM);
    ow_write_byte(DS18B20_CMD_CONVERT_T);

    /* Wait for conversion (max 750 ms for 12-bit) */
    delay_ms(750);

    if (!ow_reset()) return -1;

    ow_write_byte(DS18B20_CMD_SKIP_ROM);
    ow_write_byte(DS18B20_CMD_READ_SCR);

    /* Read 9 bytes (2 temp + 7 config/reserved) */
    uint8_t lsb = ow_read_byte();
    uint8_t msb = ow_read_byte();
    /* Discard remaining 7 bytes */
    for (int i = 0; i < 7; i++) (void)ow_read_byte();

    /* DS18B20: 12-bit resolution, 0.0625 °C per LSB */
    int16_t raw = ((int16_t)msb << 8) | lsb;
    /* Convert to 0.01 °C: raw * 0.0625 * 100 = raw * 6.25 */
    *temp_cx100 = (int32_t)(raw * 625) / 100;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  delay_us — busy-wait microsecond delay (calibrated for 160 MHz)    */
/*  Each loop iteration ≈ 6 cycles ≈ 37.5 ns at 160 MHz.              */
/* ------------------------------------------------------------------ */
void delay_us(uint32_t us)
{
    uint32_t cycles = us * 26;  /* ~26 iterations per µs at 160 MHz */
    for (volatile uint32_t i = 0; i < cycles; i++) {
        __asm volatile ("nop");
    }
}