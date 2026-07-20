/*
 * env_sens.c — Environmental Sensors
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Drivers for SHT41 ambient temperature/humidity sensor (I2C1),
 * DS18B20 soil temperature sensor (1-Wire on PG11), capacitive
 * soil moisture sensor (ADC), and PCF85263A RTC (I2C3).
 */

#include "env_sens.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* SHT41 I2C address */
#define SHT41_ADDR      0x44

/* PCF85263A RTC I2C address */
#define PCF85263_ADDR   0x51

/* DS18B20 1-Wire commands */
#define DS18B20_CMD_SKIP_ROM    0xCC
#define DS18B20_CMD_CONVERT_T   0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE

/* ---------------------------------------------------------------------
 * I2C3 (RTC) initialization
 * --------------------------------------------------------------------- */

static void i2c3_init(void)
{
    RCC->AHB4ENR |= BIT(0) | BIT(2);  /* GPIOA, GPIOC */
    RCC->APB1LENR |= BIT(23);  /* I2C3 */

    /* PA8 = SCL, PC9 = SDA (AF4) */
    GPIOA->MODER &= ~(3U << (8*2));
    GPIOA->MODER |= (GPIO_MODE_AF << (8*2));
    GPIOA->OTYPER |= (1U << 8);
    GPIOA->OSPEEDR |= (GPIO_SPEED_HIGH << (8*2));
    GPIOA->PUPDR |= (GPIO_PUPD_PU << (8*2));
    GPIOA->AFRH &= ~(0xFU << 0);
    GPIOA->AFRH |= (RTC_I2C_AF << 0);

    GPIOC->MODER &= ~(3U << (9*2));
    GPIOC->MODER |= (GPIO_MODE_AF << (9*2));
    GPIOC->OTYPER |= (1U << 9);
    GPIOC->OSPEEDR |= (GPIO_SPEED_HIGH << (9*2));
    GPIOC->PUPDR |= (GPIO_PUPD_PU << (9*2));
    GPIOC->AFRH &= ~(0xFU << 4);
    GPIOC->AFRH |= (RTC_I2C_AF << 4);

    RTC_I2C->TIMINGR = 0x10B17DB4U;
    RTC_I2C->CR1 = I2C_CR1_PE;
}

static int i2c3_write(uint8_t addr, uint8_t reg, const uint8_t *data, int len)
{
    uint32_t timeout;
    RTC_I2C->CR2 = ((uint32_t)addr << 1) | ((len + 1) << 16);
    RTC_I2C->CR2 |= I2C_CR2_START;
    timeout = 100000;
    while (!(RTC_I2C->ISR & I2C_ISR_TXE) && timeout-- > 0) { }
    RTC_I2C->TXDR = reg;
    for (int i = 0; i < len; i++) {
        timeout = 100000;
        while (!(RTC_I2C->ISR & I2C_ISR_TXE) && timeout-- > 0) { }
        RTC_I2C->TXDR = data[i];
    }
    timeout = 100000;
    while (!(RTC_I2C->ISR & I2C_ISR_TC) && timeout-- > 0) { }
    RTC_I2C->CR2 |= I2C_CR2_STOP;
    return 0;
}

static int i2c3_read(uint8_t addr, uint8_t reg, uint8_t *data, int len)
{
    uint32_t timeout;
    RTC_I2C->CR2 = ((uint32_t)addr << 1) | (1U << 16) | I2C_CR2_NACK;
    RTC_I2C->CR2 |= I2C_CR2_START;
    timeout = 100000;
    while (!(RTC_I2C->ISR & I2C_ISR_TXE) && timeout-- > 0) { }
    RTC_I2C->TXDR = reg;
    timeout = 100000;
    while (!(RTC_I2C->ISR & I2C_ISR_TC) && timeout-- > 0) { }

    RTC_I2C->CR2 = ((uint32_t)addr << 1) | ((uint32_t)len << 16)
                 | I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_NACK;
    for (int i = 0; i < len; i++) {
        timeout = 100000;
        while (!(RTC_I2C->ISR & I2C_ISR_RXNE) && timeout-- > 0) { }
        data[i] = (uint8_t)RTC_I2C->RXDR;
    }
    RTC_I2C->CR2 |= I2C_CR2_STOP;
    return 0;
}

