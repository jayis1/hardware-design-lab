/* Author: jayis1 */
#ifndef PIPEWHISPER_ENVIRONMENT_H
#define PIPEWHISPER_ENVIRONMENT_H

#include "../board.h"

void environment_init(environment_frame_t *frame, pipe_profile_t profile);
void environment_sample(environment_frame_t *frame, pipe_profile_t profile, uint32_t minute, const flow_frame_t *flow);

#endif
