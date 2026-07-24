/*
 * dac_drv.c — Multi-tone DDS DAC driver implementation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Drives the STM32G474's DAC1 Channel 1 via DMA1 Channel 1 in circular
 * mode, continuously outputting the pre-computed multi-tone DDS table.
 * This generates the 4-frequency excitation signal for the probe coil.
 */

#include "dac_drv.h"
#include "registers.h"
#include "board.h"
#include "lockin.h"

static bool dac_running = false;
static float current_amplitude = 1.0f;

/* Scaled DDS table (amplitude-adjusted copy) */
static uint16_t dds_scaled[DDS_TABLE_SIZE];

static void dac_drv_update_table(void)
{
    const uint16_t *base = lockin_get_dds_table();
    for (int i = 0; i < DDS_TABLE_SIZE; i++) {
        /* Scale around midpoint (2048) */
        int32_t val = (int32_t)base[i] - 2048;
        val = (int32_t)(val * current_amplitude) + 2048;
        if (val < 0) val = 0;
        if (val > 4095) val = 4095;
        dds_scaled[i] = (uint16_t)val;
    }
}

void dac_drv_init(void)
{
    /* Enable DAC clock */
    RCC_AHB2ENR |= RCC_AHB2ENR_DAC1;

    /* Enable DMA1 clock */
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA1;

    /* Configure DAC1 Channel 1:
     * - 12-bit right-aligned data
     * - DMA enabled
     * - Normal mode (no wave generation) */
    DAC1_MCR = 0;  /* Default: DAC mode 0, normal + external pin */

    /* Configure DMA1 Channel 1 for DAC1 */
    /* Disable DMA channel first */
    DMA1_Ch1->CCR = 0;

    /* Configure DMAMUX channel 0 for DAC1 Channel 1 request */
    DMAMUX1_Channel0 = DMAMUX_REQ_DAC1_CH1;

    /* DAC DMA: memory -> peripheral, circular, 16-bit, high priority */
    DMA1_Ch1->CCR = DMA_CCR_DIR_M2P
                  | DMA_CCR_CIRC
                  | DMA_CCR_MINC
                  | DMA_CCR_PSIZE_16B
                  | DMA_CCR_MSIZE_16B
                  | DMA_CCR_PL_VHIGH;

    DMA1_Ch1->CPAR = (uint32_t)&DAC1_DHR12R1;
    DMA1_Ch1->CM0AR = (uint32_t)dds_scaled;
    DMA1_Ch1->CNDR = DDS_TABLE_SIZE;

    /* Build the DDS table */
    lockin_init_dds_table();
    dac_drv_update_table();
}

void dac_drv_start(void)
{
    if (dac_running)
        return;

    dac_drv_update_table();

    /* Enable DMA channel */
    DMA1_Ch1->CCR |= DMA_CCR_EN;

    /* Enable DAC channel */
    DAC1_CR |= DAC_CR_EN1;
    DAC1_CR |= DAC_CR_DMAEN1;

    dac_running = true;
}

void dac_drv_stop(void)
{
    if (!dac_running)
        return;

    /* Disable DAC */
    DAC1_CR &= ~DAC_CR_EN1;
    DAC1_CR &= ~DAC_CR_DMAEN1;

    /* Disable DMA */
    DMA1_Ch1->CCR &= ~DMA_CCR_EN;

    dac_running = false;
}

bool dac_drv_is_running(void)
{
    return dac_running;
}

void dac_drv_set_amplitude(float amp)
{
    /* Clamp to [0, 1] */
    if (amp < 0.0f) amp = 0.0f;
    if (amp > 1.0f) amp = 1.0f;
    current_amplitude = amp;

    if (dac_running) {
        dac_drv_stop();
        dac_drv_update_table();
        dac_drv_start();
    }
}