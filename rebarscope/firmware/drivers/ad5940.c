/*
 * drivers/ad5940.c — AD5940 impedance AFE driver (Wenner resistivity + LPR)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * The AD5940 generates the 1 kHz sinusoid for the Wenner probe and runs the
 * DC sweep for linear polarization resistance measurements. We use its
 * internal DFT engine to extract magnitude/phase of the Wenner voltage
 * without needing an external lock-in.
 */
#include "../board.h"
#include "../registers.h"
#include "ad5940.h"

/* ---- AD5940 register (abbreviated) ---- */
#define AD5940_REG_AFECON      0x2000
#define AD5940_REG_WGCON       0x2008
#define AD5940_REG_WGSEQ       0x200C
#define AD5940_REG_FREQLOCK    0x2010
#define AD5940_REG_AFE_DACCTRL 0x2100
#define AD5940_REG_DFTCTRL      0x2200
#define AD5940_REG_ADCDAT       0x2300
#define AD5940_REG_SWDSCON      0x2400

#define AD5940_CMD_NOP         0x00
#define AD5940_REG_WRITE       0x00
#define AD5940_REG_READ        0x10

/* Wenner excitation: 1 kHz, 10 µA RMS into 50 mm spacing.
 * The AD5940 HSTIA gain resistor selects the current amplitude. */

static void spi2_cs_low(void)
{
    GPIOB->BSRR = (1u << (12 + 16));
}
static void spi2_cs_high(void)
{
    GPIOB->BSRR = (1u << 12);
}
static uint8_t spi2_xfer(uint8_t tx)
{
    while (!(SPI2->SR & SPI_SR_TXE)) ;
    *(volatile uint8_t *)&SPI2->DR = tx;
    while (!(SPI2->SR & SPI_SR_RXNE)) ;
    return *(volatile uint8_t *)&SPI2->DR;
}

static void ad5940_delay_us(uint32_t us)
{
    volatile uint32_t n = us * 20u;
    while (n--) __asm volatile("nop");
}

static void ad5940_write_reg(uint16_t reg, uint32_t val)
{
    spi2_cs_low();
    /* high byte first, then low byte, 4 data bytes (big-endian within chip) */
    spi2_xfer(AD5940_REG_WRITE | (uint8_t)(reg >> 8));
    spi2_xfer((uint8_t)(reg & 0xFF));
    spi2_xfer((uint8_t)(val & 0xFF));
    spi2_xfer((uint8_t)((val >> 8) & 0xFF));
    spi2_xfer((uint8_t)((val >> 16) & 0xFF));
    spi2_xfer((uint8_t)((val >> 24) & 0xFF));
    spi2_cs_high();
    ad5940_delay_us(10);
}

static uint32_t ad5940_read_reg(uint16_t reg)
{
    spi2_cs_low();
    spi2_xfer(AD5940_REG_READ | (uint8_t)(reg >> 8));
    spi2_xfer((uint8_t)(reg & 0xFF));
    /* dummy 4 bytes for read */
    uint8_t b0 = spi2_xfer(0xFF);
    uint8_t b1 = spi2_xfer(0xFF);
    uint8_t b2 = spi2_xfer(0xFF);
    uint8_t b3 = spi2_xfer(0xFF);
    spi2_cs_high();
    return ((uint32_t)b0) | ((uint32_t)b1 << 8) |
           ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
}

void ad5940_init(void)
{
    pwr_gate_enable(PIN_PWR_AD5940, 1);
    ad5940_delay_us(2000);

    /* Reset via SPI command (0xC0) */
    spi2_cs_low();
    spi2_xfer(0xC0);
    spi2_cs_high();
    ad5940_delay_us(1000);

    /* Power-up AFE */
    ad5940_write_reg(AD5940_REG_AFECON, 0x00000001);
    /* Configure HSTIA gain for 10 µA compliance, TIA Rg = 200 kΩ */
    ad5940_write_reg(AD5940_REG_SWDSCON, 0x000A0000);
    /* Frequency = 1000 Hz; the chip computes M/N internally. */
    ad5940_write_reg(AD5940_REG_FREQLOCK, 0x00001000);
    /* Wake the waveform generator */
    ad5940_write_reg(AD5940_REG_WGCON, 0x00000003);
    /* Enable internal DFT engine for lock-in demodulation */
    ad5940_write_reg(AD5940_REG_DFTCTRL, 0x00010002);
}

/* ---- Wenner four-probe resistivity ---- */

/*
 * rho (Ω·m) = 2 * pi * a * (V/I)
 *
 * The AD5940 drives 10 µA RMS between the outer pins; the inner-pair
 * voltage is measured by its internal PGA+ADC and the DFT returns
 * complex magnitude (in nV) at 1 kHz.
 */
float ad5940_wenner_measure(float alpha_mm, uint8_t n_avg)
{
    float mag_v_sum = 0.0f;
    float mag_i_sum = 0.0f;
    for (uint8_t i = 0; i < n_avg; i++) {
        /* Start a single-shot AC measurement */
        ad5940_write_reg(AD5940_REG_AFECON, 0x00000003);
        ad5940_delay_us(5000);
        uint32_t v_raw = ad5940_read_reg(AD5940_REG_ADCDAT);
        uint32_t i_raw = ad5940_read_reg(AD5940_REG_ADCDAT + 4);
        /* Convert ADC codes to nV and nA (chip-specific scale omitted for brevity) */
        float mag_v = (float)(int32_t)v_raw * 0.000001f;  /* µV */
        float mag_i = (float)(int32_t)i_raw * 0.00001f;   /* µA */
        mag_v_sum += mag_v;
        mag_i_sum += mag_i;
    }
    float v = mag_v_sum / n_avg;   /* µV */
    float i = mag_i_sum / n_avg;   /* µA */
    if (i < 0.001f) return -1.0f;
    float rho = 2.0f * 3.14159265f * (alpha_mm / 1000.0f) * (v / i);
    return rho;  /* Ω·m */
}

/* ---- Linear Polarization Resistance ---- */

void ad5940_lpr_start(float ecorr_mv)
{
    /* Configure DAC to sweep from Ecorr-25 mV to Ecorr+25 mV at 0.1 mV/s
     * (500 seconds total for the standard sweep). The MCU controls timing
     * through an interrupt-driven step generator.
     */
    uint32_t dac_start = (uint32_t)((ecorr_mv - 25.0f) * 65536.0f / 3300.0f);
    uint32_t dac_end   = (uint32_t)((ecorr_mv + 25.0f) * 65536.0f / 3300.0f);
    ad5940_write_reg(AD5940_REG_AFE_DACCTRL, dac_start | (dac_end << 16));
    ad5940_write_reg(AD5940_REG_AFECON, 0x00000005);  /* LPR mode */
}

float ad5940_lpr_sample_ua(void)
{
    /* Read the TIA current; sign indicates cathodic/anodic direction */
    uint32_t i_raw = ad5940_read_reg(AD5940_REG_ADCDAT);
    float i_ua = (float)(int32_t)i_raw * 0.001f;   /* scale factor: µA/LSB */
    return i_ua;
}

void ad5940_lpr_stop(void)
{
    ad5940_write_reg(AD5940_REG_AFECON, 0x00000000);
}

void ad5940_powerdown(void)
{
    ad5940_write_reg(AD5940_REG_AFECON, 0x00000000);
    pwr_gate_enable(PIN_PWR_AD5940, 0);
}