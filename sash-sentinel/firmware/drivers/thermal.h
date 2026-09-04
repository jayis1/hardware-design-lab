/*
 * sash-sentinel/firmware/drivers/thermal.h
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef SASH_SENTINEL_THERMAL_H
#define SASH_SENTINEL_THERMAL_H

#include "../board.h"

void thermal_init(void);
thermal_snapshot_t thermal_sample(uint32_t tick, const env_snapshot_t *env);
float thermal_mean_frame_temp(const thermal_snapshot_t *snapshot);

#endif
