/*
 * track.c — 3-D muon track fitting
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * A muon traversing the three detector layers produces one hit per
 * layer. We reconstruct its 3-D line by least-squares fitting the three
 * hit positions. Because the layers alternate strip orientation, each
 * layer gives one coordinate strongly (the strip that fired) and the
 * other coordinate only loosely (the layer's plane). We use the strongly
 * measured coordinate from each layer and infer the cross-coordinate
 * from layer spacing.
 */
#include "track.h"
#include "board.h"
#include "registers.h"
#include <math.h>

void hit_to_xyz(const muon_hit_t *h, float *x, float *y, float *z)
{
    if (!h || !x || !y || !z) return;
    /* Strip index gives the strongly-measured coordinate; layer index
     * sets z; the cross-coordinate is the layer's centreline. */
    float strip_pos = (h->f.strip - (STRIPS_PER_LAYER - 1) / 2.0f)
                    * STRIP_PITCH_CM;
    switch (h->f.layer) {
    case 0:  /* top, strips along X */
        *x = strip_pos;
        *y = 0.0f;
        *z = 0.0f;
        break;
    case 1:  /* middle, strips along Y */
        *x = 0.0f;
        *y = strip_pos;
        *z = -LAYER_SPACING_CM;
        break;
    case 2:  /* bottom, strips along X */
        *x = strip_pos;
        *y = 0.0f;
        *z = -2.0f * LAYER_SPACING_CM;
        break;
    default:
        *x = *y = *z = 0.0f;
    }
}

muon_status_t track_from_hits(const muon_hit_t hits[3], muon_track_t *out)
{
    if (!hits || !out) return MUON_ERR_INVALID_ARG;
    /* Require one hit per layer */
    uint8_t seen[LAYER_COUNT] = {0};
    int idx[LAYER_COUNT] = {-1, -1, -1};
    for (int i = 0; i < 3; ++i) {
        if (hits[i].f.layer >= LAYER_COUNT) return MUON_ERR_INVALID_ARG;
        if (seen[hits[i].f.layer]) return MUON_ERR_INVALID_ARG;
        seen[hits[i].f.layer] = 1;
        idx[hits[i].f.layer] = i;
    }
    for (int l = 0; l < LAYER_COUNT; ++l)
        if (idx[l] < 0) return MUON_ERR_INVALID_ARG;

    /* Get xyz for each layer hit */
    float px[LAYER_COUNT], py[LAYER_COUNT], pz[LAYER_COUNT];
    for (int l = 0; l < LAYER_COUNT; ++l) {
        hit_to_xyz(&hits[idx[l]], &px[l], &py[l], &pz[l]);
    }

    /* Least-squares line fit in 3-D:
     * parametrize as P(t) = P0 + t * d, with t in [0,1] from top to bottom.
     * Use layer z values as the parameter: t_l = (z_l - z_top) / (z_bot - z_top)
     */
    float z_top = pz[0];
    float z_bot = pz[2];
    float dz = z_bot - z_top;
    if (fabsf(dz) < 1e-3f) return MUON_ERR_INVALID_ARG;  /* degenerate */

    float t[LAYER_COUNT];
    for (int l = 0; l < LAYER_COUNT; ++l)
        t[l] = (pz[l] - z_top) / dz;

    /* Fit x(t) = x0 + sx*t and y(t) = y0 + sy*t by least squares.
     * Sum_t (x_l - (x0 + sx*t_l))^2 minimized:
     *   S = sum(1)        Sn = sum(t)        Sn2 = sum(t^2)
     *   Sx = sum(x)       Sxt = sum(x*t)
     *   sx = (n*Sxt - Sn*Sx) / (n*Sn2 - Sn^2)
     *   x0 = (Sx - sx*Sn) / n
     */
    float n = (float)LAYER_COUNT;
    float Sn = t[0] + t[1] + t[2];
    float Sn2 = t[0]*t[0] + t[1]*t[1] + t[2]*t[2];
    float Sx = px[0] + px[1] + px[2];
    float Sy = py[0] + py[1] + py[2];
    float Sxt = px[0]*t[0] + px[1]*t[1] + px[2]*t[2];
    float Syt = py[0]*t[0] + py[1]*t[1] + py[2]*t[2];

    float denom = n * Sn2 - Sn * Sn;
    if (fabsf(denom) < 1e-6f) return MUON_ERR_INVALID_ARG;

    float sx = (n * Sxt - Sn * Sx) / denom;
    float sy = (n * Syt - Sn * Sy) / denom;
    float x0 = (Sx - sx * Sn) / n;
    float y0 = (Sy - sy * Sn) / n;

    /* Direction vector */
    float dx = sx, dy = sy, dzd = 1.0f;   /* per unit t = per z step */
    float mag = sqrtf(dx*dx + dy*dy + dzd*dzd);
    if (mag < 1e-6f) return MUON_ERR_INVALID_ARG;
    dx /= mag; dy /= mag; dzd /= mag;

    out->x = x0; out->y = y0; out->z = z_top;
    out->dx = dx; out->dy = dy; out->dz = dzd;

    /* Zenith and azimuth from direction. The detector z axis points up,
     * so the muon direction (downward) has dz negative. Compute angles on
     * the upward-normalized vector for the standard convention. */
    float ux = -dx, uy = -dy, uz = -dzd;  /* upward direction */
    out->theta = acosf(uz);
    out->phi = atan2f(uy, ux);
    if (out->phi < 0) out->phi += 2.0f * 3.14159265f;

    /* Mean TDC for timing */
    uint32_t tdc_sum = 0;
    for (int l = 0; l < LAYER_COUNT; ++l)
        tdc_sum += hits[idx[l]].f.tdc;
    out->tdc_mean = tdc_sum / LAYER_COUNT;
    out->timestamp_ms = muon_millis();

    /* Quality: chi-square of the fit residuals */
    float chi2 = 0.0f;
    for (int l = 0; l < LAYER_COUNT; ++l) {
        float xr = x0 + sx * t[l];
        float yr = y0 + sy * t[l];
        float rx = px[l] - xr;
        float ry = py[l] - yr;
        chi2 += rx*rx + ry*ry;
    }
    /* Map chi2 [0..big] to quality [255..0] */
    float q = 255.0f / (1.0f + chi2 * 0.5f);
    if (q > 255) q = 255;
    out->quality = (uint8_t)q;

    out->energy_gev = track_estimate_energy(out);
    return MUON_OK;
}

/* Rough energy estimate: muons that make it through all three layers
 * (i.e. survive ~20 cm of scintillator + air) have at least ~50 MeV.
 * We use the zenith-angle dependence of the survival probability to
 * estimate the minimum energy, then apply a flux-weighted most-likely
 * energy. This is a coarse estimate for logging only.
 */
float track_estimate_energy(const muon_track_t *t)
{
    if (!t) return 0.0f;
    /* Path length through detector */
    float path = LAYER_SPACING_CM * 2.0f / cosf(t->theta);
    /* Range in plastic scintillator: R(g/cm^2) ≈ 0.4 * E(GeV)^1.4
     * For 20 cm scintillator (~20 g/cm^2), minimum E ~ 0.05 GeV.
     * Most-likely energy from flux spectrum: log-normal, mode ~ 4 GeV.
     * Combine: E ≈ 4 GeV * (1 + 0.1*(path-20)/10) */
    float e = 4.0f * (1.0f + 0.1f * (path - 20.0f) / 10.0f);
    if (e < 0.05f) e = 0.05f;
    return e;
}