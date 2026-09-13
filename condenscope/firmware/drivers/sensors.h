/* CondenScope sensor fusion interface
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef CONDENSCOPE_SENSORS_H
#define CONDENSCOPE_SENSORS_H

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"

typedef enum {
    SENSOR_OK = 0,
    SENSOR_E_BUS = -1,
    SENSOR_E_CRC = -2,
    SENSOR_E_ID = -3,
    SENSOR_E_TIMEOUT = -4,
    SENSOR_E_CALIBRATION = -5
} sensor_result_t;

typedef struct {
    int16_t offset_centic;
    uint16_t gain_q12;
    int16_t kta_q10;
    int16_t kv_q10;
    uint16_t alpha_q20;
    int16_t tgc_q10;
    int16_t ks_ta_q10;
    int16_t cp_offset[2];
    uint16_t broken_pixel;
    uint16_t outlier_pixel;
    bool valid;
} mlx_calibration_t;

typedef struct {
    int16_t pixels_centic[THERMAL_PIXELS];
    int16_t ambient_centic;
    int16_t min_centic;
    int16_t max_centic;
    uint16_t min_index;
    uint16_t max_index;
    uint32_t sequence;
    uint8_t subpage;
    bool complete;
} thermal_frame_t;

typedef struct {
    int16_t air_temperature_centic;
    uint16_t relative_humidity_centi_percent;
    int16_t dew_point_centic;
    uint16_t distance_mm;
    uint16_t battery_mv;
    uint8_t battery_percent;
    uint32_t timestamp_ms;
} environment_t;

typedef struct {
    uint16_t pixel;
    int16_t surface_centic;
    int16_t margin_centic;
    uint8_t risk;             /* 0 safe, 1 warning, 2 danger */
    uint16_t distance_mm;
    int16_t bearing_cdeg;
    int16_t elevation_cdeg;
} map_point_t;

sensor_result_t sensors_init(void);
sensor_result_t sensors_read_environment(environment_t *environment);
sensor_result_t sensors_read_thermal(thermal_frame_t *frame,
                                     uint16_t emissivity_q15);
void sensors_generate_points(const thermal_frame_t *frame,
                             const environment_t *environment,
                             map_point_t *points, uint16_t capacity,
                             uint16_t *written);
int16_t sensors_dew_point_centic(int16_t temperature_centic,
                                uint16_t humidity_centi_percent);
uint8_t sensors_risk_level(int16_t margin_centic);
bool sensors_calibrate_ambient(uint16_t reference_temperature_centic);
const mlx_calibration_t *sensors_calibration(void);

#endif
