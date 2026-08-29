/*
 * DrainVeil power driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef DRAINVEIL_POWER_H
#define DRAINVEIL_POWER_H

#include "../board.h"

void power_init(power_state_t *state, install_profile_t profile);
void power_update(power_state_t *state, const power_state_t *previous, const drain_snapshot_t *snapshot);
float power_status_register(const power_state_t *state);

#endif
