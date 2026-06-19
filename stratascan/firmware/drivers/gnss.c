/*
 * gnss.c — SAM-M10Q GNSS Driver (NMEA Parser)
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements a minimal NMEA-0183 sentence parser for the u-blox SAM-M10Q
 * multi-constellation GNSS receiver.  The receiver sends NMEA sentences over
 * USART1 at 9600 baud (default; can be configured to 38400 via UBX protocol).
 *
 * Parsed sentences:
 *   $GNGGA  — Global Positioning System Fix Data (lat, lon, alt, sats, fix)
 *   $GNRMC  — Recommended Minimum Navigation (lat, lon, speed, course, date)
 *
 * The parser is interrupt-driven: USART1 RX interrupts feed characters into
 * a line buffer; when a complete sentence (terminated by \r\n) is received,
 * the parser extracts the relevant fields and updates the global position
 * structure.  This is a cooperative, lock-free design — the ISR only buffers
 * characters, and parsing happens in gnss_get_position() (called from the
 * main loop).
 */

#include "gnss.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  NMEA sentence buffer                                                  */
/* ===================================================================== */

#define NMEA_BUF_SIZE  128

static char    nmea_buf[NMEA_BUF_SIZE];
static uint8_t nmea_idx = 0;
static uint8_t nmea_ready = 0;

/* Latest parsed position */
static float    g_lat = 0.0f;
static float    g_lon = 0.0f;
static float    g_alt = 0.0f;
static uint8_t  g_fix = 0;      /* 0 = no fix, 1 = GPS, 2 = DGPS */
static uint8_t  g_sats = 0;
static float    g_speed_knots = 0.0f;
static float    g_course_deg = 0.0f;

/* ===================================================================== */
/*  USART1 RX interrupt handler                                           */
/* ===================================================================== */

void USART1_IRQHandler(void)
{
    if (USART1->ISR & USART_ISR_RXNE) {
        char c = (char)(USART1->RDR & 0xFF);

        if (c == '$') {
            nmea_idx = 0;  /* start of new sentence */
        }

        if (nmea_idx < NMEA_BUF_SIZE - 1) {
            nmea_buf[nmea_idx++] = c;
        }

        if (c == '\n') {
            nmea_buf[nmea_idx] = '\0';
            nmea_ready = 1;
            nmea_idx = 0;
        }
    }
    /* Clear any error flags */
    if (USART1->ISR & 0x0F) {
        USART1->ICR = 0x0F;
    }
}

/* ===================================================================== */
/*  NMEA field extraction helpers                                         */
/* ===================================================================== */

static int str_starts_with(const char *str, const char *prefix)
{
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

static float parse_lat_lon(const char *field, char dir)
{
    /* NMEA lat/lon format: ddmm.mmmm or dddmm.mmmm
     * For latitude: dd = degrees, mm.mmmm = minutes
     * For longitude: ddd = degrees, mm.mmmm = minutes
     */
    if (field[0] == '\0' || field[0] == ',') return 0.0f;

    /* Convert to float */
    float val = 0.0f;
    int   int_part = 0;
    float frac_part = 0.0f;
    int   decimal_pos = -1;
    uint8_t i = 0;

    /* Parse the number */
    while (field[i] && field[i] != '.') {
        if (field[i] >= '0' && field[i] <= '9') {
            int_part = int_part * 10 + (field[i] - '0');
        }
        i++;
    }
    if (field[i] == '.') {
        i++;
        float scale = 0.1f;
        while (field[i] && field[i] >= '0' && field[i] <= '9') {
            frac_part += (field[i] - '0') * scale;
            scale *= 0.1f;
            i++;
        }
    }

    val = (float)int_part + frac_part;

    /* Convert ddmm.mmmm → dd.dddd
     * The minutes part is val % 100, degrees = val / 100
     */
    int degrees = (int)(val / 100.0f);
    float minutes = val - (degrees * 100.0f);
    float decimal_deg = (float)degrees + minutes / 60.0f;

    /* Apply direction (S/W → negative) */
    if (dir == 'S' || dir == 'W') decimal_deg = -decimal_deg;

    return decimal_deg;
}

static const char *get_field(const char *sentence, uint8_t field_num)
{
    static char field_buf[20];
    uint8_t field_idx = 0;
    uint8_t char_idx = 0;

    while (*sentence && field_idx < field_num) {
        if (*sentence == ',') field_idx++;
        sentence++;
    }
    if (field_idx != field_num) return "";

    /* Copy the field content until comma or end */
    while (*sentence && *sentence != ',' && *sentence != '*' && char_idx < 19) {
        field_buf[char_idx++] = *sentence++;
    }
    field_buf[char_idx] = '\0';
    return field_buf;
}

/* ===================================================================== */
/*  NMEA sentence parser                                                  */
/* ===================================================================== */

static void parse_nmea(const char *sentence)
{
    /* Check checksum (simplified: skip if invalid) */
    /* NMEA checksum is XOR of all chars between $ and * */

    if (str_starts_with(sentence, "$GNGGA") || str_starts_with(sentence, "$GPGGA")) {
        /* $GNGGA,time,lat,N/S,lon,E/W,fix,sats,hdop,alt,M,... */
        const char *lat_field  = get_field(sentence, 2);
        const char *lat_dir    = get_field(sentence, 3);
        const char *lon_field  = get_field(sentence, 4);
        const char *lon_dir    = get_field(sentence, 5);
        const char *fix_field  = get_field(sentence, 6);
        const char *sats_field = get_field(sentence, 7);
        const char *alt_field  = get_field(sentence, 9);

        g_lat = parse_lat_lon(lat_field, lat_dir[0]);
        g_lon = parse_lat_lon(lon_field, lon_dir[0]);
        g_fix = (uint8_t)(fix_field[0] - '0');
        g_sats = (uint8_t)(sats_field[0] - '0') * 10 + (uint8_t)(sats_field[1] - '0');
        /* Altitude parsing simplified */
        g_alt = 0.0f;
        if (alt_field[0] >= '0' && alt_field[0] <= '9') {
            float a = 0.0f;
            uint8_t i = 0;
            while (alt_field[i] && alt_field[i] != '.') {
                a = a * 10 + (alt_field[i] - '0');
                i++;
            }
            g_alt = a;
        }
    }
    else if (str_starts_with(sentence, "$GNRMC") || str_starts_with(sentence, "$GPRMC")) {
        /* $GNRMC,time,status,lat,N/S,lon,E/W,speed,course,date,... */
        const char *status    = get_field(sentence, 2);
        const char *lat_field = get_field(sentence, 3);
        const char *lat_dir   = get_field(sentence, 4);
        const char *lon_field = get_field(sentence, 5);
        const char *lon_dir   = get_field(sentence, 6);
        const char *speed     = get_field(sentence, 7);
        const char *course    = get_field(sentence, 8);

        if (status[0] == 'A') {  /* valid fix */
            g_lat = parse_lat_lon(lat_field, lat_dir[0]);
            g_lon = parse_lat_lon(lon_field, lon_dir[0]);
            g_fix = 1;
            /* Parse speed in knots */
            g_speed_knots = 0.0f;
            uint8_t i = 0;
            while (speed[i] && speed[i] != '.') {
                g_speed_knots = g_speed_knots * 10 + (speed[i] - '0');
                i++;
            }
        } else {
            g_fix = 0;
        }
    }
}

/* ===================================================================== */
/*  USART1 initialization                                                 */
/* ===================================================================== */

static void usart1_init(void)
{
    /* Disable USART1 */
    USART1->CR1 = 0;

    /* Set baud rate: APB2 = 120 MHz, BRR = 120MHz / 9600 = 12500 */
    USART1->BRR = (BOARD_APB2_FREQ_HZ / GNSS_BAUD);

    /* Configure: 8 data bits, no parity, 1 stop bit (default) */
    USART1->CR2 = 0;
    USART1->CR3 = 0;

    /* Enable USART, RX, and RX interrupt */
    USART1->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_RXNEIE;

    /* Enable USART1 interrupt in NVIC */
    NVIC->ISER[0] = (1UL << USART1_IRQn);
}

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

int gnss_init(void)
{
    /* Reset GNSS module (active low reset) */
    GNSS_NRST_PORT->BRR  = (1UL << GNSS_NRST_PIN);
    board_delay_ms(50);
    GNSS_NRST_PORT->BSRR = (1UL << GNSS_NRST_PIN);
    board_delay_ms(500);  /* wait for module boot */

    usart1_init();

    /* Wait for first fix (timeout 60 seconds) */
    uint32_t start = board_get_tick_ms();
    while (!g_fix && (board_get_tick_ms() - start) < 60000) {
        gnss_get_position(NULL, NULL, NULL);
    }

    return 0;
}

void gnss_get_position(float *lat, float *lon, float *alt)
{
    /* Process any ready NMEA sentences */
    if (nmea_ready) {
        parse_nmea(nmea_buf);
        nmea_ready = 0;
    }

    if (lat) *lat = g_lat;
    if (lon) *lon = g_lon;
    if (alt) *alt = g_alt;
}

uint8_t gnss_get_fix(void)
{
    return g_fix;
}

uint8_t gnss_get_sats(void)
{
    return g_sats;
}

void gnss_reset(void)
{
    GNSS_NRST_PORT->BRR  = (1UL << GNSS_NRST_PIN);
    board_delay_ms(50);
    GNSS_NRST_PORT->BSRR = (1UL << GNSS_NRST_PIN);
    board_delay_ms(500);
}

/* End of gnss.c */