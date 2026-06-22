/*
 * display.c — SSD1306 OLED display driver
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Drives a 128×64 SSD1306 OLED over I2C1 with a small framebuffer and
 * minimal 5×7/8×16 font for displaying live readings on the device.
 */

#include "display.h"
#include "i2c_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>
#include <stdio.h>

static uint8_t fb[OLED_PAGES][OLED_WIDTH];
static bool dirty[OLED_PAGES];

/* ------------------------------------------------------------------ */
/*  Font: 5×7 ASCII (32..127).                                          */
/* ------------------------------------------------------------------ */

static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $ */
    {0x23,0x13,0x08,0x64,0x62}, /* % */
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
    {0x7F,0x11,0x11,0x11,0x0E}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x7F,0x41,0x41,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* \ */
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
    {0x00,0x7F,0x10,0x28,0x44}, /* k */
    {0x00,0x41,0x7F,0x40,0x00}, /* l */
    {0x7C,0x04,0x18,0x04,0x78}, /* m */
    {0x7C,0x08,0x04,0x04,0x78}, /* n */
    {0x38,0x44,0x44,0x44,0x38}, /* o */
    {0x7C,0x14,0x14,0x14,0x08}, /* p */
    {0x08,0x14,0x14,0x18,0x7C}, /* q */
    {0x7C,0x08,0x04,0x04,0x08}, /* r */
    {0x48,0x54,0x54,0x24,0x00}, /* s */
    {0x04,0x3F,0x44,0x40,0x20}, /* t */
    {0x3C,0x40,0x40,0x20,0x7C}, /* u */
    {0x1C,0x20,0x40,0x20,0x1C}, /* v */
    {0x3C,0x40,0x30,0x40,0x3C}, /* w */
    {0x44,0x28,0x10,0x28,0x44}, /* x */
    {0x0C,0x50,0x50,0x50,0x3C}, /* y */
    {0x44,0x64,0x54,0x4C,0x44}, /* z */
    {0x00,0x08,0x36,0x41,0x00}, /* { */
    {0x00,0x00,0x7F,0x00,0x00}, /* | */
    {0x00,0x41,0x36,0x08,0x00}, /* } */
    {0x02,0x01,0x02,0x04,0x02}, /* ~ */
};

#define FONT_OFFSET  32
#define FONT_CHARS   (sizeof(font5x7)/5)

/* ------------------------------------------------------------------ */

static void ssd1306_cmd(uint8_t cmd)
{
    i2c_write_reg(I2C1, SSD1306_I2C_ADDR_8BIT, 0x00, cmd);
}

static void ssd1306_data(uint8_t *data, uint16_t len)
{
    i2c_write_burst(I2C1, SSD1306_I2C_ADDR_8BIT, 0x40, data, (uint8_t)(len > 255 ? 255 : len));
}

void display_init(void)
{
    /* Init GPIO for OLED reset and I2C1. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

    GPIOB->MODER &= ~(3U << (OLED_RST_PIN*2U));
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << (OLED_RST_PIN*2U));

    /* TIM17_CH1 for backlight PWM (PB5, AF10). */
    RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
    GPIOB->MODER &= ~(3U << (5U*2U));
    GPIOB->MODER |= (GPIO_MODE_AF << (5U*2U));
    GPIOB->AFRH &= ~(0xFU << ((5U-8U)*4U));
    GPIOB->AFRH |= (10U << ((5U-8U)*4U));
    TIM17->PSC = (SYSTEM_CLK_HZ / 1000000U) - 1;  /* 1 MHz */
    TIM17->ARR = 100;
    TIM17->CCR1 = 80;
    TIM17->BDTR |= (1U << 15);
    TIM17->CCMR1 = (6U << 4);  /* PWM mode 1 */
    TIM17->CCER = 1;  /* enable CC1 output */
    TIM17->CR1 = TIM_CR1_CEN;

    i2c1_init();

    /* Hardware reset. */
    OLED_RST_LOW();
    /* delay 10 ms */
    for (volatile int i = 0; i < 800000; i++);
    OLED_RST_HIGH();
    for (volatile int i = 0; i < 800000; i++);

    /* SSD1306 initialization sequence. */
    ssd1306_cmd(0xAE);  /* display off */
    ssd1306_cmd(0xD5); ssd1306_cmd(0x80);  /* clock divide */
    ssd1306_cmd(0xA8); ssd1306_cmd(0x3F);  /* multiplex 1/64 */
    ssd1306_cmd(0xD3); ssd1306_cmd(0x00);  /* display offset 0 */
    ssd1306_cmd(0x40);  /* start line 0 */
    ssd1306_cmd(0x8D); ssd1306_cmd(0x14);  /* charge pump on */
    ssd1306_cmd(0x20); ssd1306_cmd(0x00);  /* horizontal addressing */
    ssd1306_cmd(0xA1);  /* segment remap */
    ssd1306_cmd(0xC8);  /* COM scan dir remap */
    ssd1306_cmd(0xDA); ssd1306_cmd(0x12);  /* com pins */
    ssd1306_cmd(0xD9); ssd1306_cmd(0xF1);  /* pre-charge */
    ssd1306_cmd(0xDB); ssd1306_cmd(0x40);  /* vcom deselect */
    ssd1306_cmd(0xA4);  /* display resume RAM */
    ssd1306_cmd(0xA6);  /* normal display */
    ssd1306_cmd(0xAF);  /* display on */

    display_clear();
    display_flush();
}

