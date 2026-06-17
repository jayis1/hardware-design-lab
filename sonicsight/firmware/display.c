/**
 * @file    display.c
 * @brief   SonicSight — GC9A01 240×240 IPS LCD driver.
 *          Implements SPI communication, initialisation sequence, pixel
 *          drawing, text rendering (5×7 bitmap font), and graphics helpers.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include <string.h>
#include <stdio.h>
#include "display.h"
#include "board.h"
#include "registers.h"

/* External SPI handle (initialised in CubeMX or main) */
extern SPI_HandleTypeDef hspi1;

/* ========================================================================== */
/*  Low-Level SPI Operations                                                  */
/* ========================================================================== */

static void lcd_write_cmd(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET); /* Command mode */
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

static void lcd_write_data(uint8_t data)
{
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET); /* Data mode */
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

static void lcd_write_data16(uint16_t data)
{
    uint8_t buf[2] = { (uint8_t)(data >> 8), (uint8_t)(data & 0xFF) };
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, buf, 2, 100);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

static void lcd_set_addr_window(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
    lcd_write_cmd(GC9A01_CASET);
    lcd_write_data16((uint16_t)x0);
    lcd_write_data16((uint16_t)x1);

    lcd_write_cmd(GC9A01_RASET);
    lcd_write_data16((uint16_t)y0);
    lcd_write_data16((uint16_t)y1);

    lcd_write_cmd(GC9A01_RAMWR);
}

/* ========================================================================== */
/*  Display Initialisation                                                    */
/* ========================================================================== */

void display_init(void)
{
    /* Hardware reset */
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(120);

    /* Initialisation sequence (from GC9A01 datasheet) */
    lcd_write_cmd(0xEF);   /* Unlock commands */
    lcd_write_data(0xEF);

    lcd_write_cmd(GC9A01_SWRESET);
    HAL_Delay(120);

    lcd_write_cmd(GC9A01_SETPCR); /* Porch control */
    lcd_write_data(0x0C);
    lcd_write_data(0x0C);
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);

    lcd_write_cmd(0xB7);   /* Gate control */
    lcd_write_data(0x35);

    lcd_write_cmd(GC9A01_SETDGC); /* Gamma */
    lcd_write_data(0x40);
    lcd_write_data(0x22);
    lcd_write_data(0x2A);
    lcd_write_data(0x30);
    lcd_write_data(0x68);
    lcd_write_data(0xA8);
    lcd_write_data(0x76);
    lcd_write_data(0x0A);
    lcd_write_data(0x78);
    lcd_write_data(0x4C);
    lcd_write_data(0x8A);
    lcd_write_data(0x6B);
    lcd_write_data(0x28);
    lcd_write_data(0x1D);
    lcd_write_data(0x0F);
    lcd_write_data(0x13);
    lcd_write_data(0x17);
    lcd_write_data(0x1A);

    lcd_write_cmd(GC9A01_COLMOD);
    lcd_write_data(GC9A01_COLMOD_16BIT);

    lcd_write_cmd(GC9A01_MADCTL);
    lcd_write_data(GC9A01_MAD_MX | GC9A01_MAD_MY); /* orientation */

    lcd_write_cmd(GC9A01_FRCTRL2);
    lcd_write_data(0x0F);

    lcd_write_cmd(GC9A01_INVOFF);

    lcd_write_cmd(GC9A01_SLPOUT);
    HAL_Delay(120);

    lcd_write_cmd(GC9A01_DISPON);
    HAL_Delay(50);

    /* Fill background black */
    display_fill(0x0000);
}

/* ========================================================================== */
/*  Pixel Drawing                                                             */
/* ========================================================================== */

void display_draw_pixel(uint32_t x, uint32_t y, uint16_t colour)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    lcd_set_addr_window(x, y, x, y);
    lcd_write_data16(colour);
}

/* ========================================================================== */
/*  Fill Screen                                                               */
/* ========================================================================== */

void display_fill(uint16_t colour)
{
    lcd_set_addr_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    /* Bulk write same colour (SPI optimisation) */
    uint8_t hi = (uint8_t)(colour >> 8);
    uint8_t lo = (uint8_t)(colour & 0xFF);

    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);

    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        HAL_SPI_Transmit(&hspi1, &hi, 1, 100);
        HAL_SPI_Transmit(&hspi1, &lo, 1, 100);
    }

    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/* ========================================================================== */
/*  5×7 Bitmap Font                                                           */
/* ========================================================================== */

/* ASCII characters 0x20–0x7E, 5 columns × 7 rows per character */
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
    {0x08,0x2A,0x1C,0x2A,0x08}, /* * */
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
    {0x7F,0x20,0x18,0x20,0x7F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x03,0x04,0x78,0x04,0x03}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x00,0x7F,0x41,0x41}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* \ */
    {0x41,0x41,0x7F,0x00,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* _ */
    {0x00,0x01,0x02,0x04,0x00}, /* ` */
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
    {0x00,0x7F,0x10,0x28,0x44}, /* k */
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
};

/* ========================================================================== */
/*  Text Rendering                                                            */
/* ========================================================================== */

