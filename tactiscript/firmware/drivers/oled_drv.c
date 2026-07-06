/*
 * oled_drv.c — TactiScript OLED (SSD1316) status display driver
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Drives a 0.49" monochrome OLED (64×32) over I2C (TWIM0).
 * Used for status display: mode, battery, charging, BLE connection,
 * uptime, and settings. The display is OFF in sleep mode to save power.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "oled_drv.h"
#include "../board.h"
#include "../registers.h"

/* ----------------------------------------------------------------
 * Display buffer: 4 pages × 64 columns = 256 bytes
 * Each byte is 8 vertical pixels (1 page = 8 rows).
 * ---------------------------------------------------------------- */
static uint8_t s_display_buffer[OLED_PAGES][OLED_WIDTH];
static bool s_initialized = false;

/* ----------------------------------------------------------------
 * GPIO helpers
 * ---------------------------------------------------------------- */
static inline void gpio_set(uint32_t port, uint32_t pin)
{
    GPIO_OUTSET(GPIO_PORT_BASE(port)) = (1u << pin);
}

static inline void gpio_clr(uint32_t port, uint32_t pin)
{
    GPIO_OUTCLR(GPIO_PORT_BASE(port)) = (1u << pin);
}

/* ----------------------------------------------------------------
 * I2C (TWIM0) helpers
 * ---------------------------------------------------------------- */
static void i2c_write(uint8_t addr, const uint8_t *data, uint16_t len)
{
    TWIM_ENABLE(NRF_TWIM0_BASE) = 1;
    /* Set slave address (simplified — real driver uses PSEL) */
    TWIM_TXD_PTR(NRF_TWIM0_BASE) = (uint32_t)data;
    TWIM_TXD_MAXCNT(NRF_TWIM0_BASE) = len;
    TWIM_TASKS_STARTTX(NRF_TWIM0_BASE) = 1;
    while (!TWIM_EVENTS_END(NRF_TWIM0_BASE)) ;
    TWIM_EVENTS_END(NRF_TWIM0_BASE) = 0;
    TWIM_TASKS_STOP(NRF_TWIM0_BASE) = 1;
    while (!TWIM_EVENTS_STOPPED(NRF_TWIM0_BASE)) ;
    TWIM_EVENTS_STOPPED(NRF_TWIM0_BASE) = 0;
}

/* Send a command byte to the OLED */
static void oled_command(uint8_t cmd)
{
    uint8_t buf[2];
    buf[0] = 0x00; /* Co=0, D/C=0 → command */
    buf[1] = cmd;
    i2c_write(I2C_ADDR_OLED, buf, 2);
}

/* Send a data byte to the OLED */
static void oled_data(uint8_t data)
{
    uint8_t buf[2];
    buf[0] = 0x40; /* Co=0, D/C=1 → data */
    buf[1] = data;
    i2c_write(I2C_ADDR_OLED, buf, 2);
}

/* ----------------------------------------------------------------
 * 5×7 ASCII font (subset: space, 0-9, A-Z, some punctuation)
 * Each character is 5 bytes (5 columns), each byte = 1 column (8 bits,
 * of which 7 are used for the 7-pixel-high glyph).
 * ---------------------------------------------------------------- */
