/*
 * env_drv.h — Environmental sensor aggregation (SHT45, S8 CO₂, Si1145, VEML6075).
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef ENV_DRV_H
#define ENV_DRV_H

#include <stdint.h>

typedef struct {
    int16_t co2_ppm;    /* Senseair S8 NDIR              */
    int8_t  temp_c;     /* SHT45 temperature (°C)        */
    uint8_t rh_pct;     /* SHT45 relative humidity (%)   */
    uint8_t uv_index_x10; /* Si1145 + VEML6075 combined */
} env_result_t;

int env_init(void);
int env_read(env_result_t *res);

#endif /* ENV_DRV_H */