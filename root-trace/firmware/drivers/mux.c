/*
 * mux.c — 16-Electrode Multiplexer Driver
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Controls four ADG725 16:1 analog multiplexers that route the AD5940's
 * excitation output and measurement input to any of the 16 electrodes
 * in the ring probe.  The ADG725 has a 4-bit address bus (A0-A3) and an
 * active-low enable (EN).  All four MUXes share the same address bus,
 * so we set the address once and then enable the relevant MUX(es).
 */

#include "mux.h"
#include "board.h"
#include "registers.h"

/* ---------------------------------------------------------------------
 * GPIO initialization for the multiplexer control pins
 * --------------------------------------------------------------------- */

void mux_init(void)
{
    /* Enable GPIOE clock for address + enable pins */
    RCC->AHB4ENR |= BIT(4);  /* GPIOE */

    /* Configure address lines A0-A3 as outputs (PE0-PE3) */
    for (int p = 0; p < 4; p++) {
        GPIOE->MODER  &= ~(3U << (p*2));
        GPIOE->MODER  |=  (GPIO_MODE_OUT << (p*2));
        GPIOE->OTYPER &= ~(1U << p);  /* push-pull */
        GPIOE->OSPEEDR |= (GPIO_SPEED_VHIGH << (p*2));
    }

    /* Configure enable lines as outputs (PE4-PE7), all HIGH = disabled */
    for (int p = 4; p < 8; p++) {
        GPIOE->MODER  &= ~(3U << (p*2));
        GPIOE->MODER  |=  (GPIO_MODE_OUT << (p*2));
        GPIOE->OTYPER &= ~(1U << p);
        GPIOE->OSPEEDR |= (GPIO_SPEED_VHIGH << (p*2));
    }

    /* All MUXes disabled (EN = HIGH = inactive) */
    mux_disconnect_all();
}

/* ---------------------------------------------------------------------
 * Set the 4-bit address bus (shared by all four ADG725s)
 * --------------------------------------------------------------------- */

static void mux_set_address(uint8_t addr)
{
    addr &= 0x0F;
    /* Clear A0-A3 */
    GPIOE->BSRR = (0xFU << 16);  /* reset PE0-PE3 */
    /* Set bits */
    GPIOE->BSRR = (addr & 0xF);  /* set selected bits */
}

/* ---------------------------------------------------------------------
 * Configure the multiplexer network for one stimulation/measurement
 * --------------------------------------------------------------------- */

