/*
 * storage.h — Data Logging Storage Header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_STORAGE_H
#define FERMENTIQ_STORAGE_H

#include "board.h"

/* API */
int storage_init(void);
int storage_log_sample(const fermentiq_state_t *state);
int storage_export_batch(const char *filepath);
int storage_get_stats(uint32_t *total_samples, uint32_t *batch_samples);

#endif /* FERMENTIQ_STORAGE_H */