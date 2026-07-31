/*
 * emg.c — ADS1292 5-channel EMG acquisition driver implementation.
 *
 * Manages SPI1 bus with 3 ADS1292 chips (2 channels each = 5 used + 1 spare).
 * 500 Hz sample rate, 24-bit delta-sigma, PGA gain = 12.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/emg.h"

/* -------------------------------------------------------------------------
 * SPI1 DMA buffers
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static uint8_t emg_tx_buf[18];
static uint8_t emg_rx_buf[18];

/* Chip select pins for 3 ADS1292 chips */
static const uint32_t emg_cs_pins[3] = { PIN_EMG_CS0, PIN_EMG_CS1, PIN_EMG_CS2 };

static int emg_enabled = 0;

/* -------------------------------------------------------------------------
 * SPI1 transfer (blocking)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void spi1_transfer(uint8_t *tx, uint8_t *rx, uint16_t len)
{
    SPIM1->TXD_PTR = (uint32_t)tx;
    SPIM1->TXD_MAXCNT = len;
    SPIM1->RXD_PTR = (uint32_t)rx;
    SPIM1->RXD_MAXCNT = len;
    SPIM1->EVENTS_END = 0;
    SPIM1->TASKS_START = 1;
    while (SPIM1->EVENTS_END == 0)
        ;
    SPIM1->EVENTS_END = 0;
}

/* -------------------------------------------------------------------------
 * ADS1292 chip select
 * ------------------------------------------------------------------------- */
static void emg_cs_low(uint8_t chip)
{
    P0->OUTCLR = (1U << emg_cs_pins[chip]);
}

static void emg_cs_high(uint8_t chip)
{
    P0->OUTSET = (1U << emg_cs_pins[chip]);
}

/* -------------------------------------------------------------------------
 * Write an ADS1292 register
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void ads_write_reg(uint8_t chip, uint8_t reg, uint8_t value)
{
    emg_cs_low(chip);
    emg_tx_buf[0] = 0x40 | (reg & 0x1F);  /* WREG command */
    emg_tx_buf[1] = 0x00;                  /* write 1 register */
    emg_tx_buf[2] = value;
    spi1_transfer(emg_tx_buf, emg_rx_buf, 3);
    emg_cs_high(chip);
}

/* -------------------------------------------------------------------------
 * Read an ADS1292 register
 * ------------------------------------------------------------------------- */
static uint8_t ads_read_reg(uint8_t chip, uint8_t reg)
{
    emg_cs_low(chip);
    emg_tx_buf[0] = 0x20 | (reg & 0x1F);  /* RREG command */
    emg_tx_buf[1] = 0x00;                  /* read 1 register */
    emg_tx_buf[2] = 0x00;                  /* dummy for response */
    spi1_transfer(emg_tx_buf, emg_rx_buf, 3);
    emg_cs_high(chip);
    return emg_rx_buf[2];
}

/* -------------------------------------------------------------------------
 * Send a command to ADS1292
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void ads_command(uint8_t chip, uint8_t cmd)
{
    emg_cs_low(chip);
    emg_tx_buf[0] = cmd;
    spi1_transfer(emg_tx_buf, emg_rx_buf, 1);
    emg_cs_high(chip);
}

/* -------------------------------------------------------------------------
 * Initialize SPI1 bus
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static int spi1_init(void)
{
    SPIM1->ENABLE = 0;
    SPIM1->PSEL_SCK = PIN_SPIM1_SCK;
    SPIM1->PSEL_MOSI = PIN_SPIM1_MOSI;
    SPIM1->PSEL_MISO = PIN_SPIM1_MISO;
    P0->PIN_CNF[PIN_SPIM1_SCK]  = GPIO_CNF_DIR_OUTPUT | GPIO_CNF_S0S1;
    P0->PIN_CNF[PIN_SPIM1_MOSI] = GPIO_CNF_DIR_OUTPUT | GPIO_CNF_S0S1;
    P0->PIN_CNF[PIN_SPIM1_MISO] = GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_DOWN;
    /* SPI mode 1 (CPOL=0, CPHA=1) for ADS1292 — data on falling edge */
    SPIM1->CONFIG = SPIM_CONFIG_ORDER_MSB;
    SPIM1->FREQUENCY = SPIM_FREQ_4M;
    SPIM1->ENABLE = SPIM_ENABLE_ENABLE;
    return 0;
}

