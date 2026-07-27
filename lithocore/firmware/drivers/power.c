/*
 * power.c — Power management.
 *
 * Controls the analog rail power (via TPL7407 load switch on PB12),
 * the supercapacitor charger (PC11), and BLE module power management.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "power.h"
#include "../board.h"
#include "../registers.h"

/* -------------------------------------------------------------------------
 * Power init
 * ------------------------------------------------------------------------- */
int power_init(void)
{
    /* Analog rail off, supercap charger enabled, BLE in reset */
    power_analog_off();
    power_supercap_charge_enable();
    return 0;
}

/* -------------------------------------------------------------------------
 * Analog rail control
 *
 * PB12 = ANALOG_EN: high = analog rails on (DDS, ADC, op-amps powered).
 * When off, the entire analog front end draws < 1 µA.
 * ------------------------------------------------------------------------- */
void power_analog_on(void)
{
    GPIOB->BSRR = (1U << PIN_ANALOG_EN);  /* high = on */
    /* Wait for rails to settle (op-amps + DDS startup ~ 10 ms) */
    delay_ms(20);
}

void power_analog_off(void)
{
    GPIOB->BSRR = (1U << (PIN_ANALOG_EN + 16));  /* low = off */
}

/* -------------------------------------------------------------------------
 * Supercapacitor charger
 *
 * PC11 = nCHARGE_EN: low = charging enabled, high = disabled.
 * The TPS61099 boost converter charges the 2.5 F supercap from the
 * coin cell at 5 mA. Full charge takes ~90 seconds.
 * ------------------------------------------------------------------------- */
void power_supercap_charge_enable(void)
{
    GPIOC->BSRR = (1U << (PIN_CHARGE_EN + 16));  /* low = charge enabled */
}

void power_supercap_charge_disable(void)
{
    GPIOC->BSRR = (1U << PIN_CHARGE_EN);  /* high = charge disabled */
}

/* -------------------------------------------------------------------------
 * Supercap ready check
 *
 * PB15 = SUPERCAP_OK: high = supercap voltage > 4.5 V (ready for pulse).
 * ------------------------------------------------------------------------- */
uint8_t power_supercap_is_ready(void)
{
    return (GPIOB->IDR & (1U << PIN_SUPERCAP_OK)) ? 1 : 0;
}

/* -------------------------------------------------------------------------
 * BLE module power management
 *
 * PC6 = BLE_WAKE: pulse high to wake the BL654 from sleep.
 * PC9 = nRESET_BLE: low = reset, high = running.
 * ------------------------------------------------------------------------- */
void power_ble_wake(void)
{
    GPIOC->BSRR = (1U << PIN_BLE_WAKE);  /* high = wake */
    delay_ms(1);
    GPIOC->BSRR = (1U << (PIN_BLE_WAKE + 16));  /* low = idle */
}

void power_ble_sleep(void)
{
    /* The BL654 enters sleep automatically when no BLE activity.
     * We just ensure the wake pin is low. */
    GPIOC->BSRR = (1U << (PIN_BLE_WAKE + 16));
}

/* -------------------------------------------------------------------------
 * Battery (coin cell) voltage monitor
 *
 * Reads the coin cell voltage via ADC (internal VBAT channel or external
 * divider). Returns voltage in mV.
 * ------------------------------------------------------------------------- */
uint32_t power_get_battery_mv(void)
{
    /* Read VBUS/USB detect to check if USB powered */
    uint16_t vbus = 0;
    /* ADC1 channel 7 = PC1 = AN_VBUS */
    ADC1->SQR1 = (7U << 6);
    ADC1->ISR = ADC_ISR_EOC;
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    vbus = (uint16_t)ADC1->DR;

    /* If USB is connected (vbus > threshold), report USB voltage */
    if (vbus > 2000) {
        return (uint32_t)vbus * 3300U * 2U / 4096U;
    }

    /* Otherwise, the coin cell voltage is monitored via the VBAT internal
     * channel. For simplicity, return a fixed 3000 mV (CR2477 nominal).
     * In production, read the VBAT channel with a dedicated ADC config. */
    return 3000;
}