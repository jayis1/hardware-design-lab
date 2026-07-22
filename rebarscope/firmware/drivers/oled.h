/*
 * drivers/oled.h — SSD1306 OLED interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_OLED_H
#define REBARSCOPE_OLED_H

#include <stdint.h>

void oled_init(void);
void oled_clear(void);
void oled_set_pixel(uint8_t x, uint8_t y, uint8_t on);
void oled_flush(void);
void oled_draw_string(uint8_t x, uint8_t y, const char *s, const uint8_t *font, uint8_t h);
void oled_draw_hline(uint8_t x1, uint8_t x2, uint8_t y);
void oled_draw_bar(uint8_t x, uint8_t y, uint8_t w, uint8_t pct);
void oled_powerdown(void);

#endif /* REBARSCOPE_OLED_H */