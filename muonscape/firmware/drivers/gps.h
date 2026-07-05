/*
 * gps.h — u-blox NEO-M9N GPS driver
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_GPS_H
#define MUONSCAPE_DRV_GPS_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t  lat_e7;       /* latitude  * 1e7   */
    int32_t  lon_e7;       /* longitude * 1e7   */
    int16_t  alt_mm;       /* altitude, mm      */
    uint8_t  satellites;   /* sats in fix       */
    uint8_t  fix_quality;  /* 0=no, 1=GPS, 2=DGPS */
    uint16_t hdop;         /* horizontal dilution *100 */
    uint32_t unix_time;    /* seconds since 1970 */
} gps_state_t;

muon_status_t gps_init(void);
muon_status_t gps_read(gps_state_t *st);
int           gps_has_fix(void);
uint32_t      gps_pps_count(void);   /* PPS pulse counter */

/* ISR callbacks (called from USART2 and EXTI4 IRQ handlers) */
void          gps_usart_isr(uint8_t byte);
void          gps_pps_isr(void);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_GPS_H */