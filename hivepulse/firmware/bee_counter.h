/*
 * bee_counter.h — IR break-beam bee traffic counter API for HivePulse
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef BEE_COUNTER_H
#define BEE_COUNTER_H

#include <stdint.h>
#include <stdbool.h>

/* Number of IR break-beam channels across the hive entrance */
#define BEE_COUNTER_CHANNELS  16

/* Bee traffic data — accumulated counts per poll interval */
typedef struct {
    uint32_t bees_in;        /* Count of bees entering hive */
    uint32_t bees_out;       /* Count of bees exiting hive */
    uint32_t total_crossings; /* Total beam breaks (both directions) */
    uint32_t peak_rate;      /* Max crossings/sec in this interval */
    float avg_activity;      /* Average crossings per second */
    uint8_t active_channels; /* Bitmap of channels currently blocked */
    uint32_t timestamp_ms;
} bee_traffic_t;

/* Initialize bee counter: configure GPIO EXTI, set up state tracking */
int bee_counter_init(void);

/* Poll the bee counter and return accumulated traffic since last poll */
int bee_counter_poll(bee_traffic_t *traffic);

/* Get current real-time activity (for BLE live display) */
int bee_counter_get_live(uint32_t *in_last_min, uint32_t *out_last_min);

/* Reset all counters */
void bee_counter_reset(void);

/* EXTI interrupt handler for beam-break events */
void bee_counter_exti_handler(void);

#endif /* BEE_COUNTER_H */