/*
 * drivers/gnss.h — u-blox NEO-M9N GNSS driver interface
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef GNSS_H
#define GNSS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float    latitude;
    float    longitude;
    float    altitude_m;
    uint8_t  fix_type;    /* 0=no fix, 2=2D, 3=3D */
    uint8_t  satellites;
    float    hdop;       /* Horizontal dilution of precision */
    uint32_t timestamp;
} gnss_fix_t;

int  gnss_init(void);
int  gnss_read_fix(gnss_fix_t *out, uint32_t timeout_ms);
void gnss_power_down(void);
void gnss_power_up(void);

/* NMEA sentence parsing */
int  gnss_parse_gga(const char *sentence, gnss_fix_t *out);
int  gnss_parse_rmc(const char *sentence, gnss_fix_t *out);

#ifdef __cplusplus
}
#endif

#endif /* GNSS_H */