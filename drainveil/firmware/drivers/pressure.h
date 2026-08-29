/*
 * DrainVeil pressure driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef DRAINVEIL_PRESSURE_H
#define DRAINVEIL_PRESSURE_H

#include "../board.h"

void pressure_init(pressure_state_t *state, install_profile_t profile);
void pressure_sample(pressure_state_t *state, install_profile_t profile, uint32_t minute, const flow_state_t *flow);
const char *pressure_pattern_label(const pressure_state_t *state);

#endif
