/*
 * beamformer.h — Near-field TDOA source localiser public interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */
#ifndef SOUNDLOOM_BF_H
#define SOUNDLOOM_BF_H

#include <stdint.h>

#define BF_BLOCK_LEN  1024

typedef struct {
    float x, y, z;  /* metres relative to pod centre */
} bf_pos_t;

void  bf_init(void);
void  bf_set_positions(const bf_pos_t *pos, int n);
void  bf_set_velocity(float v);
int   bf_localise(const float channels[][BF_BLOCK_LEN], int n_channels,
                  int block_len, float sample_rate,
                  bf_pos_t *out_pos, float *out_residual);
void  bf_get_positions(bf_pos_t *out, int n);
float bf_get_velocity(void);

#endif /* SOUNDLOOM_BF_H */