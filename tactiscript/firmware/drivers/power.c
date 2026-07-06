/*
 * power.c — TactiScript power management driver
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Manages:
 *   - nPM1300 PMIC (charging, fuel gauge, power path)
 *   - Battery voltage monitoring via SAADC
 *   - LRA vibration motor (whole-ring haptic alerts) via PWM0
 *   - NTC temperature sensor for PZT compensation
 *   - Skin-contact capacitive detection
 *   - Delay functions used by other drivers
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "power.h"
#include "../board.h"
#include "../registers.h"

/* ----------------------------------------------------------------
 * Private state
 * ---------------------------------------------------------------- */
static bool s_initialized = false;
static uint8_t s_lra_intensity = 50; /* default 50% */

/* ----------------------------------------------------------------
 * GPIO helpers
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
 * I2C helper for PMIC reads (reuses TWIM0)
 * ---------------------------------------------------------------- */
static uint8_t pmic_read_reg(uint8_t reg)
{
    /* Write register address, then read one byte */
    TWIM_TXD_PTR(NRF_TWIM0_BASE) = (uint32_t)&reg;
    TWIM_TXD_MAXCNT(NRF_TWIM0_BASE) = 1;
    TWIM_TASKS_STARTTX(NRF_TWIM0_BASE) = 1;
    while (!TWIM_EVENTS_END(NRF_TWIM0_BASE)) ;
    TWIM_EVENTS_END(NRF_TWIM0_BASE) = 0;

    uint8_t val = 0;
    TWIM_RXD_PTR(NRF_TWIM0_BASE) = (uint32_t)&val;
    TWIM_RXD_MAXCNT(NRF_TWIM0_BASE) = 1;
    TWIM_TASKS_STARTRX(NRF_TWIM0_BASE) = 1;
    while (!TWIM_EVENTS_END(NRF_TWIM0_BASE)) ;
    TWIM_EVENTS_END(NRF_TWIM0_BASE) = 0;
    TWIM_TASKS_STOP(NRF_TWIM0_BASE) = 1;
    while (!TWIM_EVENTS_STOPPED(NRF_TWIM0_BASE)) ;
    TWIM_EVENTS_STOPPED(NRF_TWIM0_BASE) = 0;

    return val;
}

static void pmic_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    TWIM_TXD_PTR(NRF_TWIM0_BASE) = (uint32_t)buf;
    TWIM_TXD_MAXCNT(NRF_TWIM0_BASE) = 2;
    TWIM_TASKS_STARTTX(NRF_TWIM0_BASE) = 1;
    while (!TWIM_EVENTS_END(NRF_TWIM0_BASE)) ;
    TWIM_EVENTS_END(NRF_TWIM0_BASE) = 0;
    TWIM_TASKS_STOP(NRF_TWIM0_BASE) = 1;
    while (!TWIM_EVENTS_STOPPED(NRF_TWIM0_BASE)) ;
    TWIM_EVENTS_STOPPED(NRF_TWIM0_BASE) = 0;
}

/* ----------------------------------------------------------------
 * SAADC helper for battery voltage + skin contact
 * ---------------------------------------------------------------- */
static uint16_t saadc_sample(uint8_t channel)
{
    /* Configure SAADC for single-ended sample on given channel.
     * Simplified: assumes SAADC is configured externally.
     * In production, set channel config, buffer, trigger sample, wait.
     */
    (void)channel;
    SAADC_TASKS_SAMPLE = 1;
    while (!SAADC_EVENTS_END) ;
    SAADC_EVENTS_END = 0;

    /* Read result (simplified — real code reads from result buffer) */
    static volatile uint16_t saadc_result = 0;
    return saadc_result;
}

/* ----------------------------------------------------------------
 * Initialize power management
 * ---------------------------------------------------------------- */
void power_init(void)
{
    if (s_initialized)
        return;

    /* Configure LRA pins as outputs */
    GPIO_DIRSET(NRF_GPIO_P1_BASE) = (1u << 6) | (1u << 7);
    gpio_clr(1, 6); /* LRA off initially */

    /* Configure PWM0 for LRA drive (170 Hz resonant frequency)
     * PWM0 drives the LRA via P1.07.
     * 16 MHz clock / 1000 countertop = 16 kHz carrier.
     * To get 170 Hz, we use a sequence of samples that modulate at 170 Hz. */

    /* Configure nPM1300 PMIC:
     * - Enable charging (USB + Qi)
     * - Set charge current to 100 mA (C/1.2 for 120 mAh battery)
     */
    pmic_write_reg(NPM1300_REG_CHGCONFIG, 0x01); /* enable charger */

    /* Configure SAADC for battery monitoring (channel 0, VBAT/3) */
    SAADC_ENABLE = 1;

    s_initialized = true;
}

/* ----------------------------------------------------------------
 * Battery reading
 * ---------------------------------------------------------------- */
uint16_t power_read_battery_mv(void)
{
    /* Sample SAADC on battery monitor channel.
     * VBAT is divided by SAADC_BAT_DIV (3) before hitting the ADC.
     * SAADC reference is 0.6 V, gain 1/6 → 0.1 V full scale at 12-bit.
     * So: VBAT_mV = (sample / 4095) * 3600
     */
    uint16_t sample = saadc_sample(0);
    uint32_t mv = ((uint32_t)sample * 3600) / 4095;
    return (uint16_t)mv;
}

