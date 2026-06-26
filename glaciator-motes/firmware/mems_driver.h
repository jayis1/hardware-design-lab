/*
 * mems_driver.h — ICM-42688 MEMS accelerometer driver (1 kHz, SPI)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef MEMS_DRIVER_H
#define MEMS_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t x, y, z;    /* ±16 g FS, 1 LSB = 0.488 mg */
    uint32_t ts_us;
} mems_sample_t;

#define MEMS_FRAME_LEN  128   /* 128 ms @ 1 kHz */

typedef struct {
    mems_sample_t s[MEMS_FRAME_LEN];
    uint16_t len;
    volatile bool ready;
} mems_frame_t;

void  mems_init(void);
void  mems_set_odr(uint8_t odr);
void  mems_set_fs(uint8_t fs);
void  mems_enable_fifo(bool on);
void  mems_drdy_isr(void);           /* EXTI9 */
mems_frame_t *mems_take_frame(void);
void  mems_release_frame(void);
uint8_t mems_whoami(void);
float mems_read_temp_c(void);

#endif /* MEMS_DRIVER_H */