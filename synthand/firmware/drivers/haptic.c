/*
 * haptic.c — DRV2605L haptic driver implementation for 5 fingertip LRAs.
 *
 * Manages I²C bus (TWIM0) with 5 DRV2605L chips at addresses 0x5A–0x5E.
 * Each chip drives one PSG-0508 LRA on a fingertip.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"
#include "drivers/haptic.h"

/* -------------------------------------------------------------------------
 * I²C (TWIM0) transfer
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static int twim_write(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };

    TWIM0->ADDRESS = addr;
    TWIM0->TXD_PTR = (uint32_t)buf;
    TWIM0->TXD_MAXCNT = 2;
    TWIM0->EVENTS_STOPPED = 0;
    TWIM0->EVENTS_ERROR = 0;
    TWIM0->TASKS_STARTTX = 1;

    /* Wait for stop */
    uint32_t timeout = 10000;
    while (TWIM0->EVENTS_STOPPED == 0 && TWIM0->EVENTS_ERROR == 0) {
        if (--timeout == 0) return -1;
    }

    if (TWIM0->EVENTS_ERROR) {
        TWIM0->EVENTS_ERROR = 0;
        return -2;
    }

    return 0;
}

static int twim_read(uint8_t addr, uint8_t reg, uint8_t *value)
{
    /* Write register address, then read 1 byte */
    TWIM0->ADDRESS = addr;
    TWIM0->TXD_PTR = (uint32_t)&reg;
    TWIM0->TXD_MAXCNT = 1;
    TWIM0->RXD_PTR = (uint32_t)value;
    TWIM0->RXD_MAXCNT = 1;
    TWIM0->EVENTS_STOPPED = 0;
    TWIM0->EVENTS_ERROR = 0;
    TWIM0->SHORTS = (1U << 0);  /* suspend after TX, then restart for RX */
    TWIM0->TASKS_STARTTX = 1;

    uint32_t timeout = 10000;
    while (TWIM0->EVENTS_STOPPED == 0 && TWIM0->EVENTS_ERROR == 0) {
        if (--timeout == 0) return -1;
    }

    TWIM0->SHORTS = 0;

    if (TWIM0->EVENTS_ERROR) {
        TWIM0->EVENTS_ERROR = 0;
        return -2;
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Initialize TWIM0 bus
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static int twim0_init(void)
{
    TWIM0->ENABLE = 0;
    TWIM0->PSEL_SCL = PIN_TWIM0_SCL;
    TWIM0->PSEL_SDA = PIN_TWIM0_SDA;
    P0->PIN_CNF[PIN_TWIM0_SCL] = GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_UP;
    P0->PIN_CNF[PIN_TWIM0_SDA] = GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_UP;
    TWIM0->FREQUENCY = TWIM_FREQ_400K;
    TWIM0->ENABLE = TWIM_ENABLE_ENABLE;
    return 0;
}

/* -------------------------------------------------------------------------
 * Initialize all 5 DRV2605L haptic drivers
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int haptic_init(void)
{
    if (twim0_init() != 0)
        return -1;

    /* Configure each DRV2605L chip */
    for (int i = 0; i < NUM_HAPTIC; i++) {
        uint8_t addr = DRV2605L_ADDR(i);

        /* Reset the chip by writing 0x01 to MODE, then 0x00 */
        twim_write(addr, DRV_REG_MODE, 0x01);
        for (volatile int j = 0; j < 1000; j++);
        twim_write(addr, DRV_REG_MODE, 0x00);

        /* Set LRA mode in feedback register */
        twim_write(addr, DRV_REG_FEEDBACK, DRV_FEEDBACK_LRA);

        /* Select LRA library */
        twim_write(addr, DRV_REG_LIBRARY, DRV_LIBRARY_LRA);

        /* Set rated voltage (LRA: ~1.5V, in 22.36 mV units → 67) */
        twim_write(addr, DRV_REG_RATEDVOLT, 67);

        /* Set overdrive clamp voltage (~2.0V → 90) */
        twim_write(addr, DRV_REG_OVERDRIVECLAMP, 90);

        /* Set default waveform sequence: waveform 1 = none, rest = none */
        twim_write(addr, DRV_REG_WAVESEQ1, 0);
        twim_write(addr, DRV_REG_WAVESEQ2, 0);

        /* Auto-calibration mode (one-time) */
        twim_write(addr, DRV_REG_MODE, DRV_MODE_CALIBRATE);

        /* Start calibration */
        twim_write(addr, DRV_REG_GO, 1);

        /* Wait for calibration to complete (status bit 3 = diagnostic result) */
        uint32_t timeout = 100000;
        uint8_t status = 0;
        do {
            twim_read(addr, DRV_REG_STATUS, &status);
            if (--timeout == 0) break;
        } while ((status & 0x08) == 0);  /* wait for device ready */

        /* Return to internal trigger mode */
        twim_write(addr, DRV_REG_MODE, DRV_MODE_INT_TRIG);
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Enable/disable haptic power gate
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void haptic_enable(int enable)
{
    if (enable) {
        P0->OUTSET = (1U << PIN_HAPTIC_EN);
    } else {
        P0->OUTCLR = (1U << PIN_HAPTIC_EN);
    }
}

/* -------------------------------------------------------------------------
 * Trigger a haptic waveform on a specific finger
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int haptic_trigger(uint8_t finger, uint8_t waveform_id)
{
    if (finger >= NUM_HAPTIC)
        return -1;

    uint8_t addr = DRV2605L_ADDR(finger);

    /* Set waveform sequence 1 to the desired waveform, sequence 2 to end */
    int ret = twim_write(addr, DRV_REG_WAVESEQ1, waveform_id);
    if (ret != 0) return ret;
    twim_write(addr, DRV_REG_WAVESEQ2, 0);

    /* Trigger playback */
    twim_write(addr, DRV_REG_GO, 1);

    return 0;
}

/* -------------------------------------------------------------------------
 * Set real-time playback amplitude (for continuous vibration)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int haptic_set_rtp(uint8_t finger, uint8_t amplitude)
{
    if (finger >= NUM_HAPTIC)
        return -1;

    uint8_t addr = DRV2605L_ADDR(finger);

    /* Switch to RTP mode */
    twim_write(addr, DRV_REG_MODE, DRV_MODE_RTP);

    /* Set RTP amplitude */
    twim_write(addr, DRV_REG_RTP_INPUT, amplitude);

    return 0;
}

/* -------------------------------------------------------------------------
 * Stop all haptic vibration
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void haptic_stop_all(void)
{
    for (int i = 0; i < NUM_HAPTIC; i++) {
        uint8_t addr = DRV2605L_ADDR(i);
        twim_write(addr, DRV_REG_MODE, DRV_MODE_INT_TRIG);
        twim_write(addr, DRV_REG_GO, 0);
        twim_write(addr, DRV_REG_RTP_INPUT, 0);
    }
}

/*
 * Author: jayis1
 * End of haptic.c
 */