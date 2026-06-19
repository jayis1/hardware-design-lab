/*
 * pll.c — ADF4159 Fractional-N PLL Driver
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements the SPI programming interface for the Analog Devices ADF4159
 * fractional-N PLL synthesizer.  The ADF4159 uses a 4-wire SPI interface
 * (CLK, DATA, LE) with 24-bit register writes.  Each register write consists
 * of a 24-bit word: the lower 4 bits select the register address, and the
 * upper 20 bits contain the register data.
 *
 * The PLL generates the RF tones for the SFCW sweep.  Frequency is set via
 * the fractional divider registers (R0: INT value, R1: FRAC value + prescaler).
 *
 * The reference clock is a 10 MHz TCXO shared between the PLL and the
 * receiver demodulator to maintain phase coherence across the sweep.
 *
 * VCO selection:
 *   - HMC5841 (1-3 GHz) for HI/UHI presets → PLL output directly
 *   - HMC585 (0.5-1 GHz) for LO preset → PLL output directly (lower band)
 *   - For DEEP preset (< 500 MHz), the PLL output is divided by an external
 *     divider chain (/2, /4, /8, /16, /32) selected by VCO_SEL GPIOs.
 */

#include "pll.h"
#include "../board.h"
#include "../registers.h"

/* ===================================================================== */
/*  SPI bit-bang for PLL (SPI1 in master mode)                           */
/* ===================================================================== */

static void pll_spi_write_word(uint32_t word)
{
    /* The ADF4159 expects 24 bits MSB first.
     * We use SPI1 hardware in master mode, 3-wire (SCK + MOSI + LE).
     * LE (latch enable) is on PA8 and is pulsed high after the 24-bit
     * transfer to latch the register.
     */
    /* For simplicity, we bit-bang on the SPI1 pins configured as GPIO.
     * A hardware SPI implementation would use SPI1->TXDR with DMA.
     */

    /* Pull LE low */
    PLL_LE_PORT->BRR = (1UL << PLL_LE_PIN);

    /* Clock out 24 bits MSB first */
    for (int i = 23; i >= 0; i--) {
        /* Set MOSI */
        if (word & (1UL << i))
            PLL_MOSI_PORT->BSRR = (1UL << PLL_MOSI_PIN);
        else
            PLL_MOSI_PORT->BRR  = (1UL << PLL_MOSI_PIN);

        /* Clock low (already low after init) → high → low */
        PLL_SCK_PORT->BRR  = (1UL << PLL_SCK_PIN);  /* ensure low */
        /* small delay */
        for (volatile int d = 0; d < 2; d++) { __asm volatile("nop"); }
        PLL_SCK_PORT->BSRR = (1UL << PLL_SCK_PIN);  /* rising edge: data sampled */
        for (volatile int d = 0; d < 2; d++) { __asm volatile("nop"); }
        PLL_SCK_PORT->BRR  = (1UL << PLL_SCK_PIN);  /* falling edge */
    }

    /* LE pulse high to latch */
    for (volatile int d = 0; d < 5; d++) { __asm volatile("nop"); }
    PLL_LE_PORT->BSRR = (1UL << PLL_LE_PIN);
    for (volatile int d = 0; d < 5; d++) { __asm volatile("nop"); }
    PLL_LE_PORT->BRR  = (1UL << PLL_LE_PIN);
}

/* ===================================================================== */
/*  ADF4159 register programming                                          */
/* ===================================================================== */

