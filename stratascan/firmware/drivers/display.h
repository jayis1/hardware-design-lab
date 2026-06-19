/*
 * display.h — SSD1309 OLED Display Driver
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef STRATASCAN_DISPLAY_H
#define STRATASCAN_DISPLAY_H

#include <stdint.h>
#include "../board.h"

int  display_init(void);
void display_clear(void);
void display_draw_text(uint8_t x, uint8_t y, const char *text);
void display_draw_ascan_preview(const float *trace, uint16_t n);
void display_draw_status(uint32_t traces, uint8_t battery,
                          uint8_t band, sys_state_t state);

#endif /* STRATASCAN_DISPLAY_H */