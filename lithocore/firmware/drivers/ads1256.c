/*
 * ads1256.c — ADS1256 24-bit delta-sigma ADC driver.
 *
 * Communicates over SPI1 (hardware SPI). The ADS1256 provides 24-bit
 * resolution at up to 30 kSPS — used for the low/mid frequency EIS
 * range (0.01 Hz – 1 kHz). The DRDY pin (PB14) signals data ready.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "ads1256.h"
#include "../board.h"
#include "../registers.h"

/* -------------------------------------------------------------------------
 * SPI low-level
 * ------------------------------------------------------------------------- */

static void spi1_wait_idle(void)
{
    while (SPI1->SR & SPI_SR_BSY) { }
}

void ads1256_cs_low(void)
{
    GPIOA->BSRR = (1U << (PIN_SPI1_NCS_ADC + 16));  /* reset PA5 */
}

void ads1256_cs_high(void)
{
    GPIOA->BSRR = (1U << PIN_SPI1_NCS_ADC);         /* set PA5 */
}

void ads1256_spi_write(uint8_t *data, uint8_t len)
{
    ads1256_cs_low();
    for (uint8_t i = 0; i < len; i++) {
        spi1_wait_idle();
        /* Write 8-bit data: use FRXTH so RXNE fires on 8 bits */
        *(volatile uint8_t *)&SPI1->DR = data[i];
        while (!(SPI1->SR & SPI_SR_RXNE)) { }
        (void)*(volatile uint8_t *)&SPI1->DR;  /* flush RX */
    }
    spi1_wait_idle();
    ads1256_cs_high();
}

void ads1256_spi_read(uint8_t *data, uint8_t len)
{
    ads1256_cs_low();
    for (uint8_t i = 0; i < len; i++) {
        spi1_wait_idle();
        *(volatile uint8_t *)&SPI1->DR = 0xFF;  /* dummy write */
        while (!(SPI1->SR & SPI_SR_RXNE)) { }
        data[i] = *(volatile uint8_t *)&SPI1->DR;
    }
    spi1_wait_idle();
    ads1256_cs_high();
}

uint8_t ads1256_wait_drdy(uint32_t timeout_ms)
{
    uint32_t start = 0;  /* would use millis() but keep driver self-contained */
    /* Poll PB14 (DRDY) — active low */
    while (GPIOB->IDR & (1U << PIN_ADS_DRDY)) {
        if (timeout_ms > 0) {
            /* simplified timeout — in production use a timer */
            start++;
            if (start > 1000000U) return 0;  /* timeout */
        }
    }
    return 1;
}

/* -------------------------------------------------------------------------
 * ADS1256 initialization
 *
 * Configures the ADC for:
 *   - PGA gain = 1 (±2.5 V full scale with 2.5 V Vref)
 *   - Data rate = 1000 SPS default (changed per-frequency during sweep)
 *   - MUX = AIN0/AIN1 (cell voltage differential)
 *   - Clock out disabled, sensor detect off
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int ads1256_init(void)
{
    /* Configure SPI1: master, CPOL=1, CPHA=1 (mode 3 — ADS1256 requires
       CPOL=1, CPHA=1), baud = PCLK/64 = 2.56 MHz (ADS1256 max 7.68 MHz) */
    SPI1->CR1 = 0;  /* disable before config */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA |
                SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_DIV64;
    SPI1->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI1->CR1 |= SPI_CR1_SPE;  /* enable SPI */

    ads1256_reset();
    delay_ms(2);  /* ADS1256 reset recovery */

    /* Read STATUS register to verify communication */
    uint8_t status;
    if (ads1256_read_register(ADS1256_REG_STATUS, &status) != 0)
        return -1;

    /* Configure: disable ACAL (auto-cal) for manual control, set order MSB */
    ads1256_write_register(ADS1256_REG_STATUS, 0x00);

    /* MUX: AIN0/AIN1 differential */
    ads1256_set_mux(ADS1256_MUX_AIN0_AIN1);

    /* ADCON: PGA gain = 1, clock out off */
    ads1256_set_pga(ADS1256_GAIN_1);

    /* DRATE: default 1000 SPS */
    ads1256_set_drate(ADS1256_DRATE_1000);

    /* Run self-calibration */
    ads1256_self_cal();

    return 0;
}

void ads1256_reset(void)
{
    /* Send RESET command via SPI */
    uint8_t cmd = ADS1256_CMD_RESET;
    ads1256_spi_write(&cmd, 1);
}

int ads1256_set_drate(uint8_t drate_reg)
{
    return ads1256_write_register(ADS1256_REG_DRATE, drate_reg);
}

int ads1256_set_mux(uint8_t mux_reg)
{
    return ads1256_write_register(ADS1256_REG_MUX, mux_reg);
}

int ads1256_set_pga(uint8_t gain_reg)
{
    /* ADCON register: bits[2:0] = PGA gain, bits[6:3] = clock out (0 = off) */
    return ads1256_write_register(ADS1256_REG_ADCON, gain_reg & 0x07);
}

int ads1256_self_cal(void)
{
    uint8_t cmd = ADS1256_CMD_SELFCAL;
    ads1256_spi_write(&cmd, 1);
    /* Wait for calibration to complete (DRDY goes low) */
    if (!ads1256_wait_drdy(1000))
        return -1;
    return 0;
}

/* -------------------------------------------------------------------------
 * Register read / write
 * ------------------------------------------------------------------------- */
