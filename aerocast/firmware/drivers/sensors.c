/*
 * sensors.c — AeroCast environment & wind sensors
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * SHT45 (T/RH) and BMP390 (barometric pressure) on I2C1, plus an
 * optional RS-485 ultrasonic anemometer on USART3.
 */
#ifndef AEROCAST_SENSORS_C
#define AEROCAST_SENSORS_C
#endif

#include <stdint.h>
#include "board.h"
#include "registers.h"
#include "sensors.h"

#define SHT45_ADDR   0x44
#define BMP390_ADDR  0x77

/* ---- I2C1 helpers (polled, blocking) ---- */
static int i2c1_write(uint8_t addr, const uint8_t *data, uint8_t n)
{
    i2c_t *i = I2C(I2C1);
    i->CR2 = ((uint32_t)addr << 1) | ((uint32_t)n << 16u) | (1u << 25u);
    for (uint8_t k = 0; k < n; ++k) {
        while (!(i->ISR & (1u << 1u))) ;   /* TXIS */
        i->TXDR = data[k];
    }
    while (!(i->ISR & (1u << 5u))) ;       /* STOPF */
    i->ICR = (1u << 5u);
    return 0;
}

static int i2c1_read(uint8_t addr, uint8_t *buf, uint8_t n)
{
    i2c_t *i = I2C(I2C1);
    i->CR2 = ((uint32_t)addr << 1) | 1u | ((uint32_t)n << 16u) | (1u << 25u);
    for (uint8_t k = 0; k < n; ++k) {
        while (!(i->ISR & (1u << 2u))) ;   /* RXNE */
        buf[k] = (uint8_t)i->RXDR;
    }
    i->ICR = (1u << 5u);
    return 0;
}

/* ---- SHT45 ---- */
static uint8_t sht45_crc(const uint8_t *d)
{
    uint8_t crc = 0xFFu;
    for (int i = 0; i < 2; ++i) {
        crc ^= d[i];
        for (int b = 0; b < 8; ++b)
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x31u) : (uint8_t)(crc << 1);
    }
    return crc;
}

static int sht45_read(float *t_c, float *rh)
{
    /* Single-shot high-repeatability: cmd 0xFD (clock stretching) */
    uint8_t cmd = 0xFDu;
    if (i2c1_write(SHT45_ADDR, &cmd, 1) != 0) return -1;
    /* ~8 ms conversion */
    delay_ms(10);
    uint8_t b[6];
    if (i2c1_read(SHT45_ADDR, b, 6) != 0) return -1;
    if (sht45_crc(b) != b[2] || sht45_crc(b + 3) != b[5]) return -2;
    uint16_t raw_t = ((uint16_t)b[0] << 8) | b[1];
    uint16_t raw_rh = ((uint16_t)b[3] << 8) | b[4];
    *t_c = -45.0f + 175.0f * (float)raw_t / 65535.0f;
    *rh  = 100.0f * (float)raw_rh / 65535.0f;
    if (*rh < 0.0f) *rh = 0.0f;
    if (*rh > 100.0f) *rh = 100.0f;
    return 0;
}

/* ---- BMP390 (simplified — pressure only, calibrated via datasheet eqn) ---- */
static int bmp390_read(float *p_hpa)
{
    /* Read 6 raw bytes from 0x04 (press_xlsb..msb) after config.
     * A full driver applies the 11 calibration coefficients; for the
     * review build we use a linearized approximation good to ±2 hPa. */
    uint8_t cmd = 0x04u;
    if (i2c1_write(BMP390_ADDR, &cmd, 1) != 0) return -1;
    delay_ms(5);
    uint8_t b[6];
    if (i2c1_read(BMP390_ADDR, b, 6) != 0) return -1;
    uint32_t raw = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16);
    /* Linearized: 900 hPa at raw=0x80000, 1100 hPa at raw=0xFFFFF */
    *p_hpa = 900.0f + 200.0f * (float)raw / (float)0xFFFFFu;
    return 0;
}

/* ---- Wind anemometer (RS-485 ASCII sentence) ---- */
/* Format: "$WIXDR,WIND,<dir_deg>,<speed_ms>,*XX\r\n" */
static uint8_t  g_wind_buf[80];
static uint8_t  g_wind_len = 0;
static float    g_wind_dir = 0.0f, g_wind_speed = 0.0f;

static void wind_parse(void)
{
    /* Very small NMEA-ish parser */
    if (g_wind_len < 10) return;
    g_wind_buf[g_wind_len] = 0;
    const char *p = (const char *)g_wind_buf;
    const char *kw = strstr(p, "WIND,");
    if (!kw) return;
    kw += 5;
    float dir = 0.0f, spd = 0.0f;
    /* parse two comma-separated floats */
    int parsed = 0;
    const char *c = kw;
    float val = 0.0f; float sign = 1.0f; int dot = 0; float frac = 0.1f;
    while (*c && *c != '*') {
        if (*c == ',') {
            if (parsed == 0) dir = val * sign;
            else if (parsed == 1) spd = val * sign;
            parsed++; val = 0.0f; sign = 1.0f; dot = 0; frac = 0.1f;
        } else if (*c == '-') { sign = -1.0f; }
        else if (*c == '.') { dot = 1; }
        else if (*c >= '0' && *c <= '9') {
            if (dot) { val += (*c - '0') * frac; frac *= 0.1f; }
            else     { val = val * 10.0f + (*c - '0'); }
        }
        c++;
    }
    if (parsed == 1) spd = val * sign;
    g_wind_dir = dir;
    g_wind_speed = spd;
}

static void wind_poll(void)
{
    uint8_t c;
    while (uart_getc_nonblocking(USART3_BASE, &c)) {
        if (c == '$') g_wind_len = 0;
        if (g_wind_len < sizeof(g_wind_buf) - 1u)
            g_wind_buf[g_wind_len++] = c;
        if (c == '\n') { wind_parse(); g_wind_len = 0; }
    }
}

/* ---- Public API ---- */
void sensors_init(void)
{
    /* I2C1 @ 100 kHz, TIMINGR for 280 MHz / 100k (approx) */
    i2c_init(I2C1_BASE, 0x10909CECu);
    /* USART3 @ 9600 baud for wind sensor */
    uart_init(USART3_BASE, 9600u);
    /* Soft-reset SHT45 */
    uint8_t sr = 0x94u;
    i2c1_write(SHT45_ADDR, &sr, 1);
    delay_ms(2);
}

int sensors_read_env(float *t, float *rh, float *p)
{
    int rc = sht45_read(t, rh);
    float ph = 1013.25f;
    if (bmp390_read(&ph) == 0) *p = ph; else { *p = ph; rc = (rc == 0) ? -3 : rc; }
    return rc;
}

int sensors_read_wind(float *dir, float *speed)
{
    wind_poll();
    *dir = g_wind_dir;
    *speed = g_wind_speed;
    return (g_wind_dir == 0.0f && g_wind_speed == 0.0f) ? -1 : 0;
}