/*
 * display.c — SSD1309 OLED Display Driver
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements a minimal I2C driver for the SSD1309 128×64 monochrome OLED
 * display.  The display shares I2C1 with the BMI270 IMU (same bus).
 *
 * The display shows:
 *  - Boot banner and initialization status
 *  - Survey status (trace count, battery, band, state)
 *  - A-scan preview waveform (real-time reflectivity vs. depth)
 *
 * The SSD1309 uses a page-based memory: 8 pages × 128 columns, each column
 * byte = 8 vertical pixels.  We maintain a local framebuffer (1024 bytes)
 * and flush to the display via I2C when updated.
 */

#include "display.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  SSD1309 commands                                                      */
/* ===================================================================== */

#define SSD1309_CMD_SET_CONTRAST    0x81
#define SSD1309_CMD_DISPLAY_ALLON   0xA5
#define SSD1309_CMD_DISPLAY_NORMAL  0xA4
#define SSD1309_CMD_DISPLAY_OFF     0xAE
#define SSD1309_CMD_DISPLAY_ON      0xAF
#define SSD1309_CMD_SET_PAGE        0xB0  /* + page number (0-7) */
#define SSD1309_CMD_SET_COL_LOW     0x00  /* + low nibble */
#define SSD1309_CMD_SET_COL_HIGH    0x10  /* + high nibble */
#define SSD1309_CMD_SET_SEG_REMAP   0xA1
#define SSD1309_CMD_SET_COM_REMAP   0xC8
#define SSD1309_CMD_SET_START_LINE  0x40
#define SSD1309_CMD_SET_MULTIPLEX   0xA8

/* ===================================================================== */
/*  Framebuffer                                                           */
/* ===================================================================== */

static uint8_t fb[8][128];   /* 8 pages × 128 columns */
static uint8_t fb_dirty = 0;

/* ===================================================================== */
/*  I2C write helpers (reuse IMU's I2C bus — I2C1)                        */
/* ===================================================================== */

static void i2c_delay(void)
{
    for (volatile int d = 0; d < 10; d++) { __asm volatile("nop"); }
}

static int i2c_wait_flag(uint32_t mask)
{
    uint32_t timeout = 10000;
    while ((I2C1->ISR & mask) == 0 && timeout) { timeout--; i2c_delay(); }
    return (timeout > 0) ? 0 : -1;
}

static int oled_write_cmd(uint8_t cmd)
{
    /* Control byte: 0x80 = Co=1, D/C=0 (command) */
    I2C1->CR2 = ((uint32_t)OLED_I2C_ADDR << 1) | (2U << 16) | (0U << 10);
    I2C1->CR2 |= I2C_CR2_START;
    if (i2c_wait_flag(I2C_ISR_TXIS)) return -1;
    I2C1->TXDR = 0x80;  /* control byte: command follows */
    if (i2c_wait_flag(I2C_ISR_TXIS)) return -1;
    I2C1->TXDR = cmd;
    if (i2c_wait_flag(I2C_ISR_TC)) return -1;
    I2C1->CR2 |= I2C_CR2_STOP;
    return 0;
}

static int oled_write_data(const uint8_t *data, uint8_t len)
{
    /* Control byte: 0x40 = D/C=1 (data), Co=0 (stream) */
    I2C1->CR2 = ((uint32_t)OLED_I2C_ADDR << 1) | ((uint32_t)(len + 1) << 16) | (0U << 10);
    I2C1->CR2 |= I2C_CR2_START;
    if (i2c_wait_flag(I2C_ISR_TXIS)) return -1;
    I2C1->TXDR = 0x40;  /* data stream */
    for (uint8_t i = 0; i < len; i++) {
        if (i2c_wait_flag(I2C_ISR_TXIS)) return -1;
        I2C1->TXDR = data[i];
    }
    if (i2c_wait_flag(I2C_ISR_TC)) return -1;
    I2C1->CR2 |= I2C_CR2_STOP;
    return 0;
}

/* ===================================================================== */
/*  5×8 font (printable ASCII subset)                                     */
/* ===================================================================== */

/* Compact 5x7 font — each char is 5 bytes (columns), 7 rows packed in LSB.
 * Only digits, uppercase letters, space, dash, period, colon.
 * This is a subset; full font stored in flash (omitted for brevity but
 * the driver provides char rendering via bitmap lookup).
 */
