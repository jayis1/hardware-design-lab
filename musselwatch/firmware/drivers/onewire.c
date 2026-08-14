/*
 * drivers/onewire.c — 1-Wire driver for DS18B20 water temperature
 *
 * Bit-banged 1-Wire on PB6 (open-drain).  Implements reset, write/read
 * bit, write/read byte, ROM skip, and a blocking DS18B20 conversion
 * that reads temperature in units of 0.1 °C.
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "../board.h"
#include "../registers.h"
#include "onewire.h"

/* TIM16 is used for microsecond timing.  It runs at 1 MHz (80 MHz / 80). */

static void tim16_init_us(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_TIM16EN;
    TIM16_PSC = 79u;   /* 80 MHz / 80 = 1 MHz -> 1 us per tick */
    TIM16_ARR = 0xFFFFu;
    TIM16_CR1 |= TIM16_CR1_CEN;
}

static inline void delay_us(uint16_t us)
{
    TIM16_CNT = 0u;
    while (TIM16_CNT < us) { /* wait */ }
}

static inline void ow_pin_low(void)
{
    /* Drive low: set output, output 0 */
    GPIO_MODER(GPIOB_BASE) |=  (1u << (ONEWIRE_PIN * 2u));  /* output */
    GPIO_BSRR(GPIOB_BASE) = (1u << (ONEWIRE_PIN + 16u));     /* reset */
}

static inline void ow_pin_release(void)
{
    /* Release: set input with pull-up (open-drain high) */
    GPIO_MODER(GPIOB_BASE) &= ~(3u << (ONEWIRE_PIN * 2u));   /* input */
    GPIO_PUPDR(GPIOB_BASE) |=  (1u << (ONEWIRE_PIN * 2u));   /* pull-up */
}

static inline uint8_t ow_pin_read(void)
{
    return (GPIO_IDR(GPIOB_BASE) >> ONEWIRE_PIN) & 1u;
}

bool ow_init(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    (void)RCC_AHB2ENR;
    tim16_init_us();
    ow_pin_release();
    delay_us(100);
    return true;
}

bool ow_reset(void)
{
    bool presence;
    ow_pin_low();
    delay_us(480);              /* reset pulse >= 480 us */
    ow_pin_release();
    delay_us(70);
    presence = (ow_pin_read() == 0u);  /* DS18B20 pulls low */
    delay_us(410);
    return presence;
}

void ow_write_bit(uint8_t bit)
{
    if (bit) {
        ow_pin_low();
        delay_us(6);
        ow_pin_release();
        delay_us(64);
    } else {
        ow_pin_low();
        delay_us(60);
        ow_pin_release();
        delay_us(10);
    }
}

uint8_t ow_read_bit(void)
{
    uint8_t bit;
    ow_pin_low();
    delay_us(4);
    ow_pin_release();
    delay_us(8);
    bit = ow_pin_read();
    delay_us(52);
    return bit;
}

void ow_write_byte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++) {
        ow_write_bit(byte & 0x01u);
        byte >>= 1;
    }
}

uint8_t ow_read_byte(void)
{
    uint8_t val = 0u;
    for (uint8_t i = 0; i < 8; i++) {
        val >>= 1;
        val |= (ow_read_bit() ? 0x80u : 0x00u);
    }
    return val;
}

bool ow_read_rom(uint8_t rom[8])
{
    if (!ow_reset()) return false;
    ow_write_byte(0x33u);  /* Read ROM */
    for (uint8_t i = 0; i < 8; i++) rom[i] = ow_read_byte();
    return true;
}

/*
 * Read temperature from DS18B20 in 0.1 °C units (int16_t).
 * Returns false if no sensor responds.
 */
bool ds18b20_read_temp_c10(int16_t *temp_c10)
{
    if (!ow_reset()) return false;
    ow_write_byte(0xCCu);  /* Skip ROM (single sensor on bus) */
    ow_write_byte(0x44u);  /* Convert T */
    /* 12-bit conversion: 750 ms max.  Wait for DQ to go high. */
    uint32_t timeout = 800000u;
    while (ow_pin_read() == 0u && --timeout) {
        delay_us(1);
    }

    if (!ow_reset()) return false;
    ow_write_byte(0xCCu);
    ow_write_byte(0xBEu);  /* Read scratchpad */

    uint8_t s9[9];
    for (uint8_t i = 0; i < 9; i++) s9[i] = ow_read_byte();

    /* Raw temp is 16-bit signed, 1/16 °C resolution.  Convert to 0.1 °C */
    int16_t raw = (int16_t)((s9[1] << 8) | s9[0]);
    /* raw/16 = °C ; *10 = 0.1°C ; so raw * 10 / 16 */
    *temp_c10 = (int16_t)((raw * 10) / 16);
    return true;
}