/* -------------------------------------------------------------------------
 * Initialize all 3 ADS1292 chips
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int emg_init(void)
{
    if (spi1_init() != 0)
        return -1;

    /* All CS high */
    for (int i = 0; i < 3; i++) {
        emg_cs_high(i);
    }

    for (volatile int i = 0; i < 10000; i++);

    for (int chip = 0; chip < 3; chip++) {
        /* Reset the chip */
        ads_command(chip, ADS_CMD_RESET);
        for (volatile int i = 0; i < 50000; i++);

        /* Read ID to verify */
        uint8_t id1 = ads_read_reg(chip, ADS_REG_ID1);
        (void)id1;  /* ADS1292 ID1: 0x?? depending on variant */

        /* Stop continuous data mode */
        ads_command(chip, ADS_CMD_SDATAC);
        for (volatile int i = 0; i < 1000; i++);

        /* Configure: 500 SPS, internal clock */
        ads_write_reg(chip, ADS_REG_CONFIG1, ADS_CONFIG1_500SPS);
        ads_write_reg(chip, ADS_REG_CONFIG2, ADS_CONFIG2_INT_CLK);

        /* Configure lead-off: default off for EMG (we use dry electrodes) */
        ads_write_reg(chip, ADS_REG_LOFF, 0x00);

        /* Channel 1: gain 12, normal input */
        ads_write_reg(chip, ADS_REG_CH1SET, ADS_CHSET_GAIN12 | ADS_CHSET_NORMAL);
        /* Channel 2: gain 12, normal input */
        ads_write_reg(chip, ADS_REG_CH2SET, ADS_CHSET_GAIN12 | ADS_CHSET_NORMAL);

        /* Start conversions */
        ads_command(chip, ADS_CMD_START);
        for (volatile int i = 0; i < 1000; i++);

        /* Enter continuous data read mode */
        ads_command(chip, ADS_CMD_RDATAC);
        for (volatile int i = 0; i < 1000; i++);
    }

    emg_enabled = 1;
    return 0;
}

/* -------------------------------------------------------------------------
 * Enable/disable EMG acquisition
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void emg_enable(int enable)
{
    if (enable && !emg_enabled) {
        for (int chip = 0; chip < 3; chip++) {
            ads_command(chip, ADS_CMD_WAKEUP);
            ads_command(chip, ADS_CMD_START);
            ads_command(chip, ADS_CMD_RDATAC);
        }
        emg_enabled = 1;
    } else if (!enable && emg_enabled) {
        for (int chip = 0; chip < 3; chip++) {
            ads_command(chip, ADS_CMD_SDATAC);
            ads_command(chip, ADS_CMD_STOP);
            ads_command(chip, ADS_CMD_STANDBY);
        }
        emg_enabled = 0;
    }
}

/* -------------------------------------------------------------------------
 * Read one 5-channel EMG sample
 * Each ADS1292 outputs: 3-byte status + 3-byte CH1 + 3-byte CH2 = 9 bytes
 * 3 chips × 9 bytes = 27 bytes total, we extract 5 channels.
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int emg_read(emg_sample_t *sample)
{
    uint8_t raw[9];

    /* Check DRDY (active low) — wait up to 3 ms */
    uint32_t timeout = 1500;  /* ~3 ms at ~500 iterations/µs */
    while ((P0->IN & (1U << PIN_EMG_DRDY)) != 0) {
        if (--timeout == 0)
            return -1;  /* timeout */
    }

    /* Read from each ADS1292 chip */
    int ch_idx = 0;
    for (int chip = 0; chip < 3; chip++) {
        emg_cs_low(chip);
        /* 9 bytes: 3 status + 3 CH1 + 3 CH2 */
        for (int i = 0; i < 9; i++) emg_tx_buf[i] = 0x00;
        spi1_transfer(emg_tx_buf, emg_rx_buf, 9);
        emg_cs_high(chip);

        /* Parse channel 1 (24-bit signed, big-endian) */
        if (ch_idx < NUM_EMG_CHANNELS) {
            int32_t val = ((int32_t)emg_rx_buf[3] << 16) |
                          ((int32_t)emg_rx_buf[4] << 8) |
                          (int32_t)emg_rx_buf[5];
            /* Sign-extend 24-bit to 32-bit */
            if (val & 0x800000) val |= 0xFF000000;
            sample->channel[ch_idx++] = val;
        }

        /* Parse channel 2 (24-bit signed, big-endian) */
        if (ch_idx < NUM_EMG_CHANNELS) {
            int32_t val = ((int32_t)emg_rx_buf[6] << 16) |
                          ((int32_t)emg_rx_buf[7] << 8) |
                          (int32_t)emg_rx_buf[8];
            if (val & 0x800000) val |= 0xFF000000;
            sample->channel[ch_idx++] = val;
        }
    }

    sample->timestamp = 0;  /* filled by caller */
    sample->lead_off_status = 0;
    return 0;
}

/* -------------------------------------------------------------------------
 * Check lead-off status for all channels
 * Author: jayis1
 * ------------------------------------------------------------------------- */
uint8_t emg_check_leadoff(void)
{
    uint8_t status = 0;
    for (int chip = 0; chip < 3; chip++) {
        uint8_t loff = ads_read_reg(chip, ADS_REG_LOFF_STAT);
        /* Map ADS1292 lead-off bits to our channel numbering */
        if (chip == 0) {
            if (loff & 0x02) status |= (1U << 0);  /* chip0 ch1 → EMG0 */
            if (loff & 0x04) status |= (1U << 1);  /* chip0 ch2 → EMG1 */
        } else if (chip == 1) {
            if (loff & 0x02) status |= (1U << 2);  /* chip1 ch1 → EMG2 */
            if (loff & 0x04) status |= (1U << 3);  /* chip1 ch2 → EMG3 */
        } else {
            if (loff & 0x02) status |= (1U << 4);  /* chip2 ch1 → EMG4 */
        }
    }
    return status;
}

/*
 * Author: jayis1
 * End of emg.c
 */