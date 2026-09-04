/*
 * sash-sentinel/firmware/drivers/logger.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static log_entry_t g_log[FW_LOG_CAPACITY];
static size_t g_head;
static size_t g_count;

void logger_init(void) {
    memset(g_log, 0, sizeof(g_log));
    g_head = 0u;
    g_count = 0u;
}

void logger_log(alert_level_t level, const char *category, const char *message) {
    log_entry_t *entry = &g_log[g_head];
    entry->timestamp_s = (uint32_t)time(NULL);
    entry->level = level;
    snprintf(entry->category, sizeof(entry->category), "%s", category);
    snprintf(entry->message, sizeof(entry->message), "%s", message);
    g_head = (g_head + 1u) % FW_LOG_CAPACITY;
    if (g_count < FW_LOG_CAPACITY) {
        ++g_count;
    }
}

size_t logger_copy(log_entry_t *dest, size_t capacity) {
    size_t count = g_count < capacity ? g_count : capacity;
    for (size_t i = 0; i < count; ++i) {
        size_t index = (g_head + FW_LOG_CAPACITY - count + i) % FW_LOG_CAPACITY;
        dest[i] = g_log[index];
    }
    return count;
}

size_t logger_count(void) {
    return g_count;
}
