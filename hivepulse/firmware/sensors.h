/*
 * sensors.h — Environmental sensor API for HivePulse
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>

/* Temperature sensor index in the DS18B20 chain */
typedef enum {
    TEMP_BROOD_TOP = 0,      /* Brood chamber top bar center */
    TEMP_BROOD_BOTTOM = 1,   /* Brood chamber bottom */
    TEMP_HIVE_CORNER_NE = 2, /* Bottom board NE corner */
    TEMP_HIVE_CORNER_NW = 3, /* Bottom board NW corner */
    TEMP_HIVE_CORNER_SE = 4, /* Bottom board SE corner */
    TEMP_HIVE_CORNER_SW = 5, /* Bottom board SW corner */
    TEMP_ENTRANCE = 6,       /* Entrance area */
    TEMP_EXT_AMBIENT = 7,    /* Exterior ambient */
    TEMP_SENSOR_COUNT
} temp_sensor_index_t;

/* Sensor data structure (all environmental readings) */
typedef struct {
    float temps[TEMP_SENSOR_COUNT]; /* 8× DS18B20 in °C */
    float weight_kg;                /* HX711 4-corner average in kg */
    float humidity;                 /* SHT45 in %RH */
    float humidity_temp;            /* SHT45 onboard temp in °C */
    float co2_ppm;                  /* SCD41 CO₂ in ppm */
    float co2_temp;                 /* SCD41 onboard temp in °C */
    float co2_humidity;             /* SCD41 onboard humidity in %RH */
    float imu_accel[3];             /* LSM6DSO32X accel in g (x, y, z) */
    float imu_gyro[3];              /* LSM6DSO32X gyro in deg/s */
    uint32_t timestamp_ms;          /* Reading timestamp */
    bool valid;
} sensor_data_t;

/* Initialize all sensors */
int sensors_init(void);

/* Read all sensors into sensor_data_t */
int sensors_read_all(sensor_data_t *data);

/* Put sensors into low-power sleep mode */
void sensors_sleep(void);

/* Wake sensors from sleep */
void sensors_wake(void);

/* Tare load cells (zero the weight reading) */
int sensors_tare_load_cells(void);

/* Get individual sensor readings (for BLE real-time display) */
int sensors_read_temps(float temps[TEMP_SENSOR_COUNT]);
int sensors_read_weight(float *weight_kg);
int sensors_read_humidity(float *humidity, float *temp);
int sensors_read_co2(float *co2_ppm, float *temp, float *humidity);
int sensors_read_imu(float accel[3], float gyro[3]);

/* ---- DS18B20 1-Wire Functions ---- */
int ds18b20_init(void);
int ds18b20_read_all(float temps[TEMP_SENSOR_COUNT]);
void ds18b20_start_conversion(void);
int ds18b20_read_scratchpad(uint8_t rom[8], float *temp);

/* ---- HX711 Load Cell ADC ---- */
int hx711_init(void);
int32_t hx711_read_raw(void);
int32_t hx711_read_average(int samples);
float hx711_read_weight_kg(void);
void hx711_tare(void);
void hx711_set_scale(float scale);
void hx711_power_down(void);
void hx711_power_up(void);

/* ---- SHT45 Humidity Sensor ---- */
int sht45_init(void);
int sht45_read(float *humidity, float *temp);
void sht45_soft_reset(void);
void sht45_heater_on(void);
void sht45_heater_off(void);

/* ---- SCD41 CO2 Sensor ---- */
int scd41_init(void);
int scd41_measure_single_shot(float *co2_ppm, float *temp, float *humidity);
int scd41_get_serial(uint16_t serial[3]);
void scd41_power_down(void);

/* ---- LSM6DSO32X IMU ---- */
int imu_init(void);
int imu_read(float accel[3], float gyro[3]);
void imu_power_down(void);

#endif /* SENSORS_H */