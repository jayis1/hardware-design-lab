/*
 * display.c — AeroCast ST7789V 2.4" 320×240 IPS display driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Minimal framebuffer + glyph blitter for the on-device status display.
 * SPI3 at 18 MHz drives the ST7789V. We keep a 320×240×2 byte frame-
 * buffer in SRAM (150 KB) — the STM32H7A3 has 1.1 MB so this is fine.
 *
 * The UI shows:
 *   - boot splash
 *   - live status (total particle count, flow, T, RH, mini bar chart)
 *   - alert overlay (lid open, laser fault)
 */
#ifndef AEROCAST_DISPLAY_C
#define AEROCAST_DISPLAY_C
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "board.h"
#include "registers.h"
#include "display.h"

#define DISP_W 320
#define DISP_H 240
#define FB_SZ  (DISP_W * DISP_H * 2)
static uint16_t g_fb[DISP_W * DISP_H];   /* 150 KB — RGB565 */

/* ---- SPI helpers ---- */
static inline void disp_cs(uint8_t low) { if (low) gpio_clr(PORTC, 13); else gpio_set(PORTC, 13); }
static inline void disp_dc(uint8_t data){ if (data) gpio_set(PORTC, 14); else gpio_clr(PORTC, 14); }

static void disp_spi_write(const uint8_t *p, size_t n)
{
    spi_t *s = SPI(SPI3);
    for (size_t i = 0; i < n; ++i) {
        while (!(s->SR & (1u << 1u))) ;
        *((volatile uint8_t *)&s->TXDR) = p[i];
        while (!(s->SR & (1u << 2u))) ;
        (void)s->RXDR;
    }
}

static void disp_cmd(uint8_t cmd)
{
    disp_cs(1); disp_dc(0); disp_cs(0);
    uint8_t b = cmd; disp_spi_write(&b, 1);
    disp_cs(1);
}

static void disp_data(const uint8_t *d, size_t n)
{
    disp_cs(1); disp_dc(1); disp_cs(0);
    disp_spi_write(d, n);
    disp_cs(1);
}

/* ---- ST7789V init sequence ---- */
static const uint8_t INIT_SEQ[] = {
    0x01, 0,                 /* SWRESET */
    0x11, 0,                 /* SLPOUT */
    0x36, 1, 0x60,           /* MADCTL: RGB, row/col order */
    0x3A, 1, 0x55,           /* COLMOD: 16-bit RGB565 */
    0xB2, 5, 0x0C,0x0C,0x00,0x33,0x33, /* PORCTRL */
    0xB7, 1, 0x35,           /* GCTRL */
    0xBB, 1, 0x19,           /* VCOMS */
    0xC2, 1, 0x01,           /* VDVVRHEN */
    0xC3, 1, 0x12,           /* VRHS */
    0xC4, 1, 0x20,           /* VDVSET */
    0xC5, 1, 0x20,           /* VCMOFSET */
    0xD0, 2, 0xA4,0xA1,      /* PWCTRL1 */
    0xE0,14, 0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23,
    0xE1,14, 0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23,
    0x21, 0,                 /* INVON */
    0x29, 0,                 /* DISPON */
};

static void disp_run_init(void)
{
    for (size_t i = 0; i < sizeof(INIT_SEQ); ) {
        uint8_t cmd = INIT_SEQ[i++];
        uint8_t n   = INIT_SEQ[i++];
        disp_cmd(cmd);
        if (n) disp_data(&INIT_SEQ[i], n);
        i += n;
        delay_ms(10);
    }
}

/* ---- Public API ---- */
void display_init(void)
{
    /* Reset */
    gpio_clr(PORTC, 15); delay_ms(10);
    gpio_set(PORTC, 15); delay_ms(120);
    /* Backlight on */
    gpio_set(PORTD, 0);
    /* SPI3 master, 18 MHz, 8-bit */
    spi_init_master(SPI3_BASE, /* br_div */ 6u, /* word_sz */ 7u); /* 8-bit = 0b0111 */
    disp_run_init();
    /* Fill framebuffer black */
    memset(g_fb, 0, sizeof(g_fb));
    display_flush();
}

