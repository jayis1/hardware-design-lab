/*
 * power.c — power and run-state tracking
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <math.h>
#include "power.h"

static float g_battery = 98.0f;
static float g_battery_temp = 27.0f;

void power_init(void) {
    g_battery = 98.0f;
    g_battery_temp = 27.0f;
}

void power_update(dfg_sensor_frame_t *frame, uint32_t tick) {
    if (tick < 6U) {
        frame->run_state = DFG_RUN_STARTING;
    } else if (tick < 50U) {
        frame->run_state = DFG_RUN_ACTIVE;
    } else if (tick < 72U) {
        frame->run_state = DFG_RUN_DRYDOWN;
    } else if (tick < 80U) {
        frame->run_state = DFG_RUN_COMPLETE;
    } else {
        frame->run_state = DFG_RUN_IDLE;
    }

    g_battery -= 0.006f;
    g_battery_temp = 26.5f + 2.0f * sinf((float)tick / 14.0f);

    if (g_battery < 0.0f) {
        g_battery = 0.0f;
    }

    frame->battery_pct = g_battery;
    frame->battery_temp_c = g_battery_temp;
}
