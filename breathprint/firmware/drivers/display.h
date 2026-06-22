/*
 * display.h — SSD1306 OLED display header
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <string.h>
#include "../board.h"

void display_init(void);
void display_flush(void);
void display_clear(void);
void display_set_pixel(int x, int y, int on);
void display_invert_pixel(int x, int y);
void display_draw_char(int x, int y, char c);
void display_draw_string(int x, int y, const char *str);
void display_draw_string_large(int x, int y, const char *str);
void display_show_splash(void);
void display_show_idle(uint8_t battery, uint8_t connected,
                       const breath_result_t *last);
void display_show_warmup(void);
void display_show_warmup_progress(uint32_t elapsed, uint32_t total);
void display_show_breathe_prompt(void);
void display_show_analyzing(void);
void display_show_result(const breath_result_t *result);
void display_show_error(const char *msg);
void display_show_low_battery(uint8_t pct);
void display_show_calibration(void);
void display_show_calibration_done(void);
void display_on(void);
void display_off(void);

#endif /* DISPLAY_H */