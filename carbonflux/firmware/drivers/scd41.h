/**
 * @file    scd41.h
 * @brief   SCD41 CO₂ sensor driver interface.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef SCD41_H
#define SCD41_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Sensor limits */
#define SCD41_CO2_MIN          0
#define SCD41_CO2_MAX          40000
#define SCD41_TEMP_MIN_C       -10.0f
#define SCD41_TEMP_MAX_C       60.0f
#define SCD41_RH_MIN           0
#define SCD41_RH_MAX           100
#define SCD41_MEAS_INTERVAL_MS 2000
#define SCD41_WARMUP_MS        1000

/* Initialization and lifecycle */
int  scd41_init(void);
int  scd41_start_periodic(void);
int  scd41_stop_periodic(void);
int  scd41_sleep(void);
int  scd41_wake(void);

/* Measurement */
int  scd41_read_co2(uint16_t *co2_ppm);
int  scd41_read_co2_ex(uint16_t *co2_ppm, float *temp_c, float *rh_pct);
bool scd41_data_ready(void);

/* Calibration */
int  scd41_forced_recalibration(uint16_t reference_ppm);
int  scd41_set_asc(bool enable);
int  scd41_get_asc(bool *enabled);

/* Identification */
void scd41_get_serial(char *serial_out);
uint8_t scd41_get_error_count(void);

#ifdef __cplusplus
}
#endif

#endif /* SCD41_H */