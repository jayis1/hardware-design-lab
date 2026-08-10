/**
 * @file    display.c
 * @brief   TideBand — Sharp LS013B7DH03 128×128 transflective LCD driver.
 *          Renders the dive current-rose, depth profile, and status bar
 *          using a 1-bit-per-pixel frame buffer sent over SPI4.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * The Sharp LS013B7DH03 is a 1.28" memory-in-pixel transflective LCD.
 * Key properties:
 *   - 128×128 pixels, 1 bit per pixel (black/white)
 *   - SPI interface (3-wire: SCK, MOSI, CS)
 *   - Each pixel is a latch: it retains its state with zero power
 *   - Transflective: readable in direct sunlight (reflective mode)
 *     and in darkness (transmissive backlight, not present on this model)
 *   - Ultra-low power: ~2.5 mW active, ~0.4 mW idle
 *   - Requires COM inversion every 10 seconds (toggle EXTMODE pin)
 *
 * Frame buffer: 128 rows × 128 cols × 1 bit = 2048 bytes.
 * Each row is 16 bytes (128 bits). MSB of first byte = leftmost pixel.
 *
 * Transmission format:
 *   [0x01 (write command)] [line_num+1] [16 bytes pixel data] ... [0x00 trailer]
 *   Line numbers are 1-indexed (1-128). Trailer 0x00 ends the frame.
 */

#include <string.h>
#include <math.h>
#include "board.h"
#include "registers.h"
#include "display.h"

/* ---- Frame buffer ---- */
static uint8_t framebuf[LCD_BUF_SIZE];
static display_mode_t current_mode = DISP_MODE_SURFACE;

/* ---- Local functions ---- */
static void lcd_select(void);
static void lcd_deselect(void);
static uint8_t lcd_spi_transfer(uint8_t tx);
static void lcd_write_line(uint8_t line, const uint8_t *data);
static void lcd_flush(void);
static void set_pixel(uint8_t x, uint8_t y, uint8_t val);
static void draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t fill);
static void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
static void draw_char(uint8_t x, uint8_t y, char c);
static void draw_string(uint8_t x, uint8_t y, const char *s);
static void draw_number(uint8_t x, uint8_t y, float val, uint8_t decimals);
static void draw_current_rose(float speed_ms, float heading_deg);
static void draw_depth_bar(float depth_m, float max_depth_m);
static void draw_status_bar(float battery_pct, uint32_t dive_time_s);

/* ---- 5×7 font (simplified — digits and basic chars) ---- */
static const uint8_t font5x7[][5] = {
    /* 0-9 */
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
};

/* ---- Public API ---- */

