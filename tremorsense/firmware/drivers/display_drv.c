/*
 * display_drv.c — GC9A01 1.28" round OLED display driver
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Driver for the GC9A01-based 240×240 RGB OLED display.
 * Provides initialization, drawing primitives, and TremorSense UI screens.
 */

#include "display_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static uint8_t display_initialized = 0;
static uint16_t fb[DISP_WIDTH * DISP_HEIGHT];  /* Frame buffer */
static uint8_t  current_font = 0;

/* ---- SPI low-level ---- */
static void disp_cs_low(void)  { /* gpio_set(DISP_CS_PIN, 0); */ }
static void disp_cs_high(void) { /* gpio_set(DISP_CS_PIN, 1); */ }
static void disp_dc_command(void) { /* gpio_set(DISP_DC_PIN, 0); */ }
static void disp_dc_data(void)   { /* gpio_set(DISP_DC_PIN, 1); */ }

static void disp_spi_write(const uint8_t *data, uint16_t len)
{
    /* In production: use nRF SPIM0 with DMA */
    (void)data; (void)len;
}

static void disp_write_command(uint8_t cmd)
{
    disp_dc_command();
    disp_cs_low();
    disp_spi_write(&cmd, 1);
    disp_cs_high();
}

static void disp_write_data(const uint8_t *data, uint16_t len)
{
    disp_dc_data();
    disp_cs_low();
    disp_spi_write(data, len);
    disp_cs_high();
}

/* ---- Simple 5×7 font (ASCII 32–95) ---- */
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
    {0x3E,0x41,0x5D,0x55,0x1E}, /* @ */
    {0x7C,0x12,0x11,0x12,0x7C}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x01,0x01}, /* F */
    {0x3E,0x41,0x41,0x51,0x32}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x40,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x1E,0x21,0x21,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x20,0x20,0x1F}, /* V */
    {0x7F,0x20,0x18,0x20,0x7F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x03,0x04,0x78,0x04,0x03}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
};

/* ---- Initialization ---- */
int display_init(void)
{
    /* Hardware reset */
    /* gpio_set(DISP_RST_PIN, 0); */
    /* board_delay_ms(10); */
    /* gpio_set(DISP_RST_PIN, 1); */
    /* board_delay_ms(120); */

    /* Initialization sequence for GC9A01 */
    disp_write_command(GC9A01_CMD_SWRESET);
    /* board_delay_ms(120); */

    disp_write_command(GC9A01_CMD_SLPOUT);
    /* board_delay_ms(120); */

    /* Pixel format: 16-bit RGB565 */
    disp_write_command(GC9A01_CMD_PIXFMT);
    uint8_t pixfmt = GC9A01_PIXFMT_16BIT;
    disp_write_data(&pixfmt, 1);

    /* Memory access control: MADCTL */
    disp_write_command(GC9A01_CMD_MADCTL);
    uint8_t madctl = GC9A01_MADCTL_MX | GC9A01_MADCTL_MV | GC9A01_MADCTL_BGR;
    disp_write_data(&madctl, 1);

    /* Frame rate control */
    disp_write_command(GC9A01_CMD_FRMCTR1);
    uint8_t frmctr[] = {0x01, 0x2C, 0x2D};
    disp_write_data(frmctr, 3);

    /* Power control */
    disp_write_command(GC9A01_CMD_PWCTR1);
    uint8_t pwctr1[] = {0xA4, 0xA1};
    disp_write_data(pwctr1, 2);

    disp_write_command(GC9A01_CMD_VMCTR1);
    uint8_t vmctr[] = {0x00, 0x8D};  /* VCOM=1.4V */
    disp_write_data(vmctr, 2);

    /* Gamma correction (positive) */
    disp_write_command(GC9A01_CMD_GAMCTRP1);
    uint8_t gammap[] = {0x00, 0x19, 0x21, 0x0B, 0x0F, 0x0A, 0x3C, 0x78,
                        0x4A, 0x07, 0x0F, 0x12, 0x3B, 0x3B, 0x3F, 0x3F};
    disp_write_data(gammap, 16);

    /* Gamma correction (negative) */
    disp_write_command(GC9A01_CMD_GAMCTRN1);
    uint8_t gamman[] = {0x00, 0x19, 0x21, 0x0B, 0x0F, 0x0A, 0x3C, 0x78,
                        0x4A, 0x07, 0x0F, 0x12, 0x3B, 0x3B, 0x3F, 0x3F};
    disp_write_data(gamman, 16);

    /* Turn on display */
    disp_write_command(GC9A01_CMD_NORON);
    disp_write_command(GC9A01_CMD_DISPON);

    /* Set backlight on */
    /* gpio_set(DISP_BL_PIN, 1); */

    /* Clear frame buffer */
    display_fill_screen(COLOR_BLACK);

    display_initialized = 1;
    return 0;
}

