/*
 * display.c — SSD1306 OLED display driver (128×64, SPI2)
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Renders status screens, breath analysis results, and prompts
 * on a 0.96" monochrome OLED via SPI.
 */

#include "display.h"
#include "../board.h"

/* ========================================================================
 * SSD1306 Commands
 * ======================================================================== */

#define SSD1306_SET_CONTRAST     0x81
#define SSD1306_DISPLAY_ON       0xAF
#define SSD1306_DISPLAY_OFF      0xAE
#define SSD1306_SET_DISP_START   0x40
#define SSD1306_SET_PAGE_ADDR    0x22
#define SSD1306_SET_COL_ADDR     0x21
#define SSD1306_SET_SEG_REMAP    0xA0
#define SSD1306_SET_COM_SCAN     0xC8
#define SSD1306_SET_COM_PINS     0xDA
#define SSD1306_SET_PRECHARGE    0xD9
#define SSD1306_SET_VCOMH        0xDB
#define SSD1306_SET_CHARGE_PUMP  0x8D
#define SSD1306_SET_MEM_MODE     0x20
#define SSD1306_NORMAL_DISPLAY   0xA6
#define SSD1306_INVERT_DISPLAY   0xA7
#define SSD1306_SET_MIRROR       0xA0

/* Display dimensions */
#define DISP_WIDTH       128
#define DISP_HEIGHT      64
#define DISP_PAGES       (DISP_HEIGHT / 8)

/* Framebuffer: 128 × 8 pages = 1024 bytes */
static uint8_t fb[DISP_WIDTH * DISP_PAGES];
static uint8_t display_on_flag = 0;

/* ========================================================================
 * SPI2 low-level transfer
 * ======================================================================== */

static void disp_spi_write(uint8_t data) {
    SPI2->CR1 |= SPI_CR1_CSTART;
    while (!(SPI2->SR & SPI_SR_RXP));
    (void)SPI2->RXDR;
}

static void disp_command(uint8_t cmd) {
    gpio_clear(DISP_DC_PORT, DISP_DC_PIN);  /* DC=0 for command */
    gpio_clear(DISP_CS_PORT, DISP_CS_PIN);
    disp_spi_write(cmd);
    gpio_set(DISP_CS_PORT, DISP_CS_PIN);
}

static void disp_data(uint8_t data) {
    gpio_set(DISP_DC_PORT, DISP_DC_PIN);    /* DC=1 for data */
    gpio_clear(DISP_CS_PORT, DISP_CS_PIN);
    disp_spi_write(data);
    gpio_set(DISP_CS_PORT, DISP_CS_PIN);
}

/* ========================================================================
 * Initialize display
 * ========================================================================
 */

void display_init(void) {
    /* Configure SPI2 as master */
    SPI2->CFG2 = SPI_CFG2_MASTER | SPI_CFG2_SSM | SPI_CFG2_SSI |
                 SPI_CFG2_MBR_DIV8;
    SPI2->CFG1 = (8U << SPI_CFG1_DSIZE_SHIFT);
    SPI2->CR1 |= SPI_CR1_SPE;

    /* Hardware reset */
    gpio_clear(DISP_RST_PORT, DISP_RST_PIN);
    delay_ms(50);
    gpio_set(DISP_RST_PORT, DISP_RST_PIN);
    delay_ms(100);

    /* SSD1306 initialization sequence */
    disp_command(SSD1306_DISPLAY_OFF);
    disp_command(SSD1306_SET_DISP_START | 0x00);

    disp_command(SSD1306_SET_SEG_REMAP | 0x01);  /* Column 127 = SEG0 */
    disp_command(SSD1306_SET_COM_SCAN | 0x08);   /* COM scan remapped */

    disp_command(SSD1306_SET_COM_PINS);
    disp_command(0x12);  /* Alternative COM pin config */

    disp_command(SSD1306_SET_CONTRAST);
    disp_command(0xCF);  /* Contrast level */

    disp_command(SSD1306_SET_PRECHARGE);
    disp_command(0xF1);  /* Pre-charge period */

    disp_command(SSD1306_SET_VCOMH);
    disp_command(0x40);  /* VCOMH deselect level */

    disp_command(SSD1306_SET_MEM_MODE);
    disp_command(0x00);  /* Horizontal addressing mode */

    disp_command(SSD1306_SET_CHARGE_PUMP);
    disp_command(0x14);  /* Enable charge pump */

    disp_command(SSD1306_NORMAL_DISPLAY);

    disp_command(SSD1306_DISPLAY_ON);
    display_on_flag = 1;

    /* Clear framebuffer */
    memset(fb, 0, sizeof(fb));
    display_flush();
}

