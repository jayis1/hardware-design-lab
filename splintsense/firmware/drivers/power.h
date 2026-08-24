/* Author: jayis1 */
#ifndef SPLINTSENSE_POWER_H
#define SPLINTSENSE_POWER_H

#include "../board.h"

void power_init(power_state_t *state, splint_profile_t profile);
void power_update(power_state_t *state, const recovery_snapshot_t *previous, float load_factor, uint32_t minute_index);
float power_status_register(const power_state_t *state);

#endif
