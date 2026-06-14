/*
 * ili9341.c — ILI9341 2.8" TFT LCD driver implementation
 * SPI2 interface (PB2=SCK, PB3=MOSI), PB4=DC, PB5=CS, PB6=RST
 * 320x240 RGB565, portrait orientation
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "ili9341.h"

/* ========================================================================== */
/* Font data (5x7 ASCII 32-126, stored as 5 bytes per char)                   */
/* ========================================================================== */
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00},  /* ' ' (32) */
    {0x00, 0x00, 0x5F, 0x00, 0x00},  /* '!' */
    {0x00, 0x07, 0x00, 0x07, 0x00},  /* '"' */
    {0x14, 0x7F, 0x14, 0x7F, 0x14},  /* '#' */
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},  /* '$' */
    {0x23, 0x13, 0x08, 0x64, 0x62},  /* '%' */
    {0x36, 0x49, 0x55, 0x22, 0x50},  /* '&' */
    {0x00, 0x05, 0x03, 0x00, 0x00},  /* '\'' */
    {0x00, 0x1C, 0x22, 0x41, 0x00},  /* '(' */
    {0x00, 0x41, 0x22, 0x1C, 0x00},  /* ')' */
    {0x08, 0x2A, 0x1C, 0x2A, 0x08},  /* '*' */
    {0x08, 0x08, 0x3E, 0x08, 0x08},  /* '+' */
    {0x00, 0x50, 0x30, 0x00, 0x00},  /* ',' */
    {0x08, 0x08, 0x08, 0x08, 0x08},  /* '-' */
    {0x00, 0x60, 0x60, 0x00, 0x00},  /* '.' */
    {0x20, 0x10, 0x08, 0x04, 0x02},  /* '/' */
    {0x3E, 0x51, 0x49, 0x45, 0x3E},  /* '0' */
    {0x00, 0x42, 0x7F, 0x40, 0x00},  /* '1' */
    {0x42, 0x61, 0x51, 0x49, 0x46},  /* '2' */
    {0x21, 0x41, 0x45, 0x4B, 0x31},  /* '3' */
    {0x18, 0x14, 0x12, 0x7F, 0x10},  /* '4' */
    {0x27, 0x45, 0x45, 0x45, 0x39},  /* '5' */
    {0x3C, 0x4A, 0x49, 0x49, 0x30},  /* '6' */
    {0x01, 0x71, 0x09, 0x05, 0x03},  /* '7' */
    {0x36, 0x49, 0x49, 0x49, 0x36},  /* '8' */
    {0x06, 0x49, 0x49, 0x29, 0x1E},  /* '9' */
    {0x00, 0x36, 0x36, 0x00, 0x00},  /* ':' */
    {0x00, 0x56, 0x36, 0x00, 0x00},  /* ';' */
    {0x00, 0x08, 0x14, 0x22, 0x41},  /* '<' */
    {0x14, 0x14, 0x14, 0x14, 0x14},  /* '=' */
    {0x41, 0x22, 0x14, 0x08, 0x00},  /* '>' */
    {0x02, 0x01, 0x51, 0x09, 0x06},  /* '?' */
    {0x32, 0x49, 0x79, 0x41, 0x3E},  /* '@' */
    {0x7E, 0x11, 0x11, 0x11, 0x7E},  /* 'A' */
    {0x7F, 0x49, 0x49, 0x49, 0x36},  /* 'B' */
    {0x3E, 0x41, 0x41, 0x41, 0x22},  /* 'C' */
    {0x7F, 0x41, 0x41, 0x22, 0x1C},  /* 'D' */
    {0x7F, 0x49, 0x49, 0x49, 0x41},  /* 'E' */
    {0x7F, 0x09, 0x09, 0x01, 0x01},  /* 'F' */
    {0x3E, 0x41, 0x41, 0x51, 0x32},  /* 'G' */
    {0x7F, 0x08, 0x08, 0x08, 0x7F},  /* 'H' */
    {0x00, 0x41, 0x7F, 0x41, 0x00},  /* 'I' */
    {0x20, 0x40, 0x41, 0x3F, 0x01},  /* 'J' */
    {0x7F, 0x08, 0x14, 0x22, 0x41},  /* 'K' */
    {0x7F, 0x40, 0x40, 0x40, 0x40},  /* 'L' */
    {0x7F, 0x02, 0x04, 0x02, 0x7F},  /* 'M' */
    {0x7F, 0x04, 0x08, 0x10, 0x7F},  /* 'N' */
    {0x3E, 0x41, 0x41, 0x41, 0x3E},  /* 'O' */
    {0x7F, 0x09, 0x09, 0x09, 0x06},  /* 'P' */
    {0x3E, 0x41, 0x51, 0x21, 0x5E},  /* 'Q' */
    {0x7F, 0x09, 0x19, 0x29, 0x46},  /* 'R' */
    {0x46, 0x49, 0x49, 0x49, 0x31},  /* 'S' */
    {0x01, 0x01, 0x7F, 0x01, 0x01},  /* 'T' */
    {0x3F, 0x40, 0x40, 0x40, 0x3F},  /* 'U' */
    {0x1F, 0x20, 0x40, 0x20, 0x1F},  /* 'V' */
    {0x7F, 0x20, 0x18, 0x20, 0x7F},  /* 'W' */
    {0x63, 0x14, 0x08, 0x14, 0x63},  /* 'X' */
    {0x07, 0x08, 0x70, 0x08, 0x07},  /* 'Y' */
    {0x61, 0x51, 0x49, 0x45, 0x43},  /* 'Z' */
    {0x00, 0x7F, 0x41, 0x41, 0x00},  /* '[' */
    {0x02, 0x04, 0x08, 0x10, 0x20},  /* '\\' */
    {0x00, 0x41, 0x41, 0x7F, 0x00},  /* ']' */
    {0x04, 0x02, 0x01, 0x02, 0x04},  /* '^' */
    {0x40, 0x40, 0x40, 0x40, 0x40},  /* '_' */
    {0x00, 0x01, 0x02, 0x04, 0x00},  /* '`' */
    {0x20, 0x54, 0x54, 0x54, 0x78},  /* 'a' */
    {0x7F, 0x48, 0x44, 0x44, 0x38},  /* 'b' */
    {0x38, 0x44, 0x44, 0x44, 0x20},  /* 'c' */
    {0x38, 0x44, 0x44, 0x48, 0x7F},  /* 'd' */
    {0x38, 0x54, 0x54, 0x54, 0x18},  /* 'e' */
    {0x08, 0x7E, 0x09, 0x01, 0x02},  /* 'f' */
    {0x08, 0x14, 0x54, 0x54, 0x3C},  /* 'g' */
    {0x7F, 0x08, 0x04, 0x04, 0x78},  /* 'h' */
    {0x00, 0x44, 0x7D, 0x40, 0x00},  /* 'i' */
    {0x20, 0x40, 0x44, 0x3D, 0x00},  /* 'j' */
    {0x00, 0x7F, 0x10, 0x28, 0x44},  /* 'k' */
    {0x00, 0x41, 0x7F, 0x40, 0x00},  /* 'l' */
    {0x7C, 0x04, 0x18, 0x04, 0x78},  /* 'm' */
    {0x7C, 0x08, 0x04, 0x04, 0x78},  /* 'n' */
    {0x38, 0x44, 0x44, 0x44, 0x38},  /* 'o' */
    {0x7C, 0x14, 0x14, 0x14, 0x08},  /* 'p' */
    {0x08, 0x14, 0x14, 0x18, 0x7C},  /* 'q' */
    {0x7C, 0x08, 0x04, 0x04, 0x08},  /* 'r' */
    {0x48, 0x54, 0x54, 0x54, 0x20},  /* 's' */
    {0x04, 0x3F, 0x44, 0x40, 0x20},  /* 't' */
    {0x3C, 0x40, 0x40, 0x20, 0x7C},  /* 'u' */
    {0x1C, 0x20, 0x40, 0x20, 0x1C},  /* 'v' */
    {0x3C, 0x40, 0x30, 0x40, 0x3C},  /* 'w' */
    {0x44, 0x28, 0x10, 0x28, 0x44},  /* 'x' */
    {0x0C, 0x50, 0x50, 0x50, 0x3C},  /* 'y' */
    {0x44, 0x64, 0x54, 0x4C, 0x44},  /* 'z' */
    {0x00, 0x08, 0x36, 0x41, 0x00},  /* '{' */
    {0x00, 0x00, 0x7F, 0x00, 0x00},  /* '|' */
    {0x00, 0x41, 0x36, 0x08, 0x00},  /* '}' */
    {0x08, 0x08, 0x2A, 0x1C, 0x08},  /* '~' (126) */
};

