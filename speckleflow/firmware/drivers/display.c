/*
 * display.c — ILI9341 2.8" TFT driver for SpeckleFlow
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The ILI9341 is driven over SPI4 at 40 MHz. We use DMA for both
 * command and framebuffer transfers to minimize CPU overhead.
 *
 * The display shows:
 *   - Live perfusion map (320×240, RGB565) from the FPGA flow data
 *   - HUD overlay: ROI box, flow value, battery, laser status, FPS
 *
 * The flow map (640×480, 8-bit K values) is downscaled to 320×240
 * and colorized via a 256-entry RGB565 LUT before being DMA'd to
 * the display.
 */

#include "display.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ILI9341 commands */
#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13
#define ILI9341_DINVOFF 0x20
#define ILI9341_DINVON  0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON  0x29
#define ILI9341_CASET  0x2A
#define ILI9341_PASET  0x2B
#define ILI9341_RAMWR  0x2C
#define ILI9341_RAMRD  0x2E
#define ILI9341_MADCTL 0x36
#define ILI9341_PIXFMT 0x3A
#define ILI9341_FRMCTR1 0xB1
#define ILI9341_DFUNCTR 0xB6
#define ILI9341_PWCTR1 0xC0
#define ILI9341_PWCTR2 0xC1
#define ILI9341_VMCTR1 0xC5
#define ILI9341_VMCTR2 0xC7
#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

/* Colormap LUT: 256 entries × 2 bytes (RGB565) */
static uint16_t colormap_lut[COLORMAP_ENTRIES];

/* Framebuffer: 320×240 × 2 bytes = 153,600 bytes */
static uint16_t framebuffer[DISP_WIDTH * DISP_HEIGHT] __attribute__((aligned(32)));

/* ---- Low-level SPI ------------------------------------------------------ */

static void disp_cs_low(void) {
    DISP_CS_PORT->BSRR = (1u << (DISP_CS_PIN + 16));
}

static void disp_cs_high(void) {
    DISP_CS_PORT->BSRR = (1u << DISP_CS_PIN);
}

static void disp_dc_command(void) {
    DISP_DC_PORT->BSRR = (1u << (DISP_DC_PIN + 16));  /* low = command */
}

static void disp_dc_data(void) {
    DISP_DC_PORT->BSRR = (1u << DISP_DC_PIN);  /* high = data */
}

static void spi4_wait_tx(void) {
    while (!(SPI4->SR & SPI_SR_TXP)) { }
}

static void spi4_wait_eot(void) {
    while (!(SPI4->SR & SPI_SR_EOT)) { }
    SPI4->IFCR = SPI_IFCR_CLEAR;
}

static void disp_write_cmd(uint8_t cmd) {
    disp_cs_low();
    disp_dc_command();
    spi4_wait_tx();
    *(volatile uint8_t *)&SPI4->TXDR = cmd;
    spi4_wait_eot();
    disp_cs_high();
}

static void disp_write_data(uint8_t data) {
    disp_cs_low();
    disp_dc_data();
    spi4_wait_tx();
    *(volatile uint8_t *)&SPI4->TXDR = data;
    spi4_wait_eot();
    disp_cs_high();
}

static void disp_write_cmd_data(uint8_t cmd, const uint8_t *data, uint32_t len) {
    disp_cs_low();
    disp_dc_command();
    spi4_wait_tx();
    *(volatile uint8_t *)&SPI4->TXDR = cmd;
    spi4_wait_eot();
    disp_dc_data();
    for (uint32_t i = 0; i < len; i++) {
        spi4_wait_tx();
        *(volatile uint8_t *)&SPI4->TXDR = data[i];
    }
    spi4_wait_eot();
    disp_cs_high();
}

/* ---- Colormap generation ------------------------------------------------ */

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r >> 3) << 11) |
           ((uint16_t)(g >> 2) << 5)  |
           ((uint16_t)(b >> 3));
}

static void colormap_jet(void) {
    /* Classic "jet" colormap: blue → cyan → green → yellow → red */
    for (int i = 0; i < 256; i++) {
        float t = (float)i / 255.0f;
        float r = 1.5f - (4.0f * (t - 0.75f) * (t - 0.75f));
        float g = 1.0f - (4.0f * (t - 0.5f) * (t - 0.5f));
        float b = 1.5f - (4.0f * (t - 0.25f) * (t - 0.25f));

        if (r < 0) r = 0; if (r > 1) r = 1;
        if (g < 0) g = 0; if (g > 1) g = 1;
        if (b < 0) b = 0; if (b > 1) b = 1;

        colormap_lut[i] = rgb565((uint8_t)(r * 255),
                                  (uint8_t)(g * 255),
                                  (uint8_t)(b * 255));
    }
}

