/*
 * spike_sort.h — K-means spike sorting header
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef MYCOMESH_SPIKE_SORT_H
#define MYCOMESH_SPIKE_SORT_H

#include "board.h"
#include <stdint.h>

/* Initialize the spike sorter with empty templates. */
void spike_sort_init(void);

/* Classify a spike event by matching its waveform against stored templates.
 * Updates the spike's template_id field.  If no template matches well
 * enough, a new template is created (up to SPIKE_MAX_TEMPLATES per channel). */
void spike_sort_classify(spike_event_t *spike);

/* Get the current template count for a channel. */
uint8_t spike_sort_template_count(uint8_t channel);

/* Reset templates for a specific channel or all channels (channel=0xFF). */
void spike_sort_reset(uint8_t channel);

/* Get a copy of the templates for a channel (for logging/telemetry). */
uint8_t spike_sort_get_templates(uint8_t channel,
                                 int16_t templates[SPIKE_MAX_TEMPLATES][SPIKE_TEMPLATE_LEN]);

#endif /* MYCOMESH_SPIKE_SORT_H */