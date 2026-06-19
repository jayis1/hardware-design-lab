/*
 * receiver.c — ADS131M08 24-bit ADC Driver (I/Q Capture)
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements a software-SPI driver for the Texas Instruments ADS131M08
 * 8-channel simultaneous-sampling 24-bit delta-sigma ADC.  The ADS131M08
 * provides 8 channels at 8 kSPS with a serial SPI interface.
 *
 * In the SFCW radar architecture, the ADS131M08 captures the I and Q
 * baseband outputs of the HMC595 I/Q demodulator.  At each frequency step,
 * the firmware waits for the DRDY signal (PB10, EXTI10), then reads
 * channels 0 (I) and 1 (Q) via SPI.
 *
 * The ADC uses a 2.5V internal reference and outputs 24-bit signed values.
 * The conversion to voltage is: V = raw / 8388607 × 2.5 / PGA_gain.
 *
 * SPI protocol:
 *  - DRDY goes low when new data is ready
 *  - Pull nCS low
 *  - Read 8 channels × 3 bytes = 24 bytes of data (+ 3 status bytes = 27 total)
 *  - Each channel is 24-bit signed MSB first
 *  - Pull nCS high
 */

#include "receiver.h"
#include "../board.h"
#include "../registers.h"

/* ===================================================================== */
/*  Software SPI bit-bang for ADS131M08                                  */
/* ===================================================================== */

static void adc_sck_pulse(void)
{
    ADC_SCK_PORT->BRR  = (1UL << ADC_SCK_PIN);
    for (volatile int d = 0; d < 1; d++) { __asm volatile("nop"); }
    ADC_SCK_PORT->BSRR = (1UL << ADC_SCK_PIN);
    for (volatile int d = 0; d < 1; d++) { __asm volatile("nop"); }
    ADC_SCK_PORT->BRR  = (1UL << ADC_SCK_PIN);
}

static uint8_t adc_spi_read_byte(void)
{
    uint8_t val = 0;
    for (int i = 7; i >= 0; i--) {
        adc_sck_pulse();
        if (ADC_MISO_PORT->IDR & (1UL << ADC_MISO_PIN))
            val |= (1U << i);
    }
    return val;
}

static void adc_spi_write_byte(uint8_t val)
{
    for (int i = 7; i >= 0; i--) {
        if (val & (1U << i))
            ADC_MOSI_PORT->BSRR = (1UL << ADC_MOSI_PIN);
        else
            ADC_MOSI_PORT->BRR  = (1UL << ADC_MOSI_PIN);
        adc_sck_pulse();
    }
}

/* ===================================================================== */
/*  ADC register access                                                   */
/* ===================================================================== */

/*
 * ADS131M08 command format (16-bit):
 *   [1 | W/R(1) | ADDR(7) | DATA/CRC(7)]
 * Read:  0x4000 | (addr << 7)
 * Write: 0x0000 | (addr << 7) | data
 * Null:  0x0000 (NOP)
 * Reset: 0x0011
 * Standby: 0x0022
 * Wakeup:  0x0033
 * Lock:   0x0555
 * Unlock: 0x0655
 */

#define ADC_CMD_NULL    0x0000
#define ADC_CMD_RESET   0x0011
#define ADC_CMD_STANDBY 0x0022
#define ADC_CMD_WAKEUP  0x0033
#define ADC_CMD_LOCK    0x0555
#define ADC_CMD_UNLOCK  0x0655

/* Register addresses */
#define ADC_REG_CLOCK     0x14
#define ADC_REG_GAIN      0x19
#define ADC_REG_CFG        0x11

static void adc_send_command(uint16_t cmd)
{
    ADC_NCS_PORT->BRR = (1UL << ADC_NCS_PIN);   /* nCS low */
    adc_spi_write_byte((cmd >> 8) & 0xFF);
    adc_spi_write_byte(cmd & 0xFF);
    /* Read back the response (NULL response for commands) */
    (void)adc_spi_read_byte();
    (void)adc_spi_read_byte();
    ADC_NCS_PORT->BSRR = (1UL << ADC_NCS_PIN);  /* nCS high */
}

static uint16_t adc_read_register(uint8_t addr)
{
    uint16_t cmd = 0x4000 | ((uint16_t)(addr & 0x7F) << 7);
    uint16_t resp;

    ADC_NCS_PORT->BRR  = (1UL << ADC_NCS_PIN);
    adc_spi_write_byte((cmd >> 8) & 0xFF);
    adc_spi_write_byte(cmd & 0xFF);
    /* Response comes on next frame — read NULL frame first, then read */
    (void)adc_spi_read_byte();
    (void)adc_spi_read_byte();
    /* Second frame: read response */
    uint8_t hi = adc_spi_read_byte();
    uint8_t lo = adc_spi_read_byte();
    resp = ((uint16_t)hi << 8) | lo;
    (void)adc_spi_read_byte();
    (void)adc_spi_read_byte();
    ADC_NCS_PORT->BSRR = (1UL << ADC_NCS_PIN);

    return resp;
}

static void adc_write_register(uint8_t addr, uint8_t data)
{
    /* Write command: W=0, addr, data */
    uint16_t cmd = ((uint16_t)(addr & 0x7F) << 7) | (data & 0x7F);
    /* Actually the write format is: bit15=0, bits 14:8 = addr, 7:0 = data
     * But ADS131M08 uses 16-bit commands where:
     *   write: 0 | addr(7) | data(8) ... the exact layout:
     *   Bit 15: 0 (write)
     *   Bits 14:8: address (7 bits)
     *   Bits 7:0: data (8 bits)
     */
    cmd = ((uint16_t)(addr & 0x7F) << 8) | (data & 0xFF);
    /* Send write command */
    ADC_NCS_PORT->BRR = (1UL << ADC_NCS_PIN);
    adc_spi_write_byte((cmd >> 8) & 0xFF);
    adc_spi_write_byte(cmd & 0xFF);
    /* Read response (previous frame's response) */
    (void)adc_spi_read_byte();
    (void)adc_spi_read_byte();
    ADC_NCS_PORT->BSRR = (1UL << ADC_NCS_PIN);
}

