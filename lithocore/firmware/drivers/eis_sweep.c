/*
 * eis_sweep.c — EIS frequency sweep orchestrator.
 *
 * Steps through a log-spaced list of frequencies, calling the lock-in
 * detector at each point. Handles the transition between the low-frequency
 * path (ADS1256) and the high-frequency path (MCU ADC) at 3 kHz.
 *
 * The sweep can be aborted at any point by calling eis_sweep_abort() or
 * by the user pressing the button (handled in main.c).
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "eis_sweep.h"
#include "../board.h"
#include "lockin.h"
#include "dds.h"
#include "ads1256.h"
#include "safety.h"

/* -------------------------------------------------------------------------
 * Frequency tables (log-spaced)
 *
 * Full sweep: 48 points from 0.01 Hz to 100 kHz
 *   0.01, 0.0158, 0.0251, 0.0398, 0.0631, 0.1 Hz        (6 pts, ultra-low)
 *   0.158, 0.251, 0.398, 0.631, 1.0, 1.58, 2.51, 3.98,
 *   6.31, 10.0 Hz                                        (10 pts, low)
 *   15.8, 25.1, 39.8, 63.1, 100, 158, 251, 398,
 *   631, 1000 Hz                                         (10 pts, mid)
 *   1585, 2512, 3981, 6310, 10000, 15849, 25119, 39811,
 *   63096, 100000 Hz                                     (10 pts, high)
 * Plus 12 more intermediate points = 36. We use 48 total with finer spacing.
 *
 * Fast sweep: 30 points from 10 Hz to 100 kHz (skips ultra-low + low bands)
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */

/* Full sweep frequencies in milli-Hz (to preserve sub-Hz resolution as integer) */
static const uint32_t full_freq_mhz[EIS_FULL_POINT_COUNT] = {
    /* Ultra-low: 0.01 – 0.1 Hz (6 points) */
    10, 16, 25, 40, 63, 100,
    /* Low: 0.1 – 10 Hz (12 points) */
    158, 251, 398, 631, 1000, 1585, 2512, 3981, 6310, 10000,
    15849, 25119,
    /* Mid: 10 – 1000 Hz (14 points) */
    39811, 63096, 100000, 158489, 251189, 398107, 630957, 1000000,
    1584893, 2511886, 3981072, 6309573, 10000000, 15848932,
    /* High: 10 – 100 kHz (16 points) */
    25118864, 39810717, 63095734, 100000000, 158489319, 251188642,
    398107171, 630957345, 1000000000, 1584893192, 2511886432,
    3981071706, 6309573445, 10000000000U, 15848931925U, 100000000000U,
};

/* Fast sweep frequencies in milli-Hz (10 Hz – 100 kHz) */
static const uint32_t fast_freq_mhz[EIS_FAST_POINT_COUNT] = {
    10000, 15849, 25119, 39811, 63096, 100000,
    158489, 251189, 398107, 630957, 1000000,
    1584893, 2511886, 3981072, 6309573, 10000000,
    15848932, 25118864, 39810717, 63095734, 100000000,
    158489319, 251188642, 398107171, 630957345, 1000000000,
    1584893192, 2511886432, 3981071706, 6309573445,
};

/* -------------------------------------------------------------------------
 * Sweep state
 * ------------------------------------------------------------------------- */
static volatile uint8_t  g_abort_flag = 0;
static volatile uint8_t  g_progress = 0;
static volatile uint16_t g_current_idx = 0;

/* -------------------------------------------------------------------------
 * Get frequency list
 * ------------------------------------------------------------------------- */
const uint32_t *eis_sweep_get_freq_list(uint8_t full, uint16_t *count)
{
    if (full) {
        *count = EIS_FULL_POINT_COUNT;
        return full_freq_mhz;
    } else {
        *count = EIS_FAST_POINT_COUNT;
        return fast_freq_mhz;
    }
}

/* -------------------------------------------------------------------------
 * Abort
 * ------------------------------------------------------------------------- */
void eis_sweep_abort(void)
{
    g_abort_flag = 1;
}

uint8_t eis_sweep_get_progress(void)
{
    return g_progress;
}

/* -------------------------------------------------------------------------
 * Run a sweep
 *
 * For each frequency point:
 *   1. Check safety (voltage, temperature) — abort if unsafe
 *   2. Set DDS frequency and settle
 *   3. Choose lock-in path: LF (≤ 3 kHz → ADS1256) or HF (> 3 kHz → MCU ADC)
 *   4. Compute impedance via lock-in detection
 *   5. Store result, update progress
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int eis_sweep_run(uint8_t full, eis_sweep_data_t *data)
{
    const uint32_t *freqs;
    uint16_t count;
    freqs = eis_sweep_get_freq_list(full, &count);

    data->num_points = 0;
    data->start_tick = g_ticks;  /* from main.c SysTick */
    g_abort_flag = 0;
    g_progress = 0;
    g_current_idx = 0;

    for (uint16_t i = 0; i < count; i++) {
        if (g_abort_flag) {
            dds_disable();
            return EIS_ABORTED;
        }

        /* Safety check before each point */
        uint16_t v, t;
        if (safety_check(&v, &t) != SAFETY_OK) {
            dds_disable();
            return EIS_ERROR_SAFETY;
        }

        /* Convert mHz to Hz (double for the DDS API) */
        double freq_hz = (double)freqs[i] / 1000.0;

        /* Set DDS and allow settling */
        dds_set_frequency_hz(freq_hz);
        dds_enable();

        /* Settling time: wait at least 2 cycles of the excitation */
        if (freqs[i] > 0) {
            /* settle_ms = 2000 / freq_hz = 2000000 / freq_mhz */
            uint32_t settle_ms = 2000000U / freqs[i];
            if (settle_ms > 5000) settle_ms = 5000;
            if (settle_ms < 5) settle_ms = 5;
            delay_ms(settle_ms);
        }

        /* Choose measurement path based on frequency */
        int ret;
        lockin_result_t *pt = &data->points[data->num_points];

        if (freqs[i] <= 3000000) {
            /* Low/mid frequency: ADS1256 (≤ 3 kHz) */
            ret = lockin_measure_lf((uint32_t)freq_hz, pt);
        } else {
            /* High frequency: MCU ADC (> 3 kHz) */
            ret = lockin_measure_hf((uint32_t)freq_hz, pt);
        }

        if (ret == 0 && pt->valid) {
            data->num_points++;
        }
        /* If a point fails, we skip it and continue */

        /* Update progress */
        g_current_idx = i + 1;
        g_progress = (uint8_t)((i + 1) * 100 / count);

        /* Disable DDS between points to reduce cell loading */
        dds_disable();
    }

    data->total_duration_ms = g_ticks - data->start_tick;
    g_progress = 100;

    return EIS_OK;
}

/* -------------------------------------------------------------------------
 * Custom sweep with user-specified frequencies
 * ------------------------------------------------------------------------- */
int eis_sweep_run_custom(const uint32_t *freqs, uint16_t count,
                         eis_sweep_data_t *data)
{
    if (count > EIS_CUSTOM_MAX_POINTS)
        count = EIS_CUSTOM_MAX_POINTS;

    data->num_points = 0;
    data->start_tick = g_ticks;
    g_abort_flag = 0;
    g_progress = 0;

    for (uint16_t i = 0; i < count; i++) {
        if (g_abort_flag) {
            dds_disable();
            return EIS_ABORTED;
        }

        uint16_t v, t;
        if (safety_check(&v, &t) != SAFETY_OK) {
            dds_disable();
            return EIS_ERROR_SAFETY;
        }

        double freq_hz = (double)freqs[i] / 1000.0;
        dds_set_frequency_hz(freq_hz);
        dds_enable();

        uint32_t settle_ms = (freqs[i] > 0) ? 2000000U / freqs[i] : 5000;
        if (settle_ms > 5000) settle_ms = 5000;
        if (settle_ms < 5) settle_ms = 5;
        delay_ms(settle_ms);

        int ret;
        lockin_result_t *pt = &data->points[data->num_points];

        if (freqs[i] <= 3000000)
            ret = lockin_measure_lf((uint32_t)freq_hz, pt);
        else
            ret = lockin_measure_hf((uint32_t)freq_hz, pt);

        if (ret == 0 && pt->valid)
            data->num_points++;

        g_progress = (uint8_t)((i + 1) * 100 / count);
        dds_disable();
    }

    data->total_duration_ms = g_ticks - data->start_tick;
    return EIS_OK;
}