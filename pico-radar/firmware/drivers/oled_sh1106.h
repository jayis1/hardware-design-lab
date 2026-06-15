/*
 * drivers/oled_sh1106.h — SH1106 128×64 OLED driver for PicoRadar
 * I2C1 interface on STM32H743
 */

#ifndef OLED_SH1106_H
#define OLED_SH1106_H

#include <stdint.h>

#define OLED_WIDTH    128
#define OLED_HEIGHT   64
#define OLED_PAGES    8
#define OLED_I2C_ADDR 0x3C

/* SH1106 command constants */
#define SH1106_CTRL_CMD        0x00
#define SH1106_CTRL_DATA       0x40
#define SH1106_DISPLAY_OFF     0xAE
#define SH1106_DISPLAY_ON      0xAF
#define SH1106_SET_DISP_START  0x40
#define SH1106_SET_PAGE_ADDR   0xB0
#define SH1106_SET_COL_LO      0x00
#define SH1106_SET_COL_HI      0x10
#define SH1106_SEG_REMAP       0xA1
#define SH1106_COM_SCAN_INV    0xC8
#define SH1106_SET_CONTRAST    0x81
#define SH1106_SET_MUX_RATIO   0xA8
#define SH1106_SET_DISP_OFFSET 0xD3
#define SH1106_SET_PRECHARGE   0xD9
#define SH1106_SET_VCOMH       0xDB
#define SH1106_SET_CHARGE_PUMP 0x8D

/* Font table */
extern const uint8_t oled_font8x8[96][8];

typedef enum {
    OLED_COLOR_BLACK = 0,
    OLED_COLOR_WHITE = 1,
    OLED_COLOR_INVERT = 2
} oled_color_t;

int  oled_init(void);
void oled_power_on(void);
void oled_power_off(void);
void oled_set_contrast(uint8_t level);
void oled_clear(void);
void oled_update(void);
void oled_draw_pixel(uint8_t x, uint8_t y, oled_color_t color);
void oled_draw_char(uint8_t x, uint8_t y, char c, oled_color_t color);
void oled_draw_string(uint8_t x, uint8_t y, const char *str, oled_color_t color);
void oled_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, oled_color_t color);
void oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_color_t color);
void oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, oled_color_t color);
void oled_invert_display(uint8_t on);

#endif /* OLED_SH1106_H */