/*
 * eis_sweep.h — EIS frequency sweep orchestrator
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_EIS_SWEEP_H
#define LITHOCORE_EIS_SWEEP_H

#include <stdint.h>
#include "lockin.h"

/* Sweep point counts */
#define EIS_FULL_POINT_COUNT   48   /* 0.01 Hz – 100 kHz, log-spaced */
#define EIS_FAST_POINT_COUNT   30   /* 10 Hz – 100 kHz, log-spaced */
#define EIS_CUSTOM_MAX_POINTS  64

/* Sweep result: array of impedance points + metadata */
typedef struct {
    lockin_result_t points[EIS_FULL_POINT_COUNT];
    uint16_t        num_points;
    uint32_t        total_duration_ms;
    uint32_t        start_tick;
} eis_sweep_data_t;

/* Sweep return codes */
#define EIS_OK          0
#define EIS_ERROR_ADC  -1
#define EIS_ERROR_DDS  -2
#define EIS_ERROR_SAFETY -3
#define EIS_ABORTED    -4

/* API */
int  eis_sweep_run(uint8_t full, eis_sweep_data_t *data);
int  eis_sweep_run_custom(const uint32_t *freqs, uint16_t count,
                          eis_sweep_data_t *data);
void eis_sweep_abort(void);
uint8_t eis_sweep_get_progress(void);

/* Get the frequency list for a given sweep mode */
const uint32_t *eis_sweep_get_freq_list(uint8_t full, uint16_t *count);

#endif /* LITHOCORE_EIS_SWEEP_H */