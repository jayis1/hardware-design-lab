/* Author: jayis1 */
#ifndef PIPEWHISPER_FLOW_H
#define PIPEWHISPER_FLOW_H

#include "../board.h"

void flow_init(flow_frame_t *frame, pipe_profile_t profile);
void flow_update(flow_frame_t *frame, const acoustic_frame_t *acoustic, const pressure_frame_t *pressure, uint32_t minute);
const char *flow_fixture_label(const flow_frame_t *frame);

#endif
