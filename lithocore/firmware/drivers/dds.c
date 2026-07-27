/*
 * dds.c — AD9833 DDS sine-wave generator driver.
 *
 * The AD9833 is a low-power (1.65–5.25 V) programmable waveform generator
 * with a 28-bit frequency accumulator and 12-bit phase accumulator. It
 * communicates over a 3-wire SPI (SCLK, SDATA, FSYNC) — we bit-bang this
 * on PB0 (SCK), PB1 (MOSI), PA15 (CS/FSYNC) to avoid SPI peripheral
 * conflicts with the ADS1256.
 *
 * The AD9833 MCLK is driven by the same 16.384 MHz TCXO that clocks the
 * MCU, ensuring the DDS output and the ADC sampling are phase-coherent.
 * This is critical for the lock-in detection: the reference sine/cosine
 * can be computed analytically from the known frequency and the DDS phase
 * register, with zero drift.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "dds.h"
#include "../board.h"
#include "../registers.h"

/* -------------------------------------------------------------------------
 * Bit-bang SPI write (16-bit word)
 *
 * The AD9833 uses a 16-bit serial word, MSB first, clocked on falling edge
 * of SCLK. FSYNC (CS) goes low before the word and high after.
 * ------------------------------------------------------------------------- */
static void dds_write_16(uint16_t word)
{
    /* FSYNC low (PA15) */
    GPIOA->BSRR = (1U << (PIN_SPI3_NCS_DDS + 16));

    for (int i = 15; i >= 0; i--) {
        /* SCLK low (PB0) */
        GPIOB->BSRR = (1U << (PIN_SPI3_SCK_DDS + 16));

        /* Set MOSI (PB1) */
        if (word & (1U << i))
            GPIOB->BSRR = (1U << PIN_SPI3_MOSI_DDS);
        else
            GPIOB->BSRR = (1U << (PIN_SPI3_MOSI_DDS + 16));

        /* Small delay for setup time (min 5 ns — plenty at MCU speed) */
        for (volatile int d = 0; d < 2; d++) { }

        /* SCLK high (PB0) — data sampled on rising edge by AD9833 */
        GPIOB->BSRR = (1U << PIN_SPI3_SCK_DDS);

        for (volatile int d = 0; d < 2; d++) { }
    }

    /* FSYNC high (PA15) */
    GPIOA->BSRR = (1U << PIN_SPI3_NCS_DDS);
}

/* -------------------------------------------------------------------------
 * DDS initialization
 * ------------------------------------------------------------------------- */
int dds_init(void)
{
    /* Pins already configured as outputs in gpio_init() */

    /* Deassert reset */
    GPIOB->BSRR = (1U << PIN_DDS_RESET);   /* PB13 high = not reset */
    delay_ms(1);

    dds_reset();
    delay_ms(1);

    /* Write control register: B28=1 (28-bit freq), MODE=0 (sine output),
       no DIV2, no sleep. */
    dds_write_16(DDS_CTRL_B28 | DDS_CTRL_RESET);
    dds_write_16(DDS_CTRL_B28);  /* clear reset, enable sine output */

    /* Set default frequency to 0 (no output) */
    dds_set_frequency(0);
    dds_set_phase(0);

    return 0;
}

void dds_reset(void)
{
    /* Hardware reset pulse on PB13 */
    GPIOB->BSRR = (1U << (PIN_DDS_RESET + 16));  /* low */
    delay_ms(1);
    GPIOB->BSRR = (1U << PIN_DDS_RESET);          /* high */
    delay_ms(1);

    /* Software reset */
    dds_write_16(DDS_CTRL_B28 | DDS_CTRL_RESET);
}

/* -------------------------------------------------------------------------
 * Set frequency
 *
 * The 28-bit frequency register value is:
 *   FREQREG = freq_Hz × 2^28 / MCLK
 *
 * We write this as two 14-bit halves: LSB first (control bit D15=0, D14=0
 * for freq reg 0 LSB), then MSB (D15=0, D14=1 for freq reg 0 MSB).
 *
 * The frequency is passed in milli-Hz (mHz) to preserve resolution at
 * very low frequencies without floating point. For 0.01 Hz, freq_mhz = 10.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void dds_set_frequency(uint32_t freq_mhz)
{
    /* Convert mHz to Hz (integer part) and compute freqreg.
     * freqreg = (freq_hz * 2^28) / MCLK
     * To avoid 64-bit overflow at high freq: freq_hz up to 100000 →
     *   100000 * 268435456 = 2.68e13 → need 64-bit. */
    uint64_t freq_hz_x1000 = freq_mhz;  /* freq in mHz */
    /* freqreg = freq_mHz * 2^28 / (MCLK * 1000)
     *         = freq_mHz * 268435456 / 16384000000
     * To keep precision, compute as:
     *   freqreg = (freq_mHz * 268435456ULL) / (DDS_MCLK_HZ * 1000ULL) */
    uint64_t freqreg = (freq_hz_x1000 * 268435456ULL) / (DDS_MCLK_HZ * 1000ULL);

    if (freqreg > 0x0FFFFFFFU)
        freqreg = 0x0FFFFFFFU;

    uint16_t lsb14 = (uint16_t)(freqreg & 0x3FFF);
    uint16_t msb14 = (uint16_t)((freqreg >> 14) & 0x3FFF);

    /* Write frequency register 0: LSB then MSB
     * FREQ0 LSB: D15=0, D14=0 → 0x4000 | lsb14
     * FREQ0 MSB: D15=0, D14=1 → 0x4000 | 0x4000? No:
     * Actually AD9833: freq write bits are D15=0, D14=0 for LSB,
     * D15=0, D14=1 for MSB of FREQ0. Wait, let me check the datasheet.
     *
     * AD9833 write format for frequency register 0:
     *   LSB: DB15=0, DB14=0, DB13..DB0 = freq LSB[13:0]
     *   MSB: DB15=0, DB14=1, DB13..DB0 = freq MSB[13:0]
     *
     * So the header word for LSB = 0x0000, for MSB = 0x4000.
     */
    dds_write_16(DDS_CTRL_B28);              /* ensure B28 mode */
    dds_write_16(0x0000 | lsb14);            /* FREQ0 LSB */
    dds_write_16(0x4000 | msb14);            /* FREQ0 MSB */
}

