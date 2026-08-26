/* Author: jayis1 */
#ifndef PIPEWHISPER_ACOUSTIC_H
#define PIPEWHISPER_ACOUSTIC_H

#include "../board.h"

void acoustic_init(acoustic_frame_t *frame, pipe_profile_t profile);
void acoustic_sample(acoustic_frame_t *frame, pipe_profile_t profile, uint32_t minute);
const char *acoustic_event_label(const acoustic_frame_t *frame);

#endif
