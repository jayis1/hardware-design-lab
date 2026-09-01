/*
 * CordCanary power driver
 * Author: jayis1
 */

#ifndef CORDCANARY_POWER_H
#define CORDCANARY_POWER_H

#include "../board.h"

void power_init(cc_power_frame_t *frame);
void power_step(cc_power_frame_t *frame, const cc_current_frame_t *current, const cc_inference_t *inf, unsigned tick);

#endif
