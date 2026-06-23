/*
 * ads1298.c — ADS1298 dual biomedical ADC driver
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Driver for two Texas Instruments ADS1298 biopotential measurement
 * front-end ICs, providing 16 channels of 24-bit delta-sigma ADC data
 * with integrated PGA and lead-off detection.  The two devices share a
 * common START, RESET, and PWDN signal but have separate chip selects
 * on the SPI bus.  Data-ready (DRDY) is tied together and polled.
 *
 * The ADS1298 outputs 24-bit signed samples in a status-first frame:
 *   [3B status] [3B ch1] [3B ch2] ... [3B ch8]
 * per device.  For two devices, we read sequentially with separate CS.
 */

#include "ads1298.h"
#include "registers.h"

/* ===================================================================== */
/*  SPI helpers                                                           */
/* ===================================================================== */

static void spi1_transfer(uint8_t *tx, uint8_t *rx, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        while (!(SPI1->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&SPI1->DR = tx ? tx[i] : 0xFF;
        while (!(SPI1->SR & SPI_SR_RXNE));
        if (rx) rx[i] = *(volatile uint8_t *)&SPI1->DR;
        else (void)*(volatile uint8_t *)&SPI1->DR;
    }
    while (SPI1->SR & SPI_SR_BSY);
}

static uint8_t spi1_byte(uint8_t tx)
{
    while (!(SPI1->SR & SPI_SR_TXE));
    *(volatile uint8_t *)&SPI1->DR = tx;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return *(volatile uint8_t *)&SPI1->DR;
}

/* ===================================================================== */
/*  Chip select helpers                                                   */
/* ===================================================================== */

static void cs_low(uint8_t dev)
{
    if (dev == 0)
        GPIO_RESET(ADS1298_CS0_PORT, ADS1298_CS0_PIN);
    else
        GPIO_RESET(ADS1298_CS1_PORT, ADS1298_CS1_PIN);
}

static void cs_high(uint8_t dev)
{
    if (dev == 0)
        GPIO_SET(ADS1298_CS0_PORT, ADS1298_CS0_PIN);
    else
        GPIO_SET(ADS1298_CS1_PORT, ADS1298_CS1_PIN);
}

static void wait_drdy(void)
{
    uint32_t timeout = 1000000;
    while (GPIO_READ(ADS1298_DRDY_PORT, ADS1298_DRDY_PIN) && --timeout);
}

/* ===================================================================== */
/*  Register access                                                       */
/* ===================================================================== */

void ads1298_write_reg(uint8_t dev, uint8_t reg, uint8_t value)
{
    cs_low(dev);
    spi1_byte(ADS1298_CMD_WREG | (reg & 0x1F));  /* WREG + reg addr */
    spi1_byte(0x00);                               /* write 1 register */
    spi1_byte(value);
    cs_high(dev);
    /* Small delay for register write to settle. */
    for (volatile int i = 0; i < 100; i++);
}

uint8_t ads1298_read_reg(uint8_t dev, uint8_t reg)
{
    cs_low(dev);
    spi1_byte(ADS1298_CMD_RREG | (reg & 0x1F));
    spi1_byte(0x00);  /* read 1 register */
    for (volatile int i = 0; i < 50; i++);  /* tSDECODE delay */
    uint8_t val = spi1_byte(0xFF);
    cs_high(dev);
    for (volatile int i = 0; i < 100; i++);
    return val;
}

/* ===================================================================== */
/*  Initialization                                                        */
/* ===================================================================== */

int ads1298_init(void)
{
    /* Power up the ADS1298 pair. */
    GPIO_SET(ADS1298_PWDN_PORT, ADS1298_PWDN_PIN);
    for (volatile int i = 0; i < 100000; i++);  /* ~1ms power-up delay */

    /* Hardware reset pulse (active low, min 2 tCLK = 1 µs). */
    GPIO_RESET(ADS1298_RESET_PORT, ADS1298_RESET_PIN);
    for (volatile int i = 0; i < 1000; i++);
    GPIO_SET(ADS1298_RESET_PORT, ADS1298_RESET_PIN);
    for (volatile int i = 0; i < 100000; i++);  /* wait for reset */

    /* Send software RESET command to both devices. */
    for (uint8_t dev = 0; dev < 2; dev++) {
        cs_low(dev);
        spi1_byte(ADS1298_CMD_RESET);
        for (volatile int i = 0; i < 100000; i++);  /* tRESET delay */
        cs_high(dev);
    }

    /* Stop continuous read mode on both devices. */
    for (uint8_t dev = 0; dev < 2; dev++) {
        cs_low(dev);
        spi1_byte(ADS1298_CMD_SDATAC);
        for (volatile int i = 0; i < 100; i++);
        cs_high(dev);
    }

    /* Read device ID to verify presence. */
    uint8_t id0 = ads1298_read_reg(0, ADS1298_REG_ID);
    uint8_t id1 = ads1298_read_reg(1, ADS1298_REG_ID);

    /* ADS1298 ID: 0x3E (bits [7:5] = 111 for ADS1298, [4:1] = 1111 for 8ch). */
    if ((id0 & 0xFE) != 0x3E || (id1 & 0xFE) != 0x3E) {
        return -1;  /* device not detected */
    }

    /* Configure both devices. */
    for (uint8_t dev = 0; dev < 2; dev++) {
        /* CONFIG3: enable internal reference, bias, internal signal. */
        ads1298_write_reg(dev, ADS1298_REG_CONFIG3, 0xE0);

        /* CONFIG1: set sample rate to 1 kSPS (HR mode, 0x60 = 1000 SPS).
           Bits [6:0] = sample rate code: 0x60 = 1 kSPS high-res. */
        ads1298_write_reg(dev, ADS1298_REG_CONFIG1, 0x60);

        /* CONFIG2: internal test signal off, WCT disabled. */
        ads1298_write_reg(dev, ADS1298_REG_CONFIG2, 0x80);

        /* CONFIG4: WCT disabled, continuous RDATAC mode. */
        ads1298_write_reg(dev, ADS1298_REG_CONFIG4, 0x00);

        /* Configure all 8 channels: gain = 12, normal input, MUX normal.
           CHnSET: [7:6] PD(00=normal), [5:3] MUX(000=normal), [2:0] GAIN.
           PGA gain 12 = code 0x06. */
        for (uint8_t ch = 0; ch < 8; ch++) {
            ads1298_write_reg(dev, ADS1298_REG_CH1SET + ch, 0x06);
        }

        /* LOFF: enable lead-off detection, magnitude 6 nA, 31.2 Hz. */
        ads1298_write_reg(dev, ADS1298_REG_LOFF, 0x0C);

        /* BIASSENSP/BIASSENSN: use channel 1 positive and negative for bias. */
        ads1298_write_reg(dev, ADS1298_REG_BIASSENSP, 0x01);
        ads1298_write_reg(dev, ADS1298_REG_BIASSENSN, 0x01);
    }

    return 0;
}

/* ===================================================================== */
/*  Acquisition control                                                   */
/* ===================================================================== */

void ads1298_start(void)
{
    /* Enter continuous read mode on both devices. */
    for (uint8_t dev = 0; dev < 2; dev++) {
        cs_low(dev);
        spi1_byte(ADS1298_CMD_RDATAC);
        for (volatile int i = 0; i < 100; i++);
        cs_high(dev);
    }

    /* Assert START pin (both devices share it). */
    GPIO_SET(ADS1298_START_PORT, ADS1298_START_PIN);
    for (volatile int i = 0; i < 1000; i++);
    GPIO_RESET(ADS1298_START_PORT, ADS1298_START_PIN);
}

void ads1298_stop(void)
{
    /* Stop continuous read mode. */
    for (uint8_t dev = 0; dev < 2; dev++) {
        cs_low(dev);
        spi1_byte(ADS1298_CMD_SDATAC);
        for (volatile int i = 0; i < 100; i++);
        cs_high(dev);
    }
    /* Send STOP command. */
    for (uint8_t dev = 0; dev < 2; dev++) {
        cs_low(dev);
        spi1_byte(ADS1298_CMD_STOP);
        cs_high(dev);
    }
}

void ads1298_power_down(void)
{
    /* Stop acquisition first. */
    ads1298_stop();

    /* Assert PWDN (active low power-down). */
    GPIO_RESET(ADS1298_PWDN_PORT, ADS1298_PWDN_PIN);
}

void ads1298_wake(void)
{
    /* Deassert PWDN. */
    GPIO_SET(ADS1298_PWDN_PORT, ADS1298_PWDN_PIN);
    for (volatile int i = 0; i < 100000; i++);  /* power-up delay */

    /* Hardware reset. */
    GPIO_RESET(ADS1298_RESET_PORT, ADS1298_RESET_PIN);
    for (volatile int i = 0; i < 1000; i++);
    GPIO_SET(ADS1298_RESET_PORT, ADS1298_RESET_PIN);
    for (volatile int i = 0; i < 100000; i++);

    /* Re-initialize registers (power-down loses config). */
    ads1298_init();
}

/* ===================================================================== */
/*  Data reading                                                          */
/* ===================================================================== */

/* Convert 24-bit signed (3 bytes, MSB first) to int16_t (right-shifted by 8).
 * The ADS1298 outputs 24-bit data; we reduce to 16-bit by dropping the
 * least significant byte (still 6.5 µV resolution at gain 12, ±2.5V range). */
static int16_t convert_24bit(uint8_t *raw)
{
    int32_t val = ((int32_t)raw[0] << 16) | ((int32_t)raw[1] << 8) | raw[2];
    /* Sign-extend from 24-bit to 32-bit. */
    if (val & 0x800000) val |= 0xFF000000;
    /* Right-shift by 8 to fit in 16-bit (preserves sign). */
    return (int16_t)(val >> 8);
}

int ads1298_read_sample(int16_t samples[ADS1298_TOTAL_CHANS])
{
    uint8_t raw[27];  /* 3B status + 8×3B data = 27 bytes per device */

    wait_drdy();
    if (GPIO_READ(ADS1298_DRDY_PORT, ADS1298_DRDY_PIN))
        return -1;  /* timeout */

    /* Read device 0 (channels 0–7). */
    cs_low(0);
    spi1_transfer(NULL, raw, 27);
    cs_high(0);

    for (uint8_t ch = 0; ch < 8; ch++) {
        samples[ch] = convert_24bit(&raw[3 + ch * 3]);
    }

    /* Read device 1 (channels 8–15). */
    cs_low(1);
    spi1_transfer(NULL, raw, 27);
    cs_high(1);

    for (uint8_t ch = 0; ch < 8; ch++) {
        samples[8 + ch] = convert_24bit(&raw[3 + ch * 3]);
    }

    return 0;
}

int ads1298_read_block(int16_t buf[ADS1298_TOTAL_CHANS][DSP_BLOCK_SIZE],
                       uint16_t n_samples)
{
    int16_t sample[ADS1298_TOTAL_CHANS];

    for (uint16_t s = 0; s < n_samples; s++) {
        if (ads1298_read_sample(sample) != 0)
            return -1;

        for (uint8_t ch = 0; ch < ADS1298_TOTAL_CHANS; ch++) {
            buf[ch][s] = sample[ch];
        }
    }
    return 0;
}

/* ===================================================================== */
/*  Lead-off detection                                                    */
/* ===================================================================== */

void ads1298_check_lead_off(uint8_t lead_off[ADS1298_TOTAL_CHANS])
{
    /* Read lead-off status registers from both devices. */
    uint8_t statp0 = ads1298_read_reg(0, ADS1298_REG_LOFFSTATP);
    uint8_t statn0 = ads1298_read_reg(0, ADS1298_REG_LOFFSTATN);
    uint8_t statp1 = ads1298_read_reg(1, ADS1298_REG_LOFFSTATP);
    uint8_t statn1 = ads1298_read_reg(1, ADS1298_REG_LOFFSTATN);

    for (uint8_t ch = 0; ch < 8; ch++) {
        /* A channel is "off" if either positive or negative lead is flagged. */
        uint8_t mask = (1U << ch);
        lead_off[ch] = (statp0 & mask) || (statn0 & mask) ? 1 : 0;
        lead_off[8 + ch] = (statp1 & mask) || (statn1 & mask) ? 1 : 0;
    }
}

/* ===================================================================== */
/*  Gain and sample rate configuration                                    */
/* ===================================================================== */

void ads1298_set_gain(uint8_t gain)
{
    /* ADS1298 PGA gain codes: 1→0, 2→1, 3→2, 4→3, 6→4, 8→5, 12→6. */
    static const uint8_t gain_codes[] = {0, 1, 2, 3, 4, 5, 6};
    uint8_t code = 6;  /* default 12× */
    for (uint8_t i = 0; i < 7; i++) {
        if (gain_codes[i] == gain) { code = gain; break; }
    }

    for (uint8_t dev = 0; dev < 2; dev++) {
        for (uint8_t ch = 0; ch < 8; ch++) {
            uint8_t reg = ads1298_read_reg(dev, ADS1298_REG_CH1SET + ch);
            reg = (reg & 0xF8) | (code & 0x07);
            ads1298_write_reg(dev, ADS1298_REG_CH1SET + ch, reg);
        }
    }
}

void ads1298_set_sample_rate(uint16_t sps)
{
    /* CONFIG1[6:0] sample rate codes for high-resolution mode:
       0x60=1kSPS, 0x40=500, 0x20=250, 0x00=125, 0x80=2k, 0x90=4k, ... */
    uint8_t code;
    switch (sps) {
        case 125:   code = 0x00; break;
        case 250:   code = 0x20; break;
        case 500:   code = 0x40; break;
        case 1000:  code = 0x60; break;
        case 2000:  code = 0x80; break;
        case 4000:  code = 0x90; break;
        case 8000:  code = 0xA0; break;
        case 16000: code = 0xB0; break;
        default:    code = 0x60; break;  /* default 1 kSPS */
    }

    /* Set HR bit (bit 7 = 0 for high-resolution). */
    for (uint8_t dev = 0; dev < 2; dev++) {
        ads1298_write_reg(dev, ADS1298_REG_CONFIG1, code);
    }
}

void ads1298_calibrate_offset(void)
{
    /* Stop RDATAC mode before calibration. */
    for (uint8_t dev = 0; dev < 2; dev++) {
        cs_low(dev);
        spi1_byte(ADS1298_CMD_SDATAC);
        cs_high(dev);
    }

    /* Send offset calibration command. */
    for (uint8_t dev = 0; dev < 2; dev++) {
        cs_low(dev);
        spi1_byte(ADS1298_CMD_OFFSETCAL);
        for (volatile int i = 0; i < 500000; i++);  /* wait for calibration */
        cs_high(dev);
    }

    /* Resume RDATAC mode. */
    for (uint8_t dev = 0; dev < 2; dev++) {
        cs_low(dev);
        spi1_byte(ADS1298_CMD_RDATAC);
        cs_high(dev);
    }
}