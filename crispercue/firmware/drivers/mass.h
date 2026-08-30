/*
 * CrisperCue mass tracking driver interface
 * Author: jayis1
 */
#ifndef CRISPERCUE_MASS_H
#define CRISPERCUE_MASS_H

#include "../board.h"

void mass_init(mass_state_t *state, bin_profile_t profile);
void mass_sample(mass_state_t *state, bin_profile_t profile, uint32_t cycle, const gas_state_t *gas);
const char *mass_usage_label(const mass_state_t *state);

#endif
