/* Author: jayis1 */
#ifndef CANOPY_SENTINEL_THERMAL_H
#define CANOPY_SENTINEL_THERMAL_H

#include "../board.h"

void thermal_init(void);
cs_thermal_frame_t thermal_capture(uint32_t tick, cs_crop_profile_t crop, float air_c);

#endif
