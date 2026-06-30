/*
 * drivers/gnss.c — u-blox NEO-M9N GNSS driver for SpectraPest
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Interfaces with the u-blox NEO-M9N multi-constellation GNSS receiver
 * via USART2 at 38400 baud. The module outputs NMEA 0183 sentences
 * (GGA, RMC, GSV, GSA) at 1 Hz by default.
 *
 * This driver implements a minimal NMEA parser that extracts latitude,
 * longitude, altitude, fix quality, satellite count, and HDOP from GGA
 * sentences, and timestamp from RMC sentences. The GNSS is powered down
 * between detection cycles to save energy (cold-start is <26s but warm
 * start is typically <2s, which is acceptable for a 60s detection cycle).
 */

#include "gnss.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>
#include <stdlib.h>

/* NMEA sentence buffer */
#define NMEA_MAX_LEN  128
static char nmea_buffer[NMEA_MAX_LEN];
static uint8_t nmea_index = 0;
static uint8_t nmea_in_sentence = 0;

/* Latest parsed fix */
static gnss_fix_t g_last_fix;
static uint8_t g_has_fix = 0;

/* ----------------------------------------------------------------- *
 *  USART2 interrupt handler — receives NMEA characters
 * ----------------------------------------------------------------- */
void USART2_IRQHandler(void) {
    if (USART2->ISR & USART_ISR_RXNE) {
        char c = (char)USART2->RDR;

        if (c == '$') {
            /* Start of new NMEA sentence */
            nmea_index = 0;
            nmea_in_sentence = 1;
            nmea_buffer[nmea_index++] = c;
        } else if (nmea_in_sentence) {
            if (c == '\n' || nmea_index >= NMEA_MAX_LEN - 1) {
                /* End of sentence */
                nmea_buffer[nmea_index] = '\0';
                nmea_in_sentence = 0;

                /* Parse based on sentence type */
                if (strncmp(&nmea_buffer[3], "GGA", 3) == 0) {
                    gnss_parse_gga(nmea_buffer, &g_last_fix);
                    g_has_fix = 1;
                } else if (strncmp(&nmea_buffer[3], "RMC", 3) == 0) {
                    gnss_parse_rmc(nmea_buffer, &g_last_fix);
                }
            } else {
                nmea_buffer[nmea_index++] = c;
            }
        }
    }
}

/* ----------------------------------------------------------------- *
 *  Initialize GNSS
 * ----------------------------------------------------------------- */
