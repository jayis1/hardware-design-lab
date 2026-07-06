/*
 * oled_drv.h — TactiScript OLED status display driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef TACTISCRIPT_OLED_DRV_H
#define TACTISCRIPT_OLED_DRV_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize the SSD1316 OLED over I2C */
void oled_init(void);

/* Clear the display buffer (all pixels off) */
void oled_clear(void);

/* Draw a string at (col, page) using the built-in 5×7 font.
 *   col: 0-59 (character columns, 5px each → 12 chars max on 64px)
 *   page: 0-3 (8-pixel rows, 4 pages on 32px display)
 *   str: null-terminated ASCII string
 */
void oled_draw_string(uint8_t col, uint8_t page, const char *str);

/* Draw a single character at (col, page) */
void oled_draw_char(uint8_t col, uint8_t page, char c);

/* Set a pixel at (x, y) */
void oled_set_pixel(uint8_t x, uint8_t y, bool on);

/* Flush the display buffer to the OLED via I2C */
void oled_display_flush(void);

/* Turn display on/off (for sleep mode) */
void oled_display_on(void);
void oled_display_off(void);

/* Set contrast (0-255) */
void oled_set_contrast(uint8_t level);

#endif /* TACTISCRIPT_OLED_DRV_H */