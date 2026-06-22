/*
 * display.h — SSD1306 OLED display driver
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef LUMICAST_DISPLAY_H
#define LUMICAST_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_PAGES   (OLED_HEIGHT / 8)

void display_init(void);
void display_clear(void);
void display_flush(void);
void display_set_contrast(uint8_t val);
void display_set_backlight(uint8_t pct);
void display_pixel(int x, int y, bool on);
void display_hline(int x, int y, int w);
void display_vline(int x, int y, int h);
void display_rect(int x, int y, int w, int h, bool fill);
void display_text(int x, int y, const char *str, int size);
void display_text_fmt(int x, int y, const char *str, int size, bool invert);
void display_big_num(int x, int y, float val, int digits);
void display_bar(int x, int y, int w, int h, float pct);
void display_spectrogram(int x, int y, const float *vals, int n, float max_val);

#endif /* LUMICAST_DISPLAY_H */