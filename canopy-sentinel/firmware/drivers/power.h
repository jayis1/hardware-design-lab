/* Author: jayis1 */
#ifndef CANOPY_SENTINEL_POWER_H
#define CANOPY_SENTINEL_POWER_H

#include "../board.h"

void power_init(cs_power_state_t *state);
void power_tick(cs_power_state_t *state, bool scanning, bool wifi_active);
float power_estimated_minutes_remaining(const cs_power_state_t *state);

#endif
