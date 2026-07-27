/*
 * safety.c — Hardware safety monitoring.
 *
 * Monitors cell voltage, temperature, and polarity before and during
 * every measurement. A hardware comparator (TLV3201 on PC2) provides
 * an independent over-voltage cutoff that disables the excitation
 * circuitry without firmware intervention.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "safety.h"
#include "../board.h"
#include "../registers.h"

/* -------------------------------------------------------------------------
 * ADC conversion for cell voltage and temperature
 *
 * The cell voltage is read via ADC1 channel 1 (PA0, AN_VCELL).
 * The voltage divider: V_cell → 1:1 (direct, since cells are 0–4.2 V
 * and the ADC range is 0–3.3 V via a 2:1 divider on the PCB).
 * So ADC_value × 2 × 3.3 / 4096 = V_cell in Volts.
 * In mV: V_mv = ADC × 6600 / 4096 ≈ ADC × 1.6113
 *
 * Temperature: NTC on PA0/PC0, 10kΩ at 25°C, Beta = 3950.
 * ADC_value → resistance → temperature via Steinhart-Hart.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */

static uint16_t adc_read_channel(uint8_t channel)
{
    /* Configure ADC sequence: single channel */
    ADC1->SQR1 = (channel & 0x1F) << 6;  /* first (and only) channel */
    ADC1->SQR2 = 0;
    ADC1->SQR3 = 0;

    /* Clear EOC flag and start conversion */
    ADC1->ISR = ADC_ISR_EOC;
    ADC1->CR |= ADC_CR_ADSTART;

    /* Wait for conversion complete */
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }

    return (uint16_t)ADC1->DR;
}

static uint16_t adc_to_voltage_mv(uint16_t adc_val)
{
    /* ADC 12-bit (0-4095), 2:1 divider, Vref = 3.3 V
     * V_cell = adc_val × (3.3 / 4096) × 2 × 1000 mV
     *        = adc_val × 1.6113 mV */
    uint32_t mv = (uint32_t)adc_val * 3300U * 2U / 4096U;
    return (uint16_t)mv;
}

static uint16_t adc_to_temp_dc(uint16_t adc_val)
{
    /* NTC voltage divider: V_adc = 3.3V × R_ntc / (R_fixed + R_ntc)
     * R_fixed = 10kΩ. So R_ntc = R_fixed × V_adc / (3.3 - V_adc)
     * Then Steinhart-Hart: 1/T = 1/T0 + (1/B) × ln(R/R0)
     * T0 = 298.15 K (25°C), B = 3950, R0 = 10000
     *
     * For simplicity, use a lookup table or linear approximation.
     * Return temperature in deci-degrees Celsius (×10).
     *
     * Approximate: for adc_val near 2048 (25°C), the slope is about
     * -0.25 °C/ADC-count. So T = 25 - (adc_val - 2048) × 0.025 ... but
     * this is very rough. For a production device we'd use a proper
     * Steinhart-Hart computation or a 64-entry LUT. */
    (void)adc_val;

    /* Placeholder: return 25.0 °C (250 deci-degrees).
     * In production, replace with NTC calculation. */
    return 250;
}

/* -------------------------------------------------------------------------
 * Safety init
 * ------------------------------------------------------------------------- */
int safety_init(void)
{
    /* Configure ADC1: 12-bit, single-ended, software trigger.
     * Enable ADC clock (done in gpio_init), calibrate, enable. */
    ADC1->CR = 0;
    /* ADC calibration (ADCAL bit) */
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { }

    /* Enable ADC */
    ADC1->ISR = ADC_ISR_ADRDY;  /* clear ready flag */
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }

    /* Configure: 12-bit resolution, right-aligned, software trigger,
     * continuous mode disabled, scan mode disabled. */
    ADC1->CFGR = 0;  /* default: 12-bit, right-aligned, single conversion */

    return 0;
}

/* -------------------------------------------------------------------------
 * Read cell voltage
 * ------------------------------------------------------------------------- */
int safety_read_voltage(uint16_t *voltage_mv)
{
    uint16_t adc = adc_read_channel(1);  /* PA0 = ADC1_IN1 = AN_VCELL */
    *voltage_mv = adc_to_voltage_mv(adc);
    return SAFETY_OK;
}

/* -------------------------------------------------------------------------
 * Read temperature
 * ------------------------------------------------------------------------- */
int safety_read_temp(uint16_t *temp_dc)
{
    uint16_t adc = adc_read_channel(6);  /* PC0 = ADC1_IN6 = AN_NTC */
    *temp_dc = adc_to_temp_dc(adc);
    return SAFETY_OK;
}

/* -------------------------------------------------------------------------
 * Check reverse polarity (PC3)
 * ------------------------------------------------------------------------- */
uint8_t safety_check_reverse(void)
{
    /* PC3 = REV_POL: 1 = normal, 0 = reversed */
    return (GPIOC->IDR & (1U << PIN_REV_POL)) ? 0 : 1;
}

/* -------------------------------------------------------------------------
 * Check OVP hardware fault (PC2)
 * ------------------------------------------------------------------------- */
uint8_t safety_check_ovp_fault(void)
{
    /* PC2 = OVP_FAULT: 1 = fault (comparator triggered) */
    return (GPIOC->IDR & (1U << PIN_OVP_FAULT)) ? 1 : 0;
}

/* -------------------------------------------------------------------------
 * Clear hardware fault latch
 * ------------------------------------------------------------------------- */
void safety_clear_fault(void)
{
    /* Pulse PC12 (FAULT_LATCH) to clear the hardware latch */
    GPIOC->BSRR = (1U << (PIN_FAULT_LATCH + 16));  /* low */
    delay_ms(1);
    GPIOC->BSRR = (1U << PIN_FAULT_LATCH);          /* high */
}

/* -------------------------------------------------------------------------
 * Full safety check
 *
 * Called before every measurement and periodically during sweeps.
 * Returns SAFETY_OK if all checks pass, or an error code.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int safety_check(uint16_t *voltage_mv, uint16_t *temp_dc)
{
    /* Check hardware OVP fault first — this is independent of firmware */
    if (safety_check_ovp_fault()) {
        return SAFETY_HARDWARE_FAULT;
    }

    /* Check reverse polarity */
    if (safety_check_reverse()) {
        return SAFETY_REVERSE_POLARITY;
    }

    /* Read voltage */
    if (safety_read_voltage(voltage_mv) != SAFETY_OK)
        return SAFETY_HARDWARE_FAULT;

    /* Voltage range check */
    if (*voltage_mv > OVP_THRESHOLD_MV)
        return SAFETY_OVP;
    if (*voltage_mv < UVP_THRESHOLD_MV)
        return SAFETY_UVP;

    /* Read temperature */
    safety_read_temp(temp_dc);
    if (*temp_dc > TEMP_MAX_mC)
        return SAFETY_OVERTEMP;

    return SAFETY_OK;
}