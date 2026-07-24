/*
 * oled_drv.c — SH1106 OLED display driver implementation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Drives a 128x64 monochrome OLED with SH1106 controller via SPI2.
 * Maintains a local framebuffer (1 KB) and flushes to display on demand.
 * Includes a 6x8 ASCII font for text rendering.
 */

#include "oled_drv.h"
#include "registers.h"
#include "board.h"
#include <string.h>

/* Framebuffer: 128 columns × 8 pages = 1024 bytes */
static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8];
static bool oled_initialized = false;

/* 6x8 ASCII font (printable chars 32-127, 96 chars × 6 bytes each) */
static const uint8_t font6x8[96][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, /* $ */
    {0x23,0x13,0x08,0x64,0x62,0x00}, /* % */
    {0x36,0x49,0x55,0x22,0x50,0x00}, /* & */
    {0x00,0x05,0x03,0x00,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00,0x00}, /* ) */
    {0x14,0x08,0x3E,0x08,0x14,0x00}, /* * */
    {0x08,0x08,0x3E,0x08,0x08,0x00}, /* + */
    {0x00,0x50,0x30,0x00,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08,0x00}, /* - */
    {0x00,0x60,0x60,0x00,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02,0x00}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46,0x00}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31,0x00}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10,0x00}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39,0x00}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03,0x00}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36,0x00}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E,0x00}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00,0x00}, /* ; */
    {0x08,0x14,0x22,0x41,0x00,0x00}, /* < */
    {0x14,0x14,0x14,0x14,0x14,0x00}, /* = */
    {0x00,0x41,0x22,0x14,0x08,0x00}, /* > */
    {0x02,0x01,0x51,0x09,0x06,0x00}, /* ? */
    {0x3E,0x41,0x59,0x55,0x4E,0x00}, /* @ */
    {0x7C,0x12,0x11,0x12,0x7D,0x00}, /* A */
    {0x7F,0x49,0x49,0x49,0x36,0x00}, /* B */
    {0x3E,0x41,0x41,0x41,0x22,0x00}, /* C */
    {0x7F,0x41,0x41,0x41,0x3E,0x00}, /* D */
    {0x7F,0x49,0x49,0x49,0x41,0x00}, /* E */
    {0x7F,0x09,0x09,0x09,0x01,0x00}, /* F */
    {0x3E,0x41,0x49,0x49,0x7A,0x00}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, /* H */
    {0x00,0x41,0x7F,0x41,0x00,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01,0x00}, /* J */
    {0x7F,0x08,0x14,0x22,0x41,0x00}, /* K */
    {0x7F,0x40,0x40,0x40,0x40,0x00}, /* L */
    {0x7F,0x02,0x0C,0x02,0x7F,0x00}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, /* O */
    {0x7F,0x09,0x09,0x09,0x06,0x00}, /* P */
    {0x1E,0x21,0x21,0x21,0x5E,0x00}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46,0x00}, /* R */
    {0x46,0x49,0x49,0x49,0x31,0x00}, /* S */
    {0x01,0x01,0x7F,0x01,0x01,0x00}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F,0x00}, /* W */
    {0x63,0x14,0x08,0x14,0x63,0x00}, /* X */
    {0x07,0x08,0x70,0x08,0x07,0x00}, /* Y */
    {0x61,0x51,0x49,0x45,0x43,0x00}, /* Z */
    {0x00,0x7F,0x41,0x41,0x00,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20,0x00}, /* backslash */
    {0x00,0x41,0x41,0x7F,0x00,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04,0x00}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40,0x00}, /* _ */
    {0x00,0x01,0x02,0x04,0x00,0x00}, /* ` */
    {0x20,0x54,0x54,0x54,0x78,0x00}, /* a */
    {0x7F,0x48,0x44,0x44,0x38,0x00}, /* b */
    {0x38,0x44,0x44,0x44,0x20,0x00}, /* c */
    {0x38,0x44,0x44,0x48,0x7F,0x00}, /* d */
    {0x38,0x54,0x54,0x54,0x18,0x00}, /* e */
    {0x08,0x7E,0x09,0x01,0x02,0x00}, /* f */
    {0x0C,0x52,0x52,0x52,0x3E,0x00}, /* g */
    {0x7F,0x08,0x04,0x04,0x78,0x00}, /* h */
    {0x00,0x44,0x7D,0x40,0x00,0x00}, /* i */
    {0x20,0x40,0x44,0x3D,0x00,0x00}, /* j */
    {0x7F,0x10,0x28,0x44,0x00,0x00}, /* k */
    {0x00,0x41,0x7F,0x40,0x00,0x00}, /* l */
    {0x7C,0x04,0x18,0x04,0x78,0x00}, /* m */
    {0x7C,0x08,0x04,0x04,0x78,0x00}, /* n */
    {0x38,0x44,0x44,0x44,0x38,0x00}, /* o */
    {0x7C,0x14,0x14,0x14,0x08,0x00}, /* p */
    {0x08,0x14,0x14,0x18,0x7C,0x00}, /* q */
    {0x7C,0x08,0x04,0x04,0x08,0x00}, /* r */
    {0x48,0x54,0x54,0x54,0x20,0x00}, /* s */
    {0x04,0x3F,0x44,0x40,0x20,0x00}, /* t */
    {0x3C,0x40,0x40,0x20,0x7C,0x00}, /* u */
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, /* v */
    {0x3C,0x40,0x20,0x40,0x3C,0x00}, /* w */
    {0x44,0x28,0x10,0x28,0x44,0x00}, /* x */
    {0x0C,0x50,0x50,0x50,0x3C,0x00}, /* y */
    {0x44,0x64,0x54,0x4C,0x44,0x00}, /* z */
    {0x00,0x08,0x36,0x41,0x00,0x00}, /* { */
    {0x00,0x00,0x7F,0x00,0x00,0x00}, /* | */
    {0x00,0x41,0x36,0x08,0x00,0x00}, /* } */
    {0x02,0x01,0x02,0x04,0x02,0x00}, /* ~ */
};