/* ========================================================================== */
/* SPI2 send command and data                                                  */
/* ========================================================================== */

static void ili9341_cs_low(void)
{
    CLR_BIT(GPIOB->ODR, PIN_LCD_CS);
}

static void ili9341_cs_high(void)
{
    SET_BIT(GPIOB->ODR, PIN_LCD_CS);
}

static void ili9341_dc_command(void)
{
    CLR_BIT(GPIOB->ODR, PIN_LCD_DC);  /* DC low = command */
}

static void ili9341_dc_data(void)
{
    SET_BIT(GPIOB->ODR, PIN_LCD_DC);  /* DC high = data */
}

static void ili9341_spi_send(uint8_t data)
{
    while (!(SPI2->SR & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)&(SPI2->DR) = data;
    while (!(SPI2->SR & SPI_SR_RXNE))
        ;
    (void)*(volatile uint8_t *)&(SPI2->DR);
}

static uint8_t ili9341_spi_recv(void)
{
    while (!(SPI2->SR & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)&(SPI2->DR) = 0xFF;  /* Dummy byte */
    while (!(SPI2->SR & SPI_SR_RXNE))
        ;
    return *(volatile uint8_t *)&(SPI2->DR);
}

/* ========================================================================== */
/* Write command to ILI9341                                                    */
/* ========================================================================== */
void ili9341_write_command(uint8_t cmd)
{
    ili9341_cs_low();
    ili9341_dc_command();
    ili9341_spi_send(cmd);
    ili9341_cs_high();
}

/* ========================================================================== */
/* Write 8-bit data to ILI9341                                                 */
/* ========================================================================== */
void ili9341_write_data(uint8_t data)
{
    ili9341_cs_low();
    ili9341_dc_data();
    ili9341_spi_send(data);
    ili9341_cs_high();
}

/* ========================================================================== */
/* Write 16-bit data to ILI9341                                                */
/* ========================================================================== */
void ili9341_write_data16(uint16_t data)
{
    ili9341_cs_low();
    ili9341_dc_data();
    ili9341_spi_send((uint8_t)(data >> 8));
    ili9341_spi_send((uint8_t)(data & 0xFF));
    ili9341_cs_high();
}

/* ========================================================================== */
/* Set drawing window                                                          */
/* ========================================================================== */
void ili9341_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    /* Column address set */
    ili9341_write_command(ILI9341_CMD_CASET);
    ili9341_write_data16(x0);
    ili9341_write_data16(x1);

    /* Page address set */
    ili9341_write_command(ILI9341_CMD_PASET);
    ili9341_write_data16(y0);
    ili9341_write_data16(y1);

    /* Memory write */
    ili9341_write_command(ILI9341_CMD_RAMWR);
}

/* ========================================================================== */
/* Draw a single pixel                                                         */
/* ========================================================================== */
void ili9341_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT)
        return;

    ili9341_set_window(x, y, x, y);
    ili9341_write_data16(color);
}

