/*
 * drivers/acoustic.h — Acoustic capture and FFT feature extraction
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef HIVEVOX_ACOUSTIC_H
#define HIVEVOX_ACOUSTIC_H

#include <stdint.h>
#include "../board.h"

typedef struct {
    uint16_t dominant_hz;   /* dominant frequency (Hz) */
    uint8_t  r_hi;          /* high-band energy ratio ×255 */
    uint8_t  cv_loud;       /* loudness coefficient of variation ×255 */
    int32_t  total_energy;  /* sum of magnitudes (proxy for loudness) */
} acoustic_features_t;

void acoustic_init(void);
void acoustic_capture_features(acoustic_features_t *out, uint8_t frames);
void mic_enable(void);
void mic_disable(void);

int32_t isqrt32(int32_t n);
int32_t cos_q15(int32_t angle_milli);
int32_t sin_q15(int32_t angle_milli);

volatile uint32_t dwt_cycle_count(void);

#endif /* HIVEVOX_ACOUSTIC_H */