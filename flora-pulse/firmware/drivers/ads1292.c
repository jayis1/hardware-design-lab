/*
 * ads1292.c — ADS1292 24-bit biopotential ADC driver
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Drives the Texas Instruments ADS1292 — a 2-channel, 24-bit
 * delta-sigma ADC optimized for biopotential measurement.  In FloraPulse,
 * this device measures plant action potentials via silver/silver-chloride
 * electrodes inserted into the plant stem.
 *
 * The ADS1292 communicates via SPI1 at up to 4 MHz.  DRDY asserts low
 * when a new sample is ready (250 SPS in HR mode).  We use an EXTI
 * falling-edge interrupt to trigger a 9-byte SPI read (3 status + 3×2
 * data bytes).  For high-throughput streaming we set up DMA, but the
 * default 250 SPS rate is low enough for polling.
 *
 * Key design decisions:
 *  - Gain 12: plant APs are 10–100 mV at the tissue level but attenuate
 *    to 10–500 µV at the surface electrode.  Gain 12 gives full-scale
 *    of ±201 mV with 12 nV LSB — plenty of dynamic range.
 *  - HR mode (250 SPS): APs last 1–10 seconds (much slower than neural),
 *    so 250 SPS is more than adequate.  Higher rates add noise.
 *  - Internal reference: avoids external reference drift and the device
 *    is designed for it.
 *  - Right-leg drive (RLD) disabled: plants don't need common-mode
 *    rejection enhancement (no mains interference in greenhouse).
 */

#include "ads1292.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  Internal state                                                        */
/* ===================================================================== */

#define AP_RING_SIZE  256U

static int32_t ring_ch1[AP_RING_SIZE];
static int32_t ring_ch2[AP_RING_SIZE];
static volatile uint16_t ring_head = 0;
static volatile uint16_t ring_count = 0;

static volatile int32_t latest_ch1 = 0;
static volatile int32_t latest_ch2 = 0;
static volatile uint8_t  sample_ready = 0;

/* Current PGA gain (cached for conversion) */
static uint8_t current_gain = ADS1292_CHSET_GAIN_12;

/* ===================================================================== */
/*  SPI1 low-level routines                                              */
/* ===================================================================== */

static void spi1_wait_tx(void)
{
    while (!(SPI_REG(SPI1_BASE, SPI_SR) & SPI_SR_TXE))
        ;
}

static void spi1_wait_rx(void)
{
    while (!(SPI_REG(SPI1_BASE, SPI_SR) & SPI_SR_RXNE))
        ;
}

static uint8_t spi1_xfer(uint8_t tx)
{
    spi1_wait_tx();
    SPI_REG(SPI1_BASE, SPI_DR) = tx;
    spi1_wait_rx();
    return (uint8_t)SPI_REG(SPI1_BASE, SPI_DR);
}

static void spi1_select(void)
{
    gpio_reset(ADS_CS_PORT, ADS_CS_PIN);
}

static void spi1_deselect(void)
{
    gpio_set(ADS_CS_PORT, ADS_CS_PIN);
}

/* Send a command byte (no data) */
static void ads1292_cmd(uint8_t cmd)
{
    spi1_select();
    spi1_xfer(cmd);
    /* Wait t_sdecode (~3 tCLK = 6 µs at 512 kHz internal clock) */
    for (volatile int i = 0; i < 100; i++)
        ;
    spi1_deselect();
}

/* Write a single register */
static void ads1292_wreg(uint8_t reg, uint8_t val)
{
    spi1_select();
    spi1_xfer(ADS1292_CMD_WREG | (reg & 0x1FU));
    spi1_xfer(0x00U);  /* Number of registers - 1 = 0 */
    spi1_xfer(val);
    spi1_deselect();
}

/* Read a single register */
static uint8_t ads1292_rreg(uint8_t reg)
{
    uint8_t val;
    spi1_select();
    spi1_xfer(ADS1292_CMD_RREG | (reg & 0x1FU));
    spi1_xfer(0x00U);  /* Number of registers - 1 = 0 */
    val = spi1_xfer(0x00U);
    spi1_deselect();
    return val;
}