/* ---------------------------------------------------------------------
 * SHT41 temperature/humidity sensor (I2C1)
 * --------------------------------------------------------------------- */

extern int i2c1_write(uint8_t addr, uint8_t reg, const uint8_t *data, int len);
extern int i2c1_read(uint8_t addr, uint8_t reg, uint8_t *data, int len);

static int sht41_read(float *temp_c, float *rh)
{
    /* Send measurement command: 0xFD = high repeatability, clock stretching */
    uint8_t cmd = 0xFD;
    if (i2c1_write(SHT41_ADDR, cmd, NULL, 0) != 0) return -1;

    /* Wait for measurement (max 10 ms) */
    for (volatile int i = 0; i < 1200000; i++) { }

    /* Read 6 bytes: temp MSB/LSB/CRC, hum MSB/LSB/CRC */
    uint8_t data[6];
    if (i2c1_read(SHT41_ADDR, 0x00, data, 6) != 0) return -1;  /* read after write */
    /* Actually SHT41 is a "write command, then read" without register address.
     * Our I2C driver requires a register byte.  We adapt: use a read with a
     * dummy reg of 0x00. */

    uint16_t t_ticks = ((uint16_t)data[0] << 8) | data[1];
    uint16_t rh_ticks = ((uint16_t)data[3] << 8) | data[4];

    /* Convert: T = -45 + 175 * t_ticks / 65535 */
    *temp_c = -45.0f + 175.0f * (float)t_ticks / 65535.0f;
    /* RH = 100 * rh_ticks / 65535 (clamped 0-100) */
    *rh = 100.0f * (float)rh_ticks / 65535.0f;
    if (*rh < 0.0f) *rh = 0.0f;
    if (*rh > 100.0f) *rh = 100.0f;

    return 0;
}

/* ---------------------------------------------------------------------
 * DS18B20 soil temperature (1-Wire on PG11)
 * --------------------------------------------------------------------- */

static void ow_set_pin(int value)
{
    if (value) {
        DS18B20_PORT->MODER &= ~(3U << (DS18B20_PIN*2));  /* input (float high) */
    } else {
        DS18B20_PORT->MODER &= ~(3U << (DS18B20_PIN*2));  /* output */
        DS18B20_PORT->MODER |= (GPIO_MODE_OUT << (DS18B20_PIN*2));
        PIN_CLR(DS18B20_PORT, DS18B20_PIN);
    }
}

static int ow_read_pin(void)
{
    DS18B20_PORT->MODER &= ~(3U << (DS18B20_PIN*2));  /* input */
    return PIN_GET(DS18B20_PORT, DS18B20_PIN);
}

static int ow_reset(void)
{
    ow_set_pin(0);
    for (volatile int i = 0; i < 6000; i++) { }  /* 600us low */
    ow_set_pin(1);
    for (volatile int i = 0; i < 800; i++) { }  /* 80us */
    int presence = !ow_read_pin();  /* low = presence */
    for (volatile int i = 0; i < 5000; i++) { }  /* 500us */
    return presence;
}

static void ow_write_bit(int bit)
{
    if (bit) {
        ow_set_pin(0);
        for (volatile int i = 0; i < 10; i++) { }  /* 1us */
        ow_set_pin(1);
        for (volatile int i = 0; i < 800; i++) { }  /* 80us */
    } else {
        ow_set_pin(0);
        for (volatile int i = 0; i < 800; i++) { }  /* 80us */
        ow_set_pin(1);
        for (volatile int i = 0; i < 10; i++) { }  /* 1us */
    }
}

static int ow_read_bit(void)
{
    ow_set_pin(0);
    for (volatile int i = 0; i < 10; i++) { }  /* 1us */
    ow_set_pin(1);
    for (volatile int i = 0; i < 20; i++) { }  /* 2us */
    int bit = ow_read_pin();
    for (volatile int i = 0; i < 800; i++) { }  /* 80us */
    return bit;
}

