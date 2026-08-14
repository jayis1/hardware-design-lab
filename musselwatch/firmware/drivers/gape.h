/*
 * drivers/gape.h — Bivalve shell-gape (valvometric) analysis engine
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#ifndef MUSSEL_GAPE_H
#define MUSSEL_GAPE_H

#include <stdint.h>
#include "../board.h"

void    gape_init(channel_state_t *ch);
void    gape_update(channel_state_t *ch, uint16_t raw_adc);
void    gape_calibrate(channel_state_t *ch, uint16_t raw_closed);
uint8_t gape_anomaly_score(const channel_state_t *ch);
int16_t gape_raw_to_um(uint16_t raw, uint16_t baseline);

/* Ring buffer for rolling baseline & activity computation */
#define GAPE_BUF_LEN 64u

typedef struct {
    uint16_t buf[GAPE_BUF_LEN];
    uint8_t  head;
    uint8_t  count;
    uint32_t sum;
} gape_ring_t;

void    ring_init(gape_ring_t *r);
void    ring_push(gape_ring_t *r, uint16_t val);
uint16_t ring_mean(const gape_ring_t *r);
uint16_t ring_stdev(const gape_ring_t *r);

#endif