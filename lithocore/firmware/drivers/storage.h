/*
 * storage.h — Flash ring buffer for cell test results
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_STORAGE_H
#define LITHOCORE_STORAGE_H

#include <stdint.h>
#include "soh.h"
#include "../board.h"

/* We reserve the last 16 flash pages (32 KB) for result storage.
 * Each result is stored as a compact record (the full sweep data is
 * not stored — only the summary + Randles parameters + a few key
 * impedance points). This allows storing up to 256 results. */

#define STORAGE_PAGE_START   240   /* flash page index */
#define STORAGE_PAGE_COUNT   16
#define STORAGE_MAX_RESULTS  256

int  storage_init(void);
int  storage_save_result(const soh_result_t *result);
int  storage_load_config(litho_config_t *config);
int  storage_save_config(const litho_config_t *config);
int  storage_get_result(uint16_t index, soh_result_t *result);
int  storage_get_count(uint16_t *count);
void storage_send_history_ble(void);
void storage_clear(void);

#endif /* LITHOCORE_STORAGE_H */