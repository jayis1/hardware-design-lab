/*
 * reconstruct.h — EIT Reconstruction Solver (header)
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef ROOTTRACE_RECONSTRUCT_H
#define ROOTTRACE_RECONSTRUCT_H

#include <stdint.h>
#include "eit_acq.h"

/* Reconstruction configuration */
typedef struct {
    float lambda;            /* Tikhonov regularization parameter */
    uint8_t num_iterations;  /* Gauss-Newton iterations (3 default) */
    uint8_t mesh_resolution; /* 0=coarse(196), 1=medium(576), 2=fine(1024) */
    float conductivity_min;  /* Lower bound for conductivity (S/m) */
    float conductivity_max;  /* Upper bound for conductivity (S/m) */
} recon_config_t;

void reconstruct_init(void);
void reconstruct_set_config(const recon_config_t *cfg);
void reconstruct_frame(eit_frame_t *frame);
int  reconstruct_get_image(const eit_frame_t *frame,
                            float *image_buffer,
                            int width, int height);
float reconstruct_compute_biomass(const eit_frame_t *frame);

/* FEM mesh forward model */
void reconstruct_forward_solve(const float *conductivity,
                                 float *predicted_voltages);
void reconstruct_compute_jacobian(float *jacobian);

#endif /* ROOTTRACE_RECONSTRUCT_H */