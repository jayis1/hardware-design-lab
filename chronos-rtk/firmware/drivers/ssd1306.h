/*
 * ssd1306.h — SSD1306 OLED 128x64 I2C display driver
 * For Chronos-RTK status display
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

/* Screen dimensions */
#define SSD1306_WIDTH    128
#define SSD1306_HEIGHT  64
#define SSD1306_PAGES   (SSD1306_HEIGHT / 8)  /* 8 pages */

/* SSD1306 I2C address (SA0=0) */
#define SSD1306_I2C_ADDR    0x3C

/* SSD1306 commands */
#define SSD1306_CMD_SET_DISPLAY_OFF          0xAE
#define SSD1306_CMD_SET_DISPLAY_ON           0xAF
#define SSD1306_CMD_SET_MUX_RATIO            0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET       0xD3
#define SSD1306_CMD_SET_START_LINE           0x40
#define SSD1306_CMD_SET_SEGMENT_REMAP        0xA1
#define SSD1306_CMD_SET_COM_SCAN_DIR         0xC8
#define SSD1306_CMD_SET_COM_PINS            0xDA
#define SSD1306_CMD_SET_CONTRAST            0x81
#define SSD1306_CMD_SET_PRECHARGE            0xD9
#define SSD1306_CMD_SET_VCOM_DETECT          0xDB
#define SSD1306_CMD_SET_ENTIRE_ON            0xA5
#define SSD1306_CMD_SET_ENTIRE_OFF           0xA4
#define SSD1306_CMD_SET_NORMAL_DISPLAY       0xA6
#define SSD1306_CMD_SET_INVERT_DISPLAY       0xA7
#define SSD1306_CMD_SET_OSC_FREQ            0xD5
#define SSD1306_CMD_SET_CHARGE_PUMP          0x8D
#define SSD1306_CMD_SET_MEMORY_MODE          0x20
#define SSD1306_CMD_SET_COL_ADDR             0x21
#define SSD1306_CMD_SET_PAGE_ADDR            0x22
#define SSD1306_CMD_SET_LOWER_COL            0x00
#define SSD1306_CMD_SET_HIGHER_COL           0x10

/* I2C control byte */
#define SSD1306_I2C_CMD_BYTE    0x00
#define SSD1306_I2C_DATA_BYTE   0x40

/* Font: 5x7 ASCII characters */
#define FONT_CHAR_WIDTH   6   /* 5 pixels + 1 spacing */
#define FONT_CHAR_HEIGHT   8

/**
 * Initialize SSD1306 display
 * - Reset via hardware pin
 * - Configure display parameters
 * - Clear display
 * @return 0 on success, -1 on error
 */
int ssd1306_init(void);

/**
 * Clear the display buffer (all pixels off)
 */
void ssd1306_clear(void);

/**
 * Transfer display buffer to OLED via I2C
 */
void ssd1306_display(void);

/**
 * Set a single pixel in the buffer
 * @param x     X coordinate (0-127)
 * @param y     Y coordinate (0-63)
 * @param color 1 = white, 0 = black
 */
void ssd1306_set_pixel(uint8_t x, uint8_t y, uint8_t color);

/**
 * Draw a string at the given position using the built-in font
 * @param x     X position in pixels (0-127)
 * @param page  Y page (0-7, each page is 8 pixels tall)
 * @param str   Null-terminated string
 * @param scale 1 = normal, 2 = double height
 */
void ssd1306_draw_string(uint8_t x, uint8_t page, const char *str, uint8_t scale);

/**
 * Draw a filled rectangle
 * @param x      X start
 * @param y      Y start
 * @param width  Rectangle width
 * @param height Rectangle height
 * @param color  1 = white, 0 = black
 */
void ssd1306_fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);

/**
 * Draw a horizontal line
 * @param x     X start
 * @param page  Y page (0-7)
 * @param width Line width in pixels
 * @param color 1 = white, 0 = black
 */
void ssd1306_draw_hline(uint8_t x, uint8_t page, uint8_t width, uint8_t color);

#endif /* SSD1306_H */