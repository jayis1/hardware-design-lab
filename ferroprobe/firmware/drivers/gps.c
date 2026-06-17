/*
 * gps.c — NEO-M9N GNSS NMEA parser
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Parses NMEA 0183 sentences from the u-blox NEO-M9N GNSS receiver
 * over USART1 at 38400 baud.  Handles GGA, RMC, and GSA sentences
 * to extract position, time, fix quality, and satellite count.
 */

#include "gps.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>
#include <stdlib.h>

/* ===================================================================== */
/*  USART1 init (NEO-M9N at 38400 baud)                                   */
/* ===================================================================== */

#define GPS_RX_BUF_SIZE 256

static char gps_rx_buf[GPS_RX_BUF_SIZE];
static volatile uint16_t gps_rx_idx = 0;
static volatile uint8_t gps_sentence_complete = 0;
static gps_data_t gps_latest;
static volatile uint8_t gps_rx_irq_enabled = 0;

static void gps_usart_init(void)
{
    /* Enable GPIOA and USART1 clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;

    /* PA9 = USART1_TX (AF7), PA10 = USART1_RX (AF7) */
    volatile uint32_t *afrh = (volatile uint32_t *)(GPIOA_BASE + GPIO_AFRH);
    volatile uint32_t *moder = (volatile uint32_t *)(GPIOA_BASE + GPIO_MODER);

    /* PA9 (AF7): index 1 in AFRH (pins 8-15, so pin 9 → shift (9-8)*4 = 4) */
    *afrh &= ~(0xFU << 4);
    *afrh |= ((uint32_t)GPIO_AF7 << 4);
    *moder &= ~(0x3U << 18);
    *moder |= (0x2U << 18);

    /* PA10 (AF7): index 2 in AFRH (pin 10 → shift (10-8)*4 = 8) */
    *afrh &= ~(0xFU << 8);
    *afrh |= ((uint32_t)GPIO_AF7 << 8);
    *moder &= ~(0x3U << 20);
    *moder |= (0x2U << 20);

    /* Configure USART1: 38400 baud, 8N1 */
    USART_REG(USART1_BASE, USART_CR1) = 0;  /* Disable */
    USART_REG(USART1_BASE, USART_BRR) = GPS_BRR_VAL;
    USART_REG(USART1_BASE, USART_CR2) = 0;   /* 1 stop bit */
    USART_REG(USART1_BASE, USART_CR3) = 0;

    /* Enable receiver + RXNE interrupt */
    USART_REG(USART1_BASE, USART_CR1) |= USART_CR1_RE | USART_CR1_RXNEIE;
    USART_REG(USART1_BASE, USART_CR1) |= USART_CR1_UE;  /* Enable USART */

    /* Enable NVIC interrupt for USART1 (IRQ 37) */
    NVIC_ISER0 |= (1U << 37);
    gps_rx_irq_enabled = 1;
}

/* ===================================================================== */
/*  NMEA sentence parsing                                                  */
/* ===================================================================== */

/* Parse a decimal integer from a string, advancing the pointer */
static int32_t parse_int(const char **p)
{
    int32_t val = 0;
    int8_t sign = 1;
    if (**p == '-') { sign = -1; (*p)++; }
    while (**p >= '0' && **p <= '9') {
        val = val * 10 + (**p - '0');
        (*p)++;
    }
    return val * sign;
}

/* Parse a float from a string */
static float parse_float(const char **p)
{
    float val = 0.0f;
    float frac = 0.0f;
    float div = 10.0f;
    int8_t sign = 1;
    if (**p == '-') { sign = -1; (*p)++; }
    while (**p >= '0' && **p <= '9') {
        val = val * 10.0f + (float)(**p - '0');
        (*p)++;
    }
    if (**p == '.') {
        (*p)++;
        while (**p >= '0' && **p <= '9') {
            frac += (float)(**p - '0') / div;
            div *= 10.0f;
            (*p)++;
        }
    }
    return (val + frac) * sign;
}

/* Skip to next comma-delimited field */
static void skip_field(const char **p)
{
    while (**p && **p != ',')
        (*p)++;
    if (**p == ',')
        (*p)++;
}

/* Convert NMEA ddmm.mmmm format to decimal degrees × 10^7 */
static int32_t nmea_to_deg_e7(double coord, char dir)
{
    /* coord is in ddmm.mmmm (lat) or dddmm.mmmm (lon) */
    int32_t deg = (int32_t)(coord / 100.0);
    double min = coord - (double)(deg * 100);
    double decimal = (double)deg + min / 60.0;
    int32_t result = (int32_t)(decimal * 10000000.0);
    if (dir == 'S' || dir == 'W')
        result = -result;
    return result;
}

