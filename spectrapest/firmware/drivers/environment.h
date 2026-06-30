/*
 * drivers/environment.h — Environmental sensor driver
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float    temperature_c;
    float    humidity_pct;
    float    pressure_hpa;
    uint16_t light_lux;
    uint16_t co2_ppm;
} env_data_t;

int  env_init(void);
int  env_read_all(env_data_t *out);
int  env_read_temp_humidity(float *temp_c, float *humidity_pct);
int  env_read_pressure(float *pressure_hpa);
int  env_read_light(uint16_t *lux);
int  env_read_co2(uint16_t *co2_ppm);

/* I2C helper functions */
int  i2c_write(uint8_t addr, uint8_t *data, uint8_t len);
int  i2c_read(uint8_t addr, uint8_t *data, uint8_t len);
int  i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value);
int  i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* ENVIRONMENT_H */