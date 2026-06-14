/*
 * ssd1306.c — SSD1306 OLED 128x64 I2C display driver implementation
 * For Chronos-RTK status display
 */

#include "ssd1306.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ========================================================================== */
/* Display buffer (128 x 64 / 8 = 1024 bytes)                                  */
/* ========================================================================== */
static uint8_t g_display_buf[SSD1306_WIDTH * SSD1306_PAGES];

/* ========================================================================== */
/* I2C1 access helpers                                                          */
/* ========================================================================== */

static I2C_TypeDef * const I2C1 = (I2C_TypeDef *)I2C1_BASE;

/* Wait for I2C bus to be free with timeout */
static int i2c1_wait_bus_free(void)
{
    volatile uint32_t timeout = 100000;
    while (I2C1->ISR & (1U << 15)) {  /* BUSY */
        if (--timeout == 0) return -1;
    }
    return 0;
}

/* Send I2C start condition with device address */
static int i2c1_start(uint8_t addr, uint8_t direction)
{
    i2c1_wait_bus_free();

    /* Configure CR2: address, direction, number of bytes */
    I2C1->CR2 = ((uint32_t)addr << 1)
               | (direction ? (1U << 10) : 0)  /* RD_WRN */
               | (1U << 16);  /* NBYTES = 1, will be updated */

    /* Generate START */
    I2C1->CR2 |= (1U << 13);  /* START */

    /* Wait for TXIS or NACK */
    while (!(I2C1->ISR & ((1U << 1) | (1U << 4))))  /* TXIS or NACKF */
        ;

    if (I2C1->ISR & (1U << 4))  /* NACKF */
        return -1;

    return 0;
}

/* Send a single byte via I2C */
static int i2c1_write_byte(uint8_t data)
{
    volatile uint32_t timeout = 100000;
    while (!(I2C1->ISR & (1U << 1))) {  /* TXIS */
        if (--timeout == 0) return -1;
    }
    I2C1->TXDR = data;
    return 0;
}

/* Generate I2C stop condition */
static void i2c1_stop(void)
{
    I2C1->CR2 |= (1U << 14);  /* STOP */
}

/* Write data to SSD1306 via I2C
 * mode: SSD1306_I2C_CMD_BYTE or SSD1306_I2C_DATA_BYTE
 * data: pointer to data buffer
 * len:  number of bytes
 */
static int ssd1306_i2c_write(uint8_t mode, const uint8_t *data, uint16_t len)
{
    /* For efficiency, we send control byte + data in one I2C transaction */
    i2c1_wait_bus_free();

    /* Start with address, write direction */
    I2C1->CR2 = ((uint32_t)SSD1306_I2C_ADDR << 1)
               | (0U << 10)    /* Write direction */
               | ((len + 1) << 16)  /* NBYTES = len + 1 for control byte */
               | (1U << 25);   /* RELOAD = 0, AUTOEND = 1 */

    /* Generate START */
    I2C1->CR2 |= (1U << 13);  /* START */

    /* Wait for TXIS */
    while (!(I2C1->ISR & (1U << 1)))
        ;

    /* Send control byte */
    I2C1->TXDR = mode;

    /* Send data bytes */
    for (uint16_t i = 0; i < len; i++) {
        while (!(I2C1->ISR & (1U << 1)))
            ;
        I2C1->TXDR = data[i];
    }

    /* Wait for STOPF */
    while (!(I2C1->ISR & (1U << 5)))
        ;

    /* Clear STOPF */
    I2C1->ICR = (1U << 5);

    return 0;
}

/* ========================================================================== */
/* 5x7 font (printable ASCII 0x20-0x7E)                                        */
/* ========================================================================== */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' (space) */
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
    {0x7F,0x09,0x09,0x09,0x01}, /* F */
    {0x3E,0x41,0x49,0x49,0x3A}, /* G */
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
    {0x26,0x49,0x49,0x49,0x32}, /* S */
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
    {0x00,0x7F,0x08,0x14,0x41}, /* k */
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
    {0x00,0x08,0x36,0x41,0x00}, /* { */
    {0x00,0x00,0x7F,0x00,0x00}, /* | */
    {0x00,0x41,0x36,0x08,0x00}, /* } */
    {0x08,0x08,0x2A,0x1C,0x08}, /* -> */
    {0x00,0x00,0x00,0x00,0x00}, /* ~ (fallback to blank) */
};

/* ========================================================================== */
/* Public API implementation                                                    */
/* ========================================================================== */