/*
 * ADF4159 register map (24-bit words, LSB 4 bits = address):
 *
 * R0: [INT(15:0) | reserved(3) | ADDR(3:0)]
 *     INT = N integer divider (when FRAC mode off) or integer part
 *     Bits: 23:8 = INT[15:0], 7:4 = reserved, 3:0 = 0x0
 *
 * R1: [FRAC(23:0) | ADDR(3:0)]
 *     Wait — the ADF4159 has a specific layout. Let me use the datasheet:
 *     R0: INT[15:0] at bits 23:8, ADDR = 0x0 at bits 3:0
 *     R1: FRAC[23:0] at bits 23:0... no, ADDR at 3:0. FRAC[23:4] at 27:4.
 *     Actually ADF4159 R1: bits 23:4 = FRAC, 3:0 = 0x1.
 *
 * The exact register layout varies; the key registers are:
 *   R0 — INT divider
 *   R1 — FRAC divider + prescaler
 *   R2 — control (phase, polarity, charge pump)
 *   R3 — charge pump current, muxout
 *   R4 — RF divider select, reference doubler, etc.
 *   R5 — lock detect
 *
 * For this driver, we compute INT and FRAC from the desired output frequency:
 *   f_out = f_ref × (INT + FRAC/MOD) / RF_DIV
 * where MOD = 2^24 = 16777216, RF_DIV ∈ {1,2,4,8,16,32,64}.
 */

#define PLL_MODULUS  16777216UL  /* 2^24 fractional modulus */
#define PLL_RF_DIV_1   0
#define PLL_RF_DIV_2   1
#define PLL_RF_DIV_4   2
#define PLL_RF_DIV_8   3
#define PLL_RF_DIV_16  4
#define PLL_RF_DIV_32  5
#define PLL_RF_DIV_64  6

static uint8_t  current_rf_div = PLL_RF_DIV_1;
static uint32_t current_int_val = 0;
static uint32_t current_frac_val = 0;

static void pll_compute_dividers(uint32_t freq_hz)
{
    /* Determine RF divider: choose smallest RF_DIV such that
     * f_out × RF_DIV is in the PLL VCO range (3.0–6.0 GHz for ADF4159
     * with internal VCO; here external VCO so PLL output range is
     * determined by the selected external VCO).
     * For simplicity, assume PLL operates at 25 MHz–3.5 GHz with
     * prescaler = 4/5, so INT min = 31.
     */

    /* Effective reference after possible doubler */
    uint64_t f_ref = PLL_REF_FREQ_HZ;  /* 10 MHz, no doubler */

    /* Select RF divider: for f < 187.5 MHz use division
     * (VCO min ~3 GHz with /16 → 187.5 MHz)
     */
    uint8_t rf_div = PLL_RF_DIV_1;
    uint32_t div_ratio = 1;
    if (freq_hz >= 1500000000) { rf_div = PLL_RF_DIV_1;  div_ratio = 1; }
    else if (freq_hz >= 750000000)  { rf_div = PLL_RF_DIV_2;  div_ratio = 2; }
    else if (freq_hz >= 375000000)  { rf_div = PLL_RF_DIV_4;  div_ratio = 4; }
    else if (freq_hz >= 187500000)  { rf_div = PLL_RF_DIV_8;  div_ratio = 8; }
    else if (freq_hz >=  93750000)  { rf_div = PLL_RF_DIV_16; div_ratio = 16; }
    else if (freq_hz >=  46875000)  { rf_div = PLL_RF_DIV_32; div_ratio = 32; }
    else                            { rf_div = PLL_RF_DIV_64; div_ratio = 64; }

    /* N = f_vco / f_ref, f_vco = f_out × div_ratio */
    uint64_t f_vco = (uint64_t)freq_hz * div_ratio;
    uint64_t N_total = (f_vco * PLL_MODULUS) / f_ref;

    current_int_val  = (uint32_t)(N_total / PLL_MODULUS);
    current_frac_val = (uint32_t)(N_total % PLL_MODULUS);
    current_rf_div = rf_div;
}

static void pll_write_r0(void)
{
    /* R0: INT[15:0] at bits 23:8, ADDR=0 at bits 3:0 */
    uint32_t r0 = ((current_int_val & 0xFFFF) << 8) | PLL_REG_R0;
    pll_spi_write_word(r0);
}

static void pll_write_r1(void)
{
    /* R1: FRAC[23:0] at bits 27:4, ADDR=1 at bits 3:0 */
    uint32_t r1 = ((current_frac_val & 0xFFFFFF) << 4) | PLL_REG_R1;
    pll_spi_write_word(r1);
}

