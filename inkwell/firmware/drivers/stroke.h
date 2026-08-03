/*
 * stroke.h — Stroke segmentation and journal record builder
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */
#ifndef INKWELL_DRIVERS_STROKE_H
#define INKWELL_DRIVERS_STROKE_H

#include <stdint.h>
#include <stdbool.h>

/* Stroke segment record. Matches the BLE notification payload layout. */
typedef struct {
    uint8_t  flags;      /* bit0: pen-down, bit1: stroke-start, bit2: stroke-end,
                          * bit3: optical-flow-valid, bit4-7: reserved */
    uint32_t seq;        /* monotonically increasing sequence number */
    uint32_t ts_ms;      /* session-relative timestamp */
    int32_t  dx_um;     /* body-frame x delta since last segment, µm */
    int32_t  dy_um;     /* body-frame y delta since last segment, µm */
    uint16_t p_mN;      /* mean pressure in millinewtons */
    uint8_t  crc8;      /* per-record integrity */
} stroke_segment_t;

void stroke_init(uint16_t pen_down_mN, uint16_t pen_up_mN, uint8_t debounce);
void stroke_feed(uint32_t ts_ms, int32_t dx_um, int32_t dy_um,
                 uint16_t p_mN, bool pen_down);
bool stroke_pop_segment(stroke_segment_t *out);

#endif