static const uint8_t font5x7[][5] = {
    /* ' ' = 0x20 */ {0x00, 0x00, 0x00, 0x00, 0x00},
    /* '!' = 0x21 */ {0x00, 0x00, 0x5F, 0x00, 0x00},
    /* '-' = 0x2D */ {0x00, 0x00, 0x08, 0x00, 0x00},
    /* '.' = 0x2E */ {0x00, 0x60, 0x60, 0x00, 0x00},
    /* '/' = 0x2F */ {0x20, 0x10, 0x08, 0x04, 0x02},
    /* '0' = 0x30 */ {0x3E, 0x51, 0x49, 0x45, 0x3E},
    /* '1' = 0x31 */ {0x00, 0x42, 0x7F, 0x40, 0x00},
    /* '2' = 0x32 */ {0x42, 0x61, 0x51, 0x49, 0x46},
    /* '3' = 0x33 */ {0x21, 0x41, 0x45, 0x4B, 0x31},
    /* '4' = 0x34 */ {0x18, 0x14, 0x12, 0x7F, 0x10},
    /* '5' = 0x35 */ {0x27, 0x45, 0x45, 0x45, 0x39},
    /* '6' = 0x36 */ {0x3C, 0x4A, 0x49, 0x49, 0x30},
    /* '7' = 0x37 */ {0x01, 0x71, 0x09, 0x05, 0x03},
    /* '8' = 0x38 */ {0x36, 0x49, 0x49, 0x49, 0x36},
    /* '9' = 0x39 */ {0x06, 0x49, 0x49, 0x29, 0x1E},
    /* ':' = 0x3A */ {0x00, 0x36, 0x36, 0x00, 0x00},
    /* 'A' = 0x41 */ {0x7E, 0x11, 0x11, 0x11, 0x7E},
    /* 'B' = 0x42 */ {0x7F, 0x49, 0x49, 0x49, 0x36},
    /* 'C' = 0x43 */ {0x3E, 0x41, 0x41, 0x41, 0x22},
    /* 'D' = 0x44 */ {0x7F, 0x41, 0x41, 0x22, 0x1C},
    /* 'E' = 0x45 */ {0x7F, 0x49, 0x49, 0x49, 0x41},
    /* 'F' = 0x46 */ {0x7F, 0x09, 0x09, 0x09, 0x01},
    /* 'G' = 0x47 */ {0x3E, 0x41, 0x49, 0x49, 0x7A},
    /* 'H' = 0x48 */ {0x7F, 0x08, 0x08, 0x08, 0x7F},
    /* 'I' = 0x49 */ {0x00, 0x41, 0x7F, 0x41, 0x00},
    /* 'J' = 0x4A */ {0x20, 0x40, 0x41, 0x3F, 0x01},
    /* 'K' = 0x4B */ {0x7F, 0x08, 0x14, 0x22, 0x41},
    /* 'L' = 0x4C */ {0x7F, 0x40, 0x40, 0x40, 0x40},
    /* 'M' = 0x4D */ {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    /* 'N' = 0x4E */ {0x7F, 0x04, 0x08, 0x10, 0x7F},
    /* 'O' = 0x4F */ {0x3E, 0x41, 0x41, 0x41, 0x3E},
    /* 'P' = 0x50 */ {0x7F, 0x09, 0x09, 0x09, 0x06},
    /* 'R' = 0x52 */ {0x7F, 0x09, 0x19, 0x29, 0x46},
    /* 'S' = 0x53 */ {0x46, 0x49, 0x49, 0x49, 0x31},
    /* 'T' = 0x54 */ {0x01, 0x41, 0x7F, 0x01, 0x00},
    /* 'U' = 0x55 */ {0x3F, 0x40, 0x40, 0x40, 0x3F},
    /* 'V' = 0x56 */ {0x1F, 0x20, 0x40, 0x20, 0x1F},
    /* 'W' = 0x57 */ {0x3F, 0x40, 0x38, 0x40, 0x3F},
    /* 'X' = 0x58 */ {0x63, 0x14, 0x08, 0x14, 0x63},
    /* 'Y' = 0x59 */ {0x07, 0x08, 0x70, 0x08, 0x07},
    /* 'Z' = 0x5A */ {0x61, 0x51, 0x49, 0x45, 0x43},
    /* 'a' = 0x61 */ {0x20, 0x54, 0x54, 0x54, 0x78},
    /* 'b' = 0x62 */ {0x7F, 0x48, 0x44, 0x44, 0x38},
    /* 'c' = 0x63 */ {0x38, 0x44, 0x44, 0x44, 0x20},
    /* 'd' = 0x64 */ {0x38, 0x44, 0x44, 0x48, 0x7F},
    /* 'e' = 0x65 */ {0x38, 0x54, 0x54, 0x54, 0x18},
    /* 'f' = 0x66 */ {0x08, 0x7E, 0x09, 0x01, 0x02},
    /* 'g' = 0x67 */ {0x0C, 0x52, 0x52, 0x52, 0x3E},
    /* 'h' = 0x68 */ {0x7F, 0x08, 0x04, 0x04, 0x78},
    /* 'i' = 0x69 */ {0x00, 0x44, 0x7D, 0x40, 0x00},
    /* 'j' = 0x6A */ {0x20, 0x40, 0x44, 0x3D, 0x00},
    /* 'k' = 0x6B */ {0x7F, 0x10, 0x28, 0x44, 0x00},
    /* 'l' = 0x6C */ {0x00, 0x41, 0x7F, 0x40, 0x00},
    /* 'm' = 0x6D */ {0x7C, 0x04, 0x18, 0x04, 0x78},
    /* 'n' = 0x6E */ {0x7C, 0x08, 0x04, 0x04, 0x78},
    /* 'o' = 0x6F */ {0x38, 0x44, 0x44, 0x44, 0x38},
    /* 'p' = 0x70 */ {0x7C, 0x14, 0x14, 0x14, 0x08},
    /* 'r' = 0x72 */ {0x7C, 0x08, 0x04, 0x04, 0x08},
    /* 's' = 0x73 */ {0x48, 0x54, 0x54, 0x54, 0x20},
    /* 't' = 0x74 */ {0x04, 0x3F, 0x44, 0x40, 0x20},
    /* 'u' = 0x75 */ {0x3C, 0x40, 0x40, 0x20, 0x7C},
    /* 'v' = 0x76 */ {0x1C, 0x20, 0x40, 0x20, 0x1C},
    /* 'w' = 0x77 */ {0x3C, 0x40, 0x30, 0x40, 0x3C},
    /* 'x' = 0x78 */ {0x44, 0x28, 0x10, 0x28, 0x44},
    /* 'y' = 0x79 */ {0x0C, 0x50, 0x50, 0x50, 0x3C},
    /* 'z' = 0x7A */ {0x44, 0x64, 0x54, 0x4C, 0x44},
};

static const uint8_t *get_font_col(char c)
{
    if (c >= 'a' && c <= 'z')
        return font5x7[c - 'a' + 27];  /* lowercase starts at index 27 */
    if (c >= 'A' && c <= 'Z')
        return font5x7[c - 'A' + 11];  /* uppercase starts at index 11 */
    if (c >= '0' && c <= '9')
        return font5x7[c - '0' + 5];   /* digits at 5-14 */
    switch (c) {
    case ' ': return font5x7[0];
    case '!': return font5x7[1];
    case '-': return font5x7[2];
    case '.': return font5x7[3];
    case '/': return font5x7[4];
    case ':': return font5x7[15];
    }
    return font5x7[0];  /* default: space */
}

/* ===================================================================== */
/*  Display flush (write framebuffer to OLED)                             */
/* ===================================================================== */

static void display_flush(void)
{
    for (uint8_t page = 0; page < 8; page++) {
        oled_write_cmd(SSD1309_CMD_SET_PAGE | page);
        oled_write_cmd(SSD1309_CMD_SET_COL_LOW | 0);
        oled_write_cmd(SSD1309_CMD_SET_COL_HIGH | 0);
        oled_write_data(fb[page], 128);
    }
    fb_dirty = 0;
}

/* ===================================================================== */
/*  Pixel manipulation                                                    */
/* ===================================================================== */

static void set_pixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= 128 || y >= 64) return;
    uint8_t page = y / 8;
    uint8_t bit  = 1 << (y % 8);
    if (on)
        fb[page][x] |= bit;
    else
        fb[page][x] &= ~bit;
}

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