void display_flush(void)
{
    disp_cmd(0x2A); uint8_t cx[4] = {0,0, DISP_W>>8, DISP_W&0xFF}; disp_data(cx,4);
    disp_cmd(0x2B); uint8_t cy[4] = {0,0, DISP_H>>8, DISP_H&0xFF}; disp_data(cy,4);
    disp_cmd(0x2C);
    disp_cs(1); disp_dc(1); disp_cs(0);
    /* Send framebuffer as bytes (swap to big-endian RGB565 for ST7789) */
    spi_t *s = SPI(SPI3);
    for (int i = 0; i < DISP_W * DISP_H; ++i) {
        uint16_t px = g_fb[i];
        uint8_t hi = (uint8_t)(px >> 8), lo = (uint8_t)px;
        while (!(s->SR & (1u << 1u))) ;
        *((volatile uint8_t *)&s->TXDR) = hi;
        while (!(s->SR & (1u << 2u))) ; (void)s->RXDR;
        while (!(s->SR & (1u << 1u))) ;
        *((volatile uint8_t *)&s->TXDR) = lo;
        while (!(s->SR & (1u << 2u))) ; (void)s->RXDR;
    }
    disp_cs(1);
}

/* ---- Drawing primitives ---- */
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3));
}

void display_fill(uint16_t color)
{
    for (int i = 0; i < DISP_W * DISP_H; ++i) g_fb[i] = color;
}

void display_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int j = 0; j < h; ++j) {
        int yy = y + j;
        if (yy < 0 || yy >= DISP_H) continue;
        for (int i = 0; i < w; ++i) {
            int xx = x + i;
            if (xx < 0 || xx >= DISP_W) continue;
            g_fb[yy * DISP_W + xx] = color;
        }
    }
}

/* Tiny 5×8 font (ASCII 32–127). Each char = 5 bytes, one per column,
 * bits 0–7 are rows top→bottom. */
static const uint8_t FONT_5x8[][5] = {
    /* space */ {0,0,0,0,0},{0,0,0x5F,0,0},{0,0x07,0,0x07,0},{0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},
    {0,0x05,0x03,0,0},{0,0x1C,0x22,0x41,0},{0,0x41,0x22,0x1C,0},{0x14,0x08,0x3E,0x08,0x14},
    {0x08,0x08,0x3E,0x08,0x08},{0,0x50,0x30,0,0},{0x08,0x08,0x08,0x08,0x08},
    {0,0x60,0x60,0,0},{0x20,0x10,0x08,0x04,0x02},
    /* 0-9 */ {0x1C,0x22,0x22,0x22,0x1C},{0x08,0x18,0x08,0x08,0x08},
    {0x1C,0x22,0x10,0x08,0x3E},{0x3E,0x10,0x18,0x04,0x3C},{0x08,0x14,0x22,0x3E,0x08},
    {0x3E,0x20,0x3C,0x02,0x3C},{0x1C,0x20,0x3C,0x22,0x1C},{0x3E,0x02,0x04,0x08,0x10},
    {0x1C,0x22,0x1C,0x22,0x1C},{0x1C,0x22,0x1E,0x02,0x1C},
    /* A-Z (subset) */
    {0x1C,0x22,0x3E,0x22,0x22},{0x3C,0x22,0x3C,0x22,0x3C},{0x1C,0x22,0x20,0x22,0x1C},
    {0x3C,0x22,0x22,0x22,0x3C},{0x3E,0x20,0x38,0x20,0x3E},{0x3E,0x20,0x38,0x20,0x20},
    {0x1C,0x20,0x2E,0x22,0x1C},{0x22,0x22,0x3E,0x22,0x22},{0x1C,0x08,0x08,0x08,0x1C},
    {0x0E,0x02,0x02,0x22,0x1C},{0x22,0x28,0x30,0x28,0x22},{0x20,0x20,0x20,0x20,0x3E},
    {0x22,0x36,0x2A,0x22,0x22},{0x22,0x3A,0x2A,0x26,0x22},{0x1C,0x22,0x22,0x22,0x1C},
    {0x3C,0x22,0x3C,0x20,0x20},{0x1C,0x22,0x22,0x2A,0x1C},{0x3C,0x22,0x3C,0x28,0x24},
    {0x1E,0x20,0x1C,0x02,0x3C},{0x3E,0x08,0x08,0x08,0x08},{0x22,0x22,0x22,0x22,0x1C},
    {0x22,0x22,0x22,0x14,0x08},{0x22,0x22,0x2A,0x2A,0x14},{0x22,0x14,0x08,0x14,0x22},
    {0x22,0x14,0x08,0x08,0x10},{0x3E,0x04,0x08,0x10,0x3E},
};

