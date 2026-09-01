/*
 * CordCanary strain driver
 * Author: jayis1
 */

#ifndef CORDCANARY_STRAIN_H
#define CORDCANARY_STRAIN_H

#include "../board.h"

void strain_init(cc_strain_frame_t *frame);
void strain_sample(cc_strain_frame_t *frame, cc_mode_t mode, unsigned tick);

#endif
