/*
 * actuator_drv.c — TactiScript piezoelectric actuator array driver
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Drives the 24-element (4×6) PZT bimorph micro-actuator array via
 * 6× TI DRV2700 dual-channel piezo driver ICs, multiplexed through
 * an 8:1 analog demux for subframe selection.
 *
 * Architecture:
 *   - 6 DRV2700 ICs, each with 2 output channels = 12 drive lines.
 *   - 24 PZT elements arranged in 4 groups of 6.
 *   - A 3-bit demux selects which group of 6 is active at any time
 *     (only 4 groups used; 4 of 8 demux outputs).
 *   - At 200 Hz refresh, we cycle through 4 subframes, each driving
 *     6 elements for ~500 µs within the 5 ms frame window.
 *
 * Timing per frame (5000 µs total @ 200 Hz):
 *   Subframe 0: boost warm-up (100 µs) + drive group 0 (500 µs) = 600 µs
 *   Subframe 1: drive group 1 (500 µs)                     = 500 µs
 *   Subframe 2: drive group 2 (500 µs)                     = 500 µs
 *   Subframe 3: drive group 3 (500 µs)                     = 500 µs
 *   Idle: remaining time for SPI transfers + overhead       = ~2900 µs
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "actuator_drv.h"
#include "../board.h"
#include "../registers.h"

/* ----------------------------------------------------------------
 * Private state
 * ---------------------------------------------------------------- */
static bool     s_initialized = false;
static bool     s_refresh_enabled = false;
static uint8_t  s_current_subframe = 0;
static uint32_t s_hv_millivolts = HV_TARGET_V * 1000; /* default 120 V */
static uint8_t  s_vgain_code = DRV2700_VGAIN_120V;

/* SPI transfer buffer for DRV2700 register writes.
 * DRV2700 has a simple SPI protocol: [addr][data] per byte.
 * We chain 6 ICs via daisy-chain SPI (shift registers).
 */
static uint8_t s_spi_tx_buf[DRV2700_COUNT * 2];

/* ----------------------------------------------------------------
 * Low-level GPIO helpers
 * ---------------------------------------------------------------- */
static inline void gpio_set(uint32_t port, uint32_t pin)
{
    GPIO_OUTSET(GPIO_PORT_BASE(port)) = (1u << pin);
}

static inline void gpio_clr(uint32_t port, uint32_t pin)
{
    GPIO_OUTCLR(GPIO_PORT_BASE(port)) = (1u << pin);
}

/* ----------------------------------------------------------------
 * SPI0 transfer (blocking, used for DRV2700 chain)
 * Sends s_spi_tx_buf, ignores received data.
 * ---------------------------------------------------------------- */
static void spi0_transfer(uint16_t len)
{
    SPIM_TXD_PTR(NRF_SPIM0_BASE) = (uint32_t)s_spi_tx_buf;
    SPIM_TXD_MAXCNT(NRF_SPIM0_BASE) = len;
    SPIM_RXD_MAXCNT(NRF_SPIM0_BASE) = 0;

    /* Assert CS low */
    gpio_clr(0, 11);  /* PIN_SPI0_CS_DRV = P0.11 */

    SPIM_TASKS_START(NRF_SPIM0_BASE) = 1;
    while (!SPIM_EVENTS_END(NRF_SPIM0_BASE)) ;
    SPIM_EVENTS_END(NRF_SPIM0_BASE) = 0;

    /* Deassert CS high */
    gpio_set(0, 11);
}

/* ----------------------------------------------------------------
 * DRV2700 register write (broadcast to all 6 ICs via daisy chain)
 * ---------------------------------------------------------------- */
static void drv2700_write_all(uint8_t reg, uint8_t val)
{
    /* Build the daisy-chain payload: 6 × [reg, val] */
    for (int i = 0; i < DRV2700_COUNT; i++) {
        s_spi_tx_buf[i * 2] = reg;
        s_spi_tx_buf[i * 2 + 1] = val;
    }
    spi0_transfer(DRV2700_COUNT * 2);
}

/* ----------------------------------------------------------------
 * DRV2700 single-IC write (address one specific IC in the chain)
 * ---------------------------------------------------------------- */
