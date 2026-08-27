/*
 * SealBeat door kinematics driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef SEALBEAT_DOOR_H
#define SEALBEAT_DOOR_H

#include "../board.h"

void door_init(door_state_t *state, appliance_profile_t profile);
void door_sample(door_state_t *state, appliance_profile_t profile, uint32_t minute);
const char *door_pattern_label(const door_state_t *state);

#endif
