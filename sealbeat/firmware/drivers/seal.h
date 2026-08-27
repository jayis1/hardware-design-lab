/*
 * SealBeat seal fusion driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef SEALBEAT_SEAL_H
#define SEALBEAT_SEAL_H

#include "../board.h"

void seal_init(seal_state_t *state, appliance_profile_t profile);
void seal_sample(seal_state_t *state, appliance_profile_t profile, uint32_t minute, const door_state_t *door);
const char *seal_edge_label(const seal_state_t *state);

#endif
