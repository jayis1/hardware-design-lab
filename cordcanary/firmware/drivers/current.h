/*
 * CordCanary current driver
 * Author: jayis1
 */

#ifndef CORDCANARY_CURRENT_H
#define CORDCANARY_CURRENT_H

#include "../board.h"

void current_init(cc_current_frame_t *frame);
void current_sample(cc_current_frame_t *frame, const cc_thermal_frame_t *thermal, cc_mode_t mode, unsigned tick);

#endif
