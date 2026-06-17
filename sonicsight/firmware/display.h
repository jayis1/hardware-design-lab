/**
 * @file    display.h
 * @brief   SonicSight — GC9A01 240×240 IPS LCD driver.
 *          SPI-based, with draw_pixel, text rendering, splash screen,
 *          progress bar, and image display.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "board.h"

/**
 * @brief Initialise GC9A01 LCD.
 */
void display_init(void);

/**
 * @brief Show splash screen with firmware version.
 * @param name    Device name string.
 * @param ver_maj Major version.
 * @param ver_min Minor version.
 * @param ver_pat Patch version.
 */
void display_show_splash(const char *name, uint32_t ver_maj,
                          uint32_t ver_min, uint32_t ver_pat);

/**
 * @brief Show a text message centred on the display.
 * @param msg Null-terminated message string (max 32 chars).
 */
void display_show_message(const char *msg);

/**
 * @brief Show a progress bar.
 * @param current Current value.
 * @param total   Total value.
 */
void display_show_progress(uint32_t current, uint32_t total);

/**
 * @brief Show error code on screen.
 * @param error_code Error code from register.h definitions.
 */
void display_show_error(int32_t error_code);

/**
 * @brief Draw a single pixel at (x, y).
 * @param x      Column (0..LCD_WIDTH-1).
 * @param y      Row (0..LCD_HEIGHT-1).
 * @param colour 16-bit RGB565 colour.
 */
void display_draw_pixel(uint32_t x, uint32_t y, uint16_t colour);

/**
 * @brief Draw text at (x, y).
 * @param x       Column position.
 * @param y       Row position.
 * @param text    Null-terminated string.
 * @param fg_col  Foreground colour (RGB565).
 * @param bg_col  Background colour (RGB565).
 */
void display_draw_text(uint32_t x, uint32_t y, const char *text,
                        uint16_t fg_col, uint16_t bg_col);

/**
 * @brief Fill entire screen with a colour.
 * @param colour 16-bit RGB565 colour.
 */
void display_fill(uint16_t colour);

#endif /* DISPLAY_H */