/* ===================================================================== */
/*  DRDY wait                                                              */
/* ===================================================================== */

static void adc_wait_drdy(void)
{
    /* DRDY is active low on PB10.  Poll until it goes low.
     * In a production implementation, this uses EXTI10 interrupt + DMA.
     */
    uint32_t timeout = 10000;  /* max 10k iterations */
    while ((ADC_DRDY_PORT->IDR & (1UL << ADC_DRDY_PIN)) && timeout) {
        timeout--;
    }
}

/* ===================================================================== */
/*  Read all 8 channels (24-bit signed each)                              */
/* ===================================================================== */

static void adc_read_all_channels(int32_t *channels)
{
    /* The ADS131M08 data frame:
     * - 3 bytes: status (NULL + status word)
     * - 8 channels × 3 bytes = 24 bytes of data
     * - CRC (optional, disabled)
     * Total: 3 + 24 = 27 bytes per frame (without CRC)
     */
    ADC_NCS_PORT->BRR = (1UL << ADC_NCS_PIN);  /* nCS low */

    /* Read status bytes (3) */
    for (int i = 0; i < 3; i++)
        (void)adc_spi_read_byte();

    /* Read 8 channels × 3 bytes each */
    for (int ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
        uint8_t b0 = adc_spi_read_byte();  /* MSB */
        uint8_t b1 = adc_spi_read_byte();
        uint8_t b2 = adc_spi_read_byte();  /* LSB */

        /* Combine into 24-bit signed value */
        int32_t raw = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | (int32_t)b2;
        /* Sign-extend from 24-bit to 32-bit */
        if (raw & 0x800000)
            raw |= 0xFF000000;

        channels[ch] = raw;
    }

    ADC_NCS_PORT->BSRR = (1UL << ADC_NCS_PIN);  /* nCS high */
}

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

int receiver_init(void)
{
    /* Hardware reset the ADC */
    ADC_NRST_PORT->BRR  = (1UL << ADC_NRST_PIN);  /* reset low */
    board_delay_ms(10);
    ADC_NRST_PORT->BSRR = (1UL << ADC_NRST_PIN);   /* release reset */
    board_delay_ms(10);

    /* nCS high initially */
    ADC_NCS_PORT->BSRR = (1UL << ADC_NCS_PIN);

    /* Send RESET command */
    adc_send_command(ADC_CMD_RESET);
    board_delay_ms(5);

    /* Send WAKEUP command */
    adc_send_command(ADC_CMD_WAKEUP);
    board_delay_ms(2);

    /* Configure clock register: set sample rate to 8 kSPS
     * The CLOCK register selects the OSR and clock divider.
     * OSR = 256 → 8 kSPS with 2.048 MHz clock.
     */
    adc_write_register(ADC_REG_CLOCK, 0x0F);  /* OSR=256, high-res mode */

    /* Configure gain: set all channels to gain=1 (PGA bypass)
     * GAIN register: each channel has 3-bit gain select
     * 000 = 1x (PGA bypass), 001 = 2x, ..., 111 = 128x
     * 8 channels × 3 bits = 24 bits = 3 bytes
     */
    adc_write_register(ADC_REG_GAIN, 0x00);   /* all channels gain=1 */

    /* Configure CFG register: enable DRDY pin, fixed mode */
    adc_write_register(ADC_REG_CFG, 0x01);    /* DRDY pin enabled */

    board_delay_ms(5);
    return 0;
}

void receiver_read_iq(float *i_out, float *q_out)
{
    int32_t channels[ADC_NUM_CHANNELS];

    /* Wait for DRDY */
    adc_wait_drdy();

    /* Read all 8 channels */
    adc_read_all_channels(channels);

    /* Convert to voltage: V = raw / FULLSCALE × VREF / gain */
    float gain = 1.0f;  /* PGA gain = 1 */
    float scale = (ADC_VREF_V / (ADC_FULLSCALE * gain));

    *i_out = (float)channels[ADC_CH_I] * scale;
    *q_out = (float)channels[ADC_CH_Q] * scale;
}

void receiver_read_raw(int32_t *i_raw, int32_t *q_raw)
{
    int32_t channels[ADC_NUM_CHANNELS];
    adc_wait_drdy();
    adc_read_all_channels(channels);
    *i_raw = channels[ADC_CH_I];
    *q_raw = channels[ADC_CH_Q];
}

void receiver_reset(void)
{
    adc_send_command(ADC_CMD_RESET);
    board_delay_ms(5);
    adc_send_command(ADC_CMD_WAKEUP);
    board_delay_ms(2);
}

void receiver_set_gain(uint8_t channel, uint8_t gain)
{
    /* The GAIN register is 3 bytes (24 bits), 3 bits per channel */
    if (channel >= ADC_NUM_CHANNELS || gain > 7)
        return;
    /* Read current gain, modify the channel's 3 bits, write back */
    /* Simplified: write all-gains at once for channel 0-7
     * This is a simplified version; a real driver reads-modifies-writes.
     */
    uint8_t current = adc_read_register(ADC_REG_GAIN) & 0xFF;
    uint8_t mask = 0x07 << (channel * 3 % 8);
    uint8_t new_val = (current & ~mask) | ((gain << (channel * 3 % 8)) & mask);
    adc_write_register(ADC_REG_GAIN, new_val);
}

/* End of receiver.c */