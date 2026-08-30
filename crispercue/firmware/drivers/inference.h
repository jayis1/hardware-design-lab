/*
 * CrisperCue inference engine interface
 * Author: jayis1
 */
#ifndef CRISPERCUE_INFERENCE_H
#define CRISPERCUE_INFERENCE_H

#include "../board.h"

void inference_init(inference_state_t *state);
void inference_update(inference_state_t *state, bin_profile_t profile, const crisper_snapshot_t *current, const crisper_snapshot_t *previous);
const char *inference_primary_reason(const inference_state_t *state);

#endif
