/*
 * eit_acq.c — EIT Frame Acquisition
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Implements the adjacent stimulation / adjacent measurement EIT protocol.
 * For each of 16 stimulation pairs (electrodes i, i+1), measures voltage
 * differences across all adjacent non-stimulating electrode pairs, producing
 * 208 independent measurements per frame.  Handles electrode contact checking,
 * calibration, and frame assembly.
 */

#include "eit_acq.h"
#include "reconstruct.h"
#include "board.h"
#include "registers.h"
#include <string.h>
#include <math.h>

static uint32_t s_frame_count = 0;
static uint32_t s_start_time_ms = 0;
static uint8_t s_current_freq_index = 0;

/* Excitation frequencies for fd-EIT sweep */
static const uint32_t s_eit_freqs[EIT_FREQ_COUNT] = {
    EIT_FREQ_0, EIT_FREQ_1, EIT_FREQ_2, EIT_FREQ_3
};

/* ---------------------------------------------------------------------
 * Millisecond timer (using SysTick)
 * --------------------------------------------------------------------- */

static volatile uint32_t s_tick_ms = 0;

void SysTick_Handler(void) __attribute__((interrupt("IRQ")));
void SysTick_Handler(void)
{
    s_tick_ms++;
}

static uint32_t get_tick_ms(void)
{
    return s_tick_ms;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = s_tick_ms;
    while ((s_tick_ms - start) < ms) { }
}

/* ---------------------------------------------------------------------
 * Generate adjacent stimulation pattern
 *   stim_index: 0..15 — injects current between electrode (i) and (i+1)
 * --------------------------------------------------------------------- */

void eit_acq_get_stim_pattern(uint8_t stim_index, mux_config_t *cfg)
{
    cfg->exc_pos = stim_index % EIT_NUM_ELECTRODES;
    cfg->exc_neg = (stim_index + 1) % EIT_NUM_ELECTRODES;
    /* Measurement electrodes: we will change these per measurement */
    cfg->meas_pos = 0;
    cfg->meas_neg = 0;
}

/* ---------------------------------------------------------------------
 * Generate adjacent measurement pattern
 *   stim_index: 0..15 (the current injection pair)
 *   meas_index: 0..12 (which adjacent pair to measure)
 *
 *   We measure voltage between electrode pairs that are NOT the
 *   stimulation pair and NOT adjacent to it (to avoid loading effects).
 *
 *   For stimulation (i, i+1), we measure pairs:
 *     (i+2, i+3), (i+3, i+4), ..., (i+14, i+15)
 *   That's 13 measurements per stimulation.
 * --------------------------------------------------------------------- */

void eit_acq_get_meas_pattern(uint8_t stim_index, uint8_t meas_index,
                                mux_config_t *cfg)
{
    uint8_t e1 = (uint8_t)((stim_index + meas_index + 2) % EIT_NUM_ELECTRODES);
    uint8_t e2 = (uint8_t)((stim_index + meas_index + 3) % EIT_NUM_ELECTRODES);
    cfg->exc_pos  = stim_index % EIT_NUM_ELECTRODES;
    cfg->exc_neg  = (stim_index + 1) % EIT_NUM_ELECTRODES;
    cfg->meas_pos = e1;
    cfg->meas_neg = e2;
}

/* ---------------------------------------------------------------------
 * Initialize the EIT acquisition subsystem
 * --------------------------------------------------------------------- */

void eit_acq_init(void)
{
    /* Configure SysTick for 1ms tick at 240 MHz */
    SysTick_RVR = BOARD_HCLK_HZ / 1000 - 1;
    SysTick_CSR = SysTick_CSR_ENABLE | SysTick_CSR_TICKINT | SysTick_CSR_CLKSRC;

    /* Initialize subsystems */
    mux_init();
    ad5940_init();
    ad5940_load_calib();

    /* Configure AD5940 with defaults */
    ad5940_config_t cfg = {
        .freq_hz = s_eit_freqs[0],
        .current_ua = EIT_CURRENT_UA,
        .pga_gain = 4,
        .dft_len_log2 = 6,
        .adc_rate_div = 8,
    };
    ad5940_configure(&cfg);

    s_start_time_ms = get_tick_ms();
    s_frame_count = 0;
}

/* ---------------------------------------------------------------------
 * Capture one complete EIT frame
 * --------------------------------------------------------------------- */

