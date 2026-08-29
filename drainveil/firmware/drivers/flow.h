/*
 * DrainVeil flow driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef DRAINVEIL_FLOW_H
#define DRAINVEIL_FLOW_H

#include "../board.h"

void flow_init(flow_state_t *state, install_profile_t profile);
void flow_sample(flow_state_t *state, install_profile_t profile, uint32_t minute);
const char *flow_pattern_label(const flow_state_t *state);

#endif