void display_init(void)
{
    /* LCD CS, DISP, EXTMODE — outputs */
    gpio_set_mode(LCD_CS_GPIO, LCD_CS_PIN, GPIO_MODE_OUTPUT);
    gpio_set(LCD_CS_GPIO, LCD_CS_PIN);
    gpio_set_mode(LCD_DISP_GPIO, LCD_DISP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_mode(LCD_EXTMODE_GPIO, LCD_EXTMODE_PIN, GPIO_MODE_OUTPUT);

    /* Clear frame buffer */
    memset(framebuf, 0xFF, sizeof(framebuf));  /* All white */

    /* Enable display */
    gpio_set(LCD_DISP_GPIO, LCD_DISP_PIN);
    lcd_flush();
}

void display_clear(void)
{
    memset(framebuf, 0xFF, sizeof(framebuf));
    lcd_flush();
}

void display_set_mode(display_mode_t mode)
{
    current_mode = mode;
}

void display_on(void)
{
    gpio_set(LCD_DISP_GPIO, LCD_DISP_PIN);
}

void display_off(void)
{
    gpio_clear(LCD_DISP_GPIO, LCD_DISP_PIN);
}

void display_render_dive(const doppler_result_t *doppler,
                         const depth_data_t *depth,
                         const attitude_t *att,
                         float battery_pct,
                         uint32_t dive_time_s)
{
    memset(framebuf, 0xFF, sizeof(framebuf));  /* White background */

    /* Top: status bar (battery + dive time) */
    draw_status_bar(battery_pct, dive_time_s);

    /* Center: current rose (speed + direction) */
    draw_current_rose(doppler->speed, att->yaw * 57.2958f);

    /* Bottom: depth and temperature */
    draw_number(2, 100, depth->depth_m, 1);
    draw_string(40, 100, "m");
    draw_number(70, 100, depth->temp_c, 1);
    draw_char(108, 100, 'C');  /* Approximate °C as "C" */

    /* Quality indicator */
    uint8_t q = doppler->quality;
    for (uint8_t i = 0; i < 4; i++) {
        draw_circle(8 + i * 10, 115, 3, (i <= q) ? 1 : 0);
    }

    lcd_flush();
}

void display_render_surface(float battery_pct, uint16_t dive_count,
                             float surface_temp_c)
{
    memset(framebuf, 0xFF, sizeof(framebuf));

    draw_status_bar(battery_pct, 0);

    /* "TIDEBAND" title */
    draw_string(30, 30, "TIDEBAND");

    /* Dive count */
    draw_string(20, 50, "Dives:");
    if (dive_count < 10) {
        draw_char(75, 50, '0' + dive_count);
    }

    /* Surface temperature */
    draw_string(20, 65, "Temp:");
    draw_number(60, 65, surface_temp_c, 1);
    draw_char(85, 65, 'C');

    /* "READY" indicator */
    draw_string(45, 90, "READY");

    lcd_flush();
}

void display_render_error(const char *msg)
{
    memset(framebuf, 0xFF, sizeof(framebuf));
    draw_string(10, 60, "ERROR:");
    draw_string(10, 70, msg);
    lcd_flush();
}

/* ---- Local function implementations ---- */

static void lcd_select(void)
{
    gpio_set(IMU_CS_GPIO, IMU_CS_PIN);
    gpio_set(NAND_CS_GPIO, NAND_CS_PIN);
    gpio_clear(LCD_CS_GPIO, LCD_CS_PIN);
    for (volatile int i = 0; i < 5; i++) { }
}

static void lcd_deselect(void)
{
    gpio_set(LCD_CS_GPIO, LCD_CS_PIN);
}

static uint8_t lcd_spi_transfer(uint8_t tx)
{
    *(volatile uint8_t *)&SPI4_DR = tx;
    while ((SPI4_SR & SPI_SR_RXP) == 0) { }
    return *(volatile uint8_t *)&SPI4_DR;
}

static void lcd_write_line(uint8_t line, const uint8_t *data)
{
    lcd_select();
    lcd_spi_transfer(0x01);           /* Write command */
    lcd_spi_transfer(line + 1);       /* Line number (1-indexed) */
    for (uint8_t i = 0; i < 16; i++) { /* 16 bytes = 128 bits */
        lcd_spi_transfer(data[i]);
    }
    lcd_spi_transfer(0x00);           /* Trailer */
    lcd_deselect();
}

static void lcd_flush(void)
{
    /* Send each line to the display */
    for (uint8_t line = 0; line < LCD_HEIGHT; line++) {
        uint8_t *row = &framebuf[line * 16];
        lcd_write_line(line, row);
    }
}

static void set_pixel(uint8_t x, uint8_t y, uint8_t val)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    uint16_t byte_idx = y * 16 + (x / 8);
    uint8_t bit = 7 - (x % 8);
    if (val) {
        framebuf[byte_idx] &= ~(1u << bit);  /* Black pixel */
    } else {
        framebuf[byte_idx] |= (1u << bit);   /* White pixel */
    }
}

