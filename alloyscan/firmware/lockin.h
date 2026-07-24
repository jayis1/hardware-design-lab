/*
 * lockin.h — Digital lock-in amplifier for multi-frequency I/Q demodulation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_LOCKIN_H
#define ALLOYSCAN_LOCKIN_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* Lock-in result: I and Q at each of the 4 excitation frequencies */
typedef struct {
    float I[FREQ_COUNT];   /* In-phase components   */
    float Q[FREQ_COUNT];   /* Quadrature components  */
    float magnitude[FREQ_COUNT];  /* |Z| = sqrt(I² + Q²) */
    float phase[FREQ_COUNT];      /* angle = atan2(Q, I)   */
    bool  valid;
} lockin_result_t;

/* NCO phase accumulator state */
typedef struct {
    uint32_t phase_accum[FREQ_COUNT];  /* 32-bit phase accumulators */
    uint32_t phase_inc[FREQ_COUNT];    /* Phase increment per sample */
    uint32_t sample_rate;              /* ADC sample rate in Hz */
} lockin_state_t;

/* Initialize the lock-in amplifier state
 * Sets up NCO phase increments for each excitation frequency */
void lockin_init(lockin_state_t *state, uint32_t sample_rate);

/* Process a block of ADC samples through the lock-in amplifier
 * samples: array of ADC samples (16-bit signed, centered at 0)
 * count: number of samples (should be ADC_SAMPLE_COUNT)
 * Returns I/Q components at each frequency */
lockin_result_t lockin_process(lockin_state_t *state,
                                const int16_t *samples, uint32_t count);

/* Get the DDS excitation waveform table (for DAC output)
 * Returns a pointer to the pre-computed 12-bit DAC table */
const uint16_t *lockin_get_dds_table(void);

/* Initialize the DDS lookup table (call once at startup) */
void lockin_init_dds_table(void);

/* Reset phase accumulators (for synchronization) */
void lockin_reset(lockin_state_t *state);

#endif /* ALLOYSCAN_LOCKIN_H */