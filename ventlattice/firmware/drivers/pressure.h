/* Pressure driver for VentLattice
 * Author: jayis1
 */
#ifndef VENTLATTICE_PRESSURE_H
#define VENTLATTICE_PRESSURE_H

#include "../board.h"
#include "airflow.h"

void pressure_init(pressure_state_t *state, room_profile_t profile);
void pressure_sample(pressure_state_t *state, room_profile_t profile, uint32_t hour_index, const airflow_state_t *airflow);

#endif
