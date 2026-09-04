/*
 * sash-sentinel/firmware/drivers/logger.h
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef SASH_SENTINEL_LOGGER_H
#define SASH_SENTINEL_LOGGER_H

#include "../board.h"

void logger_init(void);
void logger_log(alert_level_t level, const char *category, const char *message);
size_t logger_copy(log_entry_t *dest, size_t capacity);
size_t logger_count(void);

#endif
