/*
 * CrisperCue thermal driver interface
 * Author: jayis1
 */
#ifndef CRISPERCUE_THERMAL_H
#define CRISPERCUE_THERMAL_H

#include "../board.h"

void thermal_init(thermal_state_t *state, bin_profile_t profile);
void thermal_sample(thermal_state_t *state, bin_profile_t profile, uint32_t cycle);
const char *thermal_door_label(const thermal_state_t *state);

#endif
