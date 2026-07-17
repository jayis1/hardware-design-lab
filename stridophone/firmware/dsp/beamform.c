/*
 * beamform.c — delay-and-sum beamformer for the 4-mic square array.
 *
 * Author : jayis1
 * License: MIT
 *
 * The microphones sit at the corners of a 40 mm square. The speed of sound
 * is ~343 m/s, so the maximum inter-mic time delay for a plane wave is
 *   d*sqrt(2)/c = 0.040*1.414 / 343 = 165 µs = 7.9 samples @ 48 kHz.
 *
 * We scan 16 azimuths (every 22.5°) and for each compute the expected
 * per-mic phase shift at a representative mid-band bin (1 kHz), then
 * coherently sum the four spectra. The azimuth with the highest summed
 * energy wins, and its bin is incremented in the histogram.
 *
 * This is a coarse but robust estimator; the histogram, accumulated over
 * many frames, reveals the dominant bearing(s) of insect activity.
 */
#include "beamform.h"
#include "../board.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define SPEED_OF_SOUND  343.0f
#define MIC_SPACING_M   0.040f
#define PROBE_BIN       33   /* ~1.5 kHz at 48k/1024 */

/* Mic positions (square, meters): (x,y) relative to centre. */
static const float g_mic_xy[4][2] = {
    { -MIC_SPACING_M/2, -MIC_SPACING_M/2 },
    {  MIC_SPACING_M/2, -MIC_SPACING_M/2 },
    {  MIC_SPACING_M/2,  MIC_SPACING_M/2 },
    { -MIC_SPACING_M/2,  MIC_SPACING_M/2 },
};

static float g_dominant_energy[4];
static int   g_dominant_channel = 0;

void beamform_init(void) {
    for (int i = 0; i < 4; i++) g_dominant_energy[i] = 0.0f;
    g_dominant_channel = 0;
}

int beamform_update(uint32_t *hist, int nbins, float spec[4][DSP_FRAME_N/2+1]) {
    /* First, find per-channel broadband energy. */
    for (int ch = 0; ch < 4; ch++) {
        float e = 0.0f;
        for (int k = 1; k < 200; k++) e += spec[ch][k];
        g_dominant_energy[ch] = e;
    }
    /* Pick the loudest channel for MFCC extraction. */
    g_dominant_channel = 0;
    float best = g_dominant_energy[0];
    for (int ch = 1; ch < 4; ch++) {
        if (g_dominant_energy[ch] > best) { best = g_dominant_energy[ch]; g_dominant_channel = ch; }
    }

    /* Scan azimuths. For each azimuth compute phase shifts and sum. */
    int best_bin = 0;
    float best_score = -1.0f;
    float bin_hz = (float)AUDIO_FS_HZ / DSP_FRAME_N;
    float k_rad = 2.0f * (float)M_PI * (PROBE_BIN * bin_hz) / SPEED_OF_SOUND;

    for (int b = 0; b < nbins; b++) {
        float az = (float)b * (2.0f * (float)M_PI) / nbins;
        /* Unit direction vector (cos az, sin az, 0). */
        float dx = cosf(az), dy = sinf(az);
        float score = 0.0f;
        for (int ch = 0; ch < 4; ch++) {
            float dist = g_mic_xy[ch][0]*dx + g_mic_xy[ch][1]*dy;
            float phase = k_rad * dist;
            /* Coherent sum of complex spectra at the probe bin. */
            float re = spec[ch][PROBE_BIN] * cosf(phase);
            float im = spec[ch][PROBE_BIN] * sinf(phase);
            score += sqrtf(re*re + im*im);
        }
        if (score > best_score) { best_score = score; best_bin = b; }
    }
    if (hist) hist[best_bin]++;
    return best_bin;
}

int beamform_dominant_channel(void) { return g_dominant_channel; }