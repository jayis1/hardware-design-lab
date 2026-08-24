/* Author: jayis1 */
#ifndef SPLINTSENSE_SENSOR_HUB_H
#define SPLINTSENSE_SENSOR_HUB_H

#include "../board.h"

void sensor_hub_init(env_frame_t *env, splint_profile_t profile);
void sensor_hub_sample(env_frame_t *env, splint_profile_t profile, uint32_t minute_index);
void sensor_hub_fuse(recovery_snapshot_t *snapshot, const recovery_snapshot_t *previous);

#endif