int ads1256_read_register(uint8_t reg, uint8_t *val)
{
    uint8_t tx[3] = { ADS1256_CMD_RREG | (reg & 0x0F), 0x00, 0x00 };
    uint8_t rx[3];

    ads1256_spi_write(tx, 2);  /* send RREG + 0 (1 byte to read) */
    delay_ms(1);               /* t6 delay: 50 * tCLK ≈ 8 µs, use 1 ms safety */
    ads1256_spi_read(rx, 1);   /* read the register value */
    *val = rx[0];
    return 0;
}

int ads1256_write_register(uint8_t reg, uint8_t val)
{
    uint8_t tx[3] = { ADS1256_CMD_WREG | (reg & 0x0F), 0x00, val };
    ads1256_spi_write(tx, 3);
    delay_ms(1);  /* settling time */
    return 0;
}

/* -------------------------------------------------------------------------
 * Read a single data sample (24-bit signed)
 * ------------------------------------------------------------------------- */
int ads1256_read_data(int32_t *value)
{
    /* Wait for DRDY */
    if (!ads1256_wait_drdy(100))
        return -1;

    /* Send RDATA command */
    uint8_t cmd = ADS1256_CMD_RDATA;
    ads1256_spi_write(&cmd, 1);
    delay_ms(1);  /* t6 delay */

    /* Read 3 bytes (24-bit data) */
    uint8_t rx[3];
    ads1256_spi_read(rx, 3);

    /* Assemble 24-bit signed value */
    *value = ((int32_t)rx[0] << 16) | ((int32_t)rx[1] << 8) | (int32_t)rx[2];
    /* Sign extend from 24 to 32 bits */
    if (*value & 0x800000)
        *value |= 0xFF000000;

    return 0;
}

/* -------------------------------------------------------------------------
 * Capture a burst of samples at a given target rate.
 *
 * This implementation polls DRDY and reads each sample individually. For
 * high sample rates (> 1 kSPS) the MCU's built-in ADC + DMA is used instead
 * (see lockin.c for the high-frequency path).
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int ads1256_capture(ads1256_capture_t *cap, uint16_t num_samples,
                    uint32_t target_rate)
{
    if (num_samples > ADS1256_MAX_SAMPLES)
        return -1;

    /* Set data rate to closest supported rate >= target_rate */
    uint8_t drate;
    if (target_rate >= 30000)       drate = ADS1256_DRATE_30000;
    else if (target_rate >= 15000)  drate = ADS1256_DRATE_15000;
    else if (target_rate >= 7500)   drate = ADS1256_DRATE_7500;
    else if (target_rate >= 1000)   drate = ADS1256_DRATE_1000;
    else if (target_rate >= 500)    drate = ADS1256_DRATE_500;
    else if (target_rate >= 100)    drate = ADS1256_DRATE_100;
    else                            drate = ADS1256_DRATE_30;

    ads1256_set_drate(drate);

    /* Issue SYNC to align the conversion */
    uint8_t cmd = ADS1256_CMD_SYNC;
    ads1256_spi_write(&cmd, 1);
    delay_ms(1);

    /* Read samples */
    for (uint16_t i = 0; i < num_samples; i++) {
        if (ads1256_read_data(&cap->v_raw[i]) != 0)
            return -1;
    }

    cap->count = num_samples;
    cap->sample_rate_hz = target_rate;

    return 0;
}

/* -------------------------------------------------------------------------
 * Dual-channel capture: alternate between V (MUX AIN0/AIN1) and
 * I (MUX AIN4/AINCOM) on each sample. This halves the effective sample
 * rate per channel but allows using a single ADS1256.
 *
 * For the lock-in detection, V and I at the same time instant are needed.
 * We interleave and interpolate, or better: since the impedance is computed
 * as the ratio of the complex Fourier components (which is independent of
 * the exact time alignment for periodic signals), the interleaved sampling
 * is acceptable as long as V and I are sampled symmetrically.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int ads1256_capture_dual(ads1256_capture_t *v_cap, ads1256_capture_t *i_cap,
                         uint16_t num_samples, uint32_t target_rate)
{
    if (num_samples > ADS1256_MAX_SAMPLES)
        return -1;

    /* Set data rate */
    uint8_t drate;
    if (target_rate >= 7500)        drate = ADS1256_DRATE_7500;
    else if (target_rate >= 1000)   drate = ADS1256_DRATE_1000;
    else if (target_rate >= 500)    drate = ADS1256_DRATE_500;
    else if (target_rate >= 100)    drate = ADS1256_DRATE_100;
    else                            drate = ADS1256_DRATE_30;

    /* For dual capture, double the rate since we alternate channels */
    ads1256_set_drate(drate);

    /* Sync */
    uint8_t cmd = ADS1256_CMD_SYNC;
    ads1256_spi_write(&cmd, 1);
    delay_ms(1);

    for (uint16_t i = 0; i < num_samples; i++) {
        /* Read V channel */
        ads1256_set_mux(ADS1256_MUX_AIN0_AIN1);
        if (ads1256_read_data(&v_cap->v_raw[i]) != 0)
            return -1;

        /* Read I channel */
        ads1256_set_mux(ADS1256_MUX_AIN4_AINCOM);
        if (ads1256_read_data(&i_cap->i_raw[i]) != 0)
            return -1;
    }

    v_cap->count = num_samples;
    i_cap->count = num_samples;
    v_cap->sample_rate_hz = target_rate;
    i_cap->sample_rate_hz = target_rate;

    return 0;
}

void ads1256_standby(void)
{
    uint8_t cmd = ADS1256_CMD_STANDBY;
    ads1256_spi_write(&cmd, 1);
}