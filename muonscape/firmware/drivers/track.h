/*
 * track.h — 3-D muon track fitting from triple-layer hits
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_TRACK_H
#define MUONSCAPE_DRV_TRACK_H

#include <stdint.h>
#include "board.h"
#include "fpga.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x, y, z;       /* entry point in detector frame (cm) */
    float dx, dy, dz;     /* unit direction vector */
    float theta;          /* zenith angle (rad)  */
    float phi;            /* azimuth  (rad)      */
    float energy_gev;    /* estimated muon energy (rough) */
    uint32_t tdc_mean;   /* mean TDC bin of three hits */
    uint32_t timestamp_ms;
    uint8_t  quality;    /* 0=bad, 255=excellent (chi-square based) */
} muon_track_t;

/* Build a track from up to 3 hits (one per layer). Returns MUON_OK if a
 * valid triple coincidence was found and the track was filled. */
muon_status_t track_from_hits(const muon_hit_t hits[3], muon_track_t *out);

/* Convert layer/strip indices to (x,y,z) in detector frame.
 * Layer 0 = top,   strips run along X (strip i at x = i*pitch)
 * Layer 1 = mid,   strips run along Y (strip i at y = i*pitch)
 * Layer 2 = bottom, strips run along X (strip i at x = i*pitch)
 * Layer z positions: 0, -LAYER_SPACING, -2*LAYER_SPACING (cm)
 */
void hit_to_xyz(const muon_hit_t *h, float *x, float *y, float *z);

/* Energy estimate from track angle and path length through detector
 * (very rough — uses range-energy table approximation). */
float track_estimate_energy(const muon_track_t *t);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_TRACK_H */