static void draw_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t fill)
{
    int8_t x = r, y = 0;
    int8_t err = 1 - r;

    while (x >= y) {
        if (fill) {
            for (int8_t i = -x; i <= x; i++) {
                set_pixel(cx + i, cy + y, 1);
                set_pixel(cx + i, cy - y, 1);
            }
            for (int8_t i = -y; i <= y; i++) {
                set_pixel(cx + i, cy + x, 1);
                set_pixel(cx + i, cy - x, 1);
            }
        } else {
            set_pixel(cx + x, cy + y, 1);
            set_pixel(cx - x, cy + y, 1);
            set_pixel(cx + x, cy - y, 1);
            set_pixel(cx - x, cy - y, 1);
            set_pixel(cx + y, cy + x, 1);
            set_pixel(cx - y, cy + x, 1);
            set_pixel(cx + y, cy - x, 1);
            set_pixel(cx - y, cy - x, 1);
        }
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

static void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        set_pixel((uint8_t)x0, (uint8_t)y0, 1);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

static void draw_char(uint8_t x, uint8_t y, char c)
{
    uint8_t idx;
    if (c >= '0' && c <= '9') {
        idx = c - '0';
    } else {
        return;  /* Unsupported char */
    }
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = font5x7[idx][col];
        for (uint8_t row = 0; row < 7; row++) {
            if (bits & (1u << row)) {
                set_pixel(x + col, y + row, 1);
            }
        }
    }
}

static void draw_string(uint8_t x, uint8_t y, const char *s)
{
    /* For simplicity, only draw digits. Other chars skipped.
     * A full implementation would include the complete ASCII font. */
    uint8_t px = x;
    while (*s) {
        if (*s >= '0' && *s <= '9') {
            draw_char(px, y, *s);
            px += 6;
        } else {
            px += 5;  /* Space for unsupported chars */
        }
        s++;
    }
}

static void draw_number(uint8_t x, uint8_t y, float val, uint8_t decimals)
{
    /* Simple float-to-string for display.
     * Format: optional minus sign, integer part, decimal point, frac part.
     * For the 5x7 font, we only draw digits. */
    char buf[12];
    int int_part = (int)val;
    int frac_part = 0;
    if (decimals > 0) {
        float frac = val - int_part;
        if (frac < 0) frac = -frac;
        for (uint8_t i = 0; i < decimals; i++) frac *= 10;
        frac_part = (int)(frac + 0.5f);
    }

    int neg = 0;
    if (int_part < 0) { neg = 1; int_part = -int_part; }

    uint8_t px = x;
    if (neg) {
        /* Draw minus as horizontal line */
        draw_line(px, y + 3, px + 4, y + 3);
        px += 6;
    }

    /* Draw integer digits */
    char intbuf[8];
    int ip = 0;
    if (int_part == 0) {
        intbuf[ip++] = '0';
    } else {
        while (int_part > 0 && ip < 7) {
            intbuf[ip++] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }
    for (int i = ip - 1; i >= 0; i--) {
        draw_char(px, y, intbuf[i]);
        px += 6;
    }

    /* Decimal point as single pixel */
    if (decimals > 0) {
        set_pixel(px + 1, y + 6, 1);
        px += 4;
        for (uint8_t i = 0; i < decimals; i++) {
            int digit = frac_part;
            for (uint8_t j = i + 1; j < decimals; j++) digit /= 10;
            digit %= 10;
            draw_char(px, y, '0' + digit);
            px += 6;
        }
    }
}

static void draw_current_rose(float speed_ms, float heading_deg)
{
    uint8_t cx = 64, cy = 55;
    uint8_t r = 30;

    /* Outer circle */
    draw_circle(cx, cy, r, 0);

    /* Cardinal direction marks */
    draw_line(cx, cy - r, cx, cy - r + 4);       /* N */
    draw_line(cx, cy + r, cx, cy + r - 4);       /* S */
    draw_line(cx - r, cy, cx - r + 4, cy);       /* W */
    draw_line(cx + r, cy, cx + r - 4, cy);       /* E */

    /* Current direction arrow */
    float angle_rad = heading_deg * 0.01745329f;
    int16_t ex = cx + (int16_t)(sinf(angle_rad) * r * speed_ms / 5.0f);
    int16_t ey = cy - (int16_t)(cosf(angle_rad) * r * speed_ms / 5.0f);
    if (speed_ms > 0.01f) {
        draw_line(cx, cy, (uint8_t)ex, (uint8_t)ey);
        /* Arrowhead */
        draw_circle((uint8_t)ex, (uint8_t)ey, 2, 1);
    }

    /* Speed value below rose */
    draw_number(50, 88, speed_ms, 1);
}

static void draw_depth_bar(float depth_m, float max_depth_m)
{
    /* Simple vertical bar on right edge showing depth as fraction of max */
    uint8_t bar_x = 120;
    uint8_t bar_top = 15;
    uint8_t bar_bot = 95;
    uint8_t bar_h = bar_bot - bar_top;

    /* Bar outline */
    draw_line(bar_x, bar_top, bar_x, bar_bot);
    draw_line(bar_x + 4, bar_top, bar_x + 4, bar_bot);
    draw_line(bar_x, bar_top, bar_x + 4, bar_top);
    draw_line(bar_x, bar_bot, bar_x + 4, bar_bot);

    /* Fill proportional to depth/max_depth */
    if (max_depth_m > 0) {
        uint8_t fill_h = (uint8_t)(bar_h * depth_m / max_depth_m);
        for (uint8_t y = bar_bot - 1; y > bar_bot - fill_h; y--) {
            for (uint8_t x = bar_x + 1; x < bar_x + 4; x++) {
                set_pixel(x, y, 1);
            }
        }
    }
}

static void draw_status_bar(float battery_pct, uint32_t dive_time_s)
{
    /* Top row: battery icon (left) and dive time (right) */
    /* Battery: rectangle with fill */
    draw_line(2, 2, 18, 2);
    draw_line(2, 8, 18, 8);
    draw_line(2, 2, 2, 8);
    draw_line(18, 2, 18, 8);
    draw_line(19, 3, 19, 7);  /* Battery terminal */

    uint8_t fill_w = (uint8_t)(15.0f * battery_pct / 100.0f);
    for (uint8_t x = 3; x < 3 + fill_w; x++) {
        for (uint8_t y = 3; y < 8; y++) {
            set_pixel(x, y, 1);
        }
    }

    /* Dive time MM:SS on right side of status bar */
    uint32_t mins = dive_time_s / 60;
    uint32_t secs = dive_time_s % 60;
    if (mins < 100) {
        draw_number(95, 2, (float)mins, 0);
        /* Colon */
        set_pixel(107, 4, 1);
        set_pixel(107, 6, 1);
        draw_number(110, 2, (float)secs, 0);
    }
}