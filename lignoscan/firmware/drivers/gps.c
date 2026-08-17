/*
 * gps.c — u-blox NEO-M9N GPS Driver Implementation
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Communicates with the u-blox NEO-M9N GPS module via USART3 at 38400 baud.
 * Parses standard NMEA sentences (GGA, RMC, GSA) to extract position,
 * altitude, fix quality, and timestamp information for geotagging scans.
 */

#include "gps.h"
#include "board.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static gps_fix_t g_last_fix;
static char nmea_buffer[128];
static int nmea_idx = 0;

/* ---- Initialize USART3 for GPS communication ---- */
void gps_init(void) {
    /* Enable USART3 clock */
    RCC_APB1LENR |= RCC_APB1LENR_USART3EN;

    /* Configure baud rate: 38400
     * BRR = APB1_CLK / baud = 140MHz / 38400 = 3646 */
    GPS_UART->BRR = (APB1_FREQ / GPS_BAUD);

    /* Enable TX, RX, and UART */
    GPS_UART->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

    memset(&g_last_fix, 0, sizeof(gps_fix_t));
    nmea_idx = 0;
}

/* ---- Parse NMEA GGA sentence (fix data) ---- */
/* Format: $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,h.h,w.w,M,a.a,M,x.x,xxxx*hh */
static void parse_gga(const char *sentence) {
    /* Parse comma-separated fields */
    const char *p = sentence;
    int field = 0;
    char field_buf[20];
    int fi = 0;

    /* NMEA coordinate conversion: ddmm.mmmm → dd.dddddd */
    float lat_raw = 0, lon_raw = 0;
    char lat_dir = 'N', lon_dir = 'E';

    while (*p && *p != '*') {
        if (*p == ',') {
            field_buf[fi] = '\0';
            fi = 0;

            switch (field) {
            case 1: /* Time: hhmmss.ss */
                if (strlen(field_buf) >= 6) {
                    int hh = (field_buf[0]-'0')*10 + (field_buf[1]-'0');
                    int mm = (field_buf[2]-'0')*10 + (field_buf[3]-'0');
                    int ss = (field_buf[4]-'0')*10 + (field_buf[5]-'0');
                    snprintf(g_last_fix.timestamp, sizeof(g_last_fix.timestamp),
                             "T%02d:%02d:%02dZ", hh, mm, ss);
                }
                break;
            case 2: /* Latitude: ddmm.mmmm */
                lat_raw = atof(field_buf);
                break;
            case 3: /* N/S */
                lat_dir = field_buf[0];
                break;
            case 4: /* Longitude: dddmm.mmmm */
                lon_raw = atof(field_buf);
                break;
            case 5: /* E/W */
                lon_dir = field_buf[0];
                break;
            case 6: /* Fix quality */
                g_last_fix.fix_quality = atoi(field_buf);
                break;
            case 7: /* Number of satellites */
                g_last_fix.satellites = atoi(field_buf);
                break;
            case 8: /* HDOP */
                g_last_fix.hdop = (float)atof(field_buf);
                break;
            case 9: /* Altitude */
                g_last_fix.altitude_m = (float)atof(field_buf);
                break;
            }

            field++;
        } else {
            if (fi < sizeof(field_buf) - 1) {
                field_buf[fi++] = *p;
            }
        }
        p++;
    }

    /* Convert NMEA coordinates to decimal degrees */
    if (lat_raw > 0) {
        int lat_deg = (int)(lat_raw / 100);
        float lat_min = lat_raw - (float)lat_deg * 100;
        g_last_fix.latitude = (float)lat_deg + lat_min / 60.0f;
        if (lat_dir == 'S') g_last_fix.latitude = -g_last_fix.latitude;
    }

    if (lon_raw > 0) {
        int lon_deg = (int)(lon_raw / 100);
        float lon_min = lon_raw - (float)lon_deg * 100;
        g_last_fix.longitude = (float)lon_deg + lon_min / 60.0f;
        if (lon_dir == 'W') g_last_fix.longitude = -g_last_fix.longitude;
    }
}

