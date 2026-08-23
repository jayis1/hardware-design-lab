/*
 * power.h — StudGuard power and interval policy
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef STUDGUARD_POWER_H
#define STUDGUARD_POWER_H

#include <stdint.h>
#include "../board.h"

typedef struct {
    float battery_percent;
    float voltage_mv;
    float current_ma;
    uint32_t interval_ms;
    uint8_t charging;
} power_state_t;

void power_init(power_state_t *state);
void power_tick(power_state_t *state, sg_mode_t mode, float leak_activity, float confidence, uint32_t elapsed_ms);
uint32_t power_choose_interval(const power_state_t *state, sg_mode_t mode, float leak_activity, float confidence);

#endif
