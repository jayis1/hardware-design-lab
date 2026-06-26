/*
 * gps_discipline.c — NEO-M9N GPS UART + 1-PPS PLL for time sync
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The GPS is duty-cycled: powered 5 min every hour. While fixed, the 1-PPS
 * edge discipline a software PLL that steers the local 1 µs counter (driven
 * by the 100 ppb TCXO) to UTC. When the GPS is off, the local counter
 * free-runs with the TCXO drift (< 2 ppm).
 */

#include "gps_discipline.h"
#include "board.h"
#include <string.h>
#include <stdlib.h>

/* ---- State --------------------------------------------------------------- */
static bool     g_powered     = false;
static bool     g_fixed       = false;
static uint32_t g_utc_ms      = 0;
static int32_t  g_pps_offset_us = 0;
static float    g_lat = 0, g_lon = 0, g_hdop = 99.9f;

/* Software PLL state */
static float    g_pll_phase   = 0;   /* accumulated phase error (µs) */
static float    g_pll_freq    = 1.0f;/* frequency correction factor */
#define PLL_KP   0.05f
#define PLL_KI   0.001f
#define PLL_KD   0.0f

/* Local monotonic µs counter (driven by SysTick / TIM2) */
static volatile uint32_t g_local_us = 0;
static volatile uint32_t g_local_us_last_pps = 0;

/* NMEA parser */
static char     g_nmea[128];
static uint8_t  g_nmea_idx = 0;

/* Forward declaration for NMEA parser (defined below) */
void gps_parse_nmea(const char *s);

/* ---- UART ISR (one byte at a time) -------------------------------------- */
void gps_uart_isr(void)
{
    char c = 0; /* read from USART1 DR in real build */
    if (c == '$') { g_nmea_idx = 0; }
    if (g_nmea_idx < sizeof(g_nmea) - 1) {
        g_nmea[g_nmea_idx++] = c;
    }
    if (c == '\n') {
        g_nmea[g_nmea_idx] = '\0';
        gps_parse_nmea(g_nmea);
        g_nmea_idx = 0;
    }
}

/* ---- Minimal NMEA parser (RMC + GGA) ------------------------------------ */
static float parse_coord(const char *s, char dir)
{
    /* NMEA format: ddmm.mmmm for lat, dddmm.mmmm for lon */
    float raw = atof(s);
    int   deg;
    float min;
    if (dir == 'N' || dir == 'S') {
        deg = (int)(raw / 100);
        min = raw - deg * 100;
    } else {
        deg = (int)(raw / 100);
        min = raw - deg * 100;
    }
    float v = deg + min / 60.0f;
    if (dir == 'S' || dir == 'W') v = -v;
    return v;
}

void gps_parse_nmea(const char *s)
{
    /* $GPRMC,hhmmss.ss,A,xxxx.xxxx,N,xxxxx.xxxx,E,... */
    if (strncmp(s, "$GPRMC", 6) == 0 || strncmp(s, "$GNRMC", 6) == 0) {
        /* split by comma */
        char buf[128];
        strncpy(buf, s, sizeof(buf)-1); buf[127] = '\0';
        char *p = strtok(buf, ",");
        int field = 0;
        char time_s[16] = {0}, status = 'V';
        while (p) {
            switch (field) {
                case 1: strncpy(time_s, p, sizeof(time_s)-1); break;
                case 2: status = p[0]; break;
                default: break;
            }
            p = strtok(NULL, ",");
            field++;
        }
        if (status == 'A') {
            g_fixed = true;
            /* Decode hhmmss.ss into UTC ms */
            if (strlen(time_s) >= 6) {
                uint32_t h = (time_s[0]-'0')*10 + (time_s[1]-'0');
                uint32_t m = (time_s[2]-'0')*10 + (time_s[3]-'0');
                uint32_t sec = (time_s[4]-'0')*10 + (time_s[5]-'0');
                g_utc_ms = (h * 3600 + m * 60 + sec) * 1000;
            }
        }
    }
    if (strncmp(s, "$GPGGA", 6) == 0 || strncmp(s, "$GNGGA", 6) == 0) {
        char buf[128];
        strncpy(buf, s, sizeof(buf)-1); buf[127] = '\0';
        char *p = strtok(buf, ",");
        int field = 0;
        char lat_s[16] = {0}, lon_s[16] = {0};
        char lat_dir = 'N', lon_dir = 'E';
        char hdop_s[8] = {0};
        while (p) {
            switch (field) {
                case 2: strncpy(lat_s, p, sizeof(lat_s)-1); break;
                case 3: lat_dir = p[0]; break;
                case 4: strncpy(lon_s, p, sizeof(lon_s)-1); break;
                case 5: lon_dir = p[0]; break;
                case 8: strncpy(hdop_s, p, sizeof(hdop_s)-1); break;
                default: break;
            }
            p = strtok(NULL, ",");
            field++;
        }
        if (lat_s[0] && lon_s[0]) {
            g_lat = parse_coord(lat_s, lat_dir);
            g_lon = parse_coord(lon_s, lon_dir);
            if (hdop_s[0]) g_hdop = atof(hdop_s);
        }
    }
}

/* ---- 1-PPS ISR ---------------------------------------------------------- */
void gps_pps_isr(void)
{
    uint32_t now = g_local_us;
    uint32_t elapsed = now - g_local_us_last_pps;
    g_local_us_last_pps = now;

    /* Expected: exactly 1,000,000 µs between PPS edges.
       Error = elapsed - 1,000,000. */
    int32_t err = (int32_t)elapsed - 1000000;
    g_pps_offset_us = err;

    /* PI loop steers g_pll_freq (applied to local_us increment in 1 Hz tick) */
    g_pll_phase += (float)err;
    g_pll_freq   = 1.0f + PLL_KP * (float)err / 1000000.0f
                       + PLL_KI * g_pll_phase / 1000000.0f;

    /* Snap UTC ms to the PPS edge */
    if (g_fixed) g_utc_ms = (g_utc_ms / 1000) * 1000;   /* align to whole second */
}

/* ---- 1 Hz RTC tick ------------------------------------------------------ */
void gps_tick_1hz(void)
{
    /* Advance local counter by 1,000,000 µs adjusted by PLL freq factor */
    uint32_t inc = (uint32_t)(1000000.0f * g_pll_freq);
    g_local_us += inc;

    /* Advance UTC estimate when GPS is off */
    if (!g_powered || !g_fixed) {
        g_utc_ms += 1000;
    }
}

/* ---- Public API --------------------------------------------------------- */
void gps_init(void)
{
    g_powered = false;
    g_fixed   = false;
    g_utc_ms  = 0;
    g_pps_offset_us = 0;
    g_pll_phase = 0;
    g_pll_freq  = 1.0f;
    g_nmea_idx  = 0;
}

void gps_power_on(void)
{
    /* Drive GPS_EN_PIN high */
    g_powered = true;
}

void gps_power_off(void)
{
    /* Drive GPS_EN_PIN low */
    g_powered = false;
    g_fixed   = false;
}

bool     gps_is_fixed(void)        { return g_fixed; }
uint32_t gps_utc_ms(void)          { return g_utc_ms; }
int32_t  gps_pps_offset_us(void)   { return g_pps_offset_us; }
float    gps_lat(void)             { return g_lat; }
float    gps_lon(void)             { return g_lon; }
float    gps_hdop(void)            { return g_hdop; }