/* ---- Low-level SPI send ---- */

static void spi2_write_byte(uint8_t data)
{
    /* Wait for TX empty */
    while (!(SPI2->SR & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)&SPI2->DR = data;
    /* Wait for RX (indicates transfer complete) */
    while (!(SPI2->SR & SPI_SR_RXNE))
        ;
    (void)SPI2->DR;  /* Flush RX */
}

static void spi2_write_block(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        while (!(SPI2->SR & SPI_SR_TXE))
            ;
        *(volatile uint8_t *)&SPI2->DR = data[i];
    }
    while (SPI2->SR & SPI_SR_BSY)
        ;
    while (SPI2->SR & SPI_SR_RXNE)
        (void)SPI2->DR;  /* Flush */
}

static void oled_write_command(uint8_t cmd)
{
    /* DC=0 for command mode */
    GPIOA->BRR = (1UL << OLED_DC_PIN);
    /* CS low */
    GPIOB->BRR = (1UL << OLED_CS_PIN);
    spi2_write_byte(cmd);
    /* CS high */
    GPIOB->BSRR = (1UL << OLED_CS_PIN);
}

static void oled_write_data(uint8_t data)
{
    /* DC=1 for data mode */
    GPIOA->BSRR = (1UL << OLED_DC_PIN);
    /* CS low */
    GPIOB->BRR = (1UL << OLED_CS_PIN);
    spi2_write_byte(data);
    /* CS high */
    GPIOB->BSRR = (1UL << OLED_CS_PIN);
}

void oled_drv_init(void)
{
    if (oled_initialized)
        return;

    /* Enable clocks */
    RCC_APB1ENR1 |= RCC_APB1ENR1_SPI2;
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOA | RCC_AHB2ENR_GPIOB;

    /* Configure SPI2 pins: PB13=SCK, PB14=MISO, PB15=MOSI */
    GPIOB->MODER &= ~(3UL << (OLED_SCK_PIN * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (OLED_SCK_PIN * 2));
    GPIOB->AFRL &= ~(0xFUL << ((OLED_SCK_PIN - 8) * 4));
    /* Note: pins 13-15 use AFRH, but our struct uses AFRL/AFRH based on pin */
    GPIOB->AFRH &= ~(0xFUL << ((OLED_SCK_PIN - 8) * 4));
    GPIOB->AFRH |= (OLED_AF << ((OLED_SCK_PIN - 8) * 4));
    GPIOB->OSPEEDR |= (GPIO_OSPEED_VHIGH << (OLED_SCK_PIN * 2));

    GPIOB->MODER &= ~(3UL << (OLED_MOSI_PIN * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (OLED_MOSI_PIN * 2));
    GPIOB->AFRH &= ~(0xFUL << ((OLED_MOSI_PIN - 8) * 4));
    GPIOB->AFRH |= (OLED_AF << ((OLED_MOSI_PIN - 8) * 4));

    /* MISO not strictly needed for OLED but configure anyway */
    GPIOB->MODER &= ~(3UL << (OLED_MISO_PIN * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (OLED_MISO_PIN * 2));
    GPIOB->AFRH &= ~(0xFUL << ((OLED_MISO_PIN - 8) * 4));
    GPIOB->AFRH |= (OLED_AF << ((OLED_MISO_PIN - 8) * 4));

    /* Configure CS (PB12), DC (PA8), RST (PA9) as outputs */
    GPIOB->MODER &= ~(3UL << (OLED_CS_PIN * 2));
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << (OLED_CS_PIN * 2));
    GPIOB->BSRR = (1UL << OLED_CS_PIN);  /* CS high */

    GPIOA->MODER &= ~(3UL << (OLED_DC_PIN * 2));
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << (OLED_DC_PIN * 2));

    GPIOA->MODER &= ~(3UL << (OLED_RST_PIN * 2));
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << (OLED_RST_PIN * 2));

    /* Configure SPI2: Master, Mode 0, 8-bit, /8 baud = 21 MHz */
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM
              | SPI_CR1_BR_DIV8 | SPI_CR1_CPOL_LOW | SPI_CR1_CPHA_1EDGE;
    SPI2->CR2 = SPI_CR2_DS_8B | SPI_CR2_SSOE;
    SPI2->CR1 |= (1UL << 6);  /* SPE */

    /* Hardware reset */
    GPIOA->BRR = (1UL << OLED_RST_PIN);
    for (volatile int i = 0; i < 100000; i++)
        ;
    GPIOA->BSRR = (1UL << OLED_RST_PIN);
    for (volatile int i = 0; i < 100000; i++)
        ;

    /* SH1106 initialization sequence */
    oled_write_command(0xAE);  /* Display OFF */
    oled_write_command(0x02);  /* Set column lower nibble start = 2 (for 128px) */
    oled_write_command(0x10);  /* Set column higher nibble start */
    oled_write_command(0x40);  /* Set display start line = 0 */
    oled_write_command(0xB0);  /* Set page address = 0 */
    oled_write_command(0xC8);  /* Set COM output scan direction (remapped) */
    oled_write_command(0x81);  /* Set contrast control */
    oled_write_command(0xCF);  /* Contrast value */
    oled_write_command(0xA1);  /* Set segment remap */
    oled_write_command(0xA6);  /* Normal display (not inverted) */
    oled_write_command(0xA8);  /* Set multiplex ratio */
    oled_write_command(0x3F);  /* 1/64 duty */
    oled_write_command(0xD3);  /* Set display offset */
    oled_write_command(0x00);  /* No offset */
    oled_write_command(0xD5);  /* Set osc frequency */
    oled_write_command(0x80);  /* Default */
    oled_write_command(0xD9);  /* Set pre-charge period */
    oled_write_command(0xF1);  /* Phase 1=1, Phase 2=15 */
    oled_write_command(0xDA);  /* Set COM pins hardware config */
    oled_write_command(0x12);  /* Alternative COM pin config */
    oled_write_command(0xDB);  /* Set VCOMH deselect level */
    oled_write_command(0x40);  /* ~0.77 Vcc */
    oled_write_command(0x8D);  /* Set charge pump */
    oled_write_command(0x14);  /* Enable charge pump */
    oled_write_command(0xAF);  /* Display ON */

    oled_clear();
    oled_flush();

    oled_initialized = true;
}

