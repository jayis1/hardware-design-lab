/*
 * mesh.h — StudGuard peer correlation and origin-band solving
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef STUDGUARD_MESH_H
#define STUDGUARD_MESH_H

#include <stddef.h>
#include <stdint.h>
#include "../board.h"

typedef struct {
    sg_peer_snapshot_t peers[MAX_MESH_NODES];
    size_t count;
    float origin_band;
    float consensus_leak;
    float consensus_spread;
    float consensus_confidence;
} mesh_state_t;

void mesh_init(mesh_state_t *state, uint8_t self_id);
void mesh_synthesize_peers(mesh_state_t *state, uint32_t tick, float local_leak, float local_spread, float local_confidence);
void mesh_integrate_measurement(mesh_state_t *state, sg_measurement_t *measurement);
void mesh_summary(const mesh_state_t *state, char *buffer, size_t buffer_len);

#endif