/* ---- Parse NMEA RMC sentence (recommended minimum) ---- */
static void parse_rmc(const char *sentence) {
    /* Extract date for full timestamp: $GPRMC,hhmmss.ss,A,llll.ll,a,...,ddmmyy,... */
    const char *p = sentence;
    int field = 0;
    char field_buf[20];
    int fi = 0;
    char date_str[7] = "";

    while (*p && *p != '*') {
        if (*p == ',') {
            field_buf[fi] = '\0';
            fi = 0;

            if (field == 9 && strlen(field_buf) >= 6) {
                /* Date: ddmmyy */
                memcpy(date_str, field_buf, 6);
                date_str[6] = '\0';

                /* Build full ISO timestamp: YYYY-MM-DDThh:mm:ssZ
                 * NMEA date is ddmmyy → assume year 20yy */
                char full_ts[24];
                const char *time_part = g_last_fix.timestamp; /* T...Z */
                if (time_part[0] == 'T') {
                    snprintf(full_ts, sizeof(full_ts), "20%.2s-%.2s-%.2s%s",
                             &date_str[4], &date_str[2], &date_str[0], time_part);
                    strncpy(g_last_fix.timestamp, full_ts, sizeof(g_last_fix.timestamp));
                    g_last_fix.timestamp[sizeof(g_last_fix.timestamp)-1] = '\0';
                }
            }
            field++;
        } else {
            if (fi < sizeof(field_buf) - 1) {
                field_buf[fi++] = *p;
            }
        }
        p++;
    }
}

/* ---- Process incoming NMEA sentence ---- */
void gps_parse_nmea(const char *sentence) {
    if (strncmp(sentence, "$GPGGA", 6) == 0 ||
        strncmp(sentence, "$GNGGA", 6) == 0) {
        parse_gga(sentence);
    } else if (strncmp(sentence, "$GPRMC", 6) == 0 ||
               strncmp(sentence, "$GNRMC", 6) == 0) {
        parse_rmc(sentence);
    }
}

/* ---- Check if GPS has a valid fix ---- */
int gps_has_fix(void) {
    /* Read any available NMEA data from UART */
    while (GPS_UART->ISR & USART_ISR_RXNE) {
        uint8_t byte = (uint8_t)(GPS_UART->RDR & 0xFF);

        if (byte == '$') {
            nmea_idx = 0;
        }

        if (nmea_idx < (int)sizeof(nmea_buffer) - 1) {
            nmea_buffer[nmea_idx++] = (char)byte;
        }

        if (byte == '\n' && nmea_idx > 0) {
            nmea_buffer[nmea_idx] = '\0';
            gps_parse_nmea(nmea_buffer);
            nmea_idx = 0;
        }
    }

    return (g_last_fix.fix_quality > 0 && g_last_fix.satellites >= 4);
}

/* ---- Get current GPS fix ---- */
void gps_get_fix(gps_fix_t *fix) {
    /* Ensure we have the latest data */
    gps_has_fix();

    memcpy(fix, &g_last_fix, sizeof(gps_fix_t));

    /* If no fix, zero out */
    if (g_last_fix.fix_quality == 0) {
        fix->latitude = 0.0f;
        fix->longitude = 0.0f;
        fix->altitude_m = 0.0f;
        fix->satellites = 0;
    }
}

/* ---- Format GPS timestamp for logging ---- */
void gps_format_timestamp(gps_fix_t *fix, char *buf, int bufsize) {
    if (fix->timestamp[0] != '\0') {
        strncpy(buf, fix->timestamp, bufsize - 1);
        buf[bufsize - 1] = '\0';
    } else {
        /* Fallback: use millisecond counter */
        snprintf(buf, bufsize, "T%lu", (unsigned long)millis());
    }
}

/* EOF — gps.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */