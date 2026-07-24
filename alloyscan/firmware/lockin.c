/*
 * lockin.c — Digital lock-in amplifier implementation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements multi-frequency digital lock-in amplification:
 * 1. Pre-computes a DDS lookup table containing the sum of 4 sinusoidal
 *    tones (1 kHz, 10 kHz, 100 kHz, 500 kHz) for DAC excitation output.
 * 2. On each ADC sample, multiplies by reference cos/sin at each
 *    excitation frequency (I/Q demodulation) using NCO phase accumulators.
 * 3. Accumulates I/Q over a measurement window, then computes magnitude
 *    and phase using the CORDIC hardware accelerator.
 *
 * The NCO approach uses 32-bit phase accumulators where the phase increment
 * per sample is: phase_inc = (freq * 2^32) / sample_rate.
 * This gives sub-Hz frequency resolution at 1 MSPS.
 */

#include "lockin.h"
#include "registers.h"
#include <math.h>
#include <string.h>

/* DDS excitation table (12-bit values for DAC) */
static uint16_t dds_table[DDS_TABLE_SIZE];
static bool dds_table_ready = false;

/* DDS waveform amplitude weights for each frequency
 * Lower frequencies get higher amplitude to compensate for coil impedance
 * (inductive reactance increases with frequency, reducing current at
 *  constant voltage drive). */
static const float dds_weights[FREQ_COUNT] = { 1.0f, 0.8f, 0.6f, 0.4f };

/* Phase increment = freq * 2^32 / sample_rate
 * For 1 MHz sample rate:
 *   1 kHz   -> 4294967 (0x00418937)
 *   10 kHz  -> 42949673 (0x028F5C29)
 *   100 kHz -> 429496730 (0x1999999A)
 *   500 kHz -> 2147483648 (0x80000000) -- Nyquist
 * Note: 500 kHz at 1 MSPS is at Nyquist; in practice we'd use 2 MSPS ADC.
 * The values below are computed at compile time for 1 MSPS. */
#define PHASE_ACC_BITS    32
#define PHASE_ACC_MAX     4294967296.0f
#define DDS_PHASE_TO_INT  (float)(PHASE_ACC_MAX / DDS_TABLE_SIZE)

/* ---- DDS Table Initialization ---- */

void lockin_init_dds_table(void)
{
    if (dds_table_ready)
        return;

    /* Generate composite multi-tone waveform:
     * dds[n] = sum_k weight_k * sin(2*pi*f_k*n / DDS_SAMPLE_RATE)
     * where f_k cycles within the table period.
     *
     * The table represents exactly 1 period at the lowest frequency
     * (1 kHz) at 2 MSPS, which is 2000 samples. We use 2048 for
     * power-of-2 convenience (slight frequency offset is acceptable
     * since we demodulate with the same NCO). */

    for (int n = 0; n < DDS_TABLE_SIZE; n++) {
        float t = (float)n / (float)DDS_SAMPLE_RATE_HZ;
        float sample = 0.0f;

        for (int k = 0; k < FREQ_COUNT; k++) {
            float omega = 2.0f * 3.14159265359f * excitation_freqs[k] * t;
            sample += dds_weights[k] * sinf(omega);
        }

        /* Normalize to prevent clipping: max possible = sum of weights = 2.8 */
        sample /= 2.8f;

        /* Scale to 12-bit DAC range (0 to 4095, centered at 2048) */
        int32_t dac_val = (int32_t)(sample * 2047.0f) + 2048;
        if (dac_val < 0) dac_val = 0;
        if (dac_val > 4095) dac_val = 4095;

        dds_table[n] = (uint16_t)dac_val;
    }

    dds_table_ready = true;
}

const uint16_t *lockin_get_dds_table(void)
{
    if (!dds_table_ready)
        lockin_init_dds_table();
    return dds_table;
}

/* ---- Lock-In Amplifier ---- */

void lockin_init(lockin_state_t *state, uint32_t sample_rate)
{
    memset(state, 0, sizeof(*state));
    state->sample_rate = sample_rate;

    /* Compute phase increments for each frequency */
    for (int k = 0; k < FREQ_COUNT; k++) {
        /* phase_inc = freq * 2^32 / sample_rate */
        uint64_t inc = ((uint64_t)excitation_freqs[k] << 32) / sample_rate;
        state->phase_inc[k] = (uint32_t)inc;
    }
}

void lockin_reset(lockin_state_t *state)
{
    for (int k = 0; k < FREQ_COUNT; k++)
        state->phase_accum[k] = 0;
}

