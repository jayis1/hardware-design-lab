/*
 * dds.h — AD9833 DDS sine-wave generator driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_DDS_H
#define LITHOCORE_DDS_H

#include <stdint.h>
#include "../board.h"

/* AD9833 clock — driven by the 16.384 MHz TCXO (same as MCU HSE),
 * ensuring phase coherence between DDS output and ADC sampling. */
#define DDS_MCLK_HZ       16384000ULL

/* AD9833 frequency register is 28 bits → frequency resolution:
 *   Δf = MCLK / 2^28 = 16384000 / 268435456 = 0.06103515625 Hz
 * This gives sub-Hz resolution across the entire 0–7 MHz range. */
#define DDS_FREQ_RES_HZ   0.06103515625f

/* AD9833 control register bits */
#define DDS_CTRL_B28      (1U << 13)  /* 28-bit frequency word */
#define DDS_CTRL_DIV2     (1U << 11)  /* divide MCLK by 2 */
#define DDS_CTRL_MODE     (1U << 5)   /* 1 = sine, 0 = triangular */
#define DDS_CTRL_OPBITEN  (1U << 1)   /* square wave output */
#define DDS_CTRL_RESET    (1U << 8)   /* reset */
#define DDS_CTRL_SLEEP1   (1U << 7)   /* internal clock disabled */
#define DDS_CTRL_SLEEP12  (1U << 6)   /* DAC powered down */
#define DDS_CTRL_FSEL0    (1U << 10)  /* select freq reg 0 */
#define DDS_CTRL_FSEL1    (1U << 9)   /* select freq reg 1 */
#define DDS_CTRL_PHASE0   (1U << 3)   /* select phase reg 0 */

/* API */
int  dds_init(void);
void dds_reset(void);
void dds_set_frequency(uint32_t freq_mhz);   /* freq in mHz (milli-Hz) */
void dds_set_frequency_hz(double freq_hz);
void dds_set_phase(uint16_t phase_deg);
void dds_set_amplitude(uint8_t amplitude);   /* 0-255 via external PGA */
void dds_power_down(void);
void dds_enable(void);
void dds_disable(void);
uint32_t dds_get_phase_accumulator(void);

#endif /* LITHOCORE_DDS_H */