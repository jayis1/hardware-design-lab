/* Author: jayis1 */
#ifndef PIPEWHISPER_INFERENCE_H
#define PIPEWHISPER_INFERENCE_H

#include "../board.h"

void inference_init(inference_frame_t *frame);
void inference_update(inference_frame_t *frame, const pipe_snapshot_t *current, const pipe_snapshot_t *previous);
const char *inference_primary_reason(const pipe_snapshot_t *snapshot);

#endif
