/*
 * display.h — SSD1327 OLED Display Driver (header)
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef ROOTTRACE_DISPLAY_H
#define ROOTTRACE_DISPLAY_H

#include <stdint.h>

void display_init(void);
void display_clear(void);
void display_flush(void);
void display_set_pixel(int x, int y, uint8_t gray);  /* 0-15 */
void display_draw_text(int x, int y, const char *str, uint8_t gray);
void display_draw_rect(int x, int y, int w, int h, uint8_t gray);
void display_fill_rect(int x, int y, int w, int h, uint8_t gray);
void display_draw_conductivity_map(const float *image, int w, int h);
void display_draw_electrode_ring(int cx, int cy, int r, uint16_t contact_mask);
void display_show_status(const char *line1, const char *line2);
void display_show_menu(const char *title, const char *items[], int n_items, int selected);
void display_draw_progress(int x, int y, int w, int h, float pct);

#endif /* ROOTTRACE_DISPLAY_H */