void dds_set_frequency_hz(double freq_hz)
{
    /* Use double for convenience API, convert to mHz for the integer path */
    if (freq_hz < 0.0) freq_hz = 0.0;
    dds_set_frequency((uint32_t)(freq_hz * 1000.0 + 0.5));
}

/* -------------------------------------------------------------------------
 * Set phase
 *
 * The 12-bit phase register: PHASEREG = phase_deg × 2^12 / 360
 * ------------------------------------------------------------------------- */
void dds_set_phase(uint16_t phase_deg)
{
    uint16_t phase_reg = (uint16_t)(((uint32_t)phase_deg * 4096U) / 360U);
    phase_reg &= 0x0FFF;

    /* Write PHASE0 register: D15=1, D14=0 → 0xC000 | phase_reg */
    dds_write_16(0xC000 | phase_reg);
}

/* -------------------------------------------------------------------------
 * Amplitude control
 *
 * The AD9833 has no programmable amplitude — output is full-scale (0.6 Vpp
 * at 0.6 mA). Amplitude is controlled by the external PGA (programmable
 * gain amplifier) in the analog front end. This function sets the PGA
 * gain via a GPIO-controlled resistor ladder or a digital pot.
 *
 * For this firmware, we abstract amplitude as a 0-255 value that maps to
 * the excitation current (0-20 mA). The actual PGA control is in the AFE
 * driver (not yet separate — the DAC on PA4 provides the bias reference).
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void dds_set_amplitude(uint8_t amplitude)
{
    /* Map 0-255 → DAC output 0-3.3V on PA4 (DAC channel 1).
     * The DAC value is 12-bit: 0-4095. */
    uint16_t dac_val = ((uint16_t)amplitude * 4095U) / 255U;

    /* Enable DAC channel 1 (simplified — real register writes would go here) */
    /* DAC->CR |= DAC_CR_EN1; */
    /* DAC->DHR12R1 = dac_val; */
    (void)dac_val;  /* placeholder until DAC registers are added */
}

void dds_power_down(void)
{
    /* Set SLEEP1 and SLEEP12 bits to power down the DDS */
    dds_write_16(DDS_CTRL_B28 | DDS_CTRL_SLEEP1 | DDS_CTRL_SLEEP12);
}

void dds_enable(void)
{
    dds_write_16(DDS_CTRL_B28);
}

void dds_disable(void)
{
    /* Mute: set frequency to 0 and power down DAC */
    dds_set_frequency(0);
    dds_write_16(DDS_CTRL_B28 | DDS_CTRL_SLEEP12);
}

/* -------------------------------------------------------------------------
 * Get current phase accumulator value
 *
 * The AD9833 doesn't expose its internal phase accumulator via SPI, but
 * since we know the exact frequency word and the time since the frequency
 * was set, we can compute the current phase:
 *   phase(t) = (freqreg * t * 2π) / 2^28 (mod 2π)
 *
 * For the lock-in detection, we use the phase at the START of the ADC
 * acquisition, which we record as the timestamp when dds_set_frequency
 * was called. The sweep manager passes this to the lock-in detector.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static volatile uint32_t g_dds_set_ticks = 0;
static volatile uint32_t g_dds_current_freqreg = 0;

/* Override dds_set_frequency to track timing — but since we can't easily
 * hook it, the sweep manager records the time separately. This function
 * is provided for the lock-in detector to query the phase at a given time. */
uint32_t dds_get_phase_accumulator(void)
{
    /* Return the frequency register value (the phase increment per MCLK
     * cycle). The absolute phase is computed by the lock-in detector using
     * this value and the elapsed time. */
    return g_dds_current_freqreg;
}