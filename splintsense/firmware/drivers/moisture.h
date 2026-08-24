/* Author: jayis1 */
#ifndef SPLINTSENSE_MOISTURE_H
#define SPLINTSENSE_MOISTURE_H

#include "../board.h"

void moisture_init(moisture_frame_t *frame);
void moisture_sample(moisture_frame_t *frame, uint32_t minute_index, float humidity_rh, float motion_factor);
float moisture_burden_score(const moisture_frame_t *frame);

#endif