/* ========================================================================== */
/* Draw a line (Bresenham's algorithm)                                         */
/* ========================================================================== */
void ili9341_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    int16_t dx = (int16_t)x1 - (int16_t)x0;
    int16_t dy = (int16_t)y1 - (int16_t)y0;
    int16_t sx, sy, err, e2;

    if (dx < 0) { dx = -dx; sx = -1; } else { sx = 1; }
    if (dy < 0) { dy = -dy; sy = -1; } else { sy = 1; }

    err = dx - dy;

    /* Batch pixels in same row/column for faster rendering */
    if (dy == 0) {
        /* Horizontal line — use fill_rect for speed */
        uint16_t left = (sx > 0) ? x0 : x1;
        ili9341_fill_rect(left, y0, (uint16_t)(dx + 1), 1, color);
        return;
    }

    if (dx == 0) {
        /* Vertical line — use fill_rect for speed */
        uint16_t top = (sy > 0) ? y0 : y1;
        ili9341_fill_rect(x0, top, 1, (uint16_t)(dy + 1), color);
        return;
    }

    /* General case: draw pixel by pixel */
    for (;;) {
        ili9341_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

/* ========================================================================== */
/* Draw a rectangle outline                                                    */
/* ========================================================================== */
void ili9341_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ili9341_draw_line(x, y, x + w - 1, y, color);           /* Top */
    ili9341_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color); /* Bottom */
    ili9341_draw_line(x, y, x, y + h - 1, color);            /* Left */
    ili9341_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color); /* Right */
}

