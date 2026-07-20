/*
 * display.c — SSD1327 OLED Display Driver
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Drives the 2.4-inch SSD1327 OLED display (128×64, 4-bit grayscale)
 * via SPI2.  Provides a small framebuffer and basic drawing primitives
 * (text, rectangles, progress bars, false-color conductivity maps).
 */

#include "display.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* Framebuffer: 128×64 pixels, 4-bit grayscale = 2 pixels per byte = 4096 bytes */
static uint8_t s_fb[OLED_FB_SIZE];

/* 5×7 font (basic ASCII subset) */
static const uint8_t s_font5x7[][5] = {
    /* Space */ {0x00,0x00,0x00,0x00,0x00},
    /* ! */     {0x00,0x00,0x5F,0x00,0x00},
    /* " */     {0x00,0x07,0x00,0x07,0x00},
    /* # */     {0x14,0x7F,0x14,0x7F,0x14},
    /* $ */     {0x24,0x2A,0x7F,0x2A,0x12},
    /* % */     {0x23,0x13,0x08,0x64,0x62},
    /* & */     {0x36,0x49,0x55,0x22,0x50},
    /* ' */     {0x00,0x05,0x03,0x00,0x00},
    /* ( */     {0x00,0x1C,0x22,0x41,0x00},
    /* ) */     {0x00,0x41,0x22,0x1C,0x00},
    /* * */     {0x08,0x2A,0x1C,0x2A,0x08},
    /* + */     {0x08,0x08,0x3E,0x08,0x08},
    /* , */     {0x00,0x50,0x30,0x00,0x00},
    /* - */     {0x08,0x08,0x08,0x08,0x08},
    /* . */     {0x00,0x60,0x60,0x00,0x00},
    /* / */     {0x20,0x10,0x08,0x04,0x02},
    /* 0-9 */
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
    /* : */     {0x00,0x36,0x36,0x00,0x00},
    /* ; */     {0x00,0x56,0x36,0x00,0x00},
    /* < */     {0x00,0x08,0x14,0x22,0x41},
    /* = */     {0x14,0x14,0x14,0x14,0x14},
    /* > */     {0x41,0x22,0x14,0x08,0x00},
    /* A-Z (subset) */
    {0x7F,0x09,0x09,0x09,0x7F}, {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x01}, {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x41,0x51,0x32}, {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x04,0x02,0x7F}, {0x7F,0x02,0x0C,0x02,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
    {0x26,0x49,0x49,0x49,0x32}, {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
    {0x03,0x04,0x78,0x04,0x03}, {0x61,0x51,0x49,0x45,0x43},
    /* a-z (subset for lowercase) */
    {0x20,0x54,0x54,0x54,0x78}, {0x7C,0x54,0x54,0x54,0x28},
    {0x3C,0x40,0x40,0x40,0x3C}, {0x78,0x14,0x14,0x14,0x68},
    {0x3C,0x40,0x38,0x04,0x78}, {0x44,0x44,0x7C,0x44,0x44},
    {0x00,0x08,0x7E,0x09,0x02}, {0x18,0xA4,0xA4,0xA4,0x78},
    {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38},
    {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x14,0x7C},
    {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x24},
    {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x30,0x40,0x3C},
    {0x44,0x44,0x44,0x3C,0x04}, {0x08,0x54,0x54,0x54,0x7C},
    {0x7C,0x10,0x10,0x10,0x7C}, {0x3C,0x20,0x00,0x20,0x3C},
    {0x3C,0x10,0x20,0x10,0x3C}, {0x44,0x28,0x10,0x28,0x44},
    {0x4C,0x50,0x50,0x50,0x3C}, {0x44,0x64,0x54,0x4C,0x44},
};

/* ---------------------------------------------------------------------
 * SPI2 setup for SSD1327
 * --------------------------------------------------------------------- */

static void display_spi_init(void)
{
    RCC->AHB4ENR |= BIT(1) | BIT(2);  /* GPIOB, GPIOC */
    RCC->APB1LENR |= BIT(14);  /* SPI2 */

    /* PB10 = SPI2_SCK (AF5), PC2 = MISO (AF5), PC3 = MOSI (AF5) */
    int pb_pins[] = {10};
    for (int i = 0; i < 1; i++) {
        int p = pb_pins[i];
        GPIOB->MODER &= ~(3U << (p*2));
        GPIOB->MODER |= (GPIO_MODE_AF << (p*2));
        GPIOB->OSPEEDR |= (GPIO_SPEED_VHIGH << (p*2));
        GPIOB->AFRH &= ~(0xFU << ((p-8)*4));
        GPIOB->AFRH |= (OLED_SPI_AF << ((p-8)*4));
    }
    int pc_pins[] = {2, 3};
    for (int i = 0; i < 2; i++) {
        int p = pc_pins[i];
        GPIOC->MODER &= ~(3U << (p*2));
        GPIOC->MODER |= (GPIO_MODE_AF << (p*2));
        GPIOC->OSPEEDR |= (GPIO_SPEED_VHIGH << (p*2));
        GPIOC->AFRL &= ~(0xFU << (p*4));
        GPIOC->AFRL |= (OLED_SPI_AF << (p*4));
    }

    /* PB11 = CS, PB12 = D/C, PB13 = RESET (all output) */
    int ctrl_pins[] = {11, 12, 13};
    for (int i = 0; i < 3; i++) {
        int p = ctrl_pins[i];
        GPIOB->MODER &= ~(3U << (p*2));
        GPIOB->MODER |= (GPIO_MODE_OUT << (p*2));
        GPIOB->OTYPER &= ~(1U << p);
        GPIOB->OSPEEDR |= (GPIO_SPEED_HIGH << (p*2));
    }
    PIN_SET(OLED_CS_PORT, OLED_CS_PIN);     /* CS high (inactive) */
    PIN_SET(OLED_DC_PORT, OLED_DC_PIN);     /* D/C high (data) */
    PIN_SET(OLED_RESET_PORT, OLED_RESET_PIN); /* RESET high (inactive) */

    /* Configure SPI2: master, 8-bit, CPOL=1 CPHA=1, baud /32 */
    OLED_SPI->CR1 = 0;
    OLED_SPI->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSM | SPI_CFG2_SSOE
                   | SPI_CFG2_CPOL | SPI_CFG2_CPHA;
    OLED_SPI->CFG1 = (31U << SPI_CFG1_BAUD_SHIFT)  /* /32 -> 3.75 MHz */
                   | (7U << 0)                       /* 8-bit */
                   | BIT(0) | BIT(1);
    OLED_SPI->CR1 = SPI_CR1_SPE;
}

/* ---------------------------------------------------------------------
 * Low-level SPI send
 * --------------------------------------------------------------------- */

static void spi_send(uint8_t b)
{
    while (!(OLED_SPI->SR & SPI_SR_TXP)) { }
    *(volatile uint8_t *)&OLED_SPI->TXDR = b;
    while (!(OLED_SPI->SR & SPI_SR_EOT)) { }
    /* drain RX */
    if (OLED_SPI->SR & SPI_SR_RXP)
        (void)*(volatile uint8_t *)&OLED_SPI->RXDR;
}

static void display_send_cmd(uint8_t cmd)
{
    PIN_CLR(OLED_CS_PORT, OLED_CS_PIN);
    PIN_CLR(OLED_DC_PORT, OLED_DC_PIN);  /* D/C = 0 for command */
    spi_send(cmd);
    PIN_SET(OLED_DC_PORT, OLED_DC_PIN);
    PIN_SET(OLED_CS_PORT, OLED_CS_PIN);
}

static void display_send_data(uint8_t data)
{
    PIN_CLR(OLED_CS_PORT, OLED_CS_PIN);
    PIN_SET(OLED_DC_PORT, OLED_DC_PIN);  /* D/C = 1 for data */
    spi_send(data);
    PIN_SET(OLED_CS_PORT, OLED_CS_PIN);
}

/* ---------------------------------------------------------------------
 * SSD1327 initialization sequence
 * --------------------------------------------------------------------- */

void display_init(void)
{
    display_spi_init();

    /* Hardware reset */
    PIN_CLR(OLED_RESET_PORT, OLED_RESET_PIN);
    for (volatile int i = 0; i < 10000; i++) { }
    PIN_SET(OLED_RESET_PORT, OLED_RESET_PIN);
    for (volatile int i = 0; i < 100000; i++) { }

    /* SSD1327 command sequence */
    display_send_cmd(0xAE);  /* Display off */
    display_send_cmd(0xA0);  /* Set re-map */
    display_send_data(0x51); /* 64-color, vertical scroll, COM remap */
    display_send_cmd(0xA1);  /* Set display start line */
    display_send_data(0x00);
    display_send_cmd(0xA2);  /* Set display offset */
    display_send_data(0x00);
    display_send_cmd(0xA4);  /* Normal display (resume to RAM) */
    display_send_cmd(0xA8);  /* Set multiplex ratio */
    display_send_data(0x3F); /* 1/64 mux */
    display_send_cmd(0xAD);  /* Set master config */
    display_send_data(0x02);
    display_send_cmd(0xB0);  /* Set power save mode off */
    display_send_cmd(0xB1);  /* Phase 1/2 period */
    display_send_data(0x55);
    display_send_cmd(0xB3);  /* Clock divide */
    display_send_data(0x01);
    display_send_cmd(0xB9);  /* Linear LUT */
    display_send_cmd(0xBB);  /* Pre-charge voltage */
    display_send_data(0x1F);
    display_send_cmd(0xBE);  /* VCOMH */
    display_send_data(0x1C);
    display_send_cmd(0x81);  /* Contrast current */
    display_send_data(0x7F);
    display_send_cmd(0x15);  /* Column address */
    display_send_data(0x00);
    display_send_data(0x3F); /* 64 columns (0-63) */
    display_send_cmd(0x75);  /* Row address */
    display_send_data(0x00);
    display_send_data(0x3F);
    display_send_cmd(0xAF);  /* Display on */

    display_clear();
    display_flush();
}

/* ---------------------------------------------------------------------
 * Framebuffer operations
 * --------------------------------------------------------------------- */

void display_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

void display_set_pixel(int x, int y, uint8_t gray)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    gray &= 0x0F;
    int byte_idx = (y * OLED_WIDTH + x) / 2;
    if (x & 1) {
        s_fb[byte_idx] = (s_fb[byte_idx] & 0xF0) | gray;
    } else {
        s_fb[byte_idx] = (s_fb[byte_idx] & 0x0F) | (gray << 4);
    }
}

void display_flush(void)
{
    /* Set column and row address window */
    display_send_cmd(0x15);
    display_send_data(0x00);
    display_send_data(0x3F);
    display_send_cmd(0x75);
    display_send_data(0x00);
    display_send_data(0x3F);

    /* Write framebuffer data */
    PIN_CLR(OLED_CS_PORT, OLED_CS_PIN);
    PIN_SET(OLED_DC_PORT, OLED_DC_PIN);
    for (int i = 0; i < OLED_FB_SIZE; i++) {
        spi_send(s_fb[i]);
    }
    PIN_SET(OLED_CS_PORT, OLED_CS_PIN);
}

/* ---------------------------------------------------------------------
 * Text rendering (5×7 font)
 * --------------------------------------------------------------------- */

static int char_index(char c)
{
    if (c == ' ') return 0;
    if (c >= '!' && c <= '/') return c - '!' + 1;
    if (c >= '0' && c <= '9') return c - '0' + 16;
    if (c >= ':' && c <= '>') return c - ':' + 26;
    if (c == '?') return 31;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 32;
    if (c >= 'a' && c <= 'z') return c - 'a' + 58;
    return 0;  /* default: space */
}

void display_draw_text(int x, int y, const char *str, uint8_t gray)
{
    if (!str) return;
    int cx = x;
    for (int i = 0; str[i] && cx < OLED_WIDTH - 6; i++) {
        int idx = char_index(str[i]);
        const uint8_t *glyph = s_font5x7[idx];
        for (int col = 0; col < 5; col++) {
            uint8_t col_bits = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (col_bits & (1U << row)) {
                    display_set_pixel(cx + col, y + row, gray);
                }
            }
        }
        cx += 6;  /* 5 px + 1 spacing */
    }
}