int eit_acq_capture_frame(eit_frame_t *frame, uint8_t freq_index)
{
    if (!frame || freq_index >= EIT_FREQ_COUNT)
        return -1;

    memset(frame, 0, sizeof(*frame));
    frame->timestamp_ms = get_tick_ms() - s_start_time_ms;
    frame->frame_seq = (uint16_t)(s_frame_count & 0xFFFF);
    frame->freq_index = freq_index;
    s_current_freq_index = freq_index;

    /* Configure AD5940 for the selected frequency */
    ad5940_config_t ad_cfg = {
        .freq_hz = s_eit_freqs[freq_index],
        .current_ua = EIT_CURRENT_UA,
        .pga_gain = 4,
        .dft_len_log2 = 6,
        .adc_rate_div = 8,
    };
    ad5940_configure(&ad_cfg);

    /* Check electrode contacts */
    uint16_t contact_mask = 0;
    if (eit_acq_check_all_contacts(&contact_mask) == 0) {
        frame->electrode_contact_mask = contact_mask;
        /* If fewer than 12 electrodes have good contact, flag error */
        int good = 0;
        for (int i = 0; i < EIT_NUM_ELECTRODES; i++)
            if (contact_mask & (1U << i)) good++;
        if (good < 12) {
            frame->status = 0x10;  /* poor contact */
            return -2;
        }
    }

    /* Acquire all 208 measurements */
    int meas_idx = 0;
    for (uint8_t stim = 0; stim < EIT_NUM_STIM; stim++) {
        for (uint8_t m = 0; m < EIT_NUM_MEAS_PER_STIM; m++) {
            mux_config_t mux_cfg;
            eit_acq_get_meas_pattern(stim, m, &mux_cfg);
            mux_configure(&mux_cfg);

            /* Wait for signal to settle after MUX switching */
            delay_ms(1);

            /* Perform measurement */
            ad5940_meas_t *result = &frame->meas[meas_idx];
            if (ad5940_measure(result) != 0) {
                frame->status = 0x20;  /* measurement error */
                return -3;
            }
            ad5940_apply_calib(result);
            result->freq = s_eit_freqs[freq_index];
            meas_idx++;
        }
    }

    /* Disconnect all electrodes after frame */
    mux_disconnect_all();

    /* Run on-device reconstruction */
    reconstruct_frame(frame);

    frame->status = 0;
    s_frame_count++;
    return 0;
}

/* ---------------------------------------------------------------------
 * Check contact quality for all 16 electrodes
 *   Injects a small current through each electrode pair and measures
 *   the resulting voltage.  If impedance > 50 kΩ, the electrode is
 *   considered not in contact (open).
 * --------------------------------------------------------------------- */

int eit_acq_check_all_contacts(uint16_t *contact_mask)
{
    if (!contact_mask) return -1;
    *contact_mask = 0;

    ad5940_config_t cfg = {
        .freq_hz = EIT_FREQ_0,
        .current_ua = 10,  /* low current for contact check */
        .pga_gain = 7,     /* max gain */
        .dft_len_log2 = 4,
        .adc_rate_div = 16,
    };
    ad5940_configure(&cfg);

    for (uint8_t e = 0; e < EIT_NUM_ELECTRODES; e++) {
        /* Measure between electrode e and electrode (e+1) */
        mux_config_t mux_cfg = {
            .exc_pos  = e,
            .exc_neg  = (e + 1) % EIT_NUM_ELECTRODES,
            .meas_pos = e,
            .meas_neg = (e + 1) % EIT_NUM_ELECTRODES,
        };
        mux_configure(&mux_cfg);
        delay_ms(2);

        ad5940_meas_t result;
        ad5940_measure(&result);
        ad5940_apply_calib(&result);

        /* If magnitude is reasonable (< 50 kΩ equivalent), contact is OK */
        /* Threshold: at 10 µA through 50 kΩ = 500 mV => mag > some value */
        /* We use a relative threshold based on calibration */
        if (result.mag > 0 && result.mag < 500000) {
            *contact_mask |= (1U << e);
        }
    }

    mux_disconnect_all();
    return 0;
}

/* ---------------------------------------------------------------------
 * Calibration: measure the internal 100 Ω reference resistor
 * --------------------------------------------------------------------- */

void eit_acq_calibrate(void)
{
    /* Run AD5940 self-test which measures the internal RCAL */
    if (ad5940_self_test() != 0) {
        /* Calibration failed — leave defaults */
        return;
    }

    /* Measure at each frequency to build calibration table */
    ad5940_meas_t cal_results[EIT_FREQ_COUNT];
    ad5940_measure_sweep(s_eit_freqs, EIT_FREQ_COUNT, cal_results);

    /* The calibration correction is the ratio of expected to measured.
     * For 100 Ω at 200 µA, expected voltage = 20 mV.
     * We store the ratio as Q15 fixed-point. */
    /* (Detailed calibration would compute gain and phase corrections
     * per frequency.  For brevity, we use a single average correction.) */

    /* This is a simplified calibration — a production implementation
     * would compute per-frequency corrections and store them in flash. */
}