int display_init(void)
{
    /* Initialize SSD1309 — sequence of configuration commands */
    board_delay_ms(50);  /* display power-up settle */

    oled_write_cmd(SSD1309_CMD_DISPLAY_OFF);
    oled_write_cmd(SSD1309_CMD_SET_MULTIPLEX); oled_write_cmd(0x3F);
    oled_write_cmd(SSD1309_CMD_SET_START_LINE | 0);
    oled_write_cmd(SSD1309_CMD_SET_SEG_REMAP);    /* A1: column remap */
    oled_write_cmd(SSD1309_CMD_SET_COM_REMAP);    /* C8: COM scan remap */
    oled_write_cmd(0xDA); oled_write_cmd(0x12);  /* COM pin config */
    oled_write_cmd(0x20); oled_write_cmd(0x00);  /* horizontal addressing */
    oled_write_cmd(0x81); oled_write_cmd(0x80);  /* contrast */
    oled_write_cmd(0xD9); oled_write_cmd(0xF1);  /* precharge period */
    oled_write_cmd(0xDB); oled_write_cmd(0x40);  /* VCOM deselect */
    oled_write_cmd(0xA4);  /* display follows RAM */
    oled_write_cmd(0xA6);  /* normal display (not inverted) */
    oled_write_cmd(SSD1309_CMD_DISPLAY_ON);

    display_clear();
    return 0;
}

