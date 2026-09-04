/*
 * sash-sentinel/firmware/drivers/inference.h
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef SASH_SENTINEL_INFERENCE_H
#define SASH_SENTINEL_INFERENCE_H

#include "../board.h"

void inference_init(void);
risk_report_t inference_evaluate(const sample_history_t *history, const device_config_t *config);

#endif
