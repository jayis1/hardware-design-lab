/*
 * sash-sentinel/firmware/drivers/latch.h
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef SASH_SENTINEL_LATCH_H
#define SASH_SENTINEL_LATCH_H

#include "../board.h"

void latch_init(void);
latch_snapshot_t latch_sample(uint32_t tick, const thermal_snapshot_t *thermal);
float latch_health_score(const sample_history_t *history);

#endif
