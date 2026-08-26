/* Author: jayis1 */
#ifndef PIPEWHISPER_PRESSURE_H
#define PIPEWHISPER_PRESSURE_H

#include "../board.h"

void pressure_init(pressure_frame_t *frame, pipe_profile_t profile);
void pressure_sample(pressure_frame_t *frame, pipe_profile_t profile, uint32_t minute, const acoustic_frame_t *acoustic);

#endif
