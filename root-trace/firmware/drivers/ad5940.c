/*
 * ad5940.c — AD5940 Bioimpedance AFE Driver
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Implements SPI communication, waveform generator configuration, DFT
 * measurement control, and calibration for the Analog Devices AD5940
 * bioimpedance analog front-end.  The AD5940 integrates a waveform
 * generator, TIA, PGA, 16-bit Sigma-Delta ADC, and on-chip DFT engine.
 */

#include "ad5940.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---------------------------------------------------------------------
 * SPI low-level: send a 24-bit command (3 bytes) + data.
 * AD5940 SPI frame: [1 byte cmd+addr_high] [1 byte addr_low] [N data bytes]
 * For a register write: cmd = 0x00 (write), addr = reg
 * For a register read:  cmd = 0x01 (read), addr = reg, then dummy + read
 * --------------------------------------------------------------------- */

static void spi_wait_tx(spi_reg_t *spi)
{
    while (!(spi->SR & SPI_SR_TXP)) { /* wait for TX fifo space */ }
}

static void spi_wait_rx(spi_reg_t *spi)
{
    while (!(spi->SR & SPI_SR_RXP)) { /* wait for RX fifo data */ }
}

static void spi_wait_eot(spi_reg_t *spi)
{
    while (!(spi->SR & SPI_SR_EOT)) { /* wait end of transfer */ }
}

static uint8_t spi_xfer_byte(spi_reg_t *spi, uint8_t b)
{
    spi_wait_tx(spi);
    *(volatile uint8_t *)&spi->TXDR = b;
    spi_wait_rx(spi);
    return *(volatile uint8_t *)&spi->RXDR;
}

static void ad5940_cs_low(void)
{
    PIN_CLR(AD5940_CS_PORT, AD5940_CS_PIN);
}

static void ad5940_cs_high(void)
{
    PIN_SET(AD5940_CS_PORT, AD5940_CS_PIN);
}

/* SPI1 setup: 16-bit frame, master, CPOL=1 CPHA=1, baud div=64 (3.75 MHz) */
static void ad5940_spi_init(void)
{
    /* Enable GPIOA clock and SPI1 clock */
    RCC->AHB4ENR |= BIT(0);  /* GPIOA */
    RCC->APB2ENR |= BIT(12);  /* SPI1 */

    /* PA4 = CS (output, push-pull, high speed) */
    GPIOA->MODER  &= ~(3U << (4*2));
    GPIOA->MODER  |=  (GPIO_MODE_OUT << (4*2));
    GPIOA->OSPEEDR |= (GPIO_SPEED_VHIGH << (4*2));
    PIN_SET(AD5940_CS_PORT, AD5940_CS_PIN);

    /* PA5 = CLK, PA6 = MISO, PA7 = MOSI  (AF5) */
    int pins[3] = {5, 6, 7};
    for (int i = 0; i < 3; i++) {
        int p = pins[i];
        GPIOA->MODER  &= ~(3U << (p*2));
        GPIOA->MODER  |=  (GPIO_MODE_AF << (p*2));
        GPIOA->OSPEEDR |= (GPIO_SPEED_VHIGH << (p*2));
        if (p < 8) {
            GPIOA->AFRL &= ~(0xFU << (p*4));
            GPIOA->AFRL |=  (AD5940_SPI_AF << (p*4));
        } else {
            GPIOA->AFRH &= ~(0xFU << ((p-8)*4));
            GPIOA->AFRH |=  (AD5940_SPI_AF << ((p-8)*4));
        }
    }

    /* Configure SPI1 */
    AD5940_SPI->CR1 = 0;
    AD5940_SPI->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSM | SPI_CFG2_SSOE
                     | SPI_CFG2_CPOL | SPI_CFG2_CPHA;
    AD5940_SPI->CFG1 = (63U << SPI_CFG1_BAUD_SHIFT) /* /64 -> 3.75 MHz */
                     | (15U << 0)                   /* 16-bit data size */
                     | BIT(0)                        /* TX FIFO threshold (half) */
                     | BIT(1)                        /* RX FIFO threshold */
                     ;
    AD5940_SPI->CR1 = SPI_CR1_SPE;
}

/* ---------------------------------------------------------------------
 * AD5940 register access
 * --------------------------------------------------------------------- */

