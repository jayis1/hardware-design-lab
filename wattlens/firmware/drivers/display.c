/*
 * display.c — ILI9341 TFT driver and WattLens UI
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Drives a 2.4″ 320×240 ILI9341 TFT over SPI3.  The display shows:
 *   - Real-time power, PF, THD gauges
 *   - Per-phase voltage and current bars
 *   - NILM appliance states
 *   - Battery level and status
 */

#include "display.h"
#include "registers.h"
#include "nilm.h"
#include <string.h>

#define SPI3_CR1   SPI_REG(SPI3_BASE, SPI_CR1)
#define SPI3_CFG1  SPI_REG(SPI3_BASE, SPI_CFG1)
#define SPI3_CFG2  SPI_REG(SPI3_BASE, SPI_CFG2)
#define SPI3_SR    SPI_REG(SPI3_BASE, SPI_SR)
#define SPI3_IFCR  SPI_REG(SPI3_BASE, SPI_IFCR)
#define SPI3_TXDR  (*(volatile uint8_t *)(SPI3_BASE + SPI_TXDR))

#define DISP_CS_HIGH() (GPIO_REG(GPIOA_BASE, GPIO_BSRR_OFF) = BIT(31))
#define DISP_CS_LOW()  (GPIO_REG(GPIOA_BASE, GPIO_BSRR_OFF) = BIT(15))
#define DISP_DC_DATA() (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(6))
#define DISP_DC_CMD()  (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(22))
#define DISP_RST_HIGH() (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(7))
#define DISP_RST_LOW()  (GPIO_REG(GPIOC_BASE, GPIO_BSRR_OFF) = BIT(23))

#define SCREEN_W    320
#define SCREEN_H    240

/* Colors (RGB565) */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_GRAY    0x8410
#define COLOR_ORANGE  0xFD20
#define COLOR_DKBLUE  0x000F

/* ILI9341 commands */
#define ILI_CMD_RESET        0x01
#define ILI_CMD_SLEEP_OUT    0x11
#define ILI_CMD_GAMMA        0x26
#define ILI_CMD_PIXEL_FMT    0x3A
#define ILI_CMD_MEMORY_ACC   0x36
#define ILI_CMD_COL_ADDR_SET 0x2A
#define ILI_CMD_ROW_ADDR_SET 0x2B
#define ILI_CMD_MEM_WRITE    0x2C
#define ILI_CMD_DISPLAY_ON   0x29

static uint16_t fb[SCREEN_W * SCREEN_H];  /* frame buffer in SRAM */

/* ========================================================================
 * Low-level SPI3 transfer
 * ======================================================================== */
static void spi3_write_cmd(uint8_t cmd) {
    DISP_DC_CMD();
    DISP_CS_LOW();
    SPI3_TXDR = cmd;
    while (SPI3_SR & BIT(13)) { }  /* wait RXFIFO drained */
    SPI3_IFCR = 0xFFFFFFFF;
    DISP_CS_HIGH();
}

static void spi3_write_data(uint8_t *data, int len) {
    DISP_DC_DATA();
    DISP_CS_LOW();
    for (int i = 0; i < len; i++) {
        SPI3_TXDR = data[i];
        while (!(SPI3_SR & BIT(1))) { }  /* wait TXP */
    }
    while (SPI3_SR & BIT(13)) { }
    SPI3_IFCR = 0xFFFFFFFF;
    DISP_CS_HIGH();
}

static void spi3_write_data16(uint16_t *data, int len) {
    uint8_t buf[2];
    DISP_DC_DATA();
    DISP_CS_LOW();
    for (int i = 0; i < len; i++) {
        buf[0] = (data[i] >> 8) & 0xFF;
        buf[1] = data[i] & 0xFF;
        SPI3_TXDR = buf[0];
        while (!(SPI3_SR & BIT(1))) { }
        SPI3_TXDR = buf[1];
        while (!(SPI3_SR & BIT(1))) { }
    }
    while (SPI3_SR & BIT(13)) { }
    SPI3_IFCR = 0xFFFFFFFF;
    DISP_CS_HIGH();
}

/* ========================================================================
 * Set address window for pixel writes
 * ======================================================================== */
