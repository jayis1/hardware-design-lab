/*
 * mfcc.h — Mel-frequency cepstral coefficients.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_MFCC_H
#define STRIDOPHONE_MFCC_H

#include "../board.h"

/* Initialise the Mel filterbank (20 bands, 0-8 kHz) and 13x20 DCT matrix. */
void mfcc_init(float mel_fb[20][DSP_FRAME_N/2+1], float dct[13][20]);

/* Compute 13 MFCCs from a magnitude spectrum. */
void mfcc_compute(const float *mag, float mel_fb[20][20],
                  float dct[13][20], float *mfcc13);
#endif