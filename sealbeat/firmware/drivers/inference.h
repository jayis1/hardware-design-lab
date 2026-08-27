/*
 * SealBeat inference engine
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef SEALBEAT_INFERENCE_H
#define SEALBEAT_INFERENCE_H

#include "../board.h"

void inference_init(inference_state_t *state);
void inference_update(inference_state_t *state, const appliance_snapshot_t *current, const appliance_snapshot_t *previous, appliance_profile_t profile);
const char *inference_primary_reason(const appliance_snapshot_t *snapshot);

#endif
