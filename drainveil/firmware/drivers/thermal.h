/*
 * DrainVeil thermal driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef DRAINVEIL_THERMAL_H
#define DRAINVEIL_THERMAL_H

#include "../board.h"

void thermal_init(thermal_state_t *state, install_profile_t profile);
void thermal_sample(thermal_state_t *state, install_profile_t profile, uint32_t minute, const flow_state_t *flow, const chemistry_state_t *chemistry);
const char *thermal_status_label(const thermal_state_t *state);

#endif
