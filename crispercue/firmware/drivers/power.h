/*
 * CrisperCue power driver interface
 * Author: jayis1
 */
#ifndef CRISPERCUE_POWER_H
#define CRISPERCUE_POWER_H

#include "../board.h"

void power_init(power_state_t *state);
void power_update(power_state_t *state, const power_state_t *previous, const crisper_snapshot_t *snapshot);
unsigned power_status_register(const power_state_t *state);

#endif