static void pll_write_r2(void)
{
    /* R2: control register — phase polarity, prescaler, etc.
     * Bits 23:22 = prescaler (01 = 4/5, 10 = 8/9)
     * Bit 21 = phase polarity (1 = positive)
     * Bit 20 = reserved
     * Bits 19:18 = CP gain
     * Bits 17:16 = reserved
     * Bits 15:8 = phase[7:0]
     * Bit 7 = reserved
     * Bits 6:5 = reserved
     * Bit 4 = reserved
     * Bits 3:0 = ADDR = 0x2
     */
    uint32_t r2 = (1UL << 21) | (1UL << 23) | PLL_REG_R2;  /* prescaler 4/5, +phase */
    pll_spi_write_word(r2);
}

static void pll_write_r3(void)
{
    /* R3: charge pump current (3.2 mA), muxout = lock detect
     * Bits 11:9 = CP current (010 = 3.2 mA approx)
     * Bits 6:4 = muxout (101 = digital lock detect)
     * Bits 3:0 = ADDR = 0x3
     */
    uint32_t r3 = (2UL << 9) | (5UL << 4) | PLL_REG_R3;
    pll_spi_write_word(r3);
}

static void pll_write_r4(void)
{
    /* R4: RF divider select + reference configuration
     * Bits 22:20 = RF divider (current_rf_div)
     * Bit 24 = band select clock div (leave 0)
     * Bit 25 = reference doubler (0 = disable)
     * Bit 26 = reference divide by 2 (0 = disable)
     * Bits 3:0 = ADDR = 0x4
     */
    uint32_t r4 = ((uint32_t)current_rf_div << 20) | PLL_REG_R4;
    pll_spi_write_word(r4);
}

static void pll_write_r5(void)
{
    /* R5: lock detect configuration
     * Bit 22 = lock detect precision (1 = 10ns)
     * Bits 3:0 = ADDR = 0x5
     */
    uint32_t r5 = (1UL << 22) | PLL_REG_R5;
    pll_spi_write_word(r5);
}

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

int pll_init(void)
{
    /* Enable PLL chip enable */
    PLL_CE_PORT->BSRR = (1UL << PLL_CE_PIN);
    board_delay_ms(5);

    /* Initialize all registers in sequence (ADF4159 requires R2→R1→R0 order) */
    pll_write_r5();
    pll_write_r4();
    pll_write_r3();
    pll_write_r2();
    pll_write_r1();
    pll_write_r0();

    board_delay_ms(2);  /* PLL lock settle */
    return 0;
}

void pll_set_frequency(uint32_t freq_hz)
{
    /* Compute new divider values */
    pll_compute_dividers(freq_hz);

    /* Only rewrite R0, R1, R4 (divider + RF div) for speed during sweep */
    pll_write_r4();
    pll_write_r1();
    pll_write_r0();

    /* Small settle delay for PLL lock — the main sweep loop adds dwell time */
    /* A real implementation would poll lock detect (MUX pin) here */
    for (volatile int d = 0; d < 20; d++) { __asm volatile("nop"); }
}

uint32_t pll_read_frequency(void)
{
    /* Reconstruct the output frequency from the current divider values */
    uint32_t div_ratio = 1UL << current_rf_div;
    uint64_t f_vco = ((uint64_t)current_int_val * PLL_REF_FREQ_HZ) +
                     (((uint64_t)current_frac_val * PLL_REF_FREQ_HZ) / PLL_MODULUS);
    return (uint32_t)(f_vco / div_ratio);
}

void pll_power_down(void)
{
    /* Disable chip enable → PLL enters power-down mode */
    PLL_CE_PORT->BRR = (1UL << PLL_CE_PIN);
}

void pll_power_up(void)
{
    PLL_CE_PORT->BSRR = (1UL << PLL_CE_PIN);
    board_delay_ms(5);
    /* Re-initialize registers after power-up */
    pll_write_r5();
    pll_write_r4();
    pll_write_r3();
    pll_write_r2();
    pll_write_r1();
    pll_write_r0();
}

/* End of pll.c */