/* ---------------------------------------------------------------------
 * Rectangle / progress bar
 * --------------------------------------------------------------------- */

void display_draw_rect(int x, int y, int w, int h, uint8_t gray)
{
    for (int i = 0; i < w; i++) {
        display_set_pixel(x + i, y, gray);
        display_set_pixel(x + i, y + h - 1, gray);
    }
    for (int j = 0; j < h; j++) {
        display_set_pixel(x, y + j, gray);
        display_set_pixel(x + w - 1, y + j, gray);
    }
}

void display_fill_rect(int x, int y, int w, int h, uint8_t gray)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            display_set_pixel(x + i, y + j, gray);
}

void display_draw_progress(int x, int y, int w, int h, float pct)
{
    display_draw_rect(x, y, w, h, 0x0F);
    int fill = (int)((float)(w - 2) * pct);
    if (fill > 0)
        display_fill_rect(x + 1, y + 1, fill, h - 2, 0x0F);
}

/* ---------------------------------------------------------------------
 * Conductivity map rendering (false-color, 64×64 region centered)
 * --------------------------------------------------------------------- */

void display_draw_conductivity_map(const float *image, int w, int h)
{
    /* Map to center of screen: 64×64 area starting at (32, 0) */
    int ox = (OLED_WIDTH - 64) / 2;
    int oy = 0;
    int sz = 64;

    for (int y = 0; y < sz; y++) {
        for (int x = 0; x < sz; x++) {
            /* Sample the image buffer */
            int sx = (int)((float)x / sz * w);
            int sy = (int)((float)y / sz * h);
            if (sx >= w) sx = w - 1;
            if (sy >= h) sy = h - 1;
            float val = image[sy * w + sx];
            uint8_t gray = (uint8_t)(val * 15.0f);
            if (gray > 15) gray = 15;
            display_set_pixel(ox + x, oy + y, gray);
        }
    }

    /* Draw electrode ring outline (circle of radius 30 centered at 64,32) */
    display_draw_electrode_ring(ox + 32, oy + 32, 30, 0xFFFF);
}