/* ---- Basic drawing ---- */
void display_clear(void)
{
    display_fill_screen(COLOR_BLACK);
}

void display_fill_screen(uint16_t color)
{
    for (int i = 0; i < DISP_WIDTH * DISP_HEIGHT; i++) {
        fb[i] = color;
    }
    /* Push to display: set address window and write */
    disp_write_command(GC9A01_CMD_CASET);
    uint8_t caset[] = {0x00, 0x00, 0x00, 0xEF};
    disp_write_data(caset, 4);

    disp_write_command(GC9A01_CMD_RASET);
    uint8_t raset[] = {0x00, 0x00, 0x00, 0xEF};
    disp_write_data(raset, 4);

    disp_write_command(GC9A01_CMD_RAMWR);
    /* Write entire frame buffer */
    /* disp_write_data((uint8_t*)fb, DISP_WIDTH * DISP_HEIGHT * 2); */
}

void display_draw_pixel(int x, int y, uint16_t color)
{
    if (x < 0 || x >= DISP_WIDTH || y < 0 || y >= DISP_HEIGHT) return;
    fb[y * DISP_WIDTH + x] = color;
}

void display_draw_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    /* Bresenham's line algorithm */
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        display_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void display_draw_rect(int x, int y, int w, int h, uint16_t color)
{
    display_draw_line(x, y, x + w - 1, y, color);
    display_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    display_draw_line(x, y, x, y + h - 1, color);
    display_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void display_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            display_draw_pixel(i, j, color);
        }
    }
}

void display_draw_circle(int cx, int cy, int r, uint16_t color)
{
    /* Midpoint circle algorithm */
    int x = r, y = 0;
    int err = 1 - r;

    while (x >= y) {
        display_draw_pixel(cx + x, cy + y, color);
        display_draw_pixel(cx - x, cy + y, color);
        display_draw_pixel(cx + x, cy - y, color);
        display_draw_pixel(cx - x, cy - y, color);
        display_draw_pixel(cx + y, cy + x, color);
        display_draw_pixel(cx - y, cy + x, color);
        display_draw_pixel(cx + y, cy - x, color);
        display_draw_pixel(cx - y, cy - x, color);
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void display_fill_circle(int cx, int cy, int r, uint16_t color)
{
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                display_draw_pixel(cx + x, cy + y, color);
            }
        }
    }
}

void display_draw_char(int x, int y, char c, uint16_t color, uint8_t size)
{
    if (c < 32 || c > 90) c = 32;  /* Clamp to font range */
    int idx = c - 32;
    if (idx >= (int)(sizeof(font5x7) / sizeof(font5x7[0]))) return;

    for (int col = 0; col < 5; col++) {
        uint8_t line = font5x7[idx][col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                if (size == 1) {
                    display_draw_pixel(x + col, y + row, color);
                } else {
                    display_fill_rect(x + col * size, y + row * size,
                                      size, size, color);
                }
            }
        }
    }
}

void display_draw_text(int x, int y, const char *text, uint16_t color,
                         uint8_t size)
{
    int cx = x;
    while (*text) {
        display_draw_char(cx, y, *text, color, size);
        cx += 6 * size;
        text++;
    }
}

