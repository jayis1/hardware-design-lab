/*
 * CrisperCue gas sensing driver interface
 * Author: jayis1
 */
#ifndef CRISPERCUE_GAS_H
#define CRISPERCUE_GAS_H

#include "../board.h"

void gas_init(gas_state_t *state, bin_profile_t profile);
void gas_sample(gas_state_t *state, bin_profile_t profile, uint32_t cycle, const thermal_state_t *thermal);
const char *gas_air_quality_label(const gas_state_t *state);

#endif
