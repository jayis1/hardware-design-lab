/*
 * display.h — ILI9341 TFT display driver and UI
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_DISPLAY_H
#define WATTLENS_DISPLAY_H

#include "board.h"

void display_init(void);
void display_show_boot(const char *msg);
void display_show_main(void);
void display_show_error(const char *msg);
void display_show_warning(const char *msg);
void display_update_metrics(const power_metrics_t *m, const nilm_result_t *n, uint8_t battery);
void display_draw_gauge(int x, int y, int w, int h, float value, float min, float max, const char *label);
void display_draw_bar(int x, int y, int w, int h, float frac, uint16_t color);
void display_clear(uint16_t color);
void display_text(int x, int y, const char *str, uint16_t fg, uint16_t bg, int size);

#endif /* WATTLENS_DISPLAY_H */