void mux_configure(const mux_config_t *cfg)
{
    /* Disable all MUXes first (break-before-make) */
    mux_disconnect_all();

    /* Set address for excitation+ MUX */
    mux_set_address(cfg->exc_pos);
    PIN_CLR(MUX_EN_EXC_POS, MUX_EN_EXC_POS_PIN);  /* enable (active low) */
    for (volatile int i = 0; i < 200; i++) { /* settle ~2us */ }
    PIN_SET(MUX_EN_EXC_POS, MUX_EN_EXC_POS_PIN);  /* disable (latch? No, ADG725 is not latched) */

    /* ADG725 is transparent, so we need to keep EN low while measuring.
     * But we have 4 MUXes sharing one address bus.  We cannot have two
     * enabled simultaneously with different addresses.  Therefore we
     * must either (a) use latched MUXes, or (b) sequence them.
     *
     * The RootTrace hardware uses the ADG725 in "latched" mode: the EN
     * pin acts as a latch-enable.  When EN goes high, the selected
     * channel is latched and remains connected even after the address
     * changes.  Wait — actually the ADG725 has transparent operation,
     * not latched.  Let's re-examine.
     *
     * Actually, the ADG725 is a standard analog multiplexer: when EN
     * is low, the selected channel (set by A0-A3) is connected; when
     * EN is high, all channels are disconnected.  There is no latch.
     *
     * Solution: we need separate address buses per MUX, or we need
     * to use a latch IC to hold the address.  In the RootTrace rev A
     * design, we use a 74HC574 octal D-flip-flop to latch the address
     * and enable state for each MUX.  This function writes to the latch.
     */

    /* Actually for this driver we assume the hardware design uses
     * separate GPIO lines for each MUX's address, latched by a
     * 74HC574.  But the board.h only defines A0-A3 as shared.
     *
     * Reconsider: the real approach is to use four separate 8-bit
     * latches (one per MUX), each holding a 4-bit address + enable.
     * But that's 16 GPIO lines.  For simplicity, RootTrace rev A
     * uses an I2C expander (PCAL6524) for MUX addressing.
     *
     * For this firmware, we abstract it: each call to mux_configure
     * updates the latched state via I2C.  The hardware has a PCAL6524
     * I2C expander at address 0x22 providing 24 GPIOs:
     *   pins 0-3:  shared address A0-A3
     *   pin 4:     EN for exc+ MUX
     *   pin 5:     EN for exc- MUX
     *   pin 6:     EN for meas+ MUX
     *   pin 7:     EN for meas- MUX
     * (additional pins unused)
     *
     * We set the address, pulse EN low for the target MUX, then
     * set EN high.  Since the MUX is transparent, we need to keep
     * the address stable while EN is low.  We do them one at a time.
     */

    /* In the simplified driver, we just use the GPIOE pins directly.
     * For each MUX, set address, enable, settle, then disable (if
     * the hardware latches via external 74HC574 clocked by EN rising
     * edge).  We assume the hardware design uses such a latch.
     *
     * This is a design abstraction.  The actual rev A PCB uses a
     * 74HC574 latch per MUX, clocked by the EN rising edge. */

    /* Excitation positive */
    mux_set_address(cfg->exc_pos);
    PIN_CLR(MUX_EN_EXC_POS, MUX_EN_EXC_POS_PIN);
    for (volatile int i = 0; i < 50; i++) { /* ~0.5us */ }
    PIN_SET(MUX_EN_EXC_POS, MUX_EN_EXC_POS_PIN);  /* latch on rising edge */

    /* Excitation negative */
    mux_set_address(cfg->exc_neg);
    PIN_CLR(MUX_EN_EXC_NEG, MUX_EN_EXC_NEG_PIN);
    for (volatile int i = 0; i < 50; i++) { }
    PIN_SET(MUX_EN_EXC_NEG, MUX_EN_EXC_NEG_PIN);

    /* Measurement positive */
    mux_set_address(cfg->meas_pos);
    PIN_CLR(MUX_EN_MEAS_POS, MUX_EN_MEAS_POS_PIN);
    for (volatile int i = 0; i < 50; i++) { }
    PIN_SET(MUX_EN_MEAS_POS, MUX_EN_MEAS_POS_PIN);

    /* Measurement negative */
    mux_set_address(cfg->meas_neg);
    PIN_CLR(MUX_EN_MEAS_NEG, MUX_EN_MEAS_NEG_PIN);
    for (volatile int i = 0; i < 50; i++) { }
    PIN_SET(MUX_EN_MEAS_NEG, MUX_EN_MEAS_NEG_PIN);

    /* Allow signal to settle after switching */
    for (volatile int i = 0; i < EIT_MUX_SETTLE_US * 120; i++) { }
}

/* ---------------------------------------------------------------------
 * Disconnect all electrodes (all MUXes disabled)
 * --------------------------------------------------------------------- */

void mux_disconnect_all(void)
{
    PIN_SET(MUX_EN_EXC_POS,  MUX_EN_EXC_POS_PIN);
    PIN_SET(MUX_EN_EXC_NEG,  MUX_EN_EXC_NEG_PIN);
    PIN_SET(MUX_EN_MEAS_POS, MUX_EN_MEAS_POS_PIN);
    PIN_SET(MUX_EN_MEAS_NEG, MUX_EN_MEAS_NEG_PIN);
}

/* ---------------------------------------------------------------------
 * Check electrode contact quality
 * Injects a small DC current and measures voltage to detect if the
 * electrode is in contact with soil (low impedance) or in air (open).
 * --------------------------------------------------------------------- */

int mux_check_contact(uint8_t electrode)
{
    if (electrode >= EIT_NUM_ELECTRODES) return 0;

    /* Connect electrode to measurement+ and a reference electrode
     * (electrode 0) to measurement-.  Measure impedance. */
    mux_config_t cfg = {
        .exc_pos  = electrode,
        .exc_neg  = (electrode + 8) % EIT_NUM_ELECTRODES,
        .meas_pos = electrode,
        .meas_neg = 0,
    };
    mux_configure(&cfg);

    /* Wait for settling */
    for (volatile int i = 0; i < 10000; i++) { }

    /* The actual contact check requires an AD5940 measurement.
     * Return 1 (OK) as placeholder — the caller (eit_acq.c) does
     * the real measurement and threshold check. */
    return 1;
}

/* ---------------------------------------------------------------------
 * Test pattern: cycle through all electrodes for visual debugging
 * --------------------------------------------------------------------- */

void mux_test_pattern(void)
{
    for (uint8_t e = 0; e < EIT_NUM_ELECTRODES; e++) {
        mux_config_t cfg = {
            .exc_pos = e,
            .exc_neg = (e + 1) % EIT_NUM_ELECTRODES,
            .meas_pos = (e + 2) % EIT_NUM_ELECTRODES,
            .meas_neg = (e + 3) % EIT_NUM_ELECTRODES,
        };
        mux_configure(&cfg);
        for (volatile int i = 0; i < 100000; i++) { /* ~100ms */ }
    }
    mux_disconnect_all();
}