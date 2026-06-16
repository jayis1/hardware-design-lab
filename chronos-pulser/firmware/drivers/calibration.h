// calibration.h — Calibration Manager Header
#ifndef CALIBRATION_H
#define CALIBRATION_H
#include <stdint.h>
#include <stdbool.h>
#include "tdc_driver.h"

typedef struct {
    uint16_t adc_offset;
    uint16_t adc_gain;
    uint16_t pulse_delay_ps;
    uint32_t pulse_amplitude_cal;
    float    vga_gain_offset_db;
    uint32_t crc;
} factory_cal_t;

int  calibration_run_factory(void);
int  calibration_load_factory(const factory_cal_t *cal);
int  calibration_save_factory(factory_cal_t *cal);
int  calibration_run_field_open(void);
int  calibration_run_field_short(void);
int  calibration_run_field_load(void);
bool calibration_is_valid(void);
uint32_t calibration_compute_crc(const tdc_calibration_t *cal);
#endif