static void set_addr_window(int x0, int y0, int x1, int y1) {
    uint8_t xs[4] = {(x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF};
    uint8_t ys[4] = {(y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF};
    spi3_write_cmd(ILI_CMD_COL_ADDR_SET);
    spi3_write_data(xs, 4);
    spi3_write_cmd(ILI_CMD_ROW_ADDR_SET);
    spi3_write_data(ys, 4);
    spi3_write_cmd(ILI_CMD_MEM_WRITE);
}

/* ========================================================================
 * Initialize ILI9341 display
 * ======================================================================== */
void display_init(void) {
    /* Configure SPI3: master, 30 MHz, 8-bit */
    SPI3_CR1 = 0;
    SPI3_CFG2 = (1 << 2);  /* MASTER */
    SPI3_CFG1 = (3 << 0)   /* MBR = div 4 → 30 MHz */
              | (7 << 5);  /* FTHLV */
    SPI3_CR1 |= BIT(0);    /* SPE */

    /* Hardware reset */
    DISP_RST_LOW();
    for (volatile int i = 0; i < 100000; i++) { }
    DISP_RST_HIGH();
    for (volatile int i = 0; i < 100000; i++) { }

    /* Initialization sequence */
    spi3_write_cmd(ILI_CMD_RESET);
    for (volatile int i = 0; i < 100000; i++) { }

    spi3_write_cmd(ILI_CMD_SLEEP_OUT);
    for (volatile int i = 0; i < 100000; i++) { }

    uint8_t fmt = 0x55;  /* 16-bit pixel format */
    spi3_write_cmd(ILI_CMD_PIXEL_FMT);
    spi3_write_data(&fmt, 1);

    uint8_t madctl = 0x28;  /* MX | BGR (landscape) */
    spi3_write_cmd(ILI_CMD_MEMORY_ACC);
    spi3_write_data(&madctl, 1);

    spi3_write_cmd(ILI_CMD_DISPLAY_ON);

    display_clear(COLOR_DKBLUE);
}

/* ========================================================================
 * Clear screen
 * ======================================================================== */
void display_clear(uint16_t color) {
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
        fb[i] = color;
    }
    set_addr_window(0, 0, SCREEN_W - 1, SCREEN_H - 1);
    spi3_write_data16(fb, SCREEN_W * SCREEN_H);
}

/* ========================================================================
 * Draw text (simple 8×8 font — stub for production font table)
 * ======================================================================== */
void display_text(int x, int y, const char *str, uint16_t fg, uint16_t bg, int size) {
    (void)fg; (void)bg; (void)size;
    /* Simplified: write string to frame buffer region
     * In production, use a bitmap font table and render each glyph. */
    int len = strlen(str);
    if (len > 40) len = 40;

    /* Fill background */
    for (int py = y; py < y + 8 * size && py < SCREEN_H; py++) {
        for (int px = x; px < x + len * 6 * size && px < SCREEN_W; px++) {
            fb[py * SCREEN_W + px] = bg;
        }
    }
    /* Would render font glyphs here */
    set_addr_window(x, y, x + len * 6 * size - 1, y + 8 * size - 1);
    spi3_write_data16(&fb[y * SCREEN_W + x], len * 6 * size * 8 * size);
}

/* ========================================================================
 * Draw a filled rectangle bar
 * ======================================================================== */
void display_draw_bar(int x, int y, int w, int h, float frac, uint16_t color) {
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;

    /* Background (gray) */
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            fb[py * SCREEN_W + px] = COLOR_GRAY;
        }
    }
    /* Foreground (filled portion) */
    int fill_w = (int)(w * frac);
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + fill_w; px++) {
            fb[py * SCREEN_W + px] = color;
        }
    }
    set_addr_window(x, y, x + w - 1, y + h - 1);
    spi3_write_data16(&fb[y * SCREEN_W + x], w * h);
}

/* ========================================================================
 * Draw a gauge (value with label)
 * ======================================================================== */
void display_draw_gauge(int x, int y, int w, int h, float value,
                        float min, float max, const char *label) {
    float frac = (value - min) / (max - min);
    uint16_t color = COLOR_GREEN;
    if (frac > 0.8f) color = COLOR_RED;
    else if (frac > 0.6f) color = COLOR_YELLOW;
    display_draw_bar(x, y, w, h, frac, color);
    display_text(x, y + h + 2, label, COLOR_WHITE, COLOR_DKBLUE, 1);
}

/* ========================================================================
 * Show boot screen
 * ======================================================================== */