void display_clear(void)
{
    memset(fb, 0, sizeof(fb));
    display_flush();
}

void display_draw_text(uint8_t x, uint8_t y, const char *text)
{
    uint8_t col = x;
    while (*text && col < 123) {
        const uint8_t *glyph = get_font_col(*text);
        for (uint8_t i = 0; i < 5; i++) {
            uint8_t col_data = glyph[i];
            for (uint8_t row = 0; row < 7; row++) {
                set_pixel(col + i, y + row, (col_data >> row) & 1);
            }
        }
        col += 6;  /* 5 px + 1 px spacing */
        text++;
    }
    if (fb_dirty) display_flush();
    fb_dirty = 1;
    display_flush();
}

void display_draw_ascan_preview(const float *trace, uint16_t n)
{
    /* Draw a mini A-scan waveform in the lower half of the display (rows 32-63) */
    /* Clear the preview area first */
    for (uint8_t x = 0; x < 128; x++) {
        for (uint8_t y = 32; y < 64; y++) {
            set_pixel(x, y, 0);
        }
    }

    /* Find max amplitude for scaling */
    float max_val = 0.001f;
    for (uint16_t i = 0; i < n && i < 128; i++) {
        float v = trace[i];
        if (v < 0) v = -v;
        if (v > max_val) max_val = v;
    }

    /* Draw waveform — each column = one depth bin, height = amplitude */
    for (uint16_t i = 0; i < n && i < 128; i++) {
        float v = trace[i];
        if (v < 0) v = -v;
        uint8_t height = (uint8_t)((v / max_val) * 28.0f);
        uint8_t base_y = 60;
        for (uint8_t h = 0; h < height; h++) {
            if (base_y - h >= 32)
                set_pixel((uint8_t)i, base_y - h, 1);
        }
    }

    /* Draw baseline */
    for (uint8_t x = 0; x < 128; x++) {
        set_pixel(x, 60, 1);
    }

    fb_dirty = 1;
    display_flush();
}

void display_draw_status(uint32_t traces, uint8_t battery,
                          uint8_t band, sys_state_t state)
{
    /* Clear top portion */
    for (uint8_t x = 0; x < 128; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            set_pixel(x, y, 0);
        }
    }

    char buf[21];

    /* State indicator */
    const char *state_str;
    switch (state) {
    case STATE_BOOT:     state_str = "BOOT"; break;
    case STATE_IDLE:     state_str = "IDLE"; break;
    case STATE_CALIBRATE: state_str = "CAL "; break;
    case STATE_SURVEY:   state_str = "SURV"; break;
    case STATE_PAUSE:    state_str = "PAUS"; break;
    case STATE_SHUTDOWN: state_str = "OFF "; break;
    default:             state_str = "?"; break;
    }

    /* Format: "BAND:MED  TR:1234" */
    const char *band_names[] = {"DEEP", "LO  ", "MED ", "HI  ", "UHI "};
    snprintf_buf: {};
    /* Simple manual formatting (no snprintf in freestanding) */
    display_draw_text(0, 0, state_str);
    display_draw_text(30, 0, band_names[band < 5 ? band : 2]);

    /* Trace count (manual int to string) */
    uint32_t t = traces;
    uint8_t idx = 0;
    if (t == 0) buf[idx++] = '0';
    while (t > 0 && idx < 10) {
        buf[idx++] = '0' + (t % 10);
        t /= 10;
    }
    buf[idx] = '\0';
    /* Reverse the digits */
    for (uint8_t i = 0; i < idx / 2; i++) {
        char tmp = buf[i]; buf[i] = buf[idx - 1 - i]; buf[idx - 1 - i] = tmp;
    }
    display_draw_text(65, 0, "TR:");
    display_draw_text(80, 0, buf);

    /* Battery (manual format) */
    display_draw_text(0, 10, "BAT:");
    uint8_t b = battery;
    idx = 0;
    if (b == 0) buf[idx++] = '0';
    while (b > 0 && idx < 4) {
        buf[idx++] = '0' + (b % 10);
        b /= 10;
    }
    buf[idx] = '\0';
    for (uint8_t i = 0; i < idx / 2; i++) {
        char tmp = buf[i]; buf[i] = buf[idx - 1 - i]; buf[idx - 1 - i] = tmp;
    }
    display_draw_text(30, 10, buf);
    display_draw_text(50, 10, "%");

    fb_dirty = 1;
}

/* End of display.c */