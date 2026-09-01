/*
 * CordCanary motion driver
 * Author: jayis1
 */

#ifndef CORDCANARY_MOTION_H
#define CORDCANARY_MOTION_H

#include "../board.h"

void motion_init(cc_motion_frame_t *frame);
void motion_sample(cc_motion_frame_t *frame, const cc_strain_frame_t *strain, cc_mode_t mode, unsigned tick);

#endif