static void drv2700_write_one(uint8_t ic_index, uint8_t reg, uint8_t val)
{
    /* For daisy-chain, we send to all but only the addressed IC gets
     * the real command; others get a no-op (write to a scratch reg).
     */
    for (int i = 0; i < DRV2700_COUNT; i++) {
        if (i == ic_index) {
            s_spi_tx_buf[i * 2] = reg;
            s_spi_tx_buf[i * 2 + 1] = val;
        } else {
            /* No-op: write to status register (read-only, harmless) */
            s_spi_tx_buf[i * 2] = DRV2700_REG_STATUS;
            s_spi_tx_buf[i * 2 + 1] = 0x00;
        }
    }
    spi0_transfer(DRV2700_COUNT * 2);
}

/* ----------------------------------------------------------------
 * HV boost control
 * ---------------------------------------------------------------- */
static void hv_boost_enable(bool enable)
{
    if (enable) {
        gpio_set(1, 0);  /* PIN_HV_BOOST_EN = P1.00 */
        /* Wait for HV rail to stabilize */
        power_delay_us_stub(HV_BOOST_WARMUP_US);
    } else {
        gpio_clr(1, 0);
    }
}

/* (declared in power.c, weak stub here for link convenience) */
extern void power_delay_us_stub(uint32_t us);

/* ----------------------------------------------------------------
 * Demux selection (choose which group of 6 elements is active)
 *   group 0-3 → demux outputs 0-3
 *   A2 A1 A0 = group number
 * ---------------------------------------------------------------- */
static void demux_select(uint8_t group)
{
    if (group & 0x01) gpio_set(1, 2); else gpio_clr(1, 2); /* A0 */
    if (group & 0x02) gpio_set(1, 3); else gpio_clr(1, 3); /* A1 */
    if (group & 0x04) gpio_set(1, 4); else gpio_clr(1, 4); /* A2 */
}

/* ----------------------------------------------------------------
 * Map a 4×6 frame to 4 groups of 6 element intensity values.
 * Group g contains elements at rows [g] and all columns [0..5].
 * Wait — our array is 4 rows × 6 cols. We group by row:
 *   group 0 = row 0 (elements 0-5)
 *   group 1 = row 1 (elements 6-11)
 *   group 2 = row 2 (elements 12-17)
 *   group 3 = row 3 (elements 18-23)
 * Each DRV2700 IC drives 2 elements; 3 ICs per group × 2 = 6 elements.
 * So group g uses DRV2700 ICs [g*3 .. g*3+2].
 * ---------------------------------------------------------------- */
static const uint8_t *frame_row(const uint8_t *frame, uint8_t group)
{
    return &frame[group * ACT_COLS];
}

/* ----------------------------------------------------------------
 * Drive one group of 6 elements via the DRV2700 chain
 * The 3 DRV2700 ICs for this group get waveform peak values;
 * all other ICs get zero (outputs disabled for this subframe).
 * ---------------------------------------------------------------- */
static void drive_group(const uint8_t *frame, uint8_t group)
{
    /* Select the demux output for this group */
    demux_select(group);

    /* For each of the 6 elements in this group, program the corresponding
     * DRV2700 channel with the element's intensity (0-255 → 0-127 VGAIN).
     *
     * DRV2700 IC index within the group: ic = element / 2
     * Channel within IC: ch = element % 2
     *
     * We write WAVEFORM1 (ch0) and WAVEFORM2 (ch1) peak values.
     */
    const uint8_t *row = frame_row(frame, group);

    for (int ic = 0; ic < 3; ic++) {
        uint8_t global_ic = group * 3 + ic;
        uint8_t val0 = row[ic * 2];
        uint8_t val1 = row[ic * 2 + 1];

        /* Scale 0-255 → 0-127 for DRV2700 waveform register */
        uint8_t peak0 = val0 / 2;
        uint8_t peak1 = val1 / 2;

        drv2700_write_one(global_ic, DRV2700_REG_WAVEFORM1, peak0);
        drv2700_write_one(global_ic, DRV2700_REG_WAVEFORM2, peak1);
    }

    /* Trigger the outputs for this group */
    for (int ic = 0; ic < 3; ic++) {
        uint8_t global_ic = group * 3 + ic;
        drv2700_write_one(global_ic, DRV2700_REG_GO, 0x01);
    }

    /* Hold the drive for the drive window */
    power_delay_us_stub(ACT_DRIVE_WINDOW_US);

    /* Disable outputs for this group to prevent lingering drive */
    for (int ic = 0; ic < 3; ic++) {
        uint8_t global_ic = group * 3 + ic;
        drv2700_write_one(global_ic, DRV2700_REG_CTRL1,
                          DRV2700_RSTN); /* disable outputs, keep reset clear */
    }
}

/* ----------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------- */

