/*
 * CrisperCue event logger
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "logger.h"

void logger_init(event_log_t *events, snapshot_log_t *snapshots)
{
    events->count = 0u;
    snapshots->count = 0u;
}

void logger_push_event(event_log_t *events, uint32_t cycle, alert_level_t level, const char *code, const char *detail)
{
    crisper_event_t *event;
    if (events->count >= CRISPERCUE_EVENT_CAPACITY) {
        return;
    }
    event = &events->items[events->count++];
    event->cycle_index = cycle;
    event->level = level;
    strncpy(event->code, code, sizeof(event->code) - 1u);
    event->code[sizeof(event->code) - 1u] = '\0';
    strncpy(event->detail, detail, sizeof(event->detail) - 1u);
    event->detail[sizeof(event->detail) - 1u] = '\0';
}

void logger_push_snapshot(snapshot_log_t *snapshots, const crisper_snapshot_t *snapshot)
{
    if (snapshots->count >= CRISPERCUE_SNAPSHOT_CAPACITY) {
        return;
    }
    snapshots->items[snapshots->count++] = *snapshot;
}

void logger_print_summary(const event_log_t *events, const snapshot_log_t *snapshots)
{
    size_t i;
    float average_freshness = 0.0f;
    float peak_ethylene = 0.0f;
    float lowest_mass = snapshots->count ? snapshots->items[0].mass.tray_mass_g : 0.0f;

    for (i = 0u; i < snapshots->count; ++i) {
        average_freshness += snapshots->items[i].inference.freshness_score;
        if (snapshots->items[i].gas.ethylene_ppm > peak_ethylene) {
            peak_ethylene = snapshots->items[i].gas.ethylene_ppm;
        }
        if (snapshots->items[i].mass.tray_mass_g < lowest_mass) {
            lowest_mass = snapshots->items[i].mass.tray_mass_g;
        }
    }
    if (snapshots->count) {
        average_freshness /= (float)snapshots->count;
    }

    printf("\nSummary for %s by %s\n", CRISPERCUE_DEVICE_NAME, CRISPERCUE_AUTHOR);
    printf("  Samples captured: %zu\n", snapshots->count);
    printf("  Events logged: %zu\n", events->count);
    printf("  Average freshness: %.2f\n", average_freshness);
    printf("  Peak ethylene ppm: %.3f\n", peak_ethylene);
    printf("  Lowest tray mass: %.1f g\n", lowest_mass);

    for (i = 0u; i < events->count; ++i) {
        printf("  Event[%02zu] cycle=%02u level=%u code=%s detail=%s\n",
               i,
               events->items[i].cycle_index,
               (unsigned)events->items[i].level,
               events->items[i].code,
               events->items[i].detail);
    }
}
