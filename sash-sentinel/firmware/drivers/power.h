/*
 * sash-sentinel/firmware/drivers/power.h
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef SASH_SENTINEL_POWER_H
#define SASH_SENTINEL_POWER_H

#include "../board.h"

void power_init(void);
power_snapshot_t power_sample(uint32_t tick, bool radio_active);
void power_set_mode(power_mode_t mode);
power_mode_t power_get_mode(void);

#endif