static const uint8_t font5x7[][5] = {
    /* 0x20 space */ {0x00,0x00,0x00,0x00,0x00},
    /* 0x21 !      */ {0x00,0x00,0x5F,0x00,0x00},
    /* 0x22 "      */ {0x00,0x07,0x00,0x07,0x00},
    /* 0x23 #      */ {0x14,0x7F,0x14,0x7F,0x14},
    /* 0x24 $      */ {0x24,0x2A,0x7F,0x2A,0x12},
    /* 0x25 %      */ {0x23,0x13,0x08,0x64,0x62},
    /* 0x26 &      */ {0x36,0x49,0x55,0x20,0x50},
    /* 0x27 '      */ {0x00,0x05,0x03,0x00,0x00},
    /* 0x28 (      */ {0x00,0x1C,0x22,0x41,0x00},
    /* 0x29 )      */ {0x00,0x41,0x22,0x1C,0x00},
    /* 0x2A *      */ {0x08,0x2A,0x1C,0x2A,0x08},
    /* 0x2B +      */ {0x08,0x08,0x3E,0x08,0x08},
    /* 0x2C ,      */ {0x00,0x50,0x30,0x00,0x00},
    /* 0x2D -      */ {0x00,0x08,0x08,0x08,0x00},
    /* 0x2E .      */ {0x00,0x60,0x60,0x00,0x00},
    /* 0x2F /      */ {0x20,0x10,0x08,0x04,0x02},
    /* 0x30 0      */ {0x3E,0x51,0x49,0x45,0x3E},
    /* 0x31 1      */ {0x00,0x42,0x7F,0x40,0x00},
    /* 0x32 2      */ {0x42,0x61,0x51,0x49,0x46},
    /* 0x33 3      */ {0x21,0x41,0x45,0x4B,0x31},
    /* 0x34 4      */ {0x18,0x14,0x12,0x7F,0x10},
    /* 0x35 5      */ {0x27,0x45,0x45,0x45,0x39},
    /* 0x36 6      */ {0x3C,0x4A,0x49,0x49,0x30},
    /* 0x37 7      */ {0x01,0x71,0x09,0x05,0x03},
    /* 0x38 8      */ {0x36,0x49,0x49,0x49,0x36},
    /* 0x39 9      */ {0x06,0x49,0x49,0x29,0x1E},
    /* 0x3A :      */ {0x00,0x36,0x36,0x00,0x00},
    /* 0x3B ;      */ {0x00,0x56,0x36,0x00,0x00},
    /* 0x3C <      */ {0x00,0x08,0x14,0x22,0x41},
    /* 0x3D =      */ {0x14,0x14,0x14,0x14,0x14},
    /* 0x3E >      */ {0x41,0x22,0x14,0x08,0x00},
    /* 0x3F ?      */ {0x02,0x01,0x51,0x09,0x06},
    /* 0x40 @      */ {0x32,0x49,0x79,0x41,0x3E},
    /* 0x41 A      */ {0x7E,0x11,0x11,0x11,0x7E},
    /* 0x42 B      */ {0x7F,0x49,0x49,0x49,0x36},
    /* 0x43 C      */ {0x3E,0x41,0x41,0x41,0x22},
    /* 0x44 D      */ {0x7F,0x41,0x41,0x22,0x1C},
    /* 0x45 E      */ {0x7F,0x49,0x49,0x49,0x41},
    /* 0x46 F      */ {0x7F,0x09,0x09,0x01,0x01},
    /* 0x47 G      */ {0x3E,0x41,0x41,0x51,0x32},
    /* 0x48 H      */ {0x7F,0x08,0x08,0x08,0x7F},
    /* 0x49 I      */ {0x00,0x41,0x7F,0x41,0x00},
    /* 0x4A J      */ {0x20,0x40,0x41,0x3F,0x01},
    /* 0x4B K      */ {0x7F,0x08,0x14,0x22,0x41},
    /* 0x4C L      */ {0x7F,0x40,0x40,0x40,0x40},
    /* 0x4D M      */ {0x7F,0x02,0x04,0x02,0x7F},
    /* 0x4E N      */ {0x7F,0x04,0x08,0x10,0x7F},
    /* 0x4F O      */ {0x3E,0x41,0x41,0x41,0x3E},
    /* 0x50 P      */ {0x7F,0x09,0x09,0x09,0x06},
    /* 0x51 Q      */ {0x3E,0x41,0x51,0x21,0x5E},
    /* 0x52 R      */ {0x7F,0x09,0x19,0x29,0x46},
    /* 0x53 S      */ {0x46,0x49,0x49,0x49,0x31},
    /* 0x54 T      */ {0x01,0x01,0x7F,0x01,0x01},
    /* 0x55 U      */ {0x3F,0x40,0x40,0x40,0x3F},
    /* 0x56 V      */ {0x1F,0x20,0x40,0x20,0x1F},
    /* 0x57 W      */ {0x7F,0x20,0x18,0x20,0x7F},
    /* 0x58 X      */ {0x63,0x14,0x08,0x14,0x63},
    /* 0x59 Y      */ {0x03,0x04,0x78,0x04,0x03},
    /* 0x5A Z      */ {0x61,0x51,0x49,0x45,0x43},
    /* 0x5B [      */ {0x00,0x7F,0x41,0x41,0x00},
    /* 0x5C \      */ {0x02,0x04,0x08,0x10,0x20},
    /* 0x5D ]      */ {0x00,0x41,0x41,0x7F,0x00},
    /* 0x5E ^      */ {0x04,0x02,0x01,0x02,0x04},
    /* 0x5F _      */ {0x40,0x40,0x40,0x40,0x40},
    /* 0x60 `      */ {0x00,0x01,0x02,0x04,0x00},
    /* 0x61-0x7A a-z: same glyphs as uppercase but shifted down 1px
     * For simplicity, we use the uppercase glyphs for lowercase too. */
};

/* ----------------------------------------------------------------
 * Get font glyph for a character
 * ---------------------------------------------------------------- */
static const uint8_t *font_get_glyph(char c)
{
    if (c >= 0x20 && c <= 0x5F)
        return font5x7[c - 0x20];
    if (c >= 0x61 && c <= 0x7A)
        return font5x7[c - 0x61 + 0x41 - 0x20]; /* map to uppercase */
    return font5x7[0]; /* space for unknown chars */
}

