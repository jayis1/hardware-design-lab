/*
 * event_detect.h — Power quality event detection
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_EVENT_DETECT_H
#define WATTLENS_EVENT_DETECT_H

#include "board.h"

void event_detect_init(void);
void event_detect_check(const power_metrics_t *m, device_ctx_t *ctx);
void event_detect_nilm(const nilm_result_t *nilm, device_ctx_t *ctx);
int  event_detect_pop(device_ctx_t *ctx, pq_event_t *evt);

#endif /* WATTLENS_EVENT_DETECT_H */