void display_draw_text(uint32_t x, uint32_t y, const char *text,
                        uint16_t fg_col, uint16_t bg_col)
{
    if (!text) return;

    uint32_t orig_x = x;

    while (*text) {
        uint8_t ch = (uint8_t)(*text);

        if (ch == '\n') {
            x = orig_x;
            y += 8;
            text++;
            continue;
        }

        if (ch < 0x20 || ch > 0x7E) {
            ch = 0x20; /* Replace non-printable with space */
        }
        ch -= 0x20;

        /* Draw 5×7 character */
        for (uint8_t col = 0; col < 5; col++) {
            uint8_t line = font5x7[ch][col];
            for (uint8_t row = 0; row < 7; row++) {
                uint16_t colour = (line & (1 << row)) ? fg_col : bg_col;
                display_draw_pixel(x + col, y + row, colour);
            }
            /* 1-pixel column gap */
            display_draw_pixel(x + col, y + 7, bg_col);
        }

        /* 1-pixel character gap */
        for (uint8_t row = 0; row < 8; row++) {
            display_draw_pixel(x + 5, y + row, bg_col);
        }

        x += 6;
        text++;
    }
}

/* ========================================================================== */
/*  Splash Screen                                                             */
/* ========================================================================== */

void display_show_splash(const char *name, uint32_t ver_maj,
                          uint32_t ver_min, uint32_t ver_pat)
{
    display_fill(0x0000);

    /* Draw a centered "SonicSight" banner */
    char version[32];
    snprintf(version, sizeof(version), "v%lu.%lu.%lu",
             (unsigned long)ver_maj,
             (unsigned long)ver_min,
             (unsigned long)ver_pat);

    uint32_t cx = LCD_WIDTH / 2;
    uint32_t cy = LCD_HEIGHT / 2;

    /* Draw device name in larger-like text */
    const char *title = name;
    uint32_t tx = cx - (strlen(title) * 3); /* approximate centre */
    display_draw_text(tx, cy - 20, title, 0x07E0, 0x0000); /* green */

    /* Draw version */
    display_draw_text(cx - 24, cy + 4, version, 0xFFFF, 0x0000);

    /* Draw author line */
    display_draw_text(cx - 30, cy + 28, "by jayis1", 0x7BEF, 0x0000);

    HAL_Delay(1500);
}

/* ========================================================================== */
/*  Simple Message Display                                                    */
/* ========================================================================== */

void display_show_message(const char *msg)
{
    display_fill(0x0000);
    uint32_t cx = LCD_WIDTH / 2;
    uint32_t cy = LCD_HEIGHT / 2;
    uint32_t tx = cx - (strlen(msg) * 3); /* approx centre */
    display_draw_text(tx, cy - 4, msg, 0xFFFF, 0x0000);
}

/* ========================================================================== */
/*  Progress Bar                                                              */
/* ========================================================================== */

void display_show_progress(uint32_t current, uint32_t total)
{
    if (total == 0) return;

    /* Progress bar at bottom of screen */
    uint32_t bar_y = LCD_HEIGHT - 12;
    uint32_t bar_width = LCD_WIDTH - 8;
    uint32_t bar_x = 4;
    uint32_t fill = (current * bar_width) / total;

    /* Clear bar area */
    for (uint32_t x = bar_x; x < bar_x + bar_width; x++) {
        for (uint32_t y = bar_y; y < bar_y + 8; y++) {
            display_draw_pixel(x, y, 0x0000);
        }
    }

    /* Draw border */
    for (uint32_t x = bar_x; x < bar_x + bar_width; x++) {
        display_draw_pixel(x, bar_y, 0xFFFF);
        display_draw_pixel(x, bar_y + 7, 0xFFFF);
    }
    for (uint32_t y = bar_y; y < bar_y + 8; y++) {
        display_draw_pixel(bar_x, y, 0xFFFF);
        display_draw_pixel(bar_x + bar_width - 1, y, 0xFFFF);
    }

    /* Draw fill */
    for (uint32_t x = bar_x + 1; x < bar_x + fill - 1 && x < bar_x + bar_width - 2; x++) {
        for (uint32_t y = bar_y + 1; y < bar_y + 7; y++) {
            display_draw_pixel(x, y, 0x07E0); /* green */
        }
    }
}

/* ========================================================================== */
/*  Error Display                                                             */
/* ========================================================================== */

void display_show_error(int32_t error_code)
{
    display_fill(0xF800); /* Red background */

    char buf[32];
    snprintf(buf, sizeof(buf), "ERR %ld", (long)error_code);
    uint32_t cx = LCD_WIDTH / 2;
    uint32_t cy = LCD_HEIGHT / 2;
    display_draw_text(cx - 24, cy - 8, buf, 0xFFFF, 0xF800);

    const char *msg;
    switch (error_code) {
        case ERR_BAD_COUPLING:      msg = "COUPLING FAIL"; break;
        case ERR_LOW_BATTERY:       msg = "LOW BATTERY"; break;
        case ERR_SD_FAIL:           msg = "SD ERROR"; break;
        case ERR_ADC_SYNC:          msg = "ADC SYNC ERR"; break;
        case ERR_TOMO_SOLVER:       msg = "SOLVER FAIL"; break;
        case ERR_CALIBRATION:       msg = "CALIB ERROR"; break;
        case ERR_BLE_DISCONNECTED:  msg = "BLE LOST"; break;
        case ERR_OVER_TEMPERATURE:  msg = "OVERHEAT"; break;
        case ERR_HV_UNDERVOLTAGE:   msg = "HV LOW"; break;
        case ERR_MUX_FAILED:        msg = "MUX FAIL"; break;
        default:                    msg = "UNKNOWN"; break;
    }
    display_draw_text(cx - 30, cy + 10, msg, 0xFFFF, 0xF800);

    display_draw_text(cx - 48, cy + 30, "Press RESET", 0xFFFF, 0xF800);
}