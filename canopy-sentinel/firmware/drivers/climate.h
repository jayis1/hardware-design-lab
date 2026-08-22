/* Author: jayis1 */
#ifndef CANOPY_SENTINEL_CLIMATE_H
#define CANOPY_SENTINEL_CLIMATE_H

#include "../board.h"

void climate_init(void);
cs_climate_sample_t climate_sample(uint32_t tick, cs_crop_profile_t crop);
float climate_compute_dew_point(float air_c, float rh_percent);
float climate_compute_vpd(float air_c, float rh_percent);

#endif
