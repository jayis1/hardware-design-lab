/*
 * fft.h — 1024-point real FFT wrapper (CMSIS-DSP style).
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_FFT_H
#define STRIDOPHONE_FFT_H

#include <stdint.h>

void fft_init(void);

/* In-place real FFT with Hann window; writes magnitude spectrum
 * (length N/2+1) into mag. N must be a power of two (1024 here). */
void fft_rfft_hann(const float *pcm, float *mag, int n);

/* Raw real FFT (no window) writing interleaved complex into out (length N). */
void fft_rfft(const float *pcm, float *out, int n);

#endif