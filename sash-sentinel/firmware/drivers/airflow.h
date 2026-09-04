/*
 * sash-sentinel/firmware/drivers/airflow.h
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef SASH_SENTINEL_AIRFLOW_H
#define SASH_SENTINEL_AIRFLOW_H

#include "../board.h"

void airflow_init(void);
airflow_snapshot_t airflow_sample(uint32_t tick, const env_snapshot_t *env, const latch_snapshot_t *latch);
float airflow_energy_loss_score(const sample_history_t *history);

#endif
