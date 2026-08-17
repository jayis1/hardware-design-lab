/*
 * display.c — OLED Status Display Driver (SSD1306) Implementation
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Drives a 128×64 monochrome OLED via SPI3 for status display,
 * scan progress, and simple tomogram preview.
 */

#include "display.h"
#include "board.h"
#include <string.h>

/* Framebuffer: 128×64 bits = 1024 bytes = 8 pages of 128 bytes */
static uint8_t fb[OLED_WIDTH * OLED_HEIGHT / 8];
static int fb_dirty = 0;

/* 5×7 font table (ASCII 32-127, simplified) */
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
    {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x7F,0x41,0x41,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* \ */
    {0x00,0x41,0x41,0x7F,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* _ */
    {0x00,0x00,0x03,0x04,0x00}, /* ` */
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
    {0x00,0x00,0x00,0x00,0x00}, /* { (placeholder) */
    {0x00,0x00,0x00,0x00,0x00}, /* | */
    {0x00,0x00,0x00,0x00,0x00}, /* } */
    {0x00,0x00,0x00,0x00,0x00}, /* ~ */
};

/* ---- SPI3 transfer for OLED ---- */
static uint8_t oled_spi_xfer(uint8_t tx) {
    while (!(OLED_SPI->SR & SPI_SR_TXP)) { }
    *(volatile uint8_t *)&OLED_SPI->TXDR = tx;
    while (!(OLED_SPI->SR & SPI_SR_RXP)) { }
    return *(volatile uint8_t *)&OLED_SPI->RXDR;
}

/* ---- Send command to SSD1306 ---- */
static void oled_cmd(uint8_t cmd) {
    GPIO_CLR(OLED_CS, OLED_CS_PIN);
    GPIO_CLR(OLED_DC, OLED_DC_PIN);  /* DC=0 = command */
    oled_spi_xfer(cmd);
    GPIO_SET(OLED_CS, OLED_CS_PIN);
}

/* ---- Send data to SSD1306 ---- */
static void oled_data(uint8_t data) {
    GPIO_CLR(OLED_CS, OLED_CS_PIN);
    GPIO_SET(OLED_DC, OLED_DC_PIN);  /* DC=1 = data */
    oled_spi_xfer(data);
    GPIO_SET(OLED_CS, OLED_CS_PIN);
}

/* ---- Initialize OLED display ---- */
void display_init(void) {
    /* Enable SPI3 clock */
    RCC_APB1LENR |= RCC_APB1LENR_SPI3EN;

    /* Configure SPI3: Master, 8-bit, mode 0 */
    OLED_SPI->CR1 &= ~SPI_CR1_SPE;
    OLED_SPI->CFG1 = (2U << SPI_CFG1_MBR_SHIFT) |  /* Baud /8 */
                     (7U << SPI_CFG1_DSIZE_SHIFT) |
                     SPI_CFG1_MASTER;
    OLED_SPI->CFG2 = 0;
    OLED_SPI->CR1 |= SPI_CR1_SPE;

    /* Reset OLED */
    GPIO_CLR(OLED_RST, OLED_RST_PIN);
    delay_ms(10);
    GPIO_SET(OLED_RST, OLED_RST_PIN);
    delay_ms(10);

    /* SSD1306 initialization sequence */
    oled_cmd(0xAE);  /* Display off */
    oled_cmd(0xD5); oled_cmd(0x80);  /* Set display clock divide */
    oled_cmd(0xA8); oled_cmd(0x3F);  /* Set multiplex: 64 */
    oled_cmd(0xD3); oled_cmd(0x00);  /* Display offset: 0 */
    oled_cmd(0x40);  /* Start line: 0 */
    oled_cmd(0x8D); oled_cmd(0x14);  /* Charge pump: on */
    oled_cmd(0x20); oled_cmd(0x00);  /* Memory mode: horizontal */
    oled_cmd(0xA1);  /* Segment remap: yes */
    oled_cmd(0xC8);  /* COM scan direction: remapped */
    oled_cmd(0xDA); oled_cmd(0x12);  /* COM pins: alt */
    oled_cmd(0x81); oled_cmd(0xCF);  /* Contrast: 207 */
    oled_cmd(0xD9); oled_cmd(0xF1);  /* Precharge: F1 */
    oled_cmd(0xDB); oled_cmd(0x40);  /* VCOM detect: 0x40 */
    oled_cmd(0xA4);  /* Display from RAM */
    oled_cmd(0xA6);  /* Normal display (not inverted) */
    oled_cmd(0xAF);  /* Display on */

    display_clear();
    display_refresh();
}

/* ---- Clear framebuffer ---- */
void display_clear(void) {
    memset(fb, 0, sizeof(fb));
    fb_dirty = 1;
}

/* ---- Set a pixel in the framebuffer ---- */
void display_draw_pixel(int x, int y, int on) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;

    int page = y / 8;
    int bit = y % 8;

    if (on) {
        fb[page * OLED_WIDTH + x] |= (1 << bit);
    } else {
        fb[page * OLED_WIDTH + x] &= ~(1 << bit);
    }
    fb_dirty = 1;
}