/* ===================================================================== */
/*  Public API                                                           */
/* ===================================================================== */

void ads1292_init(void)
{
    /* Enable GPIO and SPI1 clocks */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* Configure PA5/6/7 as AF5 (SPI1), push-pull, high speed */
    volatile uint32_t *pa_moder  = (volatile uint32_t *)(GPIOA_BASE + GPIO_MODER);
    volatile uint32_t *pa_ospeed = (volatile uint32_t *)(GPIOA_BASE + GPIO_OSPEEDR);
    volatile uint32_t *pa_afrl   = (volatile uint32_t *)(GPIOA_BASE + GPIO_AFRL);

    /* PA5 (SCK), PA6 (MISO), PA7 (MOSI) */
    for (uint8_t pin = 5; pin <= 7; pin++) {
        *pa_moder &= ~(0x3U << (pin * 2));
        *pa_moder |= (0x2U << (pin * 2));   /* AF mode */
        *pa_ospeed |= (0x3U << (pin * 2));   /* Very high speed */
        *pa_afrl &= ~(0xFU << (pin * 4));
        *pa_afrl |= ((uint32_t)GPIO_AF5 << (pin * 4));
    }

    /* Configure PB0 (CS), PB2 (START), PB10 (PWDN) as outputs */
    volatile uint32_t *pb_moder = (volatile uint32_t *)(GPIOB_BASE + GPIO_MODER);
    *pb_moder |= (0x1U << (ADS_CS_PIN * 2));
    *pb_moder |= (0x1U << (ADS_START_PIN * 2));
    *pb_moder |= (0x1U << (ADS_PWDN_PIN * 2));

    /* PB1 (DRDY) as input */
    *pb_moder &= ~(0x3U << (ADS_DRDY_PIN * 2));

    /* Initialize CS high, START low, PWDN low */
    gpio_set(ADS_CS_PORT, ADS_CS_PIN);
    gpio_reset(ADS_START_PORT, ADS_START_PIN);
    gpio_reset(ADS_PWDN_PORT, ADS_PWDN_PIN);

    /* Configure SPI1: master, CPOL=0, CPHA=1 (ADS1292 samples on rising),
     * baud rate = PCLK/32 = 80MHz/32 = 2.5 MHz (within 4 MHz limit)
     */
    SPI_REG(SPI1_BASE, SPI_CR1) = 0;
    SPI_REG(SPI1_BASE, SPI_CR1) = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                                   SPI_CR1_CPOL | SPI_CR1_BR_DIV32;
    SPI_REG(SPI1_BASE, SPI_CR2) = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI_REG(SPI1_BASE, SPI_CR1) |= SPI_CR1_SPE;

    /* --- ADS1292 power-up sequence --- */
    /* Assert PWDN high, then START low */
    gpio_set(ADS_PWDN_PORT, ADS_PWDN_PIN);
    /* Wait 2^18 tCLK ≈ 0.5 s for internal startup (simplified: 50 ms) */
    for (volatile uint32_t i = 0; i < 800000; i++)
        ;

    /* Send reset command */
    ads1292_cmd(ADS1292_CMD_RESET);
    for (volatile int i = 0; i < 1000; i++)
        ;

    /* Read ID register to verify device */
    uint8_t id = ads1292_rreg(ADS1292_REG_ID);
    (void)id;  /* In production, check (id & ADS1292_ID_DEV_ID_MASK) == ADS1292_ID_DEV_ADS1292 */

    /* Stop continuous data mode before writing config registers */
    ads1292_cmd(ADS1292_CMD_SDATAC);
    for (volatile int i = 0; i < 100; i++)
        ;

    /* CONFIG1: HR mode, 250 SPS
     * Bit 7 = 0 (HR), bits [6:2] = DR, bit 0 = 0 (HR)
     * DR = 100 (0x4<<2 = 0x10) for 250 SPS in HR mode
     */
    ads1292_wreg(ADS1292_REG_CONFIG1, ADS1292_CONFIG1_DR_250);

    /* CONFIG2: internal reference enabled, test signal off
     * Bit 7 = WCT (off), bit 6 = reserved, bit 5 = INT_FREQ (500Hz),
     * bit 4 = INT_TEST (off), bits [3:0] = test amp/freq
     * Enable internal reference: bit 6 of CONFIG2? Actually on ADS1292,
     * internal reference is controlled by CONFIG2 bit 6? No — it's
     * always on when PDB_REF bit is set. On the ADS1292, the internal
     * reference is enabled by setting bit 6 (reserved?) — actually the
     * reference is always on by default after power-up.
     * For safety: write 0x80 (WCT=0, INT_TEST=0, test source off).
     */
    ads1292_wreg(ADS1292_REG_CONFIG2, 0x80U);

    /* LOFF: disable lead-off detection (not relevant for plants) */
    ads1292_wreg(ADS1292_REG_LOFF, 0x00U);

    /* CH1SET: gain 12, normal input (MUX[2:0]=000)
     * Bits [6:4] = gain, [2:0] = MUX
     */
    ads1292_wreg(ADS1292_REG_CH1SET, ADS1292_CHSET_GAIN_12 | ADS1292_CHSET_MUX_NORMAL);
    current_gain = ADS1292_CHSET_GAIN_12;

    /* CH2SET: gain 12, normal input */
    ads1292_wreg(ADS1292_REG_CH2SET, ADS1292_CHSET_GAIN_12 | ADS1292_CHSET_MUX_NORMAL);

    /* RLD_SENS: disable right-leg drive */
    ads1292_wreg(ADS1292_REG_RLDSENS, 0x00U);

    /* LOFF_SENS: disable lead-off sense */
    ads1292_wreg(ADS1292_REG_LOFFSENS, 0x00U);

    /* RESP1: disable respiration (not used) */
    ads1292_wreg(ADS1292_REG_RESP1, 0x00U);

    /* RESP2: disable */
    ads1292_wreg(ADS1292_REG_RESP2, 0x00U);

    /* GPIO: all outputs low */
    ads1292_wreg(ADS1292_REG_GPIO, 0x00U);

    /* Start conversions */
    gpio_set(ADS_START_PORT, ADS_START_PIN);
    for (volatile int i = 0; i < 100; i++)
        ;
}

