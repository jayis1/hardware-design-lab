/* Environment driver for VentLattice
 * Author: jayis1
 */
#ifndef VENTLATTICE_ENVIRONMENT_H
#define VENTLATTICE_ENVIRONMENT_H

#include "../board.h"
#include "airflow.h"

void environment_init(environment_state_t *state, room_profile_t profile);
void environment_sample(environment_state_t *state, room_profile_t profile, uint32_t hour_index, const airflow_state_t *airflow);

#endif
