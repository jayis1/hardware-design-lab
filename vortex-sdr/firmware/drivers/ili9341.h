/*
 * ili9341.h — ILI9341 2.8" TFT LCD driver (320x240, SPI, capacitive touch)
 */

#ifndef VORTEX_SDR_ILI9341_H
#define VORTEX_SDR_ILI9341_H

#include <stdint.h>

/* Display dimensions */
#define ILI9341_WIDTH          320
#define ILI9341_HEIGHT         240

/* Color definitions (RGB565) */
#define ILI9341_COLOR_BLACK    0x0000
#define ILI9341_COLOR_WHITE    0xFFFF
#define ILI9341_COLOR_RED      0xF800
#define ILI9341_COLOR_GREEN    0x07E0
#define ILI9341_COLOR_BLUE     0x001F
#define ILI9341_COLOR_CYAN     0x07FF
#define ILI9341_COLOR_MAGENTA  0xF81F
#define ILI9341_COLOR_YELLOW   0xFFE0
#define ILI9341_COLOR_ORANGE   0xFD20
#define ILI9341_COLOR_GRAY     0x8410
#define ILI9341_COLOR_DGRAY    0x4208

/* ILI9341 command definitions */
#define ILI9341_CMD_NOP            0x00
#define ILI9341_CMD_SWRESET        0x01
#define ILI9341_CMD_RDDID          0x04
#define ILI9341_CMD_RDDST          0x09
#define ILI9341_CMD_RDDPM          0x0A
#define ILI9341_CMD_RDDMADCTL     0x0B
#define ILI9341_CMD_RDDCOLMOD     0x0C
#define ILI9341_CMD_RDDIM          0x0D
#define ILI9341_CMD_RDDSDR        0x0F
#define ILI9341_CMD_SLPIN          0x10
#define ILI9341_CMD_SLPOUT         0x11
#define ILI9341_CMD_PTLON          0x12
#define ILI9341_CMD_NORON          0x13
#define ILI9341_CMD_RDDMADCTL2    0x0B
#define ILI9341_CMD_INVOFF         0x20
#define ILI9341_CMD_INVON          0x21
#define ILI9341_CMD_GAMSET         0x26
#define ILI9341_CMD_DISPOFF        0x28
#define ILI9341_CMD_DISPON         0x29
#define ILI9341_CMD_CASET          0x2A
#define ILI9341_CMD_PASET          0x2B
#define ILI9341_CMD_RAMWR          0x2C
#define ILI9341_CMD_RAMRD          0x2E
#define ILI9341_CMD_MADCTL         0x36
#define ILI9341_CMD_VSCSAD         0x37
#define ILI9341_CMD_PIXSET         0x3A
#define ILI9341_CMD_FRMCTR1        0xB1
#define ILI9341_CMD_FRMCTR2        0xB2
#define ILI9341_CMD_FRMCTR3        0xB3
#define ILI9341_CMD_INVCTR         0xB4
#define ILI9341_CMD_DFUNCTR        0xB6
#define ILI9341_CMD_PWCTR1         0xC0
#define ILI9341_CMD_PWCTR2         0xC1
#define ILI9341_CMD_PWCTR3         0xC2
#define ILI9341_CMD_PWCTR4         0xC3
#define ILI9341_CMD_PWCTR5         0xC4
#define ILI9341_CMD_VMCTR1         0xC5
#define ILI9341_CMD_VMCTR2         0xC7
#define ILI9341_CMD_RDID1          0xDA
#define ILI9341_CMD_RDID2          0xDB
#define ILI9341_CMD_RDID3          0xDC
#define ILI9341_CMD_GMCTRP1        0xE0
#define ILI9341_CMD_GMCTRN1        0xE1
#define ILI9341_CMD_IFCTL           0xF6
#define ILI9341_CMD_EN3G           0xF2

/* MADCTL flags */
#define ILI9341_MADCTL_MY          0x80
#define ILI9341_MADCTL_MX          0x40
#define ILI9341_MADCTL_MV          0x20
#define ILI9341_MADCTL_ML          0x10
#define ILI9341_MADCTL_RGB          0x00
#define ILI9341_MADCTL_BGR         0x08
#define ILI9341_MADCTL_SS          0x02

/* Font constants */
#define FONT_WIDTH                 6
#define FONT_HEIGHT                8

/* Function prototypes */
void ili9341_init(void);
void ili9341_write_command(uint8_t cmd);
void ili9341_write_data(uint8_t data);
void ili9341_write_data16(uint16_t data);
void ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ili9341_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void ili9341_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void ili9341_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ili9341_draw_text(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg);
void ili9341_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg);
void ili9341_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ili9341_scroll_up(uint16_t pixels);
void ili9341_set_rotation(uint8_t rotation);
void ili9341_set_brightness(uint8_t brightness);
void ili9341_sleep(void);
void ili9341_wake(void);
void ili9341_invert(uint8_t invert);

#endif /* VORTEX_SDR_ILI9341_H */