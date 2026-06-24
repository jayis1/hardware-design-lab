/*
 * nilm.h — Non-Intrusive Load Monitoring neural network
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_NILM_H
#define WATTLENS_NILM_H

#include "board.h"

void  nilm_init(void);
int   nilm_load_model(void);
void  nilm_infer(const power_metrics_t *m, const harmonic_data_t *h, nilm_result_t *r);
int   nilm_get_appliance_name(int class_id, char *name, int len);
int   nilm_get_num_classes(void);
void  nilm_set_appliance_name(int class_id, const char *name);

/* Appliance class names (default set — user-configurable) */
extern const char *nilm_default_names[NILM_MAX_CLASSES];

#endif /* WATTLENS_NILM_H */