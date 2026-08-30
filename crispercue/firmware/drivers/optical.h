/*
 * CrisperCue optical sensing driver interface
 * Author: jayis1
 */
#ifndef CRISPERCUE_OPTICAL_H
#define CRISPERCUE_OPTICAL_H

#include "../board.h"

void optical_init(optical_state_t *state, bin_profile_t profile);
void optical_sample(optical_state_t *state, bin_profile_t profile, uint32_t cycle, const gas_state_t *gas, const mass_state_t *mass);
const char *optical_stage_hint(const optical_state_t *state);

#endif