uint8_t power_read_battery_pct(void)
{
    uint16_t mv = power_read_battery_mv();
    if (mv <= BAT_EMPTY_MV)
        return 0;
    if (mv >= BAT_FULL_MV)
        return 100;
    /* Linear interpolation (LiPo discharge curve is not linear, but
     * this is sufficient for a status display) */
    uint32_t pct = ((uint32_t)(mv - BAT_EMPTY_MV) * 100) /
                   (BAT_FULL_MV - BAT_EMPTY_MV);
    return (uint8_t)pct;
}

/* ----------------------------------------------------------------
 * Charging status
 * ---------------------------------------------------------------- */
bool power_is_charging(void)
{
    /* Check Qi detect pin */
    uint32_t qi_state = GPIO_IN(NRF_GPIO_P0_BASE);
    bool qi = (qi_state & (1u << 28)) != 0;

    /* Check USB VBUS detect pin */
    bool usb = (qi_state & (1u << 29)) != 0;

    /* Also read PMIC charge status */
    uint8_t chg_status = pmic_read_reg(NPM1300_REG_CHGSTATUS);
    bool pmic_charging = (chg_status & (NPM1300_CHG_TRICKLE |
                                         NPM1300_CHG_CONST_C)) != 0;

    return qi || usb || pmic_charging;
}

/* ----------------------------------------------------------------
 * LRA vibration motor control
 * ---------------------------------------------------------------- */
void power_lra_enable(bool enable)
{
    if (enable)
        gpio_set(1, 6); /* LRA_EN high */
    else
        gpio_clr(1, 6); /* LRA_EN low */
}

void power_lra_set_intensity(uint8_t percent)
{
    s_lra_intensity = CLAMP(percent, 0, 100);
    /* Adjust PWM duty cycle for intensity.
     * PWM_COUNTERTOP is 1000, so duty = intensity * 10. */
    uint16_t duty = (uint16_t)s_lra_intensity * 10;
    (void)duty; /* In real code, write to PWM compare register */
}

void power_lra_pulse(uint32_t duration_ms)
{
    power_lra_enable(true);
    power_delay_ms(duration_ms);
    power_lra_enable(false);
}

/* ----------------------------------------------------------------
 * Delay functions (busy-wait)
 *   These are called from other drivers (actuator, IMU, etc.)
 * ---------------------------------------------------------------- */
void power_delay_ms(uint32_t ms)
{
    /* Busy-wait using SysTick or a hardware timer.
     * At 128 MHz, each NOP ≈ 8 ns. For 1 ms, we need ~125000 iterations.
     * Use a volatile counter to prevent optimization. */
    volatile uint32_t count;
    while (ms--) {
        count = 0;
        while (count < 32000) count++; /* calibrated for 128 MHz */
    }
}

void power_delay_us_stub(uint32_t us)
{
    volatile uint32_t count;
    while (us--) {
        count = 0;
        while (count < 32) count++; /* ~1 µs at 128 MHz */
    }
}

/* ----------------------------------------------------------------
 * Deep sleep
 * ---------------------------------------------------------------- */
void power_enter_deep_sleep(void)
{
    /* Disable all peripherals except RTC (for wake timer) and GPIOTE
     * (for IMU tap interrupt). Configure System OFF mode. */

    /* Turn off OLED */
    /* Already done by mode_enter(MODE_SLEEP) in main.c */

    /* Turn off actuator HV */
    gpio_clr(1, 0); /* HV_BOOST_EN low */

    /* Turn off LRA */
    power_lra_enable(false);

    /* Configure wake sources:
     *   - IMU INT1 (GPIOTE sense)
     *   - BLE connection (radio event)
     */
    /* Set IMU to wake-on-tap mode */
    /* (handled by imu_enter_low_power() in main.c) */

    /* Enter System OFF (lowest power, ~1 µA) */
    /* In real nRF5340 code: NRF_POWER->SYSTEMOFF = 1; */
    /* For this reference, we just WFE in a loop */
    while (1) {
        __asm volatile ("wfe");
    }
}

/* ----------------------------------------------------------------
 * Temperature (NTC) reading
 * ---------------------------------------------------------------- */
int16_t power_read_temperature(void)
{
    /* Read NTC via SAADC channel 1.
     * NTC is in a voltage divider with a 10k resistor.
     * V_div = VCC * R_ntc / (R_ntc + 10k)
     * Temperature = function of R_ntc.
     *
     * For simplicity, we return a fixed 25°C placeholder.
     * In production, use a lookup table or Steinhart-Hart equation.
     */
    uint16_t sample = saadc_sample(1);
    /* Convert ADC sample to temperature:
     * Assuming 10k NTC Beta=3950, VCC=3.3V, ref=0.6V, gain=1/6
     * This is a simplified conversion. */
    if (sample == 0)
        return 250; /* 25.0°C default */

    /* Steinhart-Hart (simplified): */
    /* R_ntc = (10000 * (4095 - sample)) / sample; */
    /* temp_K = 1 / (1/298.15 + (1/B) * ln(R/10000)); */
    /* temp_C = temp_K - 273.15; */
    /* For reference code, return 25°C */
    return 250;
}

/* ----------------------------------------------------------------
 * Skin contact detection (capacitive)
 * ---------------------------------------------------------------- */
bool power_check_skin_contact(void)
{
    /* Drive the skin-sense electrode and measure decay time.
     * If finger is present, capacitance increases, decay time longer.
     * We sample via SAADC on the skin-sense pin. */
    uint16_t sample = saadc_sample(2);
    /* Threshold: if sample > 2000 (of 4095), finger is present */
    return sample > 2000;
}

/* ----------------------------------------------------------------
 * End of power.c
 * Author: jayis1
 * ---------------------------------------------------------------- */