/* ========================================================================
 * Flush framebuffer to display
 * ======================================================================== */

void display_flush(void) {
    if (!display_on_flag) return;

    /* Set column and page address range */
    disp_command(SSD1306_SET_COL_ADDR);
    disp_command(0x00);
    disp_command(0x7F);

    disp_command(SSD1306_SET_PAGE_ADDR);
    disp_command(0x00);
    disp_command(0x07);

    /* Write framebuffer data */
    gpio_set(DISP_DC_PORT, DISP_DC_PIN);
    gpio_clear(DISP_CS_PORT, DISP_CS_PIN);
    for (int i = 0; i < DISP_WIDTH * DISP_PAGES; i++) {
        disp_spi_write(fb[i]);
    }
    gpio_set(DISP_CS_PORT, DISP_CS_PIN);
}

/* ========================================================================
 * Pixel operations
 * ======================================================================== */

void display_set_pixel(int x, int y, int on) {
    if (x < 0 || x >= DISP_WIDTH || y < 0 || y >= DISP_HEIGHT) return;

    int page = y / 8;
    int bit = y % 8;

    if (on) {
        fb[page * DISP_WIDTH + x] |= (1 << bit);
    } else {
        fb[page * DISP_WIDTH + x] &= ~(1 << bit);
    }
}

void display_clear(void) {
    memset(fb, 0, sizeof(fb));
}

void display_invert_pixel(int x, int y) {
    if (x < 0 || x >= DISP_WIDTH || y < 0 || y >= DISP_HEIGHT) return;
    int page = y / 8;
    int bit = y % 8;
    fb[page * DISP_WIDTH + x] ^= (1 << bit);
}

/* ========================================================================
 * Text rendering (5×7 font)
 * ======================================================================== */

