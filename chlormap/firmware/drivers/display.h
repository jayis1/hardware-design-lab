/*
 * display.h — SSD1306 OLED display driver interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef DRIVERS_DISPLAY_H
#define DRIVERS_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize SSD1306 OLED (128x64, SPI) */
bool display_init(void);

/* Clear the display buffer */
void display_clear(void);

/* Draw a string at (x, y) with given font size (1 or 2) */
void display_draw_string(uint8_t x, uint8_t y, const char *str, uint8_t size);

/* Draw a horizontal line */
void display_draw_hline(uint8_t x, uint8_t y, uint8_t w);

/* Draw a filled rectangle */
void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/* Set/clear a single pixel */
void display_set_pixel(uint8_t x, uint8_t y, bool on);

/* Refresh display from buffer */
void display_refresh(void);

/* Show a centered message (helper) */
void display_show_message(const char *msg);

/* Draw a simple bar chart (16-band spectrum) */
void display_draw_spectrum(const int16_t *bands_x1000, uint8_t count);

/* Power off the display */
void display_power_off(void);

/* Power on the display */
void display_power_on(void);

#endif /* DRIVERS_DISPLAY_H */