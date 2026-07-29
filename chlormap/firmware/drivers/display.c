/*
 * display.c — SSD1306 OLED display driver (SPI, 128x64)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The SSD1306 is a monochrome OLED controller with 128×64 pixels
 * organized as 8 pages of 128 columns. We communicate over SPI2.
 *
 * This driver maintains a local framebuffer (1 KB) and provides
 * text and basic graphics primitives.
 */

#include "display.h"
#include "board.h"
#include "registers.h"
#include <string.h>

static uint8_t g_fb[SSD1306_WIDTH * SSD1306_HEIGHT / 8]; /* 1024 bytes */

/* ---- SPI2 low-level ---- */
static void spi2_select(void)
{
    /* GPIOB->BSRR = (1 << 12) << 16; — CS low */
}

static void spi2_deselect(void)
{
    /* GPIOB->BSRR = (1 << 12);       — CS high */
}

static void spi2_tx_byte(uint8_t b)
{
    /* while(!(SPI2->SR & SPI_SR_TXE));
     * *(volatile uint8_t*)&SPI2->DR = b;
     * while(!(SPI2->SR & SPI_SR_TXE));
     */
    (void)b;
}

static void display_dc_command(void)
{
    /* GPIOB->BSRR = (1 << 14) << 16; — DC low (command) */
}

static void display_dc_data(void)
{
    /* GPIOB->BSRR = (1 << 14);       — DC high (data) */
}

static void display_reset(bool low)
{
    /* GPIOB->BSRR = low ? ((1 << 2) << 16) : (1 << 2); — RST */
    (void)low;
}

static void display_send_command(uint8_t cmd)
{
    display_dc_command();
    spi2_select();
    spi2_tx_byte(cmd);
    spi2_deselect();
}

static void display_send_data(const uint8_t *data, uint16_t len)
{
    display_dc_data();
    spi2_select();
    for (uint16_t i = 0; i < len; i++) spi2_tx_byte(data[i]);
    spi2_deselect();
}

/* ---- Font (5×7, ASCII 32–127) ---- */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $ */
    {0x23,0x13,0x08,0x64,0x72}, /* % */
    {0x36,0x49,0x55,0x22,0x50}, /* & */
    {0x00,0x05,0x03,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* ) */
    {0x14,0x08,0x3E,0x08,0x14}, /* * */
    {0x08,0x08,0x3E,0x08,0x08}, /* + */
    {0x00,0x50,0x30,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08}, /* - */
    {0x00,0x60,0x60,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00}, /* ; */
    {0x00,0x08,0x14,0x22,0x41}, /* < */
    {0x14,0x14,0x14,0x14,0x14}, /* = */
    {0x41,0x22,0x14,0x08,0x00}, /* > */
    {0x02,0x01,0x51,0x09,0x06}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x01,0x01}, /* F */
    {0x3E,0x41,0x41,0x51,0x32}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x7F,0x20,0x10,0x20,0x7F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x03,0x04,0x78,0x04,0x03}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x7F,0x41,0x41,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* backslash */
    {0x00,0x41,0x41,0x7F,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* _ */
    {0x00,0x01,0x02,0x04,0x00}, /* ` */
    {0x20,0x54,0x54,0x54,0x78}, /* a */
    {0x7F,0x48,0x44,0x44,0x38}, /* b */
    {0x38,0x44,0x44,0x44,0x20}, /* c */
    {0x38,0x44,0x44,0x48,0x7F}, /* d */
    {0x38,0x54,0x54,0x54,0x18}, /* e */
    {0x08,0x7E,0x09,0x01,0x02}, /* f */
    {0x08,0x14,0x54,0x54,0x3C}, /* g */
    {0x7F,0x08,0x04,0x04,0x78}, /* h */
    {0x00,0x44,0x7D,0x40,0x00}, /* i */
    {0x20,0x40,0x44,0x3D,0x00}, /* j */
    {0x7F,0x10,0x28,0x44,0x00}, /* k */
    {0x00,0x41,0x7F,0x40,0x00}, /* l */
    {0x7C,0x04,0x18,0x04,0x78}, /* m */
    {0x7C,0x08,0x04,0x04,0x78}, /* n */
    {0x38,0x44,0x44,0x44,0x38}, /* o */
    {0x7C,0x14,0x14,0x14,0x08}, /* p */
    {0x08,0x14,0x14,0x18,0x7C}, /* q */
    {0x7C,0x08,0x04,0x04,0x08}, /* r */
    {0x48,0x54,0x54,0x54,0x20}, /* s */
    {0x04,0x3F,0x44,0x40,0x20}, /* t */
    {0x3C,0x40,0x40,0x20,0x7C}, /* u */
    {0x1C,0x20,0x40,0x20,0x1C}, /* v */
    {0x3C,0x40,0x30,0x40,0x3C}, /* w */
    {0x44,0x28,0x10,0x28,0x44}, /* x */
    {0x0C,0x50,0x50,0x50,0x3C}, /* y */
    {0x44,0x64,0x54,0x4C,0x44}, /* z */
};