void oled_clear(void)
{
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_flush(void)
{
    for (int page = 0; page < OLED_PAGES; page++) {
        oled_write_command(0xB0 + page);  /* Set page address */
        oled_write_command(0x02);          /* Column low nibble (SH1106 offset) */
        oled_write_command(0x10);          /* Column high nibble */

        /* Set DC=1 for data mode */
        GPIOA->BSRR = (1UL << OLED_DC_PIN);
        GPIOB->BRR = (1UL << OLED_CS_PIN);

        spi2_write_block(&oled_buffer[page * OLED_WIDTH], OLED_WIDTH);

        GPIOB->BSRR = (1UL << OLED_CS_PIN);
    }
}

void oled_set_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT)
        return;

    int page = y / 8;
    int bit = y % 8;

    if (on)
        oled_buffer[page * OLED_WIDTH + x] |= (1UL << bit);
    else
        oled_buffer[page * OLED_WIDTH + x] &= ~(1UL << bit);
}

int oled_draw_char(int x, int y, char c, bool invert)
{
    if (c < 32 || c > 127)
        c = '?';
    int idx = c - 32;

    for (int col = 0; col < 6; col++) {
        uint8_t col_data = font6x8[idx][col];
        if (invert)
            col_data = ~col_data;
        for (int row = 0; row < 8; row++) {
            bool pixel = (col_data >> row) & 1;
            oled_set_pixel(x + col, y + row, pixel);
        }
    }
    return x + 6;
}

void oled_draw_string(int x, int y, const char *str, bool invert)
{
    int cx = x;
    while (*str) {
        cx = oled_draw_char(cx, y, *str, invert);
        str++;
    }
}

void oled_draw_hline(int x, int y, int w, bool on)
{
    for (int i = 0; i < w; i++)
        oled_set_pixel(x + i, y, on);
}

void oled_draw_vline(int x, int y, int h, bool on)
{
    for (int i = 0; i < h; i++)
        oled_set_pixel(x, y + i, on);
}

void oled_draw_rect(int x, int y, int w, int h, bool on)
{
    oled_draw_hline(x, y, w, on);
    oled_draw_hline(x, y + h - 1, w, on);
    oled_draw_vline(x, y, h, on);
    oled_draw_vline(x + w - 1, y, h, on);
}

void oled_fill_rect(int x, int y, int w, int h, bool on)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            oled_set_pixel(x + i, y + j, on);
}

void oled_draw_progress(int x, int y, int w, int h, int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    oled_draw_rect(x, y, w, h, true);
    int fill_w = ((w - 2) * percent) / 100;
    oled_fill_rect(x + 1, y + 1, fill_w, h - 2, true);
}

void oled_set_display(bool on)
{
    oled_write_command(on ? 0xAF : 0xAE);
}

void oled_set_invert(bool on)
{
    oled_write_command(on ? 0xA7 : 0xA6);
}

int oled_get_width(void)  { return OLED_WIDTH; }
int oled_get_height(void) { return OLED_HEIGHT; }