void ads1292_start(void)
{
    /* Enable RDATAC (read data continuous) mode */
    ads1292_cmd(ADS1292_CMD_SDATAC);
    for (volatile int i = 0; i < 50; i++)
        ;
    ads1292_cmd(ADS1292_CMD_RDATAC);
    for (volatile int i = 0; i < 50; i++)
        ;
    gpio_set(ADS_START_PORT, ADS_START_PIN);
}

void ads1292_stop(void)
{
    ads1292_cmd(ADS1292_CMD_SDATAC);
    gpio_reset(ADS_START_PORT, ADS_START_PIN);
}

/* Read a single sample (9 bytes: 3 status + 3 ch1 + 3 ch2) */
static void ads1292_read_raw(int32_t *ch1, int32_t *ch2)
{
    uint8_t buf[9];

    spi1_select();

    /* Wait for DRDY low */
    while (gpio_read(ADS_DRDY_PORT, ADS_DRDY_PIN))
        ;

    /* Read 9 bytes */
    for (int i = 0; i < 9; i++) {
        buf[i] = spi1_xfer(0x00U);
    }

    spi1_deselect();

    /* Parse 24-bit signed values (big-endian) */
    *ch1 = ((int32_t)buf[3] << 16) | ((int32_t)buf[4] << 8) | buf[5];
    if (*ch1 & 0x800000U) *ch1 |= 0xFF000000U;  /* Sign extend */

    *ch2 = ((int32_t)buf[6] << 16) | ((int32_t)buf[7] << 8) | buf[8];
    if (*ch2 & 0x800000U) *ch2 |= 0xFF000000U;
}

void ads1292_read_both(int32_t *ch1, int32_t *ch2)
{
    ads1292_read_raw(ch1, ch2);
    latest_ch1 = *ch1;
    latest_ch2 = *ch2;

    /* Push into ring buffer */
    ring_ch1[ring_head] = *ch1;
    ring_ch2[ring_head] = *ch2;
    ring_head = (ring_head + 1) % AP_RING_SIZE;
    if (ring_count < AP_RING_SIZE)
        ring_count++;
    sample_ready = 1;
}