static void colormap_thermal(void) {
    /* Black → red → yellow → white */
    for (int i = 0; i < 256; i++) {
        uint8_t r, g, b;
        if (i < 64) {
            r = (uint8_t)(i * 4); g = 0; b = 0;
        } else if (i < 128) {
            r = 255; g = (uint8_t)((i - 64) * 4); b = 0;
        } else if (i < 192) {
            r = 255; g = 255; b = (uint8_t)((i - 128) * 4);
        } else {
            r = 255; g = 255; b = 255;
        }
        colormap_lut[i] = rgb565(r, g, b);
    }
}

static void colormap_grayscale(void) {
    for (int i = 0; i < 256; i++) {
        colormap_lut[i] = rgb565((uint8_t)i, (uint8_t)i, (uint8_t)i);
    }
}

static void colormap_viridis(void) {
    /* Simplified viridis: purple → teal → yellow */
    for (int i = 0; i < 256; i++) {
        float t = (float)i / 255.0f;
        float r = t * t * 0.9f + 0.07f;
        float g = t * 0.5f + 0.1f;
        float b = 0.5f - t * 0.3f + 0.4f;
        if (r > 1) r = 1; if (g > 1) g = 1; if (b > 1) b = 1;
        colormap_lut[i] = rgb565((uint8_t)(r * 255),
                                  (uint8_t)(g * 255),
                                  (uint8_t)(b * 255));
    }
}

static void colormap_inferno(void) {
    /* Black → purple → orange → yellow */
    for (int i = 0; i < 256; i++) {
        float t = (float)i / 255.0f;
        float r = t * t;
        float g = t * t * t * 0.8f;
        float b = t * 0.4f * (1.0f - t * 2.0f);
        if (r > 1) r = 1; if (g > 1) g = 1; if (b > 1) b = 1;
        if (b < 0) b = 0;
        colormap_lut[i] = rgb565((uint8_t)(r * 255),
                                  (uint8_t)(g * 255),
                                  (uint8_t)(b * 255));
    }
}

/* ---- Public API --------------------------------------------------------- */

int display_init(void) {
    /* Hardware reset */
    DISP_RST_PORT->BSRR = (1u << (DISP_RST_PIN + 16));  /* low */
    for (volatile int i = 0; i < 100000; i++) { }
    DISP_RST_PORT->BSRR = (1u << DISP_RST_PIN);          /* high */
    for (volatile int i = 0; i < 500000; i++) { }

    /* Sleep out */
    disp_write_cmd(ILI9341_SLPOUT);
    for (volatile int i = 0; i < 100000; i++) { }

    /* Pixel format: 16-bit RGB565 */
    disp_write_cmd(ILI9341_PIXFMT);
    disp_write_data(0x55);

    /* MADCTL: MX=0, MY=0, MV=0, BGR=1 (for correct colors) */
    disp_write_cmd(ILI9341_MADCTL);
    disp_write_data(0x08);  /* BGR bit set */

    /* Display inversion on (better colors on this panel) */
    disp_write_cmd(ILI9341_DINVON);

    /* Power control */
    disp_write_cmd(ILI9341_PWCTR1);
    disp_write_data(0x23);
    disp_write_cmd(ILI9341_PWCTR2);
    disp_write_data(0x10);

    /* VCOM control */
    disp_write_cmd(ILI9341_VMCTR1);
    disp_write_data(0x2B);
    disp_write_data(0x2B);

    /* Frame rate: 60 Hz */
    disp_write_cmd(ILI9341_FRMCTR1);
    disp_write_data(0x00);
    disp_write_data(0x1B);

    /* Display on */
    disp_write_cmd(ILI9341_DISPON);

    /* Backlight on */
    DISP_BL_PORT->BSRR = (1u << DISP_BL_PIN);

    /* Default colormap: jet */
    display_set_colormap(CMAP_JET);

    /* Clear framebuffer to black */
    memset(framebuffer, 0, sizeof(framebuffer));
    display_present();

    return 0;
}

void display_set_colormap(enum colormap_id id) {
    switch (id) {
        case CMAP_JET:       colormap_jet();       break;
        case CMAP_THERMAL:   colormap_thermal();   break;
        case CMAP_GRAYSCALE: colormap_grayscale(); break;
        case CMAP_VIRIDIS:   colormap_viridis();   break;
        case CMAP_INFERNO:   colormap_inferno();   break;
        default:             colormap_jet();       break;
    }
}

