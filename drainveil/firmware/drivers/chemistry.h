/*
 * DrainVeil chemistry driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef DRAINVEIL_CHEMISTRY_H
#define DRAINVEIL_CHEMISTRY_H

#include "../board.h"

void chemistry_init(chemistry_state_t *state, install_profile_t profile);
void chemistry_sample(chemistry_state_t *state, install_profile_t profile, uint32_t minute, const flow_state_t *flow, const pressure_state_t *pressure);
const char *chemistry_status_label(const chemistry_state_t *state);

#endif
