/*
 * beamform.h — delay-and-sum beamformer / direction-of-arrival estimator.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_BEAMFORM_H
#define STRIDOPHONE_BEAMFORM_H

#include <stdint.h>

void beamform_init(void);

/* Update the DOA histogram using 4-channel magnitude spectra.
 * Returns the azimuth bin (0..15) of the dominant source this frame. */
int beamform_update(uint32_t *hist, int nbins, float spec[4][DSP_FRAME_N/2+1]);

/* Return the channel index with the highest broadband energy this frame. */
int beamform_dominant_channel(void);

#endif