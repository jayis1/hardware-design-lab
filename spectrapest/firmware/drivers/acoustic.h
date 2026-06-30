/*
 * drivers/acoustic.h — Acoustic wingbeat analysis driver interface
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef ACOUSTIC_H
#define ACOUSTIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ACoustic_SAMPLE_RATE   96000
#define ACOUSTIC_CAPTURE_SEC    3
#define ACOUSTIC_BUF_SAMPLES    (ACOUSTIC_CAPTURE_SEC * ACoustic_SAMPLE_RATE)
#define ACOUSTIC_FEATURE_DIM    24

typedef struct {
    float fundamental_hz;       /* Wingbeat fundamental frequency */
    float harmonic_2;            /* 2nd harmonic amplitude ratio */
    float harmonic_3;            /* 3rd harmonic amplitude ratio */
    float harmonic_ratio;        /* H2/H1 ratio (species discriminant) */
    float spectral_centroid;     /* Spectral centroid (Hz) */
    float spectral_spread;       /* Spectral spread (Hz) */
    float spectral_entropy;      /* Spectral entropy (bits) */
    float zero_crossing_rate;    /* ZCR of time-domain signal */
    float rms_amplitude;         /* RMS amplitude */
    float mfcc[16];              /* 16 mel-frequency cepstral coefficients */
} acoustic_features_t;

int  acoustic_init(void);
int  acoustic_capture(int16_t *out_samples, uint32_t max_samples);
int  acoustic_extract_features(const int16_t *samples, uint32_t n_samples,
                                 acoustic_features_t *out_features);
void acoustic_power_down(void);
void acoustic_power_up(void);

/* Internal FFT functions (implemented in acoustic.c) */
void acoustic_fft(float *real, float *imag, int n);
void acoustic_hann_window(float *window, int n);

#ifdef __cplusplus
}
#endif

#endif /* ACOUSTIC_H */