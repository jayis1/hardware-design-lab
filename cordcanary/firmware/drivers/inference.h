/*
 * CordCanary inference engine
 * Author: jayis1
 */

#ifndef CORDCANARY_INFERENCE_H
#define CORDCANARY_INFERENCE_H

#include "../board.h"

void inference_init(cc_inference_t *inf);
void inference_evaluate(cc_inference_t *inf,
                        const cc_thermal_frame_t *thermal,
                        const cc_current_frame_t *current,
                        const cc_strain_frame_t *strain,
                        const cc_motion_frame_t *motion,
                        const cc_power_frame_t *power,
                        cc_mode_t mode,
                        unsigned tick);

#endif