/* ---------------------------------------------------------------------
 * Draw electrode ring with contact indicators
 * --------------------------------------------------------------------- */

void display_draw_electrode_ring(int cx, int cy, int r, uint16_t contact_mask)
{
    /* Draw 16 electrode dots around a circle */
    for (int i = 0; i < EIT_NUM_ELECTRODES; i++) {
        float angle = 2.0f * 3.14159265f * i / EIT_NUM_ELECTRODES;
        int ex = cx + (int)(r * cosf(angle));
        int ey = cy + (int)(r * sinf(angle));
        uint8_t gray = (contact_mask & (1U << i)) ? 0x0F : 0x01;
        /* Draw a 3×3 dot */
        for (int dx = -1; dx <= 1; dx++)
            for (int dy = -1; dy <= 1; dy++)
                display_set_pixel(ex + dx, ey + dy, gray);
    }
}

/* ---------------------------------------------------------------------
 * Status / menu display
 * --------------------------------------------------------------------- */

void display_show_status(const char *line1, const char *line2)
{
    display_clear();
    display_draw_text(0, 0, "RootTrace", 0x0F);
    display_draw_text(0, 12, "-----------", 0x08);
    if (line1) display_draw_text(0, 24, line1, 0x0F);
    if (line2) display_draw_text(0, 36, line2, 0x0A);
    display_draw_text(0, 52, "jayis1 2026", 0x04);
    display_flush();
}

void display_show_menu(const char *title, const char *items[], int n_items, int selected)
{
    display_clear();
    display_draw_text(0, 0, title, 0x0F);
    display_draw_text(0, 8, "-----------", 0x06);
    int y = 18;
    for (int i = 0; i < n_items && y < 56; i++) {
        if (i == selected) {
            display_fill_rect(0, y - 1, OLED_WIDTH, 9, 0x0F);
            display_draw_text(2, y, items[i], 0x00);
        } else {
            display_draw_text(2, y, items[i], 0x0A);
        }
        y += 10;
    }
    display_flush();
}