void ad5940_reg_write(uint16_t reg, uint32_t val)
{
    ad5940_cs_low();
    spi_xfer_byte(AD5940_SPI, (AD5940_CMD_REG_WRITE << 6) | ((reg >> 8) & 0x3F));
    spi_xfer_byte(AD5940_SPI, reg & 0xFF);
    /* Send 4 data bytes, MSB first */
    spi_xfer_byte(AD5940_SPI, (val >> 24) & 0xFF);
    spi_xfer_byte(AD5940_SPI, (val >> 16) & 0xFF);
    spi_xfer_byte(AD5940_SPI, (val >> 8) & 0xFF);
    spi_xfer_byte(AD5940_SPI, val & 0xFF);
    ad5940_cs_high();
}

uint32_t ad5940_reg_read(uint16_t reg)
{
    uint32_t val = 0;
    ad5940_cs_low();
    spi_xfer_byte(AD5940_SPI, (AD5940_CMD_REG_READ << 6) | ((reg >> 8) & 0x3F));
    spi_xfer_byte(AD5940_SPI, reg & 0xFF);
    /* Dummy 4 cycles to clock out the data */
    for (int i = 0; i < 4; i++)
        val = (val << 8) | spi_xfer_byte(AD5940_SPI, 0x00);
    ad5940_cs_high();
    return val;
}

/* ---------------------------------------------------------------------
 * Initialization
 * --------------------------------------------------------------------- */

void ad5940_reset(void)
{
    /* Hardware reset via GPIO: RESET low 10us, then high, wait 10ms */
    PIN_CLR(AD5940_RESET_PORT, AD5940_RESET_PIN);
    for (volatile int i = 0; i < 1000; i++) { /* ~10us */ }
    PIN_SET(AD5940_RESET_PORT, AD5940_RESET_PIN);
    for (volatile int i = 0; i < 1000000; i++) { /* ~10ms */ }
}

void ad5940_init(void)
{
    ad5940_spi_init();

    /* Configure IRQ pin as input (PC4) and RESET pin as output (PC5) */
    RCC->AHB4ENR |= BIT(2);  /* GPIOC */
    GPIOC->MODER &= ~(3U << (4*2));  /* PC4 input (IRQ) */
    GPIOC->PUPDR |=  (GPIO_PUPD_PU << (4*2));

    GPIOC->MODER  &= ~(3U << (5*2));
    GPIOC->MODER  |=  (GPIO_MODE_OUT << (5*2));
    PIN_SET(AD5940_RESET_PORT, AD5940_RESET_PIN);

    ad5940_reset();

    /* Put AD5940 in known state: AFE reset */
    ad5940_reg_write(AD5940_REG_AFECON, AD5940_AFECON_AFERESET);
    for (volatile int i = 0; i < 10000; i++) { /* ~100us */ }

    /* Disable all AFE blocks first */
    ad5940_reg_write(AD5940_REG_AFECON, 0x00000000);

    /* Configure ADC: 16-bit, Sinc3 filter, gain = 1 */
    ad5940_reg_write(AD5940_REG_ADCCON, (1U << 4)   /* Sinc3 */
                                      | (0U << 0)); /* PGA gain code 0 = x1 */

    /* Configure DFT: 1024-point, use ADC data */
    ad5940_reg_write(AD5940_REG_DFTCON, (6U << 0)   /* log2(1024)=6 */
                                       | (1U << 8)); /* DFT on ADC0 */

    /* Configure switch matrix defaults (all disconnected) */
    ad5940_reg_write(AD5940_REG_SWCON, 0);
    ad5940_reg_write(AD5940_REG_SWVSEL0, 0);
    ad5940_reg_write(AD5940_REG_SWVSEL1, 0);
    ad5940_reg_write(AD5940_REG_SWVSEL2, 0);
    ad5940_reg_write(AD5940_REG_SWVSEL3, 0);

    /* Clear any pending interrupts */
    ad5940_reg_write(AD5940_REG_INTCLR, 0xFFFFFFFF);
}

/* ---------------------------------------------------------------------
 * Configuration: set excitation frequency and current amplitude
 * --------------------------------------------------------------------- */

static uint32_t freq_to_word(uint32_t freq_hz)
{
    /* AD5940 frequency word: freq = MCLK * (FreqWord / 2^32)
     * => FreqWord = (freq_hz << 32) / MCLK  (but use 31-bit to avoid overflow) */
    uint64_t num = ((uint64_t)freq_hz << 31) / (AD5940_MCLK_HZ / 2);
    return (uint32_t)num;
}

