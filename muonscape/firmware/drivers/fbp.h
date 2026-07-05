/*
 * fbp.h — Filtered back-projection tomographic reconstruction
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_FBP_H
#define MUONSCAPE_DRV_FBP_H

#include <stdint.h>
#include "board.h"
#include "track.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the reconstruction volume in external pSRAM.
 * The volume is VOX_X * VOX_Y * VOX_Z float32 voxels.
 */
muon_status_t fbp_init(float *volume_base);

/* Accumulate a single track into the volume: the line integral of the
 * track through the volume is incremented by the absorption weight,
 * distributed along the path using a 3-D voxel traversal (Amanatides-Woo).
 */
void fbp_accumulate(float *volume, const muon_track_t *t, float weight);

/* Run the ramp filter on the volume (Ram-Lak + optional Hann window).
 * This is the "filtered" step of FBP. Operates in-place on the volume.
 */
muon_status_t fbp_filter(float *volume, int use_hann);

/* Generate a preview PNG (downsampled 64x64 grayscale) of a z-slice
 * into the provided buffer. Returns bytes written.
 */
uint32_t fbp_render_preview(const float *volume, uint8_t slice_z,
                            uint8_t *out_png, uint32_t max_bytes);

/* Reset the volume to zero */
void fbp_clear(float *volume);

/* Statistics */
typedef struct {
    uint32_t tracks_accumulated;
    uint32_t voxels_touched;
    float    max_density;
    float    min_density;
} fbp_stats_t;

void fbp_get_stats(fbp_stats_t *st);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_FBP_H */