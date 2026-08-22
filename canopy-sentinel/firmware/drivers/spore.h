/* Author: jayis1 */
#ifndef CANOPY_SENTINEL_SPORE_H
#define CANOPY_SENTINEL_SPORE_H

#include "../board.h"

void spore_init(float threshold);
cs_spore_sample_t spore_sample(uint32_t tick, cs_crop_profile_t crop, float humidity_percent, float airflow_score);

#endif