/* Parse GGA sentence: time, lat, lon, fix quality, sats, altitude */
static void parse_gga(const char *sentence)
{
    /* $GPGGA,hhmmss.ss,llll.ll,N,yyyyy.yy,W,q,ss,h.h,g.g,M,f.f,M,xxxx*cc */
    const char *p = sentence;

    /* Skip $GPGGA, */
    skip_field(&p);  /* sentence type */

    /* Time: hhmmss.ss */
    float time_f = parse_float(&p);
    gps_latest.hour = (uint8_t)(time_f / 10000);
    gps_latest.minute = (uint8_t)((int32_t)(time_f / 100) % 100);
    gps_latest.second = (uint8_t)((int32_t)time_f % 100);
    skip_field(&p);

    /* Latitude: ddmm.mmmm */
    float lat = parse_float(&p);
    skip_field(&p);
    char ns = *p;
    skip_field(&p);

    /* Longitude: dddmm.mmmm */
    float lon = parse_float(&p);
    skip_field(&p);
    char ew = *p;
    skip_field(&p);

    /* Fix quality */
    gps_latest.fix_quality = (uint8_t)parse_int(&p);
    skip_field(&p);

    /* Satellites */
    gps_latest.satellites = (uint8_t)parse_int(&p);
    skip_field(&p);

    /* HDOP */
    skip_field(&p);

    /* Altitude (meters) */
    float alt = parse_float(&p);
    gps_latest.altitude_mm = (int32_t)(alt * 1000.0f);
    skip_field(&p);
    /* Altitude unit (M) */
    skip_field(&p);
    /* Geoid separation */
    skip_field(&p);

    /* Convert lat/lon */
    gps_latest.latitude_e7 = nmea_to_deg_e7((double)lat, ns);
    gps_latest.longitude_e7 = nmea_to_deg_e7((double)lon, ew);

    gps_latest.valid = (gps_latest.fix_quality > 0) ? 1 : 0;
}

/* Parse RMC sentence: date, speed, heading */
static void parse_rmc(const char *sentence)
{
    /* $GPRMC,hhmmss.ss,A,llll.ll,N,yyyyy.yy,W,spd,hdg,ddmmyy,magvar*cc */
    const char *p = sentence;

    skip_field(&p);  /* sentence type */
    skip_field(&p);  /* time */
    skip_field(&p);  /* status */

    /* Lat */
    float lat = parse_float(&p);
    skip_field(&p);
    char ns = *p;
    skip_field(&p);

    /* Lon */
    float lon = parse_float(&p);
    skip_field(&p);
    char ew = *p;
    skip_field(&p);

    /* Speed in knots → mm/s (1 knot = 0.514 m/s = 514 mm/s) */
    float knots = parse_float(&p);
    gps_latest.speed_mm_s = (uint16_t)(knots * 514.0f);
    skip_field(&p);

    /* Heading in degrees → ×10 */
    float hdg = parse_float(&p);
    gps_latest.heading_deg10 = (uint16_t)(hdg * 10.0f);
    skip_field(&p);

    /* Date: ddmmyy */
    int32_t date = parse_int(&p);
    gps_latest.day = (uint8_t)(date / 10000);
    gps_latest.month = (uint8_t)((date / 100) % 100);
    gps_latest.year = 2000 + (uint16_t)(date % 100);

    /* Update lat/lon from RMC too (in case GGA not received) */
    if (ns == 'S' || ns == 'N')
        gps_latest.latitude_e7 = nmea_to_deg_e7((double)lat, ns);
    if (ew == 'E' || ew == 'W')
        gps_latest.longitude_e7 = nmea_to_deg_e7((double)lon, ew);
}

/* Process a complete NMEA sentence */
static void gps_process_sentence(const char *sentence)
{
    if (strncmp(sentence, "$GPGGA", 6) == 0 || strncmp(sentence, "$GNGGA", 6) == 0) {
        parse_gga(sentence);
    } else if (strncmp(sentence, "$GPRMC", 6) == 0 || strncmp(sentence, "$GNRMC", 6) == 0) {
        parse_rmc(sentence);
    }
}

/* ===================================================================== */
/*  Public API                                                             */
/* ===================================================================== */

void gps_init(void)
{
    memset(&gps_latest, 0, sizeof(gps_latest));
    gps_rx_idx = 0;
    gps_sentence_complete = 0;
    gps_usart_init();
}

void USART1_IRQHandler(void)
{
    if (USART_REG(USART1_BASE, USART_ISR) & USART_ISR_RXNE) {
        char c = (char)USART_REG(USART1_BASE, USART_RDR);

        if (c == '$') {
            gps_rx_idx = 0;  /* Start of new sentence */
        }
        if (gps_rx_idx < GPS_RX_BUF_SIZE - 1) {
            gps_rx_buf[gps_rx_idx++] = c;
        }
        if (c == '\n') {
            gps_rx_buf[gps_rx_idx] = '\0';
            gps_sentence_complete = 1;
        }
    }
    /* Clear error flags */
    USART_REG(USART1_BASE, USART_ICR) = 0xFFFFFFFF;
}

void gps_poll(gps_data_t *data)
{
    if (gps_sentence_complete) {
        gps_process_sentence(gps_rx_buf);
        gps_sentence_complete = 0;
    }
    if (data)
        memcpy(data, &gps_latest, sizeof(gps_data_t));
}

uint8_t gps_has_fix(void)
{
    return gps_latest.fix_quality > 0 && gps_latest.satellites >= 4;
}

void gps_get_latest(gps_data_t *data)
{
    if (data)
        memcpy(data, &gps_latest, sizeof(gps_data_t));
}