/*
 * i2s_mic.h — 4-channel I2S/TDM MEMS microphone driver interface.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_I2S_MIC_H
#define STRIDOPHONE_I2S_MIC_H

#include <stdint.h>
#include "board.h"

/* Initialise SAI1 for 4-slot TDM RX at 48 kHz / 24-bit, set up the
 * double-buffered BDMA transfer into the audio ring buffer. */
void i2s_mic_init(void);

/* Begin streaming. After this call, the BDMA ISRs fire at half/full. */
void i2s_mic_start(void);

/* Stop streaming and gate the mic rail. */
void i2s_mic_stop(void);

/* Copy the most recent N samples per channel out of the DMA ring into
 * dst[4][N] as float in [-1, 1]. */
void i2s_mic_deinterleave(float dst[4][DSP_FRAME_N], int n);

/* Direct access to the raw ring for advanced consumers. */
const int32_t *i2s_mic_ring_raw(void);
int            i2s_mic_ring_capacity(void);

#endif