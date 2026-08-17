/*
 * display.h — OLED Status Display Driver (SSD1306)
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_DISPLAY_H
#define LIGNOSCAN_DISPLAY_H

#include <stdint.h>

#define OLED_WIDTH   128
#define OLED_HEIGHT  64

void display_init(void);
void display_clear(void);
void display_refresh(void);
void display_draw_pixel(int x, int y, int on);
void display_draw_string(int x, int y, const char *str, int size);
void display_draw_line(int x0, int y0, int x1, int y1);
void display_draw_rect(int x, int y, int w, int h);
void display_draw_circle(int cx, int cy, int r);
void display_draw_tomogram(int x, int y, int size, const float *velocity);

#endif /* LIGNOSCAN_DISPLAY_H */