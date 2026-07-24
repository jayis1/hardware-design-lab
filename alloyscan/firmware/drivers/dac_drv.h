/*
 * dac_drv.h — Multi-tone DDS DAC driver for excitation coil
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_DAC_DRV_H
#define ALLOYSCAN_DAC_DRV_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize DAC1 Channel 1 for DMA-driven circular output */
void dac_drv_init(void);

/* Start DAC output (begins driving the excitation coil) */
void dac_drv_start(void);

/* Stop DAC output (disables excitation) */
void dac_drv_stop(void);

/* Check if DAC is currently running */
bool dac_drv_is_running(void);

/* Set excitation amplitude (0.0 to 1.0) */
void dac_drv_set_amplitude(float amp);

#endif /* ALLOYSCAN_DAC_DRV_H */