/* ----------------------------------------------------------------
 * Initialize OLED
 * ---------------------------------------------------------------- */
void oled_init(void)
{
    if (s_initialized)
        return;

    /* Configure I2C pins */
    TWIM_PSEL_SCL(NRF_TWIM0_BASE) = GPIO_PIN(PIN_I2C_SCL);
    TWIM_PSEL_SDA(NRF_TWIM0_BASE) = GPIO_PIN(PIN_I2C_SDA);
    TWIM_ENABLE(NRF_TWIM0_BASE) = 0x06; /* enable TWIM */

    /* SSD1316 initialization sequence */
    oled_command(OLED_CMD_DISPLAY_OFF);
    oled_command(0xD5); oled_command(0x80);  /* set osc frequency */
    oled_command(0xA8); oled_command(0x1F);  /* set mux ratio (32-1) */
    oled_command(0xD3); oled_command(0x00);  /* display offset */
    oled_command(0x40);                       /* set start line */
    oled_command(0x8D); oled_command(0x14);  /* enable charge pump */
    oled_command(0x20); oled_command(0x00);  /* addressing mode: horizontal */
    oled_command(0x21); oled_command(0); oled_command(63); /* col range */
    oled_command(0x22); oled_command(0); oled_command(3); /* page range */
    oled_command(0xC8);                       /* COM scan remap */
    oled_command(0xA1);                       /* segment remap */
    oled_command(0xDA); oled_command(0x02);  /* COM pins config */
    oled_command(0x81); oled_command(0xCF);  /* contrast */
    oled_command(0xD9); oled_command(0xF1);  /* precharge period */
    oled_command(0xDB); oled_command(0x40);  /* VCOMH deselect */
    oled_command(0xA4);                       /* normal display */
    oled_command(0xA6);                       /* not inverted */
    oled_command(OLED_CMD_DISPLAY_ON);

    s_initialized = true;

    oled_clear();
    oled_display_flush();
}

/* ----------------------------------------------------------------
 * Clear display buffer
 * ---------------------------------------------------------------- */
void oled_clear(void)
{
    memset(s_display_buffer, 0, sizeof(s_display_buffer));
}

/* ----------------------------------------------------------------
 * Draw a single character
 * ---------------------------------------------------------------- */
void oled_draw_char(uint8_t col, uint8_t page, char c)
{
    if (col >= 12 || page >= 4)
        return;

    const uint8_t *glyph = font_get_glyph(c);
    uint8_t x = col * 5;

    for (uint8_t i = 0; i < 5; i++) {
        if (x + i < OLED_WIDTH) {
            s_display_buffer[page][x + i] = glyph[i];
        }
    }
    /* 1-pixel space after character */
    if (x + 5 < OLED_WIDTH)
        s_display_buffer[page][x + 5] = 0x00;
}

/* ----------------------------------------------------------------
 * Draw a string
 * ---------------------------------------------------------------- */
void oled_draw_string(uint8_t col, uint8_t page, const char *str)
{
    uint8_t c = col;
    while (*str && c < 12) {
        oled_draw_char(c, page, *str);
        str++;
        c++;
    }
}

/* ----------------------------------------------------------------
 * Set a pixel
 * ---------------------------------------------------------------- */
void oled_set_pixel(uint8_t x, uint8_t y, bool on)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;

    uint8_t page = y / 8;
    uint8_t bit = 1 << (y % 8);

    if (on)
        s_display_buffer[page][x] |= bit;
    else
        s_display_buffer[page][x] &= ~bit;
}

/* ----------------------------------------------------------------
 * Flush buffer to display
 * ---------------------------------------------------------------- */
void oled_display_flush(void)
{
    if (!s_initialized)
        return;

    /* Set column and page address ranges */
    oled_command(0x21); oled_command(0); oled_command(63);
    oled_command(0x22); oled_command(0); oled_command(3);

    /* Send all 256 bytes of display data */
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        for (uint8_t col = 0; col < OLED_WIDTH; col++) {
            oled_data(s_display_buffer[page][col]);
        }
    }
}

/* ----------------------------------------------------------------
 * Display on/off and contrast
 * ---------------------------------------------------------------- */
void oled_display_on(void)
{
    if (s_initialized)
        oled_command(OLED_CMD_DISPLAY_ON);
}

void oled_display_off(void)
{
    if (s_initialized)
        oled_command(OLED_CMD_DISPLAY_OFF);
}

void oled_set_contrast(uint8_t level)
{
    if (s_initialized) {
        oled_command(0x81);
        oled_command(level);
    }
}

/* ----------------------------------------------------------------
 * End of oled_drv.c
 * Author: jayis1
 * ---------------------------------------------------------------- */