void actuator_init(void)
{
    if (s_initialized)
        return;

    /* Configure SPI0 pins */
    SPIM_PSEL_SCK(NRF_SPIM0_BASE) = GPIO_PIN(PIN_SPI0_SCK);
    SPIM_PSEL_MOSI(NRF_SPIM0_BASE) = GPIO_PIN(PIN_SPI0_MOSI);
    SPIM_PSEL_MISO(NRF_SPIM0_BASE) = GPIO_PIN(PIN_SPI0_MISO);
    SPIM_FREQUENCY(NRF_SPIM0_BASE) = 0x08000000; /* 8 MHz */
    SPIM_CONFIG(NRF_SPIM0_BASE) = 0; /* mode 0, MSB first */
    SPIM_ENABLE(NRF_SPIM0_BASE) = 1;

    /* CS pin as output, default high (deselected) */
    gpio_set(0, 11);

    /* DRV2700 chain: reset all ICs */
    drv2700_write_all(DRV2700_REG_CTRL1, 0x00); /* clear all bits = reset */
    power_delay_us_stub(1000); /* 1 ms reset pulse */
    drv2700_write_all(DRV2700_REG_CTRL1, DRV2700_RSTN); /* release reset */

    /* Set voltage gain to default (120 V) */
    drv2700_write_all(DRV2700_REG_VGAIN, s_vgain_code);

    /* Enable HV boost in all DRV2700s */
    drv2700_write_all(DRV2700_REG_CTRL1,
                      DRV2700_RSTN | DRV2700_EN_HVBOOST);

    /* Set mode to real-time play (RTP) for all channels */
    drv2700_write_all(DRV2700_REG_CTRL2, DRV2700_MODE_RTP);

    /* Enable output channels */
    drv2700_write_all(DRV2700_REG_CTRL1,
                      DRV2700_RSTN | DRV2700_EN_HVBOOST |
                      DRV2700_EN_OUT1 | DRV2700_EN_OUT2);

    /* Enable the on-board boost converter */
    hv_boost_enable(true);

    /* Wait for HV rail to settle */
    power_delay_us_stub(5000); /* 5 ms */

    s_initialized = true;
}

void actuator_refresh_enable(bool enable)
{
    s_refresh_enabled = enable;
    if (enable) {
        TIMER_TASKS_START(NRF_TIMER0_BASE) = 1;
    } else {
        TIMER_TASKS_STOP(NRF_TIMER0_BASE) = 1;
        /* Turn off all outputs */
        drv2700_write_all(DRV2700_REG_CTRL1, DRV2700_RSTN | DRV2700_EN_HVBOOST);
        /* Disable HV boost to save power */
        hv_boost_enable(false);
    }
}

void actuator_refresh(const uint8_t *frame)
{
    if (!s_initialized || !s_refresh_enabled)
        return;

    /* Ensure HV boost is on (it may have been disabled for power saving) */
    static bool hv_on = false;
    if (!hv_on) {
        hv_boost_enable(true);
        hv_on = true;
    }

    /* Cycle through 4 subframes (one per row group) */
    drive_group(frame, s_current_subframe);

    /* Advance to next subframe */
    s_current_subframe++;
    if (s_current_subframe >= ACT_SUBFRAMES) {
        s_current_subframe = 0;
        /* After completing all 4 subframes, optionally power down HV
         * between frames to save energy. We keep it on for simplicity. */
    }
}

void actuator_set_voltage(uint32_t millivolts)
{
    s_hv_millivolts = millivolts;
    /* Map 0-120000 mV → VGAIN 0-127 */
    uint32_t vg = (millivolts * 127) / (HV_TARGET_V * 1000);
    if (vg > 127) vg = 127;
    s_vgain_code = (uint8_t)vg;

    if (s_initialized) {
        drv2700_write_all(DRV2700_REG_VGAIN, s_vgain_code);
    }
}

void actuator_emergency_off(void)
{
    /* Immediately disable all outputs and HV boost */
    drv2700_write_all(DRV2700_REG_CTRL1, 0x00); /* full reset */
    hv_boost_enable(false);
    s_refresh_enabled = false;
    TIMER_TASKS_STOP(NRF_TIMER0_BASE) = 1;
}

bool actuator_hv_ok(void)
{
    /* Read status from first DRV2700 (all should be same) */
    /* In a real implementation, we'd do an SPI read here.
     * For this reference code, we assume HV is OK if boost is enabled.
     */
    return s_initialized;
}

/* ----------------------------------------------------------------
 * End of actuator_drv.c
 * Author: jayis1
 * ---------------------------------------------------------------- */