void display_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint8_t data[4];
    disp_write_cmd(ILI9341_CASET);
    data[0] = (uint8_t)(x >> 8);
    data[1] = (uint8_t)(x & 0xFF);
    data[2] = (uint8_t)((x + w - 1) >> 8);
    data[3] = (uint8_t)((x + w - 1) & 0xFF);
    disp_write_cmd_data(ILI9341_CASET, data, 4);
    data[0] = (uint8_t)(y >> 8);
    data[1] = (uint8_t)(y & 0xFF);
    data[2] = (uint8_t)((y + h - 1) >> 8);
    data[3] = (uint8_t)((y + h - 1) & 0xFF);
    disp_write_cmd_data(ILI9341_PASET, data, 4);
    disp_write_cmd(ILI9341_RAMWR);
}

void display_present(void) {
    /* Set full-screen window and write framebuffer via SPI DMA */
    display_set_window(0, 0, DISP_WIDTH, DISP_HEIGHT);

    disp_cs_low();
    disp_dc_data();

    /* Configure DMA2 Stream1 for SPI4 TX (memory-to-peripheral) */
    DMA2->Stream[1].CR = 0;
    DMA2->Stream[1].PAR = (uint32_t)&SPI4->TXDR;
    DMA2->Stream[1].M0AR = (uint32_t)framebuffer;
    DMA2->Stream[1].NDTR = DISP_WIDTH * DISP_HEIGHT;
    DMA2->Stream[1].FCR = 0x05;
    DMA2->Stream[1].CR = DMA_CR_DIR_M2P | DMA_CR_MINC | DMA_CR_PSIZE_16 |
                          DMA_CR_MSIZE_16 | DMA_CR_PL_HIGH |
                          DMA_CR_TCIE | DMA_CR_EN;

    SPI4->CFG1 |= SPI_CFG1_TXDMAEN;

    /* Wait for DMA to complete (blocking for simplicity) */
    while (!(DMA2->Stream[1].CR & DMA_CR_EN) ||
           (DMA2->Stream[1].NDTR > 0)) { }

    SPI4->CFG1 &= ~SPI_CFG1_TXDMAEN;
    spi4_wait_eot();
    disp_cs_high();
}

void display_render_flow(const uint8_t *flow_map, uint16_t src_w,
                         uint16_t src_h) {
    /* Downscale the flow map (typically 640×480) to 320×240 and
     * apply the colormap LUT to produce RGB565 pixels. */
    uint32_t x_ratio = (src_w << 16) / DISP_WIDTH;
    uint32_t y_ratio = (src_h << 16) / DISP_HEIGHT;

    for (uint16_t dy = 0; dy < DISP_HEIGHT; dy++) {
        uint16_t sy = (uint16_t)((dy * y_ratio) >> 16);
        for (uint16_t dx = 0; dx < DISP_WIDTH; dx++) {
            uint16_t sx = (uint16_t)((dx * x_ratio) >> 16);
            uint8_t k = flow_map[sy * src_w + sx];
            framebuffer[dy * DISP_WIDTH + dx] = colormap_lut[k];
        }
    }
}

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= DISP_WIDTH || y >= DISP_HEIGHT) return;
    framebuffer[y * DISP_WIDTH + x] = color;
}

void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color) {
    for (uint16_t i = 0; i < w; i++) {
        display_draw_pixel(x + i, y, color);
        display_draw_pixel(x + i, y + h - 1, color);
    }
    for (uint16_t j = 0; j < h; j++) {
        display_draw_pixel(x, y + j, color);
        display_draw_pixel(x + w - 1, y + j, color);
    }
}

void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                       uint16_t color) {
    for (uint16_t j = 0; j < h; j++) {
        for (uint16_t i = 0; i < w; i++) {
            display_draw_pixel(x + i, y + j, color);
        }
    }
}

void display_draw_text(uint16_t x, uint16_t y, const char *text,
                       uint16_t fg, uint16_t bg) {
    /* Simple 8×8 font renderer. We draw a minimal monospace font
     * by rendering each character as a bitmap from a lookup table.
     * For brevity, this implementation draws a bar for each character. */
    (void)bg;
    uint16_t cx = x;
    for (const char *p = text; *p; p++) {
        if (*p == ' ') { cx += 8; continue; }
        /* Draw a simple glyph outline (placeholder for full font) */
        for (int i = 0; i < 6; i++) {
            display_draw_pixel(cx + i, y, fg);
            display_draw_pixel(cx + i, y + 7, fg);
        }
        for (int j = 0; j < 8; j++) {
            display_draw_pixel(cx, y + j, fg);
            display_draw_pixel(cx + 5, y + j, fg);
        }
        cx += 8;
    }
}

void display_clear(uint16_t color) {
    for (uint32_t i = 0; i < DISP_WIDTH * DISP_HEIGHT; i++) {
        framebuffer[i] = color;
    }
}

uint16_t display_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return rgb565(r, g, b);
}