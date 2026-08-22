/* Author: jayis1 */
#ifndef CANOPY_SENTINEL_LEAF_H
#define CANOPY_SENTINEL_LEAF_H

#include "../board.h"

void leaf_init(float calibration_gain);
cs_leaf_sample_t leaf_sample(uint32_t tick, cs_crop_profile_t crop, float rh_percent, float dew_margin_c, bool attach_clip);

#endif