int ssd1306_init(void)
{
    /* Hardware reset: assert RST low for 10 ms, then high */
    GPIOB->ODR &= ~(1U << PIN_OLED_RST);
    for (volatile int i = 0; i < 170000; i++)  /* ~1 ms */
        ;
    GPIOB->ODR |= (1U << PIN_OLED_RST);
    for (volatile int i = 0; i < 1700000; i++)  /* ~10 ms */
        ;

    /* Initialization sequence for 128x64 OLED */
    const uint8_t init_cmds[] = {
        SSD1306_CMD_SET_DISPLAY_OFF,               /* 0xAE */
        SSD1306_CMD_SET_MUX_RATIO, 0x3F,           /* 0xA8, 63 */
        SSD1306_CMD_SET_DISPLAY_OFFSET, 0x00,       /* 0xD3, 0 */
        SSD1306_CMD_SET_START_LINE | 0x00,          /* 0x40 */
        SSD1306_CMD_SET_SEGMENT_REMAP,               /* 0xA1 (column 127 mapped to SEG0) */
        SSD1306_CMD_SET_COM_SCAN_DIR,               /* 0xC8 (remapped mode) */
        SSD1306_CMD_SET_COM_PINS, 0x12,             /* 0xDA, 0x12 (sequential) */
        SSD1306_CMD_SET_CONTRAST, 0xCF,             /* 0x81, 0xCF */
        SSD1306_CMD_SET_ENTIRE_OFF,                 /* 0xA4 (normal) */
        SSD1306_CMD_SET_NORMAL_DISPLAY,             /* 0xA6 */
        SSD1306_CMD_SET_OSC_FREQ, 0x80,             /* 0xD5, freq=0, div=1 */
        SSD1306_CMD_SET_CHARGE_PUMP, 0x14,          /* 0x8D, 0x14 (enable) */
        SSD1306_CMD_SET_MEMORY_MODE, 0x00,          /* 0x20, 0x00 (horizontal) */
        SSD1306_CMD_SET_DISPLAY_ON,                 /* 0xAF */
    };

    /* Send each command */
    for (uint16_t i = 0; i < sizeof(init_cmds); i++) {
        ssd1306_i2c_write(SSD1306_I2C_CMD_BYTE, &init_cmds[i], 1);
    }

    /* Clear display */
    ssd1306_clear();
    ssd1306_display();

    return 0;
}

void ssd1306_clear(void)
{
    memset(g_display_buf, 0, sizeof(g_display_buf));
}

void ssd1306_display(void)
{
    /* Set column and page address range */
    uint8_t col_cmds[] = {
        SSD1306_CMD_SET_COL_ADDR, 0, SSD1306_WIDTH - 1,
        SSD1306_CMD_SET_PAGE_ADDR, 0, SSD1306_PAGES - 1
    };
    ssd1306_i2c_write(SSD1306_I2C_CMD_BYTE, col_cmds, sizeof(col_cmds));

    /* Send display buffer in chunks (I2C max ~32 bytes per transaction) */
    const uint16_t chunk_size = 16;
    for (uint16_t offset = 0; offset < sizeof(g_display_buf); offset += chunk_size) {
        uint16_t len = chunk_size;
        if (offset + len > sizeof(g_display_buf)) {
            len = sizeof(g_display_buf) - offset;
        }
        ssd1306_i2c_write(SSD1306_I2C_DATA_BYTE, &g_display_buf[offset], len);
    }
}

void ssd1306_set_pixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    uint16_t idx = x + (y / 8) * SSD1306_WIDTH;
    if (color) {
        g_display_buf[idx] |= (1U << (y % 8));
    } else {
        g_display_buf[idx] &= ~(1U << (y % 8));
    }
}

void ssd1306_draw_string(uint8_t x, uint8_t page, const char *str, uint8_t scale)
{
    if (page >= SSD1306_PAGES) return;

    uint8_t cursor_x = x;
    while (*str && cursor_x < SSD1306_WIDTH - FONT_CHAR_WIDTH) {
        char c = *str++;
        if (c < 0x20 || c > 0x7E) c = 0x20;  /* Fallback to space */

        const uint8_t *glyph = font5x7[c - 0x20];
        uint8_t font_width = (scale > 1) ? FONT_CHAR_WIDTH * scale : FONT_CHAR_WIDTH;

        for (uint8_t col = 0; col < 5; col++) {
            uint8_t line = glyph[col];
            for (uint8_t row = 0; row < 8; row++) {
                if (line & (1U << row)) {
                    if (scale > 1) {
                        /* Double-sized: 2x2 pixels per font pixel */
                        for (uint8_t dx = 0; dx < scale; dx++) {
                            for (uint8_t dy = 0; dy < scale; dy++) {
                                ssd1306_set_pixel(
                                    cursor_x + col * scale + dx,
                                    page * 8 + row * scale + dy,
                                    1);
                            }
                        }
                    } else {
                        ssd1306_set_pixel(cursor_x + col, page * 8 + row, 1);
                    }
                }
            }
        }

        cursor_x += font_width;
    }
}

void ssd1306_fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    for (uint8_t px = x; px < x + width && px < SSD1306_WIDTH; px++) {
        for (uint8_t py = y; py < y + height && py < SSD1306_HEIGHT; py++) {
            ssd1306_set_pixel(px, py, color);
        }
    }
}

void ssd1306_draw_hline(uint8_t x, uint8_t page, uint8_t width, uint8_t color)
{
    if (page >= SSD1306_PAGES) return;
    for (uint8_t px = x; px < x + width && px < SSD1306_WIDTH; px++) {
        if (color) {
            g_display_buf[px + page * SSD1306_WIDTH] = 0xFF;
        } else {
            g_display_buf[px + page * SSD1306_WIDTH] = 0x00;
        }
    }
}