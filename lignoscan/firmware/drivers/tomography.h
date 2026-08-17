/*
 * tomography.h — SART Tomographic Reconstruction for Acoustic Imaging
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_TOMOGRAPHY_H
#define LIGNOSCAN_TOMOGRAPHY_H

#include <stdint.h>
#include "afe.h"

#define MAX_RAYS    256     /* Max TX-RX ray paths (16*15=240) */
#define MAX_CELLS   128     /* Max grid cells (8 radial * 16 angular) */

/* Ray path descriptor */
typedef struct {
    int tx_idx;             /* Transmitter sensor index */
    int rx_idx;             /* Receiver sensor index */
    float measured_tof_ns;  /* Measured time of flight (ns) */
    float chord_length_m;   /* Straight-line distance between sensors (m) */
    float angle_start;      /* Starting angle (radians) */
    float angle_end;        /* Ending angle (radians) */
    int cells[MAX_CELLS];   /* Indices of cells this ray passes through */
    float cell_lengths[MAX_CELLS]; /* Length of ray segment in each cell (m) */
    int num_cells;          /* Number of cells in path */
} ray_t;

/* Tomography reconstruction context */
typedef struct {
    int num_sensors;
    float sensor_pos[16];       /* Angular positions (radians) */
    float trunk_radius_m;       /* Trunk radius in meters */

    int n_radial;               /* Number of radial cells */
    int n_angular;              /* Number of angular cells */
    int n_cells;                /* Total cells = n_radial * n_angular */

    ray_t rays[MAX_RAYS];       /* All ray paths */
    int num_rays;               /* Count of valid rays */

    /* Reconstruction arrays */
    float slowness[MAX_CELLS];  /* Cell slowness (s/m) — being solved */
    float velocity_map[16][16]; /* Final velocity map (m/s) — stored as 2D */
    uint8_t classification[MAX_CELLS]; /* Decay classification per cell */

    int iteration;              /* Current SART iteration */
} tomo_ctx_t;

/* Decay classification codes */
#define DECAY_SOUND     0   /* velocity > SOUND_WOOD_VMIN */
#define DECAY_MODERATE  1   /* MOD_DECAY_VMIN < velocity < SOUND_WOOD_VMIN */
#define DECAY_SEVERE    2   /* velocity < MOD_DECAY_VMIN */
#define DECAY_HOLLOW    3   /* velocity < 500 m/s (near-air) */

void tomo_init(tomo_ctx_t *ctx, int num_sensors, float *positions,
               float diameter_cm, int n_radial, int n_angular);
void tomo_add_ray(tomo_ctx_t *ctx, int tx, int rx, float tof_ns);
void tomo_trace_rays(tomo_ctx_t *ctx);
void tomo_sart_iterate(tomo_ctx_t *ctx);
void tomo_finalize(tomo_ctx_t *ctx);
float tomo_compute_tdi(tomo_ctx_t *ctx, float v_sound, float v_moderate);

/* Helper: compute chord length between two sensors */
float tomo_chord_length(float radius, float angle1, float angle2);

/* Helper: trace a single ray through the polar grid */
void tomo_trace_ray(tomo_ctx_t *ctx, ray_t *ray);

#endif /* LIGNOSCAN_TOMOGRAPHY_H */