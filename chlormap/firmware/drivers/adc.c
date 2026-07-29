/*
 * adc.c — ADS1255 24-bit ADC driver (SPI, multiplexed photodiode array)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The ADS1255 is a 24-bit delta-sigma ADC with an 8-channel input MUX.
 * We use it to sequentially sample the 128-element photodiode array by
 * driving the array's analog MUX select lines (external 7-to-128
 * analog mux, CD4051 × 3 cascaded) via GPIO. Each element is read as
 * a 24-bit signed value.
 *
 * The driver handles SPI framing, DRDY polling, MUX channel switching,
 * and integration timing.
 */

#include "adc.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- SPI low-level (stub: in real build uses STM32L4 LL_SPI) ---- */
static void spi1_select(void)
{
    /* GPIOA->BSRR = (1 << 4) << 16;  — PA4 low */
}

static void spi1_deselect(void)
{
    /* GPIOA->BSRR = (1 << 4);        — PA4 high */
}

static uint8_t spi1_transfer(uint8_t tx)
{
    /* In real build:
     * while(!(SPI1->SR & SPI_SR_TXE));
     * *(volatile uint8_t*)&SPI1->DR = tx;
     * while(!(SPI1->SR & SPI_SR_RXNE));
     * return *(volatile uint8_t*)&SPI1->DR;
     */
    (void)tx;
    return 0; /* stub */
}

static void spi1_delay_us(uint32_t us)
{
    /* Busy-loop or TIM-based microsecond delay */
    (void)us;
}

/* ---- ADS1255 command helpers ---- */
static void ads1255_wait_drdy(uint32_t timeout_ms)
{
    /* Poll PA1 (DRDY) low, or wait for EXTI flag.
     * DRDY goes low when new data is ready.
     */
    (void)timeout_ms;
}

static void ads1255_send_cmd(uint8_t cmd)
{
    spi1_select();
    spi1_transfer(cmd);
    spi1_deselect();
    spi1_delay_us(10); /* t11: min 4 × tCLKIN after CS */
}

/* ---- Public API ---- */

bool adc_init(void)
{
    /* 1. Hardware reset via PDN pin */
    /* GPIOB->BSRR = (1 << 0) << 16; — PB0 low (PDN) */
    spi1_delay_us(1000);
    /* GPIOB->BSRR = (1 << 0);       — PB0 high (PDN) */
    spi1_delay_us(1000); /* ADS1255 needs ~1 ms after PDN */

    /* 2. Software reset */
    adc_reset();

    /* 3. Configure STATUS: disable buffer, set MSB first */
    adc_write_reg(ADS1255_REG_STATUS, 0x00);

    /* 4. Configure ADCON: PGA gain = 1, clock out off */
    adc_write_reg(ADS1255_REG_ADCON, ADS1255_GAIN_1);

    /* 5. Configure DRATE: 1000 SPS (good balance of speed + resolution) */
    adc_write_reg(ADS1255_REG_DRATE, ADS1255_DRATE_1000);

    /* 6. Self-calibration */
    adc_self_cal();

    return true;
}

void adc_reset(void)
{
    ads1255_send_cmd(ADS1255_CMD_RESET);
    spi1_delay_us(200); /* t17: ~100 µs after RESET */
}

bool adc_self_cal(void)
{
    ads1255_send_cmd(ADS1255_CMD_SELFCAL);
    ads1255_wait_drdy(500); /* self-cal takes ~400 ms at 1 kHz */
    return true;
}

uint8_t adc_read_reg(uint8_t reg)
{
    spi1_select();
    spi1_transfer(ADS1255_CMD_RREG | reg); /* 0x1A + reg */
    spi1_transfer(0x00);                   /* read 1 register */
    spi1_delay_us(10);                      /* t6: 50 × tCLKIN */
    uint8_t val = spi1_transfer(0x00);
    spi1_deselect();
    return val;
}

void adc_write_reg(uint8_t reg, uint8_t val)
{
    spi1_select();
    spi1_transfer(ADS1255_CMD_WREG | reg); /* 0x2A + reg */
    spi1_transfer(0x00);                   /* write 1 register */
    spi1_transfer(val);
    spi1_deselect();
    spi1_delay_us(10);
}

void adc_set_gain(uint8_t gain)
{
    uint8_t adcon = adc_read_reg(ADS1255_REG_ADCON);
    adcon = (adcon & 0xF8) | (gain & 0x07);
    adc_write_reg(ADS1255_REG_ADCON, adcon);
}

void adc_set_drate(uint8_t drate_reg)
{
    adc_write_reg(ADS1255_REG_DRATE, drate_reg);
}

int32_t adc_read_single(void)
{
    /* Sync + RDATA */
    ads1255_send_cmd(ADS1255_CMD_SYNC);
    spi1_delay_us(5);
    ads1255_send_cmd(ADS1255_CMD_WAKEUP);
    spi1_delay_us(5);

    spi1_select();
    spi1_transfer(ADS1255_CMD_RDATA);
    spi1_delay_us(10); /* t6 */

    /* Read 3 bytes (24-bit signed) */
    uint8_t b0 = spi1_transfer(0x00);
    uint8_t b1 = spi1_transfer(0x00);
    uint8_t b2 = spi1_transfer(0x00);
    spi1_deselect();

    int32_t val = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;
    /* Sign-extend from 24-bit */
    if (val & 0x800000) val |= 0xFF000000;
    return val;
}

/* ---- Photodiode array MUX control ----
 *
 * The 128-element photodiode array uses a 7-bit address to select one
 * of 128 elements. We drive the 7 address bits via GPIO (3 + 4 bits
 * on two ports, or a shift register). The selected element's output
 * is fed into the ADS1255 AIN0 input.
 */
static void select_element(uint8_t idx)
{
    idx &= 0x7F; /* 7-bit, 0–127 */
    /* In real build: write idx to GPIO port outputs or shift register */
    /* GPIOA->ODR = (GPIOA->ODR & 0xFF00) | (idx & 0x0F);       — PA0-PA3 */
    /* GPIOB->ODR = (GPIOB->ODR & 0x00FF) | ((idx & 0x70) << 4); — PB4-PB6 */
    spi1_delay_us(50); /* mux settling: <100 µs */
}

bool adc_acquire_frame(int32_t *frame, uint32_t integ_ms)
{
    if (!frame) return false;

    /* Set MUX to AIN0 (photodiode array output) */
    adc_write_reg(ADS1255_REG_MUX, ADS1255_MUX_AIN0);

    /* Start continuous read mode for efficiency */
    spi1_select();
    spi1_transfer(ADS1255_CMD_RDATAC);
    spi1_delay_us(10);
    spi1_deselect();

    for (int i = 0; i < ARRAY_ELEMENTS; i++) {
        select_element((uint8_t)i);

        /* Wait for integration time (LED light accumulation) */
        spi1_delay_us(integ_ms * 1000);

        /* Wait for DRDY */
        ads1255_wait_drdy(100);

        /* Read 3 bytes in continuous mode */
        spi1_select();
        spi1_delay_us(5);
        uint8_t b0 = spi1_transfer(0x00);
        uint8_t b1 = spi1_transfer(0x00);
        uint8_t b2 = spi1_transfer(0x00);
        spi1_deselect();

        int32_t val = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;
        if (val & 0x800000) val |= 0xFF000000; /* sign-extend */

        frame[i] = val;
    }

    /* Stop continuous read */
    ads1255_send_cmd(ADS1255_CMD_SDATAC);

    return true;
}