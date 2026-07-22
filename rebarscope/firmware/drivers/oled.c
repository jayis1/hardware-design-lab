/*
 * drivers/oled.c — SSD1306 128x64 monochrome OLED framebuffer
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "../board.h"
#include "../registers.h"
#include "oled.h"

static uint8_t fb[128 * 64 / 8];
static uint8_t dirty = 1;

static void oled_delay_us(uint32_t us)
{
    volatile uint32_t n = us * 20u;
    while (n--) __asm volatile("nop");
}

static void spi1_cs_oled_low(void)   { GPIOB->BRR = (1u << 1); }
static void spi1_cs_oled_high(void)  { GPIOB->BSRR = (1u << 1); }

static void spi1_xfer_oled(uint8_t t)
{
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    *(volatile uint8_t *)&SPI1->DR = t;
    while (!(SPI1->SR & SPI_SR_RXNE)) ;
    (void)*(volatile uint8_t *)&SPI1->DR;
}

static void oled_write_cmd(uint8_t c)
{
    spi1_cs_oled_low();
    /* DC = 0 → command */
    GPIOB->BRR = (1u << 2);
    spi1_xfer_oled(c);
    spi1_cs_oled_high();
}

static void oled_write_data(uint8_t d)
{
    spi1_cs_oled_low();
    /* DC = 1 → data */
    GPIOB->BSRR = (1u << 2);
    spi1_xfer_oled(d);
    spi1_cs_oled_high();
}

void oled_init(void)
{
    pwr_gate_enable(PIN_PWR_OLED, 1);
    oled_delay_us(1000);
    /* Reset */
    GPIOA->BRR = (1u << 8);
    oled_delay_us(1000);
    GPIOA->BSRR = (1u << 8);
    oled_delay_us(1000);

    oled_write_cmd(0xAE);   /* display off */
    oled_write_cmd(0xD5); oled_write_cmd(0x80);  /* clock divide */
    oled_write_cmd(0xA8); oled_write_cmd(0x3F);   /* multiplex 1/64 */
    oled_write_cmd(0xD3); oled_write_cmd(0x00);   /* display offset */
    oled_write_cmd(0x40);                        /* start line 0 */
    oled_write_cmd(0x8D); oled_write_cmd(0x14);  /* charge pump on */
    oled_write_cmd(0x20); oled_write_cmd(0x00);  /* horizontal addressing */
    oled_write_cmd(0xA1);                        /* segment remap */
    oled_write_cmd(0xC8);                        /* COM scan reverse */
    oled_write_cmd(0xDA); oled_write_cmd(0x12);  /* COM pins */
    oled_write_cmd(0x81); oled_write_cmd(0xCF);  /* contrast */
    oled_write_cmd(0xD9); oled_write_cmd(0xF1);  /* pre-charge */
    oled_write_cmd(0xDB); oled_write_cmd(0x40);  /* VCOMH */
    oled_write_cmd(0xA4);                        /* display RAM */
    oled_write_cmd(0xA6);                        /* normal (not inverted) */
    oled_write_cmd(0xAF);                        /* display on */

    oled_clear();
    oled_flush();
}

void oled_clear(void)
{
    for (uint16_t i = 0; i < sizeof(fb); i++) fb[i] = 0;
    dirty = 1;
}

void oled_set_pixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= 128 || y >= 64) return;
    uint16_t idx = x + (y / 8) * 128;
    if (on) fb[idx] |=  (1u << (y & 7));
    else    fb[idx] &= ~(1u << (y & 7));
    dirty = 1;
}

void oled_flush(void)
{
    if (!dirty) return;
    oled_write_cmd(0x21); oled_write_cmd(0); oled_write_cmd(127);
    oled_write_cmd(0x22); oled_write_cmd(0); oled_write_cmd(7);
    for (uint16_t i = 0; i < sizeof(fb); i++) oled_write_data(fb[i]);
    dirty = 0;
}

void oled_draw_string(uint8_t x, uint8_t y, const char *s, const uint8_t *font, uint8_t h)
{
    /* Generic stub: uses font pointer (application supplies a 5x7 or 8x16 font) */
    while (*s && x < 128 - 6) {
        uint8_t ch = (uint8_t)*s;
        for (uint8_t cx = 0; cx < 5; cx++) {
            uint8_t col = font[(ch - 32) * 5 + cx];
            for (uint8_t cy = 0; cy < 7; cy++) {
                oled_set_pixel(x + cx, y + cy, (col >> cy) & 1);
            }
        }
        x += 6;
        s++;
    }
}

void oled_draw_hline(uint8_t x1, uint8_t x2, uint8_t y)
{
    if (x2 < x1) { uint8_t t = x1; x1 = x2; x2 = t; }
    for (uint8_t x = x1; x <= x2; x++) oled_set_pixel(x, y, 1);
}

void oled_draw_bar(uint8_t x, uint8_t y, uint8_t w, uint8_t pct)
{
    if (pct > 100) pct = 100;
    uint8_t filled = (w * pct) / 100u;
    for (uint8_t i = 0; i < w; i++)
        oled_draw_hline(x, x + i, y);
}

void oled_powerdown(void)
{
    oled_write_cmd(0xAE);
    pwr_gate_enable(PIN_PWR_OLED, 0);
}