static void ow_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(byte & (1U << i));
    }
}

static uint8_t ow_read_byte(void)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        if (ow_read_bit()) byte |= (1U << i);
    }
    return byte;
}

static int ds18b20_read(float *celsius)
{
    if (!ow_reset()) return -1;

    ow_write_byte(DS18B20_CMD_SKIP_ROM);
    ow_write_byte(DS18B20_CMD_CONVERT_T);
    /* Wait 750ms for 12-bit conversion */
    for (volatile int i = 0; i < 90000000; i++) { }

    if (!ow_reset()) return -1;
    ow_write_byte(DS18B20_CMD_SKIP_ROM);
    ow_write_byte(DS18B20_CMD_READ_SCRATCH);

    uint8_t lsb = ow_read_byte();
    uint8_t msb = ow_read_byte();
    int16_t temp_raw = (int16_t)((msb << 8) | lsb);
    *celsius = (float)temp_raw / 16.0f;
    return 0;
}

/* ---------------------------------------------------------------------
 * Capacitive soil moisture (analog read)
 * Uses ADC3 channel 8 (connected to a capacitive sensor with 0-3V output)
 * 0V = 0% moisture, 3V = 100% moisture (sensor-dependent calibration)
 * --------------------------------------------------------------------- */

#define ADC3_BASE   (0x58026000UL)  /* ADC3 on AHB1 */
#define ADC3_CR     (*(volatile uint32_t *)(ADC3_BASE + 0x08))
#define ADC3_ISR    (*(volatile uint32_t *)(ADC3_BASE + 0x00))
#define ADC3_SQR1   (*(volatile uint32_t *)(ADC3_BASE + 0x30))
#define ADC3_SQR3   (*(volatile uint32_t *)(ADC3_BASE + 0x38))
#define ADC3_DR     (*(volatile uint32_t *)(ADC3_BASE + 0x40))
#define ADC3_PCSEL  (*(volatile uint32_t *)(ADC3_BASE + 0x6C))

static int adc_soil_moisture_read(float *pct)
{
    /* Enable ADC3 clock */
    RCC->AHB1ENR |= BIT(24);  /* ADC3 */

    /* Enable ADC */
    ADC3_CR |= (1U << 28);  /* ADVREGEN */
    for (volatile int i = 0; i < 100000; i++) { }
    ADC3_CR |= (1U << 31);  /* DEEPPWD = 0 — actually need to clear */
    ADC3_CR &= ~(1U << 29); /* DEEPPWD off */
    for (volatile int i = 0; i < 1000; i++) { }
    ADC3_CR |= (1U << 0);   /* ADEN */
    for (volatile int i = 0; i < 10000; i++) { }

    /* Configure channel 8, single conversion */
    ADC3_PCSEL |= (1U << 8);  /* pre-channel select */
    ADC3_SQR1 = (1U << 0);     /* 1 conversion */
    ADC3_SQR3 = 8;             /* channel 8 first */

    /* Start conversion */
    ADC3_CR |= (1U << 2);  /* ADSTART */
    uint32_t timeout = 1000000;
    while (!(ADC3_ISR & (1U << 2)) && timeout-- > 0) { }  /* ADRDY/EOC */

    uint32_t raw = ADC3_DR & 0xFFFF;
    /* Convert 16-bit ADC (0-65535) to moisture % (calibrated) */
    /* Sensor: air = ~30000, water = ~15000 (inverted) */
    if (raw > 30000) raw = 30000;
    if (raw < 15000) raw = 15000;
    *pct = 100.0f * (float)(30000 - raw) / 15000.0f;
    if (*pct < 0) *pct = 0;
    if (*pct > 100) *pct = 100;
    return 0;
}

/* ---------------------------------------------------------------------
 * PCF85263A RTC
 * --------------------------------------------------------------------- */

