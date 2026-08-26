/* Author: jayis1 */
#ifndef PIPEWHISPER_POWER_H
#define PIPEWHISPER_POWER_H

#include "../board.h"

void power_init(power_frame_t *frame, pipe_profile_t profile);
void power_update(power_frame_t *frame, const power_frame_t *previous, const pipe_snapshot_t *snapshot);
float power_status_register(const power_frame_t *frame);

#endif
