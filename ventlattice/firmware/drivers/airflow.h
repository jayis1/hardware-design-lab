/* Airflow driver for VentLattice
 * Author: jayis1
 */
#ifndef VENTLATTICE_AIRFLOW_H
#define VENTLATTICE_AIRFLOW_H

#include "../board.h"

void airflow_init(airflow_state_t *state, room_profile_t profile);
void airflow_sample(airflow_state_t *state, room_profile_t profile, uint32_t hour_index);
const char *airflow_state_label(const airflow_state_t *state);

#endif
