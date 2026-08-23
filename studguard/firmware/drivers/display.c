/*
 * display.c — StudGuard local UI abstraction
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "display.h"
#include <stdio.h>

static const char *mode_name(sg_mode_t mode) {
    switch (mode) {
        case MODE_BASELINE: return "BASELINE";
        case MODE_MONITORING: return "MONITOR";
        case MODE_DIAGNOSTIC: return "DIAG";
        case MODE_REPAIR_VERIFICATION: return "VERIFY";
        case MODE_SLEEPY: return "SLEEP";
        default: return "UNKNOWN";
    }
}

static const char *event_name(sg_event_t event) {
    switch (event) {
        case EVENT_NONE: return "none";
        case EVENT_CONDENSATION: return "condensation";
        case EVENT_INTERMITTENT_LEAK: return "intermittent";
        case EVENT_ACTIVE_PRESSURE_LEAK: return "active leak";
        case EVENT_POST_REPAIR_DRYING: return "drying";
        case EVENT_SENSOR_FAULT: return "sensor fault";
        default: return "unknown";
    }
}

void display_init(void) {
    printf("[display] StudGuard local display ready\n");
}

void display_render_status(const sg_device_status_t *status, const sg_measurement_t *measurement) {
    printf("[display] node=%u mode=%s batt=%.1f%% leak=%.2f spread=%.2f conf=%.2f origin=%.2fm event=%s\n",
           status->node_id,
           mode_name(status->mode),
           status->battery_percent,
           measurement->leak_activity,
           measurement->wetness_spread,
           measurement->confidence,
           measurement->origin_band,
           event_name(measurement->event));
}

void display_render_banner(const char *text) {
    printf("[display] %s\n", text);
}
