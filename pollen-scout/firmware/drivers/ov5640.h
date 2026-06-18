/*
 * ov5640.h - OV5640 camera driver header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef OV5640_H
#define OV5640_H
#include <stdint.h>
#include <stddef.h>

typedef enum {
    OV5640_STROBE_WHITE = 0,
    OV5640_STROBE_UV    = 1,
} ov5640_strobe_t;

int  ov5640_init(void);
void ov5640_strobe_select(ov5640_strobe_t sel);
int  ov5640_capture(uint8_t *buf, size_t len);
const uint8_t *ov5640_last_frame(void);

#endif /* OV5640_H */