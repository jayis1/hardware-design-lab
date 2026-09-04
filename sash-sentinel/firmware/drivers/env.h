/*
 * sash-sentinel/firmware/drivers/env.h
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef SASH_SENTINEL_ENV_H
#define SASH_SENTINEL_ENV_H

#include "../board.h"

void env_init(void);
env_snapshot_t env_sample(uint32_t tick, const device_config_t *config, float ventilation_bias);
float env_compute_dew_point(float temp_c, float humidity_pct);
float env_compute_mold_index(const sample_history_t *history);

#endif