void display_draw_arc(int cx, int cy, int r, int start_angle, int end_angle,
                       uint16_t color)
{
    for (int a = start_angle; a <= end_angle; a++) {
        float rad = (float)a * (float)M_PI / 180.0f;
        int x = cx + (int)(cosf(rad) * r);
        int y = cy + (int)(sinf(rad) * r);
        display_draw_pixel(x, y, color);
    }
}

void display_set_font(uint8_t font_id)
{
    current_font = font_id;
}

void display_on(void)
{
    disp_write_command(GC9A01_CMD_DISPON);
    /* gpio_set(DISP_BL_PIN, 1); */
}

void display_off(void)
{
    disp_write_command(GC9A01_CMD_DISPOFF);
    /* gpio_set(DISP_BL_PIN, 0); */
}

void display_set_backlight(uint8_t on)
{
    /* gpio_set(DISP_BL_PIN, on ? 1 : 0); */
    (void)on;
}

/* ---- UI screens ---- */
void display_show_splash(const char *title, const char *subtitle)
{
    display_fill_screen(COLOR_DARKBLUE);
    int title_w = strlen(title) * 6;
    display_draw_text((DISP_WIDTH - title_w) / 2, 100, title, COLOR_WHITE, 2);
    int sub_w = strlen(subtitle) * 6;
    display_draw_text((DISP_WIDTH - sub_w) / 2, 130, subtitle, COLOR_CYAN, 1);
}

void display_show_idle(uint16_t tremor_score, uint8_t battery_pct,
                        uint8_t tremor_active)
{
    display_fill_screen(COLOR_BLACK);

    /* Draw circular outer ring */
    display_draw_circle(DISP_WIDTH / 2, DISP_HEIGHT / 2, 115, COLOR_GRAY);

    /* Draw tremor score as arc (0–100 maps to 0–270 degrees) */
    int score_angle = (tremor_score * 270) / 100;
    uint16_t score_color = (tremor_score < 30) ? COLOR_GREEN :
                            (tremor_score < 70) ? COLOR_YELLOW : COLOR_RED;
    display_draw_arc(DISP_WIDTH / 2, DISP_HEIGHT / 2, 110,
                     -135, -135 + score_angle, score_color);

    /* Display numeric score in center */
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", tremor_score);
    int text_w = strlen(buf) * 12;
    display_draw_text((DISP_WIDTH - text_w) / 2, 105, buf, COLOR_WHITE, 2);
    display_draw_text((DISP_WIDTH - 36) / 2, 130, "SCORE", COLOR_GRAY, 1);

    /* Battery indicator (bottom) */
    snprintf(buf, sizeof(buf), "BAT %d%%", battery_pct);
    display_draw_text((DISP_WIDTH - 36) / 2, 160, buf, COLOR_CYAN, 1);

    /* Tremor status (top) */
    if (tremor_active) {
        display_draw_text((DISP_WIDTH - 30) / 2, 20, "TREMOR", COLOR_RED, 1);
    } else {
        display_draw_text((DISP_WIDTH - 42) / 2, 20, "STABLE", COLOR_GREEN, 1);
    }
}

void display_show_waveform(const float *data, int n_samples,
                             uint8_t tremor_active, float dom_freq)
{
    display_fill_screen(COLOR_BLACK);

    /* Draw center line */
    int mid_y = DISP_HEIGHT / 2;
    display_draw_line(0, mid_y, DISP_WIDTH - 1, mid_y, COLOR_GRAY);

    /* Draw waveform: map float data to screen coordinates */
    uint16_t color = tremor_active ? COLOR_RED : COLOR_GREEN;
    int prev_x = 0, prev_y = mid_y;

    for (int i = 0; i < n_samples && i < DISP_WIDTH; i++) {
        int x = i * DISP_WIDTH / n_samples;
        /* Scale: assume data range is approximately [-0.5, 0.5] g */
        int y = mid_y - (int)(data[i] * 100.0f);
        if (y < 0) y = 0;
        if (y >= DISP_HEIGHT) y = DISP_HEIGHT - 1;

        display_draw_line(prev_x, prev_y, x, y, color);
        prev_x = x;
        prev_y = y;
    }

    /* Show dominant frequency */
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f Hz", dom_freq);
    display_draw_text(5, 5, buf, COLOR_WHITE, 1);
}

