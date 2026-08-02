/*
 * drivers/eis.h — AD5940 impedance-analyzer SoC driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef HYDRASCAN_EIS_H
#define HYDRASCAN_EIS_H

#include "../board.h"

typedef struct {
    float re;    /* real part (ohms)        */
    float im;    /* imaginary part (ohms)   */
} eis_point_t;

hydra_err_t eis_init(void);
/* Sweep EIS_FREQ_POINTS log-spaced from 1 Hz..100 kHz; out[EIS_FREQ_POINTS]
 * populated with complex impedance. Averaged EIS_AVG_SWEEPS times. */
hydra_err_t eis_sweep(eis_point_t out[EIS_FREQ_POINTS]);
void        eis_powerdown(void);

extern const float eis_freq_table[EIS_FREQ_POINTS];

#endif