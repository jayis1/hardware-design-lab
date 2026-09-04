/*
 * sash-sentinel/firmware/drivers/power.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "power.h"

#include <math.h>

static power_mode_t g_mode = POWER_MODE_BOOT;

void power_init(void) {
    g_mode = POWER_MODE_ACTIVE;
}

void power_set_mode(power_mode_t mode) {
    g_mode = mode;
}

power_mode_t power_get_mode(void) {
    return g_mode;
}

power_snapshot_t power_sample(uint32_t tick, bool radio_active) {
    power_snapshot_t snapshot;
    float load = radio_active ? 1.0f : 0.0f;

    snapshot.battery_voltage_v = 4.08f - (float)(tick % 50u) * 0.011f;
    snapshot.battery_percent = board_clampf((snapshot.battery_voltage_v - 3.3f) / 0.9f * 100.0f, 1.0f, 100.0f);
    snapshot.current_ma = 32.0f + load * 18.0f + fabsf(sinf((float)tick * 0.52f)) * 6.0f;
    snapshot.usb_present = (tick % 19u) == 0u;
    snapshot.mode = snapshot.usb_present ? POWER_MODE_CHARGING : g_mode;
    return snapshot;
}