void display_show_spectrum(const float *psd, int n_bins, float dom_freq)
{
    display_fill_screen(COLOR_BLACK);

    /* Draw bar chart of PSD */
    int max_bar = DISP_HEIGHT - 30;
    float max_psd = 0.001f;
    for (int i = 0; i < n_bins && i < 60; i++) {
        if (psd[i] > max_psd) max_psd = psd[i];
    }

    for (int i = 0; i < n_bins && i < 60; i++) {
        int bar_h = (int)((psd[i] / max_psd) * (float)max_bar);
        int x = i * (DISP_WIDTH / 60);
        int w = (DISP_WIDTH / 60) - 1;
        uint16_t color = (i == (int)(dom_freq / FREQ_RESOLUTION_HZ)) ?
                         COLOR_RED : COLOR_CYAN;
        display_fill_rect(x, DISP_HEIGHT - bar_h - 20, w, bar_h, color);
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "Peak: %.1f Hz", dom_freq);
    display_draw_text(5, 5, buf, COLOR_WHITE, 1);
}

void display_show_score(uint16_t score, uint32_t tremor_seconds,
                          uint8_t tremor_class, float confidence)
{
    display_fill_screen(COLOR_BLACK);

    char buf[24];

    /* Score */
    snprintf(buf, sizeof(buf), "Score: %d/100", score);
    display_draw_text(20, 40, buf, COLOR_WHITE, 2);

    /* Tremor time today */
    uint16_t mins = (uint16_t)(tremor_seconds / 60);
    snprintf(buf, sizeof(buf), "Tremor: %dm", mins);
    display_draw_text(40, 80, buf, COLOR_YELLOW, 1);

    /* Current class */
    const char *cls_name = "None";
    if (tremor_class < 5) {
        const char *names[] = {"Parkin", "Essent", "Cereb", "Physio", "Drug"};
        cls_name = names[tremor_class];
    }
    snprintf(buf, sizeof(buf), "Type: %s", cls_name);
    display_draw_text(40, 110, buf, COLOR_CYAN, 1);

    /* Confidence */
    snprintf(buf, sizeof(buf), "Conf: %d%%", (int)(confidence * 100));
    display_draw_text(40, 130, buf, COLOR_GREEN, 1);
}

void display_show_battery(uint8_t pct, uint8_t charging,
                            uint16_t runtime_hours)
{
    display_fill_screen(COLOR_BLACK);

    /* Draw battery icon */
    display_draw_rect(60, 80, 120, 60, COLOR_WHITE);
    display_fill_rect(180, 95, 10, 30, COLOR_WHITE);

    /* Fill battery based on percentage */
    uint16_t fill_color = (pct > 20) ? COLOR_GREEN : COLOR_RED;
    int fill_w = (118 * pct) / 100;
    display_fill_rect(61, 81, fill_w, 58, fill_color);

    /* Percentage text */
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    display_draw_text((DISP_WIDTH - 18) / 2, 100, buf, COLOR_WHITE, 2);

    if (charging) {
        display_draw_text(60, 155, "CHARGING", COLOR_YELLOW, 1);
    } else {
        snprintf(buf, sizeof(buf), "~%dh left", runtime_hours);
        display_draw_text(50, 155, buf, COLOR_CYAN, 1);
    }
}

void display_show_low_battery(void)
{
    display_fill_screen(COLOR_BLACK);
    display_draw_text(30, 100, "LOW BATTERY", COLOR_RED, 2);
    display_draw_text(50, 130, "Please charge", COLOR_YELLOW, 1);
}

void display_show_message(const char *line1, const char *line2)
{
    display_fill_screen(COLOR_BLACK);
    int w1 = strlen(line1) * 6;
    display_draw_text((DISP_WIDTH - w1) / 2, 100, line1, COLOR_WHITE, 1);
    int w2 = strlen(line2) * 6;
    display_draw_text((DISP_WIDTH - w2) / 2, 130, line2, COLOR_CYAN, 1);
}

/* ---- End of display_drv.c ---- */