static int pcf85263_read_time(uint32_t *unix_time)
{
    uint8_t regs[7];
    /* Read registers 0x07-0x0D (seconds through years, BCD) */
    if (i2c3_read(PCF85263_ADDR, 0x07, regs, 7) != 0) return -1;

    /* Convert BCD to binary */
    uint8_t sec  = (regs[0] >> 4) * 10 + (regs[0] & 0x0F);
    uint8_t min  = (regs[1] >> 4) * 10 + (regs[1] & 0x0F);
    uint8_t hour = (regs[2] >> 4) * 10 + (regs[2] & 0x0F);
    uint8_t day  = (regs[3] >> 4) * 10 + (regs[3] & 0x0F);
    uint8_t month = (regs[5] >> 4) * 10 + (regs[5] & 0x0F);
    uint8_t year = (regs[6] >> 4) * 10 + (regs[6] & 0x0F) + 2000;

    /* Simplified Unix time (not leap-year accurate) */
    static const uint16_t days_before_month[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    uint32_t days = (year - 1970) * 365 + days_before_month[month - 1] + day - 1;
    /* Add leap days (simplified) */
    days += (year - 1969) / 4;
    *unix_time = days * 86400 + hour * 3600 + min * 60 + sec;
    return 0;
}

static int pcf85263_write_time(uint32_t unix_time)
{
    /* Convert Unix timestamp to BCD time components */
    uint32_t secs = unix_time % 60;
    uint32_t mins = (unix_time / 60) % 60;
    uint32_t hours = (unix_time / 3600) % 24;
    uint32_t days = unix_time / 86400;

    /* Simplified date conversion (ignores leap years for brevity) */
    uint16_t year = 1970 + days / 365;
    uint16_t day_of_year = days % 365;
    uint8_t month = 1;
    static const uint16_t days_before[] = {0,31,59,90,120,151,181,212,243,273,304,334,365};
    while (month < 12 && day_of_year >= days_before[month]) month++;
    uint8_t day = day_of_year - days_before[month - 1] + 1;

    uint8_t regs[7];
    regs[0] = ((secs / 10) << 4) | (secs % 10);
    regs[1] = ((mins / 10) << 4) | (mins % 10);
    regs[2] = ((hours / 10) << 4) | (hours % 10);
    regs[3] = ((day / 10) << 4) | (day % 10);
    regs[4] = 0;  /* weekday (not used) */
    regs[5] = ((month / 10) << 4) | (month % 10);
    regs[6] = (((year - 2000) / 10) << 4) | ((year - 2000) % 10);

    return i2c3_write(PCF85263_ADDR, 0x07, regs, 7);
}

/* ---------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

void env_sens_init(void)
{
    /* I2C1 already initialized by power.c */
    i2c3_init();

    /* Configure DS18B20 pin (PG11) with pull-up */
    RCC->AHB4ENR |= BIT(6);  /* GPIOG */
    GPIOG->PUPDR |= (GPIO_PUPD_PU << (DS18B20_PIN*2));

    /* Configure soil moisture ADC pin */
    RCC->AHB4ENR |= BIT(5);  /* GPIOF */
    GPIOF->MODER |= (GPIO_MODE_ANALOG << (SOIL_MOIST_PIN*2));
}

int env_sens_read_soil_moisture(float *pct)
{
    return adc_soil_moisture_read(pct);
}

int env_sens_read_soil_temp(float *celsius)
{
    return ds18b20_read(celsius);
}

int env_sens_read_ambient(float *temp_c, float *rh)
{
    return sht41_read(temp_c, rh);
}

int env_sens_rtc_get(uint32_t *unix_time)
{
    return pcf85263_read_time(unix_time);
}

int env_sens_rtc_set(uint32_t unix_time)
{
    return pcf85263_write_time(unix_time);
}

int env_sens_read_all(env_data_t *data)
{
    if (!data) return -1;
    memset(data, 0, sizeof(*data));

    env_sens_read_soil_moisture(&data->soil_moisture_pct);
    env_sens_read_soil_temp(&data->soil_temp_c);
    env_sens_read_ambient(&data->ambient_temp_c, &data->ambient_rh);
    env_sens_rtc_get(&data->rtc_unix_time);

    return 0;
}