void ad5940_configure(const ad5940_config_t *cfg)
{
    /* Set excitation frequency (waveform generator) */
    ad5940_reg_write(AD5940_REG_FREQWORD, freq_to_word(cfg->freq_hz));

    /* Set current amplitude: DAC code = current_ua / 0.5 µA per LSB (approx) */
    uint32_t dac_code = (uint32_t)cfg->current_ua * 2;  /* 0.5 µA/LSB */
    if (dac_code > 0x3FF) dac_code = 0x3FF;
    ad5940_reg_write(AD5940_REG_DACCON, dac_code);

    /* Set PGA gain */
    uint32_t adccon = ad5940_reg_read(AD5940_REG_ADCCON);
    adccon &= ~(0x7U << 8);
    adccon |= ((uint32_t)(cfg->pga_gain & 0x7) << 8);
    ad5940_reg_write(AD5940_REG_ADCCON, adccon);

    /* Configure DFT length */
    uint32_t dftcon = ad5940_reg_read(AD5940_REG_DFTCON);
    dftcon &= ~(0x7U << 0);
    dftcon |= ((uint32_t)(cfg->dft_len_log2 & 0x7) << 0);
    ad5940_reg_write(AD5940_REG_DFTCON, dftcon);

    /* ADC rate divider */
    uint32_t filter = ad5940_reg_read(AD5940_REG_ADCFILTER);
    filter &= ~(0xFFU << 0);
    filter |= ((uint32_t)cfg->adc_rate_div & 0xFF);
    ad5940_reg_write(AD5940_REG_ADCFILTER, filter);
}

/* ---------------------------------------------------------------------
 * Start a DFT measurement and wait for completion
 * --------------------------------------------------------------------- */

void ad5940_start_dft(void)
{
    /* Enable AFE blocks: WG + ADC + DFT + INAMP */
    ad5940_reg_write(AD5940_REG_AFECON,
        AD5940_AFECON_ADCAFEEN | AD5940_AFECON_WGAFEEN
        | AD5940_AFECON_DFTEN | AD5940_AFECON_INAMPMODE);

    /* Start the sequence engine */
    ad5940_reg_write(AD5940_REG_AFECON,
        AD5940_AFECON_ADCAFEEN | AD5940_AFECON_WGAFEEN
        | AD5940_AFECON_DFTEN | AD5940_AFECON_INAMPMODE
        | AD5940_AFECON_SEQSTART);
}

void ad5940_wait_dft_done(uint32_t timeout_ms)
{
    uint32_t status;
    uint32_t waited = 0;
    do {
        status = ad5940_reg_read(AD5940_REG_AFE_STATUS);
        if (status & BIT(4))  /* DFT done bit */
            break;
        for (volatile int i = 0; i < 1200; i++) { /* ~1ms */ }
        waited++;
    } while (waited < timeout_ms);
}

/* ---------------------------------------------------------------------
 * Perform a single measurement at the configured frequency
 * --------------------------------------------------------------------- */

int ad5940_measure(ad5940_meas_t *result)
{
    if (!result) return -1;

    ad5940_start_dft();
    ad5940_wait_dft_done(100);

    /* Read DFT results: real and imaginary parts (32-bit signed) */
    uint32_t real_raw = ad5940_reg_read(AD5940_REG_DFTREAL);
    uint32_t imag_raw = ad5940_reg_read(AD5940_REG_DFTIMAG);

    /* Convert to signed (two's complement) */
    result->real = (int32_t)real_raw;
    result->imag = (int32_t)imag_raw;
    result->freq = 0;  /* filled by caller */

    /* Compute magnitude: sqrt(re^2 + im^2), approximate via integer math */
    int64_t re_sq = (int64_t)result->real * result->real;
    int64_t im_sq = (int64_t)result->imag * result->imag;
    int64_t mag_sq = re_sq + im_sq;
    /* Integer sqrt approximation (64-bit) */
    if (mag_sq <= 0) {
        result->mag = 0;
        result->phase_mdeg = 0;
    } else {
        uint64_t m = (uint64_t)mag_sq;
        uint32_t res = 0;
        uint32_t bit = 0x80000000U;
        /* Find highest set bit */
        while (bit > m) bit >>= 2;
        while (bit) {
            if (m >= (uint64_t)(res + bit)) {
                m -= (res + bit);
                res = (res >> 1) + bit;
            } else {
                res >>= 1;
            }
            bit >>= 2;
        }
        result->mag = res;
        /* Phase = atan2(im, re) in millidegrees */
        if (result->real == 0) {
            result->phase_mdeg = (result->imag >= 0) ? 90000 : -90000;
        } else {
            /* atan approximation via small-angle + quadrant correction */
            int32_t y = result->imag;
            int32_t x = result->real;
            int quad = 0;
            if (x < 0) { x = -x; y = -y; quad = 2; }
            if (y < 0) { y = -y; quad |= 1; }
            /* atan(y/x) approx: y/x * (1 - 0.333*(y/x)^2) (for x>=y) */
            int32_t ratio = (x == 0) ? 0 : (int32_t)((int64_t)y * 180000 / 31415 / x);
            int32_t angle = ratio * (1000 - ratio * ratio / 3000);
            if (y > x) angle = 90000 - angle;
            angle += quad * 180000;
            result->phase_mdeg = angle;
        }
    }

    return 0;
}