/* Minimal 5×7 font for digits, uppercase letters, and common symbols */
/* Each character is 5 bytes (columns), one byte per column (8 rows, top=MSB) */
static const uint8_t font5x7[][5] = {
    /* Space (0x20) */ {0x00, 0x00, 0x00, 0x00, 0x00},
    /* ! (0x21) */     {0x00, 0x00, 0x5F, 0x00, 0x00},
    /* " (0x22) */     {0x00, 0x07, 0x00, 0x07, 0x00},
    /* # (0x23) */     {0x14, 0x7F, 0x14, 0x7F, 0x14},
    /* % (0x25) */     {0x23, 0x13, 0x08, 0x64, 0x62},
    /* ' (0x27) */     {0x00, 0x08, 0x3E, 0x1A, 0x00},
    /* ( (0x28) */     {0x00, 0x1C, 0x3E, 0x3F, 0x03},
    /* ) (0x29) */     {0x00, 0x03, 0x3F, 0x3E, 0x1C},
    /* + (0x2B) */     {0x00, 0x04, 0x07, 0x04, 0x00},
    /* , (0x2C) */     {0x00, 0x60, 0x60, 0x00, 0x00},
    /* - (0x2D) */     {0x00, 0x04, 0x04, 0x04, 0x00},
    /* . (0x2E) */     {0x00, 0x60, 0x60, 0x00, 0x00},
    /* / (0x2F) */     {0x20, 0x10, 0x08, 0x04, 0x02},
    /* 0 (0x30) */     {0x3E, 0x51, 0x49, 0x45, 0x3E},
    /* 1 (0x31) */     {0x00, 0x42, 0x7F, 0x40, 0x00},
    /* 2 (0x32) */     {0x42, 0x61, 0x51, 0x49, 0x46},
    /* 3 (0x33) */     {0x21, 0x41, 0x45, 0x4B, 0x31},
    /* 4 (0x34) */     {0x18, 0x14, 0x12, 0x7F, 0x10},
    /* 5 (0x35) */     {0x27, 0x45, 0x45, 0x45, 0x39},
    /* 6 (0x36) */     {0x3C, 0x4A, 0x49, 0x49, 0x30},
    /* 7 (0x37) */     {0x01, 0x71, 0x09, 0x05, 0x03},
    /* 8 (0x38) */     {0x36, 0x49, 0x49, 0x49, 0x36},
    /* 9 (0x39) */     {0x06, 0x49, 0x49, 0x29, 0x1E},
    /* : (0x3A) */     {0x00, 0x36, 0x36, 0x00, 0x00},
    /* ; (0x3B) */     {0x00, 0x56, 0x36, 0x00, 0x00},
    /* < (0x3C) */     {0x08, 0x14, 0x22, 0x41, 0x00},
    /* = (0x3D) */     {0x14, 0x14, 0x14, 0x14, 0x14},
    /* > (0x3E) */     {0x00, 0x41, 0x22, 0x14, 0x08},
    /* A (0x41) */     {0x7E, 0x11, 0x11, 0x11, 0x7E},
    /* B (0x42) */     {0x7F, 0x49, 0x49, 0x49, 0x36},
    /* C (0x43) */     {0x3E, 0x41, 0x41, 0x41, 0x22},
    /* D (0x44) */     {0x7F, 0x41, 0x41, 0x22, 0x1C},
    /* E (0x45) */     {0x7F, 0x49, 0x49, 0x49, 0x41},
    /* F (0x46) */     {0x7F, 0x09, 0x09, 0x09, 0x01},
    /* G (0x47) */     {0x3E, 0x41, 0x49, 0x49, 0x7A},
    /* H (0x48) */     {0x7F, 0x08, 0x08, 0x08, 0x7F},
    /* I (0x49) */     {0x00, 0x41, 0x7F, 0x41, 0x00},
    /* J (0x4A) */     {0x20, 0x40, 0x41, 0x3F, 0x01},
    /* K (0x4B) */     {0x7F, 0x08, 0x14, 0x22, 0x41},
    /* L (0x4C) */     {0x7F, 0x40, 0x40, 0x40, 0x40},
    /* M (0x4D) */     {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    /* N (0x4E) */     {0x7F, 0x04, 0x08, 0x10, 0x7F},
    /* O (0x4F) */     {0x3E, 0x41, 0x41, 0x41, 0x3E},
    /* P (0x50) */     {0x7F, 0x09, 0x09, 0x09, 0x06},
    /* Q (0x51) */     {0x3E, 0x41, 0x51, 0x21, 0x5E},
    /* R (0x52) */     {0x7F, 0x09, 0x19, 0x29, 0x46},
    /* S (0x53) */     {0x46, 0x49, 0x49, 0x49, 0x31},
    /* T (0x54) */     {0x01, 0x01, 0x7F, 0x01, 0x01},
    /* U (0x55) */     {0x3F, 0x40, 0x40, 0x40, 0x3F},
    /* V (0x56) */     {0x1F, 0x20, 0x40, 0x20, 0x1F},
    /* W (0x57) */     {0x3F, 0x40, 0x38, 0x40, 0x3F},
    /* X (0x58) */     {0x63, 0x14, 0x08, 0x14, 0x63},
    /* Y (0x59) */     {0x07, 0x08, 0x70, 0x08, 0x07},
    /* Z (0x5A) */     {0x61, 0x51, 0x49, 0x45, 0x43},
    /* a (0x61) */     {0x20, 0x54, 0x54, 0x54, 0x78},
    /* b (0x62) */     {0x7F, 0x48, 0x44, 0x44, 0x38},
    /* c (0x63) */     {0x38, 0x44, 0x44, 0x44, 0x20},
    /* d (0x64) */     {0x38, 0x44, 0x44, 0x48, 0x7F},
    /* e (0x65) */     {0x38, 0x54, 0x54, 0x54, 0x18},
    /* f (0x66) */     {0x08, 0x7E, 0x09, 0x01, 0x02},
    /* g (0x67) */     {0x0C, 0x52, 0x52, 0x52, 0x3E},
    /* h (0x68) */     {0x7F, 0x08, 0x04, 0x04, 0x78},
    /* i (0x69) */     {0x00, 0x44, 0x7D, 0x40, 0x00},
    /* k (0x6B) */     {0x20, 0x40, 0x3D, 0x04, 0x02},
    /* l (0x6C) */     {0x00, 0x41, 0x7F, 0x40, 0x00},
    /* m (0x6D) */     {0x7C, 0x04, 0x18, 0x04, 0x78},
    /* n (0x6E) */     {0x7C, 0x08, 0x04, 0x04, 0x78},
    /* o (0x6F) */     {0x38, 0x44, 0x44, 0x44, 0x38},
    /* p (0x70) */     {0x7C, 0x14, 0x14, 0x14, 0x08},
    /* q (0x71) */     {0x08, 0x14, 0x14, 0x18, 0x7C},
    /* r (0x72) */     {0x7C, 0x08, 0x04, 0x04, 0x08},
    /* s (0x73) */     {0x48, 0x54, 0x54, 0x54, 0x20},
    /* t (0x74) */     {0x04, 0x3F, 0x44, 0x40, 0x20},
    /* u (0x75) */     {0x3C, 0x40, 0x40, 0x20, 0x7C},
    /* v (0x76) */     {0x1C, 0x20, 0x40, 0x20, 0x1C},
    /* w (0x77) */     {0x3C, 0x40, 0x30, 0x40, 0x3C},
    /* x (0x78) */     {0x44, 0x28, 0x10, 0x28, 0x44},
    /* y (0x79) */     {0x0C, 0x50, 0x50, 0x50, 0x3C},
    /* z (0x7A) */     {0x44, 0x64, 0x54, 0x4C, 0x44},
    /* ° (0xB0) */     {0x06, 0x09, 0x06, 0x00, 0x00},  /* Degree sign */
    /* μ (0xB5) */     {0x44, 0x48, 0x7C, 0x04, 0x04},  /* Micro sign */
    /* [ (0x5B) */     {0x00, 0x7F, 0x41, 0x41, 0x00},
    /* ] (0x5D) */     {0x00, 0x41, 0x41, 0x7F, 0x00},
};

static int char_to_font_idx(char c) {
    if (c == ' ') return 0;
    if (c >= 'A' && c <= 'Z') return (c - 'A') + 33;
    if (c >= 'a' && c <= 'z') return (c - 'a') + 65;
    if (c >= '0' && c <= '9') return (c - '0') + 16;
    if (c == '!') return 1;
    if (c == '.') return 14;
    if (c == ',') return 13;
    if (c == '-') return 12;
    if (c == '+') return 10;
    if (c == ':') return 26;
    if (c == '/') return 15;
    if (c == '%') return 4;
    if (c == 0xB0) return 91;  /* Degree */
    return 0;  /* Default to space */
}

void display_draw_char(int x, int y, char c) {
    int idx = char_to_font_idx(c);
    for (int col = 0; col < 5; col++) {
        uint8_t col_data = font5x7[idx][col];
        for (int row = 0; row < 7; row++) {
            if (col_data & (1 << row)) {
                display_set_pixel(x + col, y + row, 1);
            } else {
                display_set_pixel(x + col, y + row, 0);
            }
        }
    }
}

void display_draw_string(int x, int y, const char *str) {
    int cx = x;
    while (*str) {
        display_draw_char(cx, y, *str);
        cx += 6;  /* 5 pixels + 1 space */
        str++;
    }
}

void display_draw_string_large(int x, int y, const char *str) {
    /* 2× scaled text (10×14 pixels) */
    int cx = x;
    while (*str) {
        int idx = char_to_font_idx(*str);
        for (int col = 0; col < 5; col++) {
            uint8_t col_data = font5x7[idx][col];
            for (int row = 0; row < 7; row++) {
                if (col_data & (1 << row)) {
                    for (int dx = 0; dx < 2; dx++) {
                        for (int dy = 0; dy < 2; dy++) {
                            display_set_pixel(cx + col*2 + dx,
                                             y + row*2 + dy, 1);
                        }
                    }
                }
            }
        }
        cx += 12;
        str++;
    }
}

/* ========================================================================
 * UI Screens
 * ======================================================================== */

void display_show_splash(void) {
    display_clear();
    display_draw_string_large(8, 4, "BREATH");
    display_draw_string_large(8, 24, "PRINT");
    display_draw_string(20, 52, "by jayis1");
    display_flush();
}

void display_show_idle(uint8_t battery, uint8_t connected,
                       const breath_result_t *last) {
    display_clear();

    /* Status bar */
    display_draw_string(0, 0, "BAT:");
    display_draw_char(24, 0, '0' + battery / 10);
    display_draw_char(30, 0, '0' + battery % 10);
    display_draw_char(36, 0, '%');

    if (connected) {
        display_draw_string(90, 0, "BLE");
    }

    /* Last result */
    if (last && last->breath_quality == BREATH_VALID) {
        display_draw_string(0, 12, "Last:");
        const char *state_name = "Unknown";
        switch (last->metabolic_state) {
        case STATE_BASELINE:     state_name = "Baseline"; break;
        case STATE_KETOTIC:      state_name = "Ketotic"; break;
        case STATE_POSTMEAL:     state_name = "Post-Meal"; break;
        case STATE_POSTEXERCISE: state_name = "Post-Exer"; break;
        case STATE_GUT_FERMENT:  state_name = "Gut Ferm."; break;
        default: break;
        }
        display_draw_string(0, 22, state_name);

        /* Key metrics */
        display_draw_string(0, 34, "Acet:");
        display_draw_string(36, 34, "ppm");
        display_draw_string(0, 44, "H2:");
        display_draw_string(36, 44, "ppm");
        display_draw_string(0, 54, "CH4:");
        display_draw_string(36, 54, "ppm");
    } else {
        display_draw_string(16, 20, "Ready");
        display_draw_string(4, 36, "Press button");
        display_draw_string(8, 46, "to analyze");
    }

    display_flush();
}

void display_show_warmup(void) {
    display_clear();
    display_draw_string_large(4, 4, "WARMING");
    display_draw_string_large(4, 24, "UP...");
    display_flush();
}

void display_show_warmup_progress(uint32_t elapsed, uint32_t total) {
    display_clear();
    display_draw_string(0, 0, "Warming up");

    /* Progress bar */
    int bar_width = 100;
    int filled = (int)(elapsed * bar_width / total);
    for (int x = 14; x < 14 + bar_width; x++) {
        display_set_pixel(x, 20, 1);
        display_set_pixel(x, 22, 1);
    }
    for (int x = 14; x < 14 + filled; x++) {
        for (int y = 21; y <= 21; y++) {
            display_set_pixel(x, y, 1);
        }
    }

    /* Percentage */
    int pct = (int)(elapsed * 100 / total);
    display_draw_char(50, 35, '0' + pct / 10);
    display_draw_char(56, 35, '0' + pct % 10);
    display_draw_char(62, 35, '%');

    display_flush();
}

void display_show_breathe_prompt(void) {
    display_clear();
    display_draw_string_large(4, 0, "BREATHE");
    display_draw_string_large(20, 20, "INTO");
    display_draw_string_large(12, 40, "MOUTH");
    display_draw_string_large(40, 52, "PIECE");
    display_flush();
}

void display_show_analyzing(void) {
    display_clear();
    display_draw_string_large(4, 12, "ANALYZ");
    display_draw_string_large(4, 32, "ING...");
    display_flush();
}

void display_show_result(const breath_result_t *result) {
    display_clear();

    /* State name */
    const char *state_name = "Unknown";
    switch (result->metabolic_state) {
    case STATE_BASELINE:     state_name = "Baseline"; break;
    case STATE_KETOTIC:      state_name = "Ketotic"; break;
    case STATE_POSTMEAL:     state_name = "Post-Meal"; break;
    case STATE_POSTEXERCISE: state_name = "Post-Exer"; break;
    case STATE_GUT_FERMENT:  state_name = "Gut Ferm."; break;
    default: break;
    }
    display_draw_string(0, 0, state_name);

    /* Quality */
    if (result->breath_quality != BREATH_VALID) {
        display_draw_string(0, 10, "INVALID!");
        display_draw_string(0, 20, "Retry");
        display_flush();
        return;
    }

    /* Key metrics */
    display_draw_string(0, 12, "Acetone:");
    display_draw_string(66, 12, "ppm");

    display_draw_string(0, 22, "H2:");
    display_draw_string(66, 22, "ppm");

    display_draw_string(0, 32, "CH4:");
    display_draw_string(66, 32, "ppm");

    display_draw_string(0, 42, "VOC:");
    display_draw_string(66, 42, "idx");

    display_draw_string(0, 54, "CO2:");
    display_draw_string(66, 54, "ppm");

    display_flush();
}

void display_show_error(const char *msg) {
    display_clear();
    display_draw_string_large(4, 0, "ERROR");
    display_draw_string(0, 20, msg);
    display_flush();
}

void display_show_low_battery(uint8_t pct) {
    display_clear();
    display_draw_string_large(4, 0, "LOW");
    display_draw_string_large(4, 20, "BATT");
    display_draw_char(50, 44, '0' + pct / 10);
    display_draw_char(56, 44, '0' + pct % 10);
    display_draw_char(62, 44, '%');
    display_flush();
}

void display_show_calibration(void) {
    display_clear();
    display_draw_string_large(4, 4, "CALIB");
    display_draw_string_large(4, 24, "RATING");
    display_draw_string(4, 52, "Keep in air");
    display_flush();
}

void display_show_calibration_done(void) {
    display_clear();
    display_draw_string_large(4, 4, "CALIB");
    display_draw_string_large(4, 24, "DONE!");
    display_flush();
    delay_ms(2000);
}

void display_on(void) {
    disp_command(SSD1306_DISPLAY_ON);
    display_on_flag = 1;
}

void display_off(void) {
    disp_command(SSD1306_DISPLAY_OFF);
    display_on_flag = 0;
}