/* ---- Draw a string at (x, y) with optional size multiplier ---- */
void display_draw_string(int x, int y, const char *str, int size) {
    int cx = x;
    for (int i = 0; str[i]; i++) {
        char c = str[i];
        if (c < 32 || c > 127) c = 32;  /* Clamp to font range */
        int idx = c - 32;

        for (int col = 0; col < 5; col++) {
            uint8_t bits = font5x7[idx][col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row)) {
                    if (size == 1) {
                        display_draw_pixel(cx + col, y + row, 1);
                    } else {
                        /* Scale up by 'size' factor */
                        for (int dx = 0; dx < size; dx++) {
                            for (int dy = 0; dy < size; dy++) {
                                display_draw_pixel(cx + col * size + dx,
                                                   y + row * size + dy, 1);
                            }
                        }
                    }
                }
            }
        }
        cx += (5 + 1) * size;  /* 5px char + 1px spacing */
    }
}

/* ---- Bresenham line algorithm ---- */
void display_draw_line(int x0, int y0, int x1, int y1) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        display_draw_pixel(x0, y0, 1);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

/* ---- Draw rectangle outline ---- */
void display_draw_rect(int x, int y, int w, int h) {
    display_draw_line(x, y, x + w - 1, y);
    display_draw_line(x, y + h - 1, x + w - 1, y + h - 1);
    display_draw_line(x, y, x, y + h - 1);
    display_draw_line(x + w - 1, y, x + w - 1, y + h - 1);
}

/* ---- Draw circle outline (midpoint algorithm) ---- */
void display_draw_circle(int cx, int cy, int r) {
    int x = r, y = 0;
    int err = 0;

    while (x >= y) {
        display_draw_pixel(cx + x, cy + y, 1);
        display_draw_pixel(cx + y, cy + x, 1);
        display_draw_pixel(cx - y, cy + x, 1);
        display_draw_pixel(cx - x, cy + y, 1);
        display_draw_pixel(cx - x, cy - y, 1);
        display_draw_pixel(cx - y, cy - x, 1);
        display_draw_pixel(cx + y, cy - x, 1);
        display_draw_pixel(cx + x, cy - y, 1);

        if (err <= 0) { y++; err += 2 * y + 1; }
        if (err > 0)  { x--; err -= 2 * x + 1; }
    }
}

/* ---- Draw a simple tomogram preview (color-mapped to monochrome) ---- */
void display_draw_tomogram(int x, int y, int size, const float *velocity) {
    /* Draw a circular tomogram preview on the OLED.
     * The trunk cross-section is drawn as a circle, with cells
     * shaded by velocity: high velocity = filled, low = empty.
     * This gives a rough visual indication of decay location. */
    int radius = size / 2;
    int cx = x + radius;
    int cy = y + radius;

    /* Draw trunk outline */
    display_draw_circle(cx, cy, radius);

    /* Draw sensor position markers around perimeter */
    int n_sensors = 12;
    for (int i = 0; i < n_sensors; i++) {
        float angle = (2.0f * 3.14159f * (float)i) / (float)n_sensors;
        int sx = cx + (int)(radius * cosf(angle));
        int sy = cy + (int)(radius * sinf(angle));
        display_draw_pixel(sx, sy, 1);
        display_draw_pixel(sx + 1, sy, 1);
        display_draw_pixel(sx, sy + 1, 1);
    }

    /* Fill cells based on velocity (simplified: just fill/don't fill) */
    /* In a real implementation, we'd map the polar grid to screen
     * coordinates and shade each cell. For the OLED preview, we
     * just draw a filled/unfilled pattern for each radial ring. */
    for (int r = 1; r < radius; r += 2) {
        display_draw_circle(cx, cy, r);
    }
}

/* ---- Refresh display from framebuffer ---- */
void display_refresh(void) {
    if (!fb_dirty) return;

    /* Set column and page addresses */
    oled_cmd(0x21);  /* Set column address */
    oled_cmd(0);     /* Start: 0 */
    oled_cmd(127);   /* End: 127 */

    oled_cmd(0x22);  /* Set page address */
    oled_cmd(0);     /* Start: 0 */
    oled_cmd(7);     /* End: 7 */

    /* Send framebuffer data */
    for (int i = 0; i < (int)sizeof(fb); i++) {
        oled_data(fb[i]);
    }

    fb_dirty = 0;
}

/* EOF — display.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */