/*
 * oled_drv.h — SH1106 OLED display driver (128x64, SPI)
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_OLED_DRV_H
#define ALLOYSCAN_OLED_DRV_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize OLED display */
void oled_drv_init(void);

/* Clear the display buffer */
void oled_clear(void);

/* Flush buffer to display */
void oled_flush(void);

/* Set a pixel in the buffer (0,0 = top-left) */
void oled_set_pixel(int x, int y, bool on);

/* Draw a character at (x, y) using 6x8 font.
 * Returns the x position after the character (x + 6) */
int oled_draw_char(int x, int y, char c, bool invert);

/* Draw a string at (x, y) */
void oled_draw_string(int x, int y, const char *str, bool invert);

/* Draw a horizontal line */
void oled_draw_hline(int x, int y, int w, bool on);

/* Draw a vertical line */
void oled_draw_vline(int x, int y, int h, bool on);

/* Draw a rectangle outline */
void oled_draw_rect(int x, int y, int w, int h, bool on);

/* Draw a filled rectangle */
void oled_fill_rect(int x, int y, int w, int h, bool on);

/* Draw a progress bar (0-100%) */
void oled_draw_progress(int x, int y, int w, int h, int percent);

/* Set display on/off */
void oled_set_display(bool on);

/* Set inversion mode */
void oled_set_invert(bool on);

/* Get display width/height */
int oled_get_width(void);
int oled_get_height(void);

#endif /* ALLOYSCAN_OLED_DRV_H */