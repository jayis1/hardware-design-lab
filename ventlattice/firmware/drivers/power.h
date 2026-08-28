/* Power driver for VentLattice
 * Author: jayis1
 */
#ifndef VENTLATTICE_POWER_H
#define VENTLATTICE_POWER_H

#include "../board.h"

void power_init(power_state_t *state, room_profile_t profile);
void power_update(power_state_t *state, const power_state_t *previous, const vent_snapshot_t *snapshot);
float power_status_register(const power_state_t *state);

#endif