static int font_index(char c)
{
    if (c == ' ') return 0;
    if (c == '!') return 1; if (c == '"') return 2; if (c == '#') return 3;
    if (c == '$') return 4; if (c == '%') return 5; if (c == '&') return 6;
    if (c == '\'')return 7; if (c == '(') return 8; if (c == ')') return 9;
    if (c == '*') return 10;if (c == '+') return 11;if (c == ',') return 12;
    if (c == '-') return 13;if (c == '.') return 14;if (c == '/') return 15;
    if (c >= '0' && c <= '9') return 16 + (c - '0');
    if (c >= 'A' && c <= 'Z') return 26 + (c - 'A');
    return 0;
}

void display_text(int x, int y, const char *s, uint16_t fg, uint16_t bg)
{
    int cx = x;
    while (*s) {
        int idx = font_index(*s);
        const uint8_t *glyph = FONT_5x8[idx];
        for (int col = 0; col < 5; ++col) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 8; ++row) {
                int px = cx + col, py = y + row;
                if (px >= 0 && px < DISP_W && py >= 0 && py < DISP_H)
                    g_fb[py * DISP_W + px] = (bits & (1u << row)) ? fg : bg;
            }
        }
        cx += 6;
        s++;
    }
}

/* ---- High-level screens ---- */
void display_boot_splash(const char *msg)
{
    display_fill(rgb565(0,0,0));
    display_text(40, 80, "AeroCast", rgb565(0,255,180), rgb565(0,0,0));
    display_text(40, 100, "by jayis1", rgb565(180,180,180), rgb565(0,0,0));
    if (msg) display_text(20, 140, msg, rgb565(255,255,0), rgb565(0,0,0));
    display_flush();
}

void display_status(uint32_t total_count, float flow, float t, float rh,
                    const uint32_t *prev_counts)
{
    char line[40];
    display_fill(rgb565(0,0,0));
    display_text(4, 4, "AeroCast  jayis1", rgb565(0,255,180), rgb565(0,0,0));

    snprintf(line, sizeof(line), "Particles: %lu", (unsigned long)total_count);
    display_text(4, 28, line, rgb565(255,255,255), rgb565(0,0,0));
    snprintf(line, sizeof(line), "Flow: %.1f L/min", flow);
    display_text(4, 44, line, rgb565(200,255,200), rgb565(0,0,0));
    snprintf(line, sizeof(line), "T: %.1fC  RH: %.0f%%", t, rh);
    display_text(4, 60, line, rgb565(200,200,255), rgb565(0,0,0));

    /* Mini bar chart of prev minute's per-class counts (first 12 classes) */
    display_text(4, 84, "Last minute:", rgb565(255,255,0), rgb565(0,0,0));
    if (prev_counts) {
        uint32_t maxc = 1;
        for (int i = 1; i < 12; ++i) if (prev_counts[i] > maxc) maxc = prev_counts[i];
        for (int i = 1; i < 12; ++i) {
            int h = (int)((float)prev_counts[i] / (float)maxc * 60.0f);
            display_rect(8 + (i - 1) * 24, 200 - h, 20, h, rgb565(80,180,255));
        }
    }
    display_flush();
}

void display_alert(const char *msg)
{
    display_fill(rgb565(0,0,0));
    display_rect(0, 100, 320, 40, rgb565(180,0,0));
    if (msg) display_text(10, 112, msg, rgb565(255,255,255), rgb565(180,0,0));
    display_flush();
}