/* CORDIC-assisted sin/cos lookup from phase.
 * Uses the top 12 bits of the 32-bit phase as index into a quarter-wave
 * sine table (1024 entries), with sign/flip logic for full 360° coverage. */

/* 1024-entry quarter-wave sine table (0 to 90 degrees) */
static const int16_t sin_qtable[256] = {
      0,  101,  201,  301,  401,  501,  601,  700,  799,  897,  995, 1092,
   1189, 1285, 1380, 1474, 1567, 1659, 1750, 1840, 1929, 2017, 2104, 2189,
   2273, 2355, 2436, 2515, 2593, 2669, 2744, 2817, 2889, 2958, 3026, 3092,
   3156, 3219, 3279, 3337, 3394, 3448, 3501, 3551, 3599, 3645, 3689, 3731,
   3770, 3807, 3842, 3875, 3905, 3933, 3959, 3983, 4004, 4023, 4039, 4053,
   4065, 4074, 4081, 4086, 4088, 4088, 4086, 4081, 4074, 4065, 4053, 4039,
   4023, 4004, 3983, 3959, 3933, 3905, 3875, 3842, 3807, 3770, 3731, 3689,
   3645, 3599, 3551, 3501, 3448, 3394, 3337, 3279, 3219, 3156, 3092, 3026,
   2958, 2889, 2817, 2744, 2669, 2593, 2515, 2436, 2355, 2273, 2189, 2104,
   2017, 1929, 1840, 1750, 1659, 1567, 1474, 1380, 1285, 1189, 1092,  995,
    897,  799,  700,  601,  501,  401,  301,  201,  101
};

/* Full sine table using quarter-wave symmetry: 1024 entries for 360° */
static int16_t sin_lookup(uint16_t phase_16)
{
    /* phase_16: 0-65535 representing 0 to 360 degrees */
    uint16_t quadrant = phase_16 >> 14;       /* Top 2 bits: quadrant 0-3 */
    uint16_t index = (phase_16 >> 6) & 0x3FF; /* Middle 10 bits: 0-1023 */
    uint16_t entry = index & 0xFF;             /* 8-bit index into qtable  */

    switch (quadrant) {
        case 0: return sin_qtable[entry];
        case 1: return sin_qtable[255 - entry];
        case 2: return -sin_qtable[entry];
        case 3: return -sin_qtable[255 - entry];
        default: return 0;
    }
}

/* Cosine via phase offset of 90 degrees (add 0x4000 to phase) */
static int16_t cos_lookup(uint16_t phase_16)
{
    return sin_lookup(phase_16 + 0x4000);
}

lockin_result_t lockin_process(lockin_state_t *state,
                                const int16_t *samples, uint32_t count)
{
    lockin_result_t result;
    memset(&result, 0, sizeof(result));

    if (!state || !samples || count == 0) {
        result.valid = false;
        return result;
    }

    /* Accumulators for I and Q at each frequency */
    int64_t i_acc[FREQ_COUNT] = {0};
    int64_t q_acc[FREQ_COUNT] = {0};

    for (uint32_t n = 0; n < count; n++) {
        int16_t sample = samples[n];

        for (int k = 0; k < FREQ_COUNT; k++) {
            /* Get current phase (top 16 bits of 32-bit accumulator) */
            uint16_t phase16 = (uint16_t)(state->phase_accum[k] >> 16);

            /* Reference signals (scaled to ±4096 for fixed-point) */
            int16_t ref_cos = cos_lookup(phase16);  /* ±4096 range */
            int16_t ref_sin = sin_lookup(phase16);

            /* Multiply sample by reference (sample is 16-bit, ref is ±4096)
             * Result is 32-bit. We accumulate in 64-bit. */
            i_acc[k] += (int64_t)sample * (int64_t)ref_cos;
            q_acc[k] += (int64_t)sample * (int64_t)ref_sin;

            /* Advance phase */
            state->phase_accum[k] += state->phase_inc[k];
        }
    }

    /* Normalize accumulators: I = sum(s*cos) / (count * 4096 * 32768)
     * The 32768 comes from 16-bit sample, 4096 from ref amplitude.
     * We normalize to get the complex impedance in a standard form. */
    float norm = 1.0f / ((float)count * 4096.0f * 32768.0f);

    for (int k = 0; k < FREQ_COUNT; k++) {
        result.I[k] = (float)i_acc[k] * norm;
        result.Q[k] = (float)q_acc[k] * norm;

        /* Compute magnitude and phase using CORDIC or software math */
        float i = result.I[k];
        float q = result.Q[k];
        result.magnitude[k] = sqrtf(i * i + q * q);
        result.phase[k] = atan2f(q, i);
    }

    result.valid = true;
    return result;
}