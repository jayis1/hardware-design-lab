/*
 * display.h — ILI9341 TFT display driver interface
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef SPECKLEFLOW_DISPLAY_H
#define SPECKLEFLOW_DISPLAY_H

#include <stdint.h>
#include "board.h"

/**
 * Initialize the ILI9341 display (reset, config, backlight on).
 * @return 0 on success
 */
int display_init(void);

/**
 * Set the active colormap for flow-map rendering.
 */
void display_set_colormap(enum colormap_id id);

/**
 * Set the active drawing window (for partial updates).
 */
void display_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/**
 * Push the framebuffer to the display via SPI DMA.
 */
void display_present(void);

/**
 * Render a flow map (8-bit K values) to the framebuffer with the
 * active colormap, downscaling to 320×240.
 */
void display_render_flow(const uint8_t *flow_map, uint16_t src_w,
                         uint16_t src_h);

/**
 * Draw a single pixel in the framebuffer.
 */
void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * Draw a rectangle outline.
 */
void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color);

/**
 * Fill a rectangle.
 */
void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color);

/**
 * Draw text (simple 8×8 font).
 */
void display_draw_text(uint16_t x, uint16_t y, const char *text,
                       uint16_t fg, uint16_t bg);

/**
 * Clear the framebuffer to a solid color.
 */
void display_clear(uint16_t color);

/**
 * Convert RGB888 to RGB565.
 */
uint16_t display_rgb565(uint8_t r, uint8_t g, uint8_t b);

#endif /* SPECKLEFLOW_DISPLAY_H */