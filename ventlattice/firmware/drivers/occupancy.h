/* Occupancy driver for VentLattice
 * Author: jayis1
 */
#ifndef VENTLATTICE_OCCUPANCY_H
#define VENTLATTICE_OCCUPANCY_H

#include "../board.h"
#include "environment.h"

void occupancy_init(occupancy_state_t *state, room_profile_t profile);
void occupancy_sample(occupancy_state_t *state, room_profile_t profile, uint32_t hour_index, const environment_state_t *environment);

#endif
