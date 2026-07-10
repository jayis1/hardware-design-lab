/*
 * flash_log.h — QSPI flash event buffering for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef OCCLUSOGRAPH_FLASH_LOG_H
#define OCCLUSOGRAPH_FLASH_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "classify.h"

/* Initialize flash and the event-log write pointer. */
int  flash_log_init(void);

/* Append one event record to the log. Returns 0 on success. */
int  flash_log_append(const event_record_t *evt);

/* Read event records from a given start index. Returns count actually read. */
uint32_t flash_log_read(uint32_t start_idx, event_record_t *out, uint32_t max_count);

/* Get total number of events currently stored. */
uint32_t flash_log_count(void);

/* Erase all events (used after sync with app). */
int  flash_log_erase_all(void);

/* Mark events as synced (up to and including the given index). */
int  flash_log_mark_synced(uint32_t upto_idx);

/* Get the highest unsynced index. */
uint32_t flash_log_unsynced_idx(void);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSOGRAPH_FLASH_LOG_H */