/* ========================================================================== */
/* Fill a rectangle (optimized for bulk pixel write)                          */
/* ========================================================================== */
void ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT)
        return;
    if (x + w > ILI9341_WIDTH) w = ILI9341_WIDTH - x;
    if (y + h > ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

    uint32_t num_pixels = (uint32_t)w * (uint32_t)h;
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);

    ili9341_set_window(x, y, x + w - 1, y + h - 1);

    ili9341_cs_low();
    ili9341_dc_data();

    for (uint32_t i = 0; i < num_pixels; i++) {
        ili9341_spi_send(hi);
        ili9341_spi_send(lo);
    }

    ili9341_cs_high();
}

/* ========================================================================== */
/* Draw a single character (5x7 font)                                          */
/* ========================================================================== */
void ili9341_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg)
{
    if (c < 32 || c > 126) c = 32;  /* Default to space */
    uint8_t idx = (uint8_t)(c - 32);

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t line = font5x7[idx][col];
        for (uint8_t row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                ili9341_draw_pixel(x + col, y + row, fg);
            } else {
                ili9341_draw_pixel(x + col, y + row, bg);
            }
        }
    }
    /* 1-pixel gap between characters */
    for (uint8_t row = 0; row < 7; row++) {
        ili9341_draw_pixel(x + 5, y + row, bg);
    }
}

/* ========================================================================== */
/* Draw a text string                                                          */
/* ========================================================================== */
void ili9341_draw_text(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg)
{
    uint16_t cx = x;
    while (*str) {
        if (*str == '\n') {
            cx = x;
            y += FONT_HEIGHT + 1;
        } else {
            ili9341_draw_char(cx, y, *str, fg, bg);
            cx += FONT_WIDTH;
            if (cx + FONT_WIDTH > ILI9341_WIDTH) {
                cx = x;
                y += FONT_HEIGHT + 1;
            }
        }
        str++;
    }
}

/* ========================================================================== */
/* Scroll display up by N pixels (waterfall use)                              */
/* ========================================================================== */
void ili9341_scroll_up(uint16_t pixels)
{
    /* Use hardware vertical scroll */
    ili9341_write_command(ILI9341_CMD_VSCSAD);
    ili9341_write_data16(pixels);
}

/* ========================================================================== */
/* Set display rotation                                                        */
/* ========================================================================== */
void ili9341_set_rotation(uint8_t rotation)
{
    uint8_t madctl;
    switch (rotation) {
    case 0:   madctl = ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR; break;
    case 1:   madctl = ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR; break;
    case 2:   madctl = ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR; break;
    case 3:   madctl = ILI9341_MADCTL_MY | ILI9341_MADCTL_MX | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR; break;
    default:  madctl = ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR; break;
    }
    ili9341_write_command(ILI9341_CMD_MADCTL);
    ili9341_write_data(madctl);
}

/* ========================================================================== */
/* Enable/disable display inversion                                           */
/* ========================================================================== */
void ili9341_invert(uint8_t invert)
{
    if (invert) {
        ili9341_write_command(ILI9341_CMD_INVON);
    } else {
        ili9341_write_command(ILI9341_CMD_INVOFF);
    }
}

/* ========================================================================== */
/* Sleep mode (low power)                                                     */
/* ========================================================================== */
void ili9341_sleep(void)
{
    ili9341_write_command(ILI9341_CMD_SLPIN);
}

/* ========================================================================== */
/* Wake from sleep                                                             */
/* ========================================================================== */
void ili9341_wake(void)
{
    ili9341_write_command(ILI9341_CMD_SLPOUT);
    /* Wait for display to wake (120ms required) */
    for (volatile uint32_t i = 0; i < 1200000; i++)
        ;
}

/* ========================================================================== */
/* Set brightness (via PWM on backlight pin, not available on all boards)      */
/* ========================================================================== */
void ili9341_set_brightness(uint8_t brightness)
{
    (void)brightness;
    /* Brightness control via TIM1 PWM would go here.
     * For now, display is always at full brightness.
     */
}