void display_show_boot(const char *msg) {
    display_clear(COLOR_DKBLUE);
    display_text(60, 100, "WattLens", COLOR_WHITE, COLOR_DKBLUE, 2);
    display_text(40, 120, "by jayis1", COLOR_CYAN, COLOR_DKBLUE, 1);
    display_text(40, 150, msg, COLOR_GREEN, COLOR_DKBLUE, 1);
}

/* ========================================================================
 * Show main measurement screen
 * ======================================================================== */
void display_show_main(void) {
    display_clear(COLOR_DKBLUE);
    display_text(4, 2, "WattLens", COLOR_WHITE, COLOR_DKBLUE, 1);
    display_text(250, 2, "BAT", COLOR_CYAN, COLOR_DKBLUE, 1);
}

/* ========================================================================
 * Update display with real-time metrics
 * ======================================================================== */
void display_update_metrics(const power_metrics_t *m, const nilm_result_t *n, uint8_t battery) {
    char buf[24];

    /* Header: battery */
    display_draw_bar(280, 4, 30, 8, (float)battery / 100.0f, COLOR_GREEN);

    /* Total power gauge */
    display_draw_gauge(4, 16, 100, 12, m->p_total_real, 0, 10000, "Power (W)");

    /* Power factor gauge */
    display_draw_gauge(4, 40, 100, 12, m->pf_total, 0, 1, "PF");

    /* THD gauge */
    display_draw_gauge(4, 64, 100, 12,
                       (m->thd_v[0] + m->thd_v[1] + m->thd_v[2]) / 3.0f, 0, 20, "THD-V %");

    /* Per-phase voltage bars */
    display_text(120, 16, "V1", COLOR_CYAN, COLOR_DKBLUE, 1);
    display_draw_bar(140, 16, 80, 10, m->v_rms[0] / 300.0f, COLOR_GREEN);
    display_text(120, 30, "V2", COLOR_CYAN, COLOR_DKBLUE, 1);
    display_draw_bar(140, 30, 80, 10, m->v_rms[1] / 300.0f, COLOR_GREEN);
    display_text(120, 44, "V3", COLOR_CYAN, COLOR_DKBLUE, 1);
    display_draw_bar(140, 44, 80, 10, m->v_rms[2] / 300.0f, COLOR_GREEN);

    /* Per-phase current bars */
    display_text(230, 16, "I1", COLOR_CYAN, COLOR_DKBLUE, 1);
    display_draw_bar(250, 16, 60, 10, m->i_rms[0] / 50.0f, COLOR_ORANGE);
    display_text(230, 30, "I2", COLOR_CYAN, COLOR_DKBLUE, 1);
    display_draw_bar(250, 30, 60, 10, m->i_rms[1] / 50.0f, COLOR_ORANGE);
    display_text(230, 44, "I3", COLOR_CYAN, COLOR_DKBLUE, 1);
    display_draw_bar(250, 44, 60, 10, m->i_rms[2] / 50.0f, COLOR_ORANGE);

    /* NILM appliance states */
    display_text(4, 90, "Loads (NILM):", COLOR_YELLOW, COLOR_DKBLUE, 1);
    int y = 104;
    int shown = 0;
    for (int c = 0; c < NILM_MAX_CLASSES && shown < 8; c++) {
        if (n->nilm_state[c]) {
            char name[24];
            nilm_get_appliance_name(c, name, sizeof(name));
            /* Display: "  [ON] ApplianceName  XXXX W" */
            display_text(8, y, ">", COLOR_GREEN, COLOR_DKBLUE, 1);
            display_text(18, y, name, COLOR_WHITE, COLOR_DKBLUE, 1);
            y += 12;
            shown++;
        }
    }
    if (shown == 0) {
        display_text(8, y, "No loads detected", COLOR_GRAY, COLOR_DKBLUE, 1);
    }

    /* Frequency and totals at bottom */
    display_text(4, 210, "f:", COLOR_CYAN, COLOR_DKBLUE, 1);
    /* Would format freq value here */
    (void)buf;
}

void display_show_error(const char *msg) {
    display_clear(COLOR_RED);
    display_text(10, 100, "ERROR:", COLOR_WHITE, COLOR_RED, 2);
    display_text(10, 130, msg, COLOR_WHITE, COLOR_RED, 1);
}

void display_show_warning(const char *msg) {
    display_text(4, 225, msg, COLOR_YELLOW, COLOR_DKBLUE, 1);
}