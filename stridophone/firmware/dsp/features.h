/*
 * features.h — vibration + environmental feature aggregation.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_FEATURES_H
#define STRIDOPHONE_FEATURES_H

#include "../board.h"

void features_init(void);

/* Vibration-band magnitude spectrum (RMS of 3 axes -> FFT). */
void features_vibe_spectrum(float axis[3][DSP_FRAME_N],
                            float *mag, int n);

/* Pack the full 44-dim feature vector for the classifier:
 *   [0..38]  MFCC + delta + delta-delta (39)
 *   [39]     vibration spectral crest
 *   [40]     vibration kurtosis
 *   [41]     pulse repetition rate (Hz)
 *   [42]     temperature (C)
 *   [43]     humidity (%)                                         */
void features_pack(float *feat, const float *mfcc13,
                   const float *vibe_mag,
                   float axis[3][DSP_FRAME_N], int n,
                   float temp_c, float rh_pct);

#endif