void display_clear(void)
{
    memset(fb, 0, sizeof(fb));
    for (int p = 0; p < OLED_PAGES; p++) dirty[p] = true;
}

void display_flush(void)
{
    for (int p = 0; p < OLED_PAGES; p++) {
        if (!dirty[p]) continue;
        ssd1306_cmd(0xB0 + p);       /* set page address */
        ssd1306_cmd(0x00);           /* col low */
        ssd1306_cmd(0x10);           /* col high */
        ssd1306_data(fb[p], OLED_WIDTH);
        dirty[p] = false;
    }
}

void display_set_contrast(uint8_t val)
{
    ssd1306_cmd(0x81);
    ssd1306_cmd(val);
}

void display_set_backlight(uint8_t pct)
{
    if (pct > 100) pct = 100;
    TIM17->CCR1 = pct;
}

void display_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    int p = y / 8;
    int b = y % 8;
    if (on) fb[p][x] |= (1 << b);
    else    fb[p][x] &= ~(1 << b);
    dirty[p] = true;
}

void display_hline(int x, int y, int w)
{
    for (int i = 0; i < w; i++) display_pixel(x + i, y, true);
}

void display_vline(int x, int y, int h)
{
    for (int i = 0; i < h; i++) display_pixel(x, y + i, true);
}

void display_rect(int x, int y, int w, int h, bool fill)
{
    if (fill) {
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++)
                display_pixel(x + i, y + j, true);
    } else {
        display_hline(x, y, w);
        display_hline(x, y + h - 1, w);
        display_vline(x, y, h);
        display_vline(x + w - 1, y, h);
    }
}

static void draw_char(int x, int y, char c, bool invert)
{
    int idx = c - FONT_OFFSET;
    if (idx < 0 || idx >= (int)FONT_CHARS) idx = 0;
    for (int i = 0; i < 5; i++) {
        uint8_t col = font5x7[idx][i];
        if (invert) col = ~col;
        for (int j = 0; j < 7; j++) {
            display_pixel(x + i, y + j, (col >> j) & 1);
        }
    }
}

void display_text(int x, int y, const char *str, int size)
{
    display_text_fmt(x, y, str, size, false);
}

void display_text_fmt(int x, int y, const char *str, int size, bool invert)
{
    int cx = x;
    int cy = y;
    int spacing = (size == 1) ? 6 : 12;
    while (*str) {
        if (size == 1) {
            draw_char(cx, cy, *str, invert);
        } else {
            /* 2x scaling. */
            for (int i = 0; i < 5; i++) {
                int idx = *str - FONT_OFFSET;
                if (idx < 0 || idx >= (int)FONT_CHARS) idx = 0;
                uint8_t col = font5x7[idx][i];
                if (invert) col = ~col;
                for (int j = 0; j < 7; j++) {
                    bool on = (col >> j) & 1;
                    if (on) {
                        for (int dx = 0; dx < 2; dx++)
                            for (int dy = 0; dy < 2; dy++)
                                display_pixel(cx + i*2 + dx, cy + j*2 + dy, true);
                    } else if (invert) {
                        for (int dx = 0; dx < 2; dx++)
                            for (int dy = 0; dy < 2; dy++)
                                display_pixel(cx + i*2 + dx, cy + j*2 + dy, true);
                    }
                }
            }
        }
        cx += spacing;
        str++;
    }
}

void display_big_num(int x, int y, float val, int digits)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%.*f", digits, val);
    display_text(x, y, buf, 2);
}

void display_bar(int x, int y, int w, int h, float pct)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    display_rect(x, y, w, h, false);
    int fill_w = (int)((w - 2) * pct / 100.0f);
    if (fill_w > 0) display_rect(x + 1, y + 1, fill_w, h - 2, true);
}

void display_spectrogram(int x, int y, const float *vals, int n, float max_val)
{
    int bar_w = OLED_WIDTH / n;
    if (bar_w < 2) bar_w = 2;
    for (int i = 0; i < n; i++) {
        float v = vals[i];
        if (v < 0) v = 0;
        if (v > max_val) v = max_val;
        int h = (int)(v / max_val * 40.0f);
        display_vline(x + i * bar_w, y + 40 - h, h);
        display_vline(x + i * bar_w + 1, y + 40 - h, h);
    }
}