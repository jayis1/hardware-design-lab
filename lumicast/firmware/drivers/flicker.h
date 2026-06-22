/*
 * flicker.h — Photodiode flicker sampling & FFT analysis
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c)  2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef LUMICAST_FLICKER_H
#define LUMICAST_FLICKER_H

#include <stdint.h>
#include "../board.h"

/* Flicker analysis result. */
typedef struct {
    float flicker_percent;     /* peak-to-peak flicker % */
    float flicker_index;       /* Mod(%) IEEE 1789 metric */
    float fundamental_hz;     /* dominant flicker frequency */
    float fundamental_depth;   /* amplitude at fundamental, % of DC */
    uint16_t dc_level;         /* average ADC count */
    uint16_t ac_peak;          /* peak deviation from DC */
    float percent_flicker;     /* legacy % flicker */
    float safety_rating;       /* 0-100 (100 = no risk) per IEEE 1789 */
} flicker_result_t;

void    flicker_init(void);
void    flicker_start_capture(void);
int     flicker_is_ready(void);
int     flicker_get_result(flicker_result_t *out);

#endif /* LUMICAST_FLICKER_H */