/*
 * CordCanary thermal driver
 * Author: jayis1
 */

#ifndef CORDCANARY_THERMAL_H
#define CORDCANARY_THERMAL_H

#include "../board.h"

void thermal_init(cc_thermal_frame_t *frame);
void thermal_sample(cc_thermal_frame_t *frame, cc_mode_t mode, unsigned tick);

#endif
