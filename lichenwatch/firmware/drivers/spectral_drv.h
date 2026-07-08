/*
 * spectral_drv.h — AS7341 11-channel spectral sensor driver interface.
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef SPECTRAL_DRV_H
#define SPECTRAL_DRV_H

#include <stdint.h>

typedef struct {
    /* Raw channel readings (415..670 nm + NIR + clear) */
    uint16_t ch[11];
    /* Derived lichen indices */
    float lndvi;         /* (NIR - Red) / (NIR + Red)            */
    float chl_index;    /* R(560) / R(670) — chlorophyll proxy  */
    float melanin_proxy; /* R(415) / R(560) — UV pigment proxy    */
} spectral_result_t;

int spectral_init(void);
int spectral_read(spectral_result_t *res);

#endif /* SPECTRAL_DRV_H */