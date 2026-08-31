/*
 * Threshold Veil logger driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "logger.h"

#include <stdio.h>
#include <string.h>

#include "inference.h"

void logger_init(tv_logger_t *logger)
{
    memset(logger, 0, sizeof(*logger));
}

void logger_push(tv_logger_t *logger,
                 uint32_t tick,
                 const tv_inference_t *inf,
                 const tv_env_frame_t *env,
                 const tv_power_frame_t *power)
{
    tv_log_entry_t *entry = &logger->entries[logger->head];
    entry->tick = tick;
    entry->state = inf->state;
    entry->ingress_score = inf->ingress_score;
    entry->battery_pct = power->battery_pct;
    entry->corridor_pm25_ugm3 = env->corridor_pm25_ugm3;
    entry->pressure_pa = env->pressure_pa;
    snprintf(entry->note, sizeof(entry->note), "%s", inf->recommendation);

    logger->head = (logger->head + 1U) % TV_EVENT_LOG_SIZE;
    if (logger->count < TV_EVENT_LOG_SIZE) {
        logger->count++;
    }
}

void logger_print(const tv_logger_t *logger)
{
    size_t start = (logger->head + TV_EVENT_LOG_SIZE - logger->count) % TV_EVENT_LOG_SIZE;
    puts("\nRecent event log:");
    for (size_t i = 0; i < logger->count; ++i) {
        const tv_log_entry_t *entry = &logger->entries[(start + i) % TV_EVENT_LOG_SIZE];
        printf("  [%02u] %-14s ingress=%4.2f pm25=%5.1f pressure=%5.2f batt=%5.1f%% %s\n",
               entry->tick,
               inference_state_name(entry->state),
               entry->ingress_score,
               entry->corridor_pm25_ugm3,
               entry->pressure_pa,
               entry->battery_pct,
               entry->note);
    }
}
