/*
 * sdlog.h — microSD FAT32 logging
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_SDLOG_H
#define WATTLENS_SDLOG_H

#include "board.h"

void sdlog_init(void);
int  sdlog_write_metrics(const power_metrics_t *m, const nilm_result_t *n, uint32_t uptime);
int  sdlog_write_event(const pq_event_t *evt);
int  sdlog_write_waveform(const float *v[3], const float *i[4], uint32_t timestamp);
int  sdlog_flush(void);
bool sdlog_is_present(void);

#endif /* WATTLENS_SDLOG_H */