static int char_to_index(char c)
{
    if (c >= 'A' && c <= 'Z') return 33 + (c - 'A');
    if (c >= 'a' && c <= 'z') return 65 + (c - 'a');
    if (c >= '0' && c <= '9') return 16 + (c - '0');
    if (c == ' ') return 0;
    if (c == '.') return 14;
    if (c == ':') return 26;
    if (c == '-') return 13;
    if (c == '/') return 15;
    if (c == '%') return 5;
    if (c == ',') return 12;
    if (c == '!') return 1;
    if (c == '=') return 29;
    if (c == '+') return 11;
    if (c == '?') return 31;
    if (c == '_') return 39;
    if (c == '<') return 28;
    if (c == '>') return 30;
    return 0; /* default: space */
}

/* ---- Pixel operations ---- */
void display_set_pixel(uint8_t x, uint8_t y, bool on)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    uint16_t idx = x + (y / 8) * SSD1306_WIDTH;
    if (on) g_fb[idx] |= (1 << (y & 7));
    else    g_fb[idx] &= ~(1 << (y & 7));
}

void display_clear(void)
{
    memset(g_fb, 0, sizeof(g_fb));
}

void display_draw_string(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    uint8_t cx = x;
    uint8_t cy = y;

    while (*str) {
        int fi = char_to_index(*str);
        const uint8_t *glyph = font5x7[fi];

        for (int col = 0; col < 5; col++) {
            uint8_t line = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (line & (1 << row)) {
                    for (int sy = 0; sy < size; sy++) {
                        for (int sx = 0; sx < size; sx++) {
                            display_set_pixel(
                                cx + col * size + sx,
                                cy + row * size + sy,
                                true);
                        }
                    }
                }
            }
        }
        cx += 6 * size;
        if (cx >= SSD1306_WIDTH - 6) {
            cx = x;
            cy += 8 * size;
        }
        str++;
    }
}

void display_draw_hline(uint8_t x, uint8_t y, uint8_t w)
{
    for (uint8_t i = 0; i < w; i++) display_set_pixel(x + i, y, true);
}

void display_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t i = 0; i < w; i++) {
        display_set_pixel(x + i, y, true);
        display_set_pixel(x + i, y + h - 1, true);
    }
    for (uint8_t i = 0; i < h; i++) {
        display_set_pixel(x, y + i, true);
        display_set_pixel(x + w - 1, y + i, true);
    }
}

void display_refresh(void)
{
    for (uint8_t page = 0; page < SSD1306_PAGES; page++) {
        display_send_command(SSD1306_SET_PAGE | page);
        display_send_command(SSD1306_SET_LOW_COL | 0);
        display_send_command(SSD1306_SET_HIGH_COL | 0);
        display_send_data(&g_fb[page * SSD1306_WIDTH], SSD1306_WIDTH);
    }
}

void display_show_message(const char *msg)
{
    display_clear();
    /* Center the message roughly */
    uint8_t y = 24;
    display_draw_string(0, y, msg, 1);
    display_refresh();
}

void display_draw_spectrum(const int16_t *bands_x1000, uint8_t count)
{
    display_clear();
    display_draw_string(0, 0, "Spectrum", 1);
    display_draw_hline(0, 10, SSD1306_WIDTH);

    /* Draw 16 bars (8 px wide each = 128 px total) */
    uint8_t bar_w = SSD1306_WIDTH / count;
    for (uint8_t i = 0; i < count && i < 16; i++) {
        /* Reflectance 0–1.0 → bar height 0–50 px */
        int16_t val = bands_x1000[i];
        if (val < 0) val = 0;
        if (val > 1000) val = 1000;
        uint8_t h = (uint8_t)((val * 50) / 1000);
        uint8_t x = i * bar_w + 1;
        uint8_t y = 63 - h;
        for (uint8_t bx = 0; bx < bar_w - 1; bx++) {
            for (uint8_t by = y; by < 63; by++) {
                display_set_pixel(x + bx, by, true);
            }
        }
    }
    display_refresh();
}

bool display_init(void)
{
    /* Hardware reset */
    display_reset(true);
    /* delay 10 ms */
    display_reset(false);
    /* delay 10 ms */

    /* Init sequence for 128x64 SSD1306 */
    display_send_command(SSD1306_DISPLAY_OFF);
    display_send_command(SSD1306_SET_DISPCLK_DIV | 0x80);
    display_send_command(SSD1306_SET_MULTIPLEX | 0x3F);
    display_send_command(SSD1306_SET_DISP_OFFSET | 0x00);
    display_send_command(SSD1306_SET_START_LINE | 0x00);
    display_send_command(SSD1306_CHARGE_PUMP | 0x14);
    display_send_command(SSD1306_SEG_REMAP_127);
    display_send_command(SSD1306_COM_SCAN_REMAPPED);
    display_send_command(SSD1306_SET_COMPINS | 0x12);
    display_send_command(SSD1306_SET_CONTRAST | 0xCF);
    display_send_command(SSD1306_SET_PRECHARGE | 0xF1);
    display_send_command(SSD1306_SET_VCOM_DETECT | 0x40);
    display_send_command(SSD1306_ENTIRE_ON);
    display_send_command(SSD1306_NORMAL_DISPLAY);
    display_send_command(SSD1306_DISPLAY_ON);

    display_clear();
    display_refresh();
    return true;
}

void display_power_off(void)
{
    display_send_command(SSD1306_DISPLAY_OFF);
}

void display_power_on(void)
{
    display_send_command(SSD1306_DISPLAY_ON);
}