/* ---------------------------------------------------------------------
 * Sweep across multiple frequencies
 * --------------------------------------------------------------------- */

int ad5940_measure_sweep(const uint32_t *freqs, int n_freq,
                          ad5940_meas_t *results)
{
    ad5940_config_t cfg = {
        .freq_hz = 1000,
        .current_ua = EIT_CURRENT_UA,
        .pga_gain = 4,        /* PGA gain 4 */
        .dft_len_log2 = 6,    /* 1024-point DFT */
        .adc_rate_div = 8,
    };

    for (int i = 0; i < n_freq; i++) {
        cfg.freq_hz = freqs[i];
        ad5940_configure(&cfg);
        ad5940_measure(&results[i]);
        results[i].freq = freqs[i];
    }
    return 0;
}

/* ---------------------------------------------------------------------
 * Self-test: measure a known 100 Ω internal calibration resistor
 * --------------------------------------------------------------------- */

int ad5940_self_test(void)
{
    /* Route AD5940 excitation to internal calibration resistor (100 Ω)
     * via the switch matrix.  This is done by setting SWVSEL registers
     * to connect the internal RCAL path.  See AD5940 datasheet §7.6. */
    ad5940_reg_write(AD5940_REG_SWCON, 0x000003F0);  /* internal RCAL route */
    ad5940_reg_write(AD5940_REG_SWVSEL0, 0x01);       /* RCAL positive */
    ad5940_reg_write(AD5940_REG_SWVSEL1, 0x02);       /* RCAL negative */

    ad5940_config_t cfg = {
        .freq_hz = 1000,
        .current_ua = 100,
        .pga_gain = 2,
        .dft_len_log2 = 6,
        .adc_rate_div = 8,
    };
    ad5940_configure(&cfg);

    ad5940_meas_t result;
    ad5940_measure(&result);

    /* Expected magnitude ~ proportional to 100 Ω × 100 µA = 10 mV
     * Check that magnitude is within reasonable bounds */
    if (result.mag < 100 || result.mag > 1000000)
        return -1;

    /* Restore switch matrix to external electrodes */
    ad5940_reg_write(AD5940_REG_SWCON, 0);
    ad5940_reg_write(AD5940_REG_SWVSEL0, 0);
    ad5940_reg_write(AD5940_REG_SWVSEL1, 0);

    return 0;
}

void ad5940_sleep(void)
{
    ad5940_reg_write(AD5940_REG_AFECON, 0);
    ad5940_reg_write(AD5940_REG_TOPCTL0, 0);  /* enter sleep */
}

void ad5940_wakeup(void)
{
    ad5940_reg_write(AD5940_REG_TOPCTL0, 1);  /* wakeup */
    for (volatile int i = 0; i < 100000; i++) { /* ~1ms settle */ }
}

/* ---------------------------------------------------------------------
 * Calibration: store calibration coefficients in flash
 * --------------------------------------------------------------------- */

typedef struct {
    uint32_t magic;
    uint32_t version;
    int32_t  gain_corr;    /* magnitude gain correction (Q15) */
    int32_t  phase_corr;   /* phase offset correction (mdeg) */
    uint32_t reserved[4];
} ad5940_calib_t;

static ad5940_calib_t s_calib = {
    .magic = CALIB_MAGIC,
    .version = CALIB_VERSION,
    .gain_corr = 32767,  /* unity gain in Q15 */
    .phase_corr = 0,
};

void ad5940_load_calib(void)
{
    ad5940_calib_t *flash_calib = (ad5940_calib_t *)CALIB_FLASH_BASE;
    if (flash_calib->magic == CALIB_MAGIC
        && flash_calib->version == CALIB_VERSION) {
        memcpy(&s_calib, flash_calib, sizeof(s_calib));
    }
}

void ad5940_apply_calib(ad5940_meas_t *m)
{
    /* Apply gain and phase calibration corrections */
    int64_t corrected = (int64_t)m->mag * s_calib.gain_corr / 32768;
    m->mag = (uint32_t)corrected;
    m->phase_mdeg += s_calib.phase_corr;
}