int gnss_init(void) {
    /* Enable USART2 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    /* Configure USART2 pins: TX=PA2, RX=PA3 */
    gpio_set_mode(GPIOA, 2, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 2, AF_USART2_TX);
    gpio_set_ospeed(GPIOA, 2, GPIO_SPEED_HIGH);

    gpio_set_mode(GPIOA, 3, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 3, AF_USART2_RX);

    /* Configure GNSS enable pin */
    gpio_set_mode(GNSS_EN_PORT, GNSS_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_write(GNSS_EN_PORT, GNSS_EN_PIN, 1);  /* Active high enable */

    /* Configure USART2: 38400 baud, 8N1 */
    USART2->BRR = PCLK1_FREQ / 38400;  /* 120 MHz / 38400 = 3125 */
    USART2->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_RXNEIE;

    /* Enable NVIC for USART2 */
    NVIC_ISER0 |= (1 << IRQ_USART2);

    /* Wait for first fix (up to 30 seconds cold start) */
    g_has_fix = 0;
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Parse NMEA coordinate (ddmm.mmmm format) to decimal degrees
 * ----------------------------------------------------------------- */
static float parse_nmea_coord(const char *str, char dir) {
    float raw = atof(str);
    int   degrees = (int)(raw / 100.0f);
    float minutes = raw - (float)degrees * 100.0f;
    float decimal = (float)degrees + minutes / 60.0f;

    if (dir == 'S' || dir == 'W') decimal = -decimal;
    return decimal;
}

/* ----------------------------------------------------------------- *
 *  Parse GGA sentence
 *  $xxGGA,hhmmss.ss,ddmm.mmmm,N,dddmm.mmmm,W,fix,sv,hdop,alt,M,sep,M,age,ref*cs
 * ----------------------------------------------------------------- */
int gnss_parse_gga(const char *sentence, gnss_fix_t *out) {
    if (!sentence || !out) return -1;
    if (strncmp(sentence, "$", 1) != 0) return -1;

    /* Make a working copy */
    char buf[NMEA_MAX_LEN];
    strncpy(buf, sentence, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Split by commas */
    char *fields[15];
    int field_count = 0;
    char *tok = buf;
    char *p = buf;

    while (*p && field_count < 15) {
        if (*p == ',') {
            *p = '\0';
            fields[field_count++] = tok;
            tok = p + 1;
        }
        p++;
    }
    fields[field_count++] = tok;

    /* GGA fields:
     * 0: $xxGGA
     * 1: time (hhmmss.ss)
     * 2: latitude (ddmm.mmmm)
     * 3: N/S
     * 4: longitude (dddmm.mmmm)
     * 5: E/W
     * 6: fix quality (0=none, 1=GPS, 2=DGPS)
     * 7: satellites
     * 8: HDOP
     * 9: altitude
     * 10: altitude unit (M)
     * 11: geoid separation
     * 12: unit
     * 13: age of differential
     * 14: differential reference station ID
     */

    if (field_count < 10) return -1;

    /* Parse fields if non-empty */
    if (fields[2][0] && fields[3][0]) {
        out->latitude = parse_nmea_coord(fields[2], fields[3][0]);
    }
    if (fields[4][0] && fields[5][0]) {
        out->longitude = parse_nmea_coord(fields[4], fields[5][0]);
    }
    if (fields[6][0]) {
        out->fix_type = (uint8_t)atoi(fields[6]);
    }
    if (fields[7][0]) {
        out->satellites = (uint8_t)atoi(fields[7]);
    }
    if (fields[8][0]) {
        out->hdop = (float)atof(fields[8]);
    }
    if (fields[9][0]) {
        out->altitude_m = (float)atof(fields[9]);
    }

    return 0;
}

/* ----------------------------------------------------------------- *
 *  Parse RMC sentence (time + date)
 *  $xxRMC,hhmmss.ss,A,ddmm.mmmm,N,dddmm.mmmm,W,speed,track,ddmmyy,..*cs
 * ----------------------------------------------------------------- */
int gnss_parse_rmc(const char *sentence, gnss_fix_t *out) {
    if (!sentence || !out) return -1;

    char buf[NMEA_MAX_LEN];
    strncpy(buf, sentence, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *fields[12];
    int field_count = 0;
    char *tok = buf;
    char *p = buf;

    while (*p && field_count < 12) {
        if (*p == ',') {
            *p = '\0';
            fields[field_count++] = tok;
            tok = p + 1;
        }
        p++;
    }
    fields[field_count++] = tok;

    if (field_count < 10) return -1;

    /* RMC field 2: status (A=active, V=void) */
    if (fields[2][0] == 'A') {
        out->fix_type = (out->fix_type > 0) ? out->fix_type : 2;
    }

    /* Parse date+time into epoch-like timestamp (simplified) */
    if (fields[1][0] && fields[9][0]) {
        /* hhmmss.ss */
        int hh = (fields[1][0] - '0') * 10 + (fields[1][1] - '0');
        int mm = (fields[1][2] - '0') * 10 + (fields[1][3] - '0');
        int ss = (fields[1][4] - '0') * 10 + (fields[1][5] - '0');

        /* ddmmyy */
        int day = (fields[9][0] - '0') * 10 + (fields[9][1] - '0');
        int mon = (fields[9][2] - '0') * 10 + (fields[9][3] - '0');
        int year = 2000 + (fields[9][4] - '0') * 10 + (fields[9][5] - '0');

        /* Simple epoch calculation (not accounting for leap years precisely) */
        uint32_t days = (uint32_t)(year - 1970) * 365 + (uint32_t)(mon - 1) * 30 + day;
        out->timestamp = days * 86400 + hh * 3600 + mm * 60 + ss;
    }

    return 0;
}

/* ----------------------------------------------------------------- *
 *  Read current GNSS fix (blocking with timeout)
 * ----------------------------------------------------------------- */
int gnss_read_fix(gnss_fix_t *out, uint32_t timeout_ms) {
    if (!out) return -1;

    /* Wait for a valid fix */
    uint32_t start = board_get_tick_ms();
    while (!g_has_fix || g_last_fix.fix_type == 0) {
        if (board_get_tick_ms() - start > timeout_ms) {
            /* Return last known position even if no current fix */
            if (g_has_fix) {
                *out = g_last_fix;
                return 1;  /* Stale fix */
            }
            return -1;
        }
    }

    *out = g_last_fix;
    return 0;
}

void gnss_power_down(void) {
    gpio_write(GNSS_EN_PORT, GNSS_EN_PIN, 0);
    RCC->APB1ENR1 &= ~RCC_APB1ENR1_USART2EN;
}

void gnss_power_up(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    gpio_write(GNSS_EN_PORT, GNSS_EN_PIN, 1);
    board_delay_ms(100);
}