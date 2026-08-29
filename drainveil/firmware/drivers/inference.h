/*
 * DrainVeil inference driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef DRAINVEIL_INFERENCE_H
#define DRAINVEIL_INFERENCE_H

#include "../board.h"

void inference_init(inference_state_t *state);
void inference_update(inference_state_t *state, const drain_snapshot_t *current, const drain_snapshot_t *previous, install_profile_t profile);
const char *inference_primary_reason(const drain_snapshot_t *snapshot);

#endif
