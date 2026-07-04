/*
 * trajectory.h — Dead-reckoning trajectory reconstruction + normalization
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef GLYPHFLOW_TRAJECTORY_H
#define GLYPHFLOW_TRAJECTORY_H

#include <stdint.h>
#include "imu.h"

/* A reconstructed, normalized 2D trajectory of one stroke.
 * Length TRAJ_NPOINTS, channels {x, y, |v|}, all in int16 Q15. */
typedef struct {
    int16_t x[TRAJ_NPOINTS];
    int16_t y[TRAJ_NPOINTS];
    int16_t v[TRAJ_NPOINTS];   /* speed magnitude, normalized */
} trajectory_t;

/* Reconstruct a 2D trajectory from a buffer of IMU samples.
 * Gravity is removed with an exponential low-pass; velocity is
 * dead-reckoned from world-frame accel and projected onto a plane
 * normal to the average forearm axis over the stroke window.
 *
 * Returns 0 on success, -1 if too few samples. */
int trajectory_reconstruct(const imu_sample_t *samples,
                           uint16_t count,
                           trajectory_t *out);

/* Normalize a trajectory: resample to TRAJ_NPOINTS by arc length,
 * center on centroid, scale so the bounding-box diagonal = 1.0
 * (preserving aspect), and fill the speed channel. */
int trajectory_normalize(trajectory_t *t);

#endif /* GLYPHFLOW_TRAJECTORY_H */