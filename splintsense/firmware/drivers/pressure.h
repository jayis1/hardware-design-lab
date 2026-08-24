/* Author: jayis1 */
#ifndef SPLINTSENSE_PRESSURE_H
#define SPLINTSENSE_PRESSURE_H

#include "../board.h"

void pressure_init(pressure_frame_t *frame, splint_profile_t profile);
void pressure_sample(pressure_frame_t *frame, splint_profile_t profile, uint32_t minute_index, float motion_factor);
float pressure_fit_score(const pressure_frame_t *frame);

#endif
