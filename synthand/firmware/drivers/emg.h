/*
 * emg.h — ADS1292 5-channel EMG acquisition driver header.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_EMG_H
#define SYNTHAND_EMG_H

#include <stdint.h>
#include "board.h"

/* EMG sample — 5 channels at 500 Hz, 24-bit signed each */
typedef struct {
    int32_t channel[NUM_EMG_CHANNELS]; /* raw 24-bit signed ADC values */
    uint32_t timestamp;                /* sample timestamp (ms) */
    uint8_t  lead_off_status;          /* bit per channel: 1 = lead-off */
} emg_sample_t;

/* Initialize SPI1 bus and all 3 ADS1292 chips.
 * Configures PGA gain = 12, sample rate = 500 SPS per channel.
 * Returns 0 on success, nonzero on error. */
int emg_init(void);

/* Enable/disable EMG acquisition.
 * When enabled, ADS1292 is in continuous conversion mode.
 * When disabled, enters standby (1 µA per chip). */
void emg_enable(int enable);

/* Read one 5-channel EMG sample (reads all 3 ADS1292 chips).
 * Blocks until DRDY or timeout (3 ms).
 * Returns 0 on success, nonzero on error/timeout. */
int emg_read(emg_sample_t *sample);

/* Perform lead-off detection check.
 * Returns bitmask of channels with lead-off (bit 0 = ch0). */
uint8_t emg_check_leadoff(void);

/* ADS1292 register addresses */
#define ADS_REG_ID1         0x00
#define ADS_REG_ID2         0x01
#define ADS_REG_CONFIG1     0x01
#define ADS_REG_CONFIG2     0x02
#define ADS_REG_LOFF        0x03
#define ADS_REG_CH1SET      0x04
#define ADS_REG_CH2SET      0x05
#define ADS_REG_LOFF_SENS   0x06
#define ADS_REG_LOFF_STAT   0x07
#define ADS_REG_RESP1       0x08
#define ADS_REG_RESP2       0x09
#define ADS_REG_GPIO        0x0A

/* Commands */
#define ADS_CMD_WAKEUP      0x02
#define ADS_CMD_STANDBY     0x04
#define ADS_CMD_RESET       0x06
#define ADS_CMD_START       0x08
#define ADS_CMD_STOP        0x0A
#define ADS_CMD_RDATAC      0x10
#define ADS_CMD_SDATAC      0x11
#define ADS_CMD_RDATA       0x12

/* Config1: 500 SPS, continuous mode */
#define ADS_CONFIG1_500SPS  0x00  /* HR mode, 500 SPS */
#define ADS_CONFIG2_INT_CLK 0x80  /* internal clock, test signal off */

/* CHnSET: gain 12, normal input */
#define ADS_CHSET_GAIN12    0x60  /* GAIN=12 (bits 6:4) */
#define ADS_CHSET_NORMAL    0x00  /* normal electrode input */

#endif /* SYNTHAND_EMG_H */