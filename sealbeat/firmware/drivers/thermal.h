/*
 * SealBeat thermal recovery driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef SEALBEAT_THERMAL_H
#define SEALBEAT_THERMAL_H

#include "../board.h"

void thermal_init(thermal_state_t *state, appliance_profile_t profile);
void thermal_sample(thermal_state_t *state, appliance_profile_t profile, uint32_t minute, const door_state_t *door, const seal_state_t *seal);
const char *thermal_status_label(const thermal_state_t *state);

#endif
