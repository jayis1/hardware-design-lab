/*
 * afe.h — Analog Front End Driver
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_AFE_H
#define LIGNOSCAN_AFE_H

#include <stdint.h>

/* Signal quality classification */
typedef enum {
    QUALITY_SKIP = 0,
    QUALITY_GOOD = 1,
    QUALITY_MARGINAL = 2,
    QUALITY_POOR = 3,
    QUALITY_NO_SIGNAL = 4,
} signal_quality_t;

void afe_init(void);
void afe_set_vga_gain(uint8_t gain_db);
uint8_t afe_get_vga_gain(void);
void afe_auto_gain(int channel);
float afe_measure_amplitude(void);
void afe_set_threshold(uint16_t mv);
void afe_power_down(void);

/* ADC for waveform capture */
void afe_adc_init(void);
uint16_t afe_adc_sample(void);
void afe_capture_waveform(uint16_t *buf, int len);

#endif /* LIGNOSCAN_AFE_H */