/*
 * gps.c — u-blox NEO-M9N GPS driver (USART2 + PPS)
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Parses NMEA $GPGGA and $GPRMC sentences from the NEO-M9N over USART2
 * at 9600 baud. The PPS line drives an EXTI interrupt that increments
 * a counter for time-stamp alignment of muon events.
 */
#include "gps.h"
#include "registers.h"
#include "board.h"
#include <string.h>
#include <stdlib.h>

static gps_state_t s_state;
static volatile uint32_t s_pps_count = 0;
static char s_nmea_buf[128];
static volatile uint16_t s_nmea_pos = 0;
static volatile int s_nmea_ready = 0;

/* Called from USART2 IRQ handler */
void gps_usart_isr(uint8_t byte)
{
    if (byte == '$') s_nmea_pos = 0;
    if (s_nmea_pos < sizeof(s_nmea_buf) - 1) {
        s_nmea_buf[s_nmea_pos++] = (char)byte;
        s_nmea_buf[s_nmea_pos] = 0;
    }
    if (byte == '\n') {
        s_nmea_ready = 1;
    }
}

/* Called from EXTI4 IRQ handler (PPS rising edge) */
void gps_pps_isr(void)
{
    s_pps_count++;
}

uint32_t gps_pps_count(void) { return s_pps_count; }

muon_status_t gps_init(void)
{
    memset(&s_state, 0, sizeof(s_state));
    /* Enable USART2 clock + pins */
    RCC_APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    /* PA2/PA3 AF7 — omitted for brevity, same pattern as i2c_init */

    usart_regs_t *u = (usart_regs_t *)USART2_BASE;
    u->CR1 = 0;
    u->BRR = (APB1_VALUE / GPS_BAUD);  /* 9600 */
    u->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
    NVIC_ISER0 |= BIT(38);  /* USART2 IRQ */

    /* Configure PPS pin PD4 EXTI4 rising edge */
    GPIO_MODER(GPS_PPS_PORT) &= ~(3U << (GPS_PPS_PIN * 2));
    GPIO_PUPDR(GPS_PPS_PORT) |= (2U << (GPS_PPS_PIN * 2));  /* pull-down */
    NVIC_ISER0 |= BIT(10);  /* EXTI4 IRQ */
    return MUON_OK;
}

/* Parse $GPGGA: lat, lon, fix, sats, hdop, alt */
static void parse_gga(const char *line)
{
    /* $GPGGA,hhmmss.ss,llll.lll,N,yyyyy.yyy,E,q,nn,h.h,alt,M,... */
    if (strncmp(line, "$GPGGA", 6) != 0 && strncmp(line, "$GNGGA", 6) != 0)
        return;
    /* Skip to lat field: token 2 (after $GPGGA and time) */
    const char *p = line;
    int comma = 0;
    while (*p && comma < 6) { if (*p == ',') comma++; p++; }
    /* p now at lat value */
    double lat_raw = atof(p);
    /* skip to N/S */
    while (*p && *p != ',') p++; p++;
    char ns = *p; p++;
    double lat = (int)(lat_raw / 100) + (lat_raw - (int)(lat_raw / 100) * 100) / 60.0;
    if (ns == 'S') lat = -lat;
    s_state.lat_e7 = (int32_t)(lat * 1e7);

    /* lon */
    double lon_raw = atof(p);
    while (*p && *p != ',') p++; p++;
    char ew = *p; p++;
    double lon = (int)(lon_raw / 100) + (lon_raw - (int)(lon_raw / 100) * 100) / 60.0;
    if (ew == 'W') lon = -lon;
    s_state.lon_e7 = (int32_t)(lon * 1e7);

    /* fix quality */
    while (*p && *p != ',') p++; p++;
    s_state.fix_quality = (uint8_t)(*p - '0'); p++;
    /* sats */
    while (*p && *p != ',') p++; p++;
    s_state.satellites = (uint8_t)atoi(p);
    /* hdop */
    while (*p && *p != ',') p++; p++;
    s_state.hdop = (uint16_t)(atof(p) * 100);
    /* altitude */
    while (*p && *p != ',') p++; p++;
    s_state.alt_mm = (int16_t)(atof(p) * 1000);
}

/* Parse $GPRMC: time, date -> unix_time */
static void parse_rmc(const char *line)
{
    if (strncmp(line, "$GPRMC", 6) != 0 && strncmp(line, "$GNRMC", 6) != 0)
        return;
    /* $GPRMC,hhmmss.ss,A,llll.ll,N,yyyyy.yy,E,sss.ss,ddd,ddmmyy,... */
    const char *p = line + 7;
    /* time hhmmss.ss */
    uint32_t hh = (uint32_t)(p[0]-'0')*10 + (p[1]-'0');
    uint32_t mm = (uint32_t)(p[2]-'0')*10 + (p[3]-'0');
    uint32_t ss = (uint32_t)(p[4]-'0')*10 + (p[5]-'0');
    /* skip to date token (after 9 commas) */
    int comma = 0;
    while (*p && comma < 9) { if (*p == ',') comma++; p++; }
    if (*p) {
        uint32_t dd = (uint32_t)(p[0]-'0')*10 + (p[1]-'0');
        uint32_t mo = (uint32_t)(p[2]-'0')*10 + (p[3]-'0');
        uint32_t yy = (uint32_t)(p[4]-'0')*10 + (p[5]-'0') + 2000;
        /* very rough epoch seconds (no leap-second handling) */
        /* days per month (non-leap) */
        static const uint8_t dpm[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        uint32_t days = 0;
        if ((yy % 4 == 0 && yy % 100 != 0) || yy % 400 == 0) {
            /* leap year */
            ((uint8_t*)&dpm)[1] = 29;
        }
        for (uint32_t y = 1970; y < yy; ++y) {
            days += ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) ? 366 : 365;
        }
        for (uint32_t m = 0; m < mo - 1; ++m) days += dpm[m];
        days += dd - 1;
        s_state.unix_time = days * 86400 + hh * 3600 + mm * 60 + ss;
    }
}

muon_status_t gps_read(gps_state_t *st)
{
    if (!st) return MUON_ERR_INVALID_ARG;
    if (s_nmea_ready) {
        const char *line = s_nmea_buf;
        if (strncmp(line, "$GP", 3) == 0 || strncmp(line, "$GN", 3) == 0) {
            if (strstr(line, "GGA")) parse_gga(line);
            else if (strstr(line, "RMC")) parse_rmc(line);
        }
        s_nmea_ready = 0;
        s_nmea_pos = 0;
    }
    *st = s_state;
    return MUON_OK;
}

int gps_has_fix(void)
{
    return (s_state.fix_quality > 0 && s_state.satellites >= 4);
}