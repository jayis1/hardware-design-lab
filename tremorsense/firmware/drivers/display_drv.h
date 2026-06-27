/*
 * display_drv.h — GC9A01 OLED display driver header
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
#ifndef TREMORSENSE_DISPLAY_DRV_H
#define TREMORSENSE_DISPLAY_DRV_H

#include <stdint.h>

int   display_init(void);
void  display_clear(void);
void  display_on(void);
void  display_off(void);
void  display_set_backlight(uint8_t on);

/* Drawing primitives */
void  display_fill_screen(uint16_t color);
void  display_draw_pixel(int x, int y, uint16_t color);
void  display_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void  display_draw_rect(int x, int y, int w, int h, uint16_t color);
void  display_fill_rect(int x, int y, int w, int h, uint16_t color);
void  display_draw_circle(int cx, int cy, int r, uint16_t color);
void  display_fill_circle(int cx, int cy, int r, uint16_t color);
void  display_draw_text(int x, int y, const char *text, uint16_t color,
                         uint8_t size);
void  display_draw_char(int x, int y, char c, uint16_t color, uint8_t size);
void  display_draw_arc(int cx, int cy, int r, int start_angle, int end_angle,
                       uint16_t color);

/* High-level UI screens */
void  display_show_splash(const char *title, const char *subtitle);
void  display_show_idle(uint16_t tremor_score, uint8_t battery_pct,
                        uint8_t tremor_active);
void  display_show_waveform(const float *data, int n_samples,
                             uint8_t tremor_active, float dom_freq);
void  display_show_spectrum(const float *psd, int n_bins, float dom_freq);
void  display_show_score(uint16_t score, uint32_t tremor_seconds,
                          uint8_t tremor_class, float confidence);
void  display_show_battery(uint8_t pct, uint8_t charging,
                            uint16_t runtime_hours);
void  display_show_low_battery(void);
void  display_show_message(const char *line1, const char *line2);

/* Font support */
void  display_set_font(uint8_t font_id);

#endif /* TREMORSENSE_DISPLAY_DRV_H */