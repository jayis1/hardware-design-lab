/*
 * SealBeat acoustic driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef SEALBEAT_ACOUSTIC_H
#define SEALBEAT_ACOUSTIC_H

#include "../board.h"

void acoustic_init(acoustic_state_t *state, appliance_profile_t profile);
void acoustic_sample(acoustic_state_t *state, appliance_profile_t profile, uint32_t minute, const door_state_t *door, const seal_state_t *seal);
const char *acoustic_label(const acoustic_state_t *state);

#endif
