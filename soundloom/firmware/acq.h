/*
 * acq.h — Acquisition driver public interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */
#ifndef SOUNDLOOM_ACQ_H
#define SOUNDLOOM_ACQ_H

#include <stdint.h>

#define ACQ_OK              0
#define ACQ_ERR_NO_DEVICE  (-1)
#define ACQ_ERR_SPI        (-2)
#define ACQ_ERR_DMA        (-3)

typedef void (*acq_callback_t)(const int32_t *samples, uint32_t n);

int      acq_init(void);
void     acq_start(void);
void     acq_stop(void);
int      acq_poll(int32_t *out_samples);
void     acq_set_callback(acq_callback_t cb);
void     acq_set_gain(int8_t db);
uint32_t acq_get_block_count(void);
int32_t  acq_get_offset(uint8_t ch);

#endif /* SOUNDLOOM_ACQ_H */