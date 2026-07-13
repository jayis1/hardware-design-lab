/*
 * dsp.h — DSP API: FFT, spectral feature extraction for HivePulse
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef DSP_H
#define DSP_H

#include <stdint.h>
#include <stdbool.h>
#include "audio.h"

/* Number of features extracted per audio window */
#define NUM_FEATURES 32

/* Number of sub-bands for energy computation */
#define NUM_SUBBANDS 8

/* Number of MFCC-like cepstral coefficients */
#define NUM_CEPSTRAL 12

/* Feature vector: the input to the ML classifier */
typedef struct {
    float fundamental_freq;       /* Peak frequency in PSD (Hz) */
    float spectral_centroid;      /* Weighted mean frequency */
    float spectral_spread;        /* Standard deviation of spectrum */
    float spectral_flatness;      /* Geometric/arithmetic mean ratio */
    float spectral_rolloff;       /* 85th percentile frequency */
    float subband_energy[NUM_SUBBANDS]; /* Energy in 8 frequency bands */
    float am_depth;               /* Amplitude modulation depth */
    float am_rate;                /* Amplitude modulation frequency */
    float hnr;                    /* Harmonic-to-noise ratio */
    float cepstral[NUM_CEPSTRAL]; /* MFCC-like coefficients */
} feature_vector_t;

/* Initialize DSP subsystem (CMSIS-DSP tables, window coefficients) */
int dsp_init(void);

/* Extract features from an audio snapshot
 * Uses channel 0 (interior brood chamber mic) as primary */
int dsp_extract_features(const audio_snapshot_t *snap, feature_vector_t *feat);

/* Compute 4096-point FFT using CMSIS-DSP arm_rfft_fast_f32 */
void dsp_compute_fft(const float *input, float *output_mag, int n);

/* Compute power spectral density from FFT output */
void dsp_compute_psd(const float *fft_output, float *psd, int n);

/* Apply Hann window to input data */
void dsp_apply_hann_window(float *data, int n);

/* Compute MFCC-like cepstral coefficients from PSD */
void dsp_compute_cepstral(const float *psd, int n, float *cepstral, int num_cep);

/* Detect fundamental frequency via peak picking on PSD */
float dsp_detect_fundamental(const float *psd, int n, float fs);

/* Compute spectral centroid, spread, flatness, rolloff */
void dsp_compute_spectral_stats(const float *psd, int n, float fs,
                                 float *centroid, float *spread,
                                 float *flatness, float *rolloff);

/* Compute sub-band energies */
void dsp_compute_subband_energy(const float *psd, int n, float fs,
                                 float energies[NUM_SUBBANDS]);

/* Compute amplitude modulation (envelope analysis) */
void dsp_compute_am(const float *signal, int n, float fs,
                     float *depth, float *rate);

/* Compute harmonic-to-noise ratio */
float dsp_compute_hnr(const float *psd, int n, float fs, float f0);

#endif /* DSP_H */