/* ========================================================================== */
/* Initialize ILI9341 display                                                  */
/* ========================================================================== */
void ili9341_init(void)
{
    /* Hardware reset pulse */
    CLR_BIT(GPIOB->ODR, PIN_LCD_RST);
    for (volatile uint32_t i = 0; i < 100000; i++)
        ;
    SET_BIT(GPIOB->ODR, PIN_LCD_RST);
    for (volatile uint32_t i = 0; i < 100000; i++)
        ;

    /* Software reset */
    ili9341_write_command(ILI9341_CMD_SWRESET);
    for (volatile uint32_t i = 0; i < 500000; i++)
        ;

    /* Out of sleep mode */
    ili9341_write_command(ILI9341_CMD_SLPOUT);
    for (volatile uint32_t i = 0; i < 500000; i++)
        ;

    /* Normal display mode */
    ili9341_write_command(ILI9341_CMD_NORON);
    for (volatile uint32_t i = 0; i < 50000; i++)
        ;

    /* Pixel format: 16-bit RGB565 */
    ili9341_write_command(ILI9341_CMD_PIXSET);
    ili9341_write_data(0x55);  /* 16-bit/pixel */

    /* Memory access control: portrait, BGR */
    ili9341_write_command(ILI9341_CMD_MADCTL);
    ili9341_write_data(ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);

    /* Column address set: 0-319 */
    ili9341_write_command(ILI9341_CMD_CASET);
    ili9341_write_data16(0);
    ili9341_write_data16(319);

    /* Page address set: 0-239 */
    ili9341_write_command(ILI9341_CMD_PASET);
    ili9341_write_data16(0);
    ili9341_write_data16(239);

    /* Frame rate control: 60 Hz */
    ili9341_write_command(ILI9341_CMD_FRMCTR1);
    ili9341_write_data(0x00);  /* Division ratio = fosc */
    ili9341_write_data(0x06);  /* Frame rate = 60 Hz */

    /* Display function control */
    ili9341_write_command(ILI9341_CMD_DFUNCTR);
    ili9341_write_data(0x08);  /* PTG: interval scan */
    ili9341_write_data(0x82);  /* Source output: scan direction */
    ili9341_write_data(0x27);  /* 320 lines */

    /* Power control 1 */
    ili9341_write_command(ILI9341_CMD_PWCTR1);
    ili9341_write_data(0x17);  /* GVDD = 4.6V */
    ili9341_write_data(0x15);  /* AVDD = 4.4V */

    /* Power control 2 */
    ili9341_write_command(ILI9341_CMD_PWCTR2);
    ili9341_write_data(0x41);  /* VGH25 = 2.4V, VGL = -10V */

    /* VCOM control 1 */
    ili9341_write_command(ILI9341_CMD_VMCTR1);
    ili9341_write_data(0x00);  /* VCOMH = 0.0V */
    ili9341_write_data(0x30);  /* VCOML = -1.8V */

    /* Positive gamma correction */
    ili9341_write_command(ILI9341_CMD_GMCTRP1);
    ili9341_write_data(0x3F);  /* Gamma curve 1 */
    ili9341_write_data(0x25);
    ili9341_write_data(0x1C);
    ili9341_write_data(0x1E);
    ili9341_write_data(0x20);
    ili9341_write_data(0x12);
    ili9341_write_data(0x2A);
    ili9341_write_data(0x90);
    ili9341_write_data(0x24);
    ili9341_write_data(0x06);
    ili9341_write_data(0x06);
    ili9341_write_data(0x1C);
    ili9341_write_data(0x1A);
    ili9341_write_data(0x00);
    ili9341_write_data(0x00);

    /* Negative gamma correction */
    ili9341_write_command(ILI9341_CMD_GMCTRN1);
    ili9341_write_data(0x20);
    ili9341_write_data(0x20);
    ili9341_write_data(0x20);
    ili9341_write_data(0x20);
    ili9341_write_data(0x05);
    ili9341_write_data(0x00);
    ili9341_write_data(0x15);
    ili9341_write_data(0xA7);
    ili9341_write_data(0x3D);
    ili9341_write_data(0x18);
    ili9341_write_data(0x25);
    ili9341_write_data(0x2A);
    ili9341_write_data(0x2B);
    ili9341_write_data(0x2B);
    ili9341_write_data(0x3A);

    /* Display ON */
    ili9341_write_command(ILI9341_CMD_DISPON);
    for (volatile uint32_t i = 0; i < 100000; i++)
        ;
}