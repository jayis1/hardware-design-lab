/* Inference driver for VentLattice
 * Author: jayis1
 */
#ifndef VENTLATTICE_INFERENCE_H
#define VENTLATTICE_INFERENCE_H

#include "../board.h"

void inference_init(inference_state_t *state);
void inference_update(inference_state_t *state, const vent_snapshot_t *current, const vent_snapshot_t *previous);
const char *inference_primary_reason(const vent_snapshot_t *snapshot);

#endif