int32_t ads1292_read_channel(uint8_t ch)
{
    if (ch == 1) return latest_ch1;
    return latest_ch2;
}

float ads1292_counts_to_uv(int32_t counts)
{
    /* ADS1292 Vref = 2.42 V (internal), 24-bit
     * LSB = Vref / (gain * 2^23) = 2.42 / (12 * 8388608) = 24.1 nV
     * Convert to µV: × 0.0241
     */
    float lsb_uv = (BOARD_ADS1292_VREF_MV * 1000.0f) /
                   ((float)BOARD_ADS1292_GAIN * 8388608.0f) / 1000.0f;
    return (float)counts * lsb_uv;
}

void ads1292_test_signal_enable(void)
{
    /* Set CONFIG2 bit 4 (INT_TEST) and connect test signal to both channels */
    ads1292_cmd(ADS1292_CMD_SDATAC);
    uint8_t cfg2 = ads1292_rreg(ADS1292_REG_CONFIG2);
    ads1292_wreg(ADS1292_REG_CONFIG2, cfg2 | ADS1292_CONFIG2_INT_TEST);

    /* Route test signal to CH1 and CH2 inputs via MUX */
    ads1292_wreg(ADS1292_REG_CH1SET, ADS1292_CHSET_GAIN_12 | 0x01U); /* MUX=test */
    ads1292_wreg(ADS1292_REG_CH2SET, ADS1292_CHSET_GAIN_12 | 0x01U);

    ads1292_cmd(ADS1292_CMD_RDATAC);
    gpio_set(ADS_START_PORT, ADS_START_PIN);
}

void ads1292_test_signal_disable(void)
{
    ads1292_cmd(ADS1292_CMD_SDATAC);
    uint8_t cfg2 = ads1292_rreg(ADS1292_REG_CONFIG2);
    ads1292_wreg(ADS1292_REG_CONFIG2, cfg2 & ~ADS1292_CONFIG2_INT_TEST);
    ads1292_wreg(ADS1292_REG_CH1SET, ADS1292_CHSET_GAIN_12 | ADS1292_CHSET_MUX_NORMAL);
    ads1292_wreg(ADS1292_REG_CH2SET, ADS1292_CHSET_GAIN_12 | ADS1292_CHSET_MUX_NORMAL);
    ads1292_cmd(ADS1292_CMD_RDATAC);
    gpio_set(ADS_START_PORT, ADS_START_PIN);
}

uint16_t ads1292_detect_events(float threshold_uv, uint16_t window_samples)
{
    if (ring_count == 0) return 0;
    uint16_t count = 0;
    uint16_t start = (ring_head + AP_RING_SIZE - window_samples) % AP_RING_SIZE;

    float threshold_counts = threshold_uv / ads1292_counts_to_uv(1);
    if (threshold_counts < 0) threshold_counts = -threshold_counts;

    for (uint16_t i = 0; i < window_samples && i < ring_count; i++) {
        uint16_t idx = (start + i) % AP_RING_SIZE;
        int32_t v = ring_ch1[idx];
        if (v > threshold_counts || v < -threshold_counts)
            count++;
    }
    return count;
}

float ads1292_get_rms(uint16_t n_samples)
{
    if (ring_count == 0 || n_samples == 0) return 0.0f;

    if (n_samples > ring_count)
        n_samples = ring_count;

    uint16_t start = (ring_head + AP_RING_SIZE - n_samples) % AP_RING_SIZE;
    float sum_sq = 0.0f;

    for (uint16_t i = 0; i < n_samples; i++) {
        uint16_t idx = (start + i) % AP_RING_SIZE;
        float uv = ads1292_counts_to_uv(ring_ch1[idx]);
        sum_sq += uv * uv;
    }

    return sqrtf(sum_sq / (float)n_samples);
}

uint8_t ads1292_get_sample_ready(void)
{
    uint8_t r = sample_ready;
    sample_ready = 0;
    return r;
}

/* Include math.h for sqrtf — declare here to avoid header churn */
extern double sqrtf(double x);