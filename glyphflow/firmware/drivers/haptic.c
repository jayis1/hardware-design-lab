/*
 * haptic.c — DRV2605L haptic driver (I²C, internal waveform library)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Uses the DRV2605L's built-in 123-waveform ROM library. GlyphFlow
 * triggers waveforms for pen-down tick, recognition confirm, and error.
 * The I²C bus is shared with the MAX17048 fuel gauge at 100 kHz.
 */
#include "haptic.h"
#include "../registers.h"
#include "../board.h"
#include "i2c.h"

int haptic_init(void)
{
    i2c_init();
    /* Reset the DRV2605L by writing 0x80 to MODE, then set to internal
     * trigger mode (MODE = 0x00) and select library 5 (default LRA lib). */
    i2c_write_reg(DRV2605L_ADDR, DRV2605L_MODE, 0x80);
    i2c_write_reg(DRV2605L_ADDR, DRV2605L_MODE, 0x00);
    /* Waveform sequence: just slot 1, others 0. */
    i2c_write_reg(DRV2605L_ADDR, DRV2605L_WAVESEQ1, 0);  /* set at play time */
    i2c_write_reg(DRV2605L_ADDR, DRV2605L_WAVESEQ2, 0);
    /* Library selection in the MODE register top nibble (lib 5 for LRA). */
    i2c_write_reg(DRV2605L_ADDR, DRV2605L_MODE, 0x50);
    return 0;
}

int haptic_play(uint8_t waveform_id)
{
    if (waveform_id > 123) waveform_id = 0;
    i2c_write_reg(DRV2605L_ADDR, DRV2605L_WAVESEQ1, waveform_id);
    i2c_write_reg(DRV2605L_ADDR, DRV2605L_GO, 0x01);
    return 0;
}

int haptic_tick(void)    { return haptic_play(17); }   /* sharp click */
int haptic_confirm(void) { return haptic_play(54); }   /* strong buzz  */
int haptic_error(void)   { return haptic_play(16); haptic_play(16); return 0; }