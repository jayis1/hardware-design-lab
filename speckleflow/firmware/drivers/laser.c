/*
 * laser.c — 785 nm VCSEL laser driver with TEC stabilization
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The VCSEL is driven by a constant-current source controlled by DAC1
 * channel 1 (12-bit, PA4). A TEC (thermoelectric cooler) stabilizes
 * the laser junction temperature to ±0.1 °C using a PID loop driven
 * by TIM8 PWM (H-bridge, MAX1968). The thermistor is read via ADC1
 * channel 1 (PA1).
 *
 * Safety features per IEC 60825-1 Class 3R:
 *   - 5-second soft-start ramp
 *   - Hardware interlock (PC14, active-low)
 *   - Key switch (PB2)
 *   - Auto-shutoff after 30 s of no trigger
 *   - DAC hardware clamp via current-sense comparator
 */

#include "laser.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- State -------------------------------------------------------------- */

static struct {
    uint8_t  enabled;
    uint8_t  ramp_active;
    uint32_t ramp_start_ms;
    uint32_t last_trigger_ms;
    uint16_t dac_target;       /* target DAC value (0–4095) */
    uint16_t dac_current;      /* current DAC value (ramping) */
    int32_t  tec_pid_int;     /* PID integrator */
    int32_t  tec_pid_prev;    /* PID previous error */
    uint16_t setpoint_raw;    /* thermistor ADC setpoint */
} laser;

extern volatile uint32_t system_ms;  /* from main.c 1 ms tick */

/* ---- DAC ---------------------------------------------------------------- */

static void dac_write(uint16_t val) {
    /* Clamp to 12 bits and hardware safety limit */
    if (val > LASER_DAC_30MW) val = LASER_DAC_30MW;
    DAC1->DHR12R1 = val;
    /* Trigger via software */
    DAC1->SWTRIGR = 0x01;
}

/* ---- ADC (thermistor + current sense) ----------------------------------- */

static uint16_t adc_read(uint8_t channel) {
    /* Configure ADC channel and start conversion.
     * We use single-channel, software-triggered, 12-bit mode.
     * This is a simplified blocking read.
     */
    ADC1->SQR1 = (1u << 0) | ((uint32_t)channel << 6);  /* 1 conversion, ch */
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    return (uint16_t)(ADC1->DR & 0xFFF);
}

/* ---- TEC PID loop ------------------------------------------------------- */

static void tec_pid_update(void) {
    /* Read thermistor (ADC channel 1) */
    uint16_t therm = adc_read(ADC_THERM_CH);

    /* Error: positive = too hot, negative = too cold */
    int32_t error = (int32_t)laser.setpoint_raw - (int32_t)therm;

    /* Integral (with anti-windup) */
    laser.tec_pid_int += error * TEC_KI;
    if (laser.tec_pid_int >  100000) laser.tec_pid_int =  100000;
    if (laser.tec_pid_int < -100000) laser.tec_pid_int = -100000;

    /* Derivative */
    int32_t deriv = error - laser.tec_pid_prev;
    laser.tec_pid_prev = error;

    /* PID output */
    int32_t output = error * TEC_KP + laser.tec_pid_int + deriv * TEC_KD;
    output /= 16;  /* scale down */

    /* Clamp to PWM range */
    if (output >  TEC_PWM_MAX) output =  TEC_PWM_MAX;
    if (output < -TEC_PWM_MAX) output = -TEC_PWM_MAX;

    /* Write to TIM8 CH1 (bidirectional H-bridge: sign = direction) */
    if (output >= 0) {
        TIM8->CCR1 = (uint32_t)output;
        /* Set H-bridge direction A (cool) */
        GPIOC->BSRR = (1u << 7);       /* IN1 high */
        GPIOC->BSRR = (1u << (8 + 16)); /* IN2 low */
    } else {
        TIM8->CCR1 = (uint32_t)(-output);
        GPIOC->BSRR = (1u << (7 + 16)); /* IN1 low */
        GPIOC->BSRR = (1u << 8);        /* IN2 high (heat) */
    }
}

/* ---- Public API --------------------------------------------------------- */

int laser_init(void) {
    memset(&laser, 0, sizeof(laser));
    laser.setpoint_raw = TEC_SETPOINT_RAW;

    /* Enable DAC channel 1 */
    DAC1->CR |= DAC_CR_EN1;

    /* Set DAC to 0 (laser off) */
    dac_write(0);

    /* Configure TIM8 for TEC PWM (10 kHz, center-aligned) */
    TIM8->PSC = 12000 - 1;   /* 120 MHz / 12000 = 10 kHz */
    TIM8->ARR = TEC_PWM_MAX;
    TIM8->CCR1 = 0;
    TIM8->BDTR = (1u << 15);  /* MOE = main output enable */
    TIM8->CR1 = TIM_CR1_CEN;

    /* Status LED off */
    LED_LASER_PORT->BSRR = (1u << (LED_LASER_PIN + 16));

    return 0;
}

int laser_enable(uint8_t on) {
    if (on) {
        /* Check safety conditions */
        if (!(KEY_SW_PORT->IDR & (1u << KEY_SW_PIN))) {
            /* Key switch off — refuse to enable */
            return -1;
        }
        if (!(INTERLOCK_PORT->IDR & (1u << INTERLOCK_PIN))) {
            /* Interlock open — refuse to enable */
            return -2;
        }

        laser.enabled = 1;
        laser.ramp_active = 1;
        laser.ramp_start_ms = system_ms;
        laser.dac_target = LASER_DAC_30MW;
        laser.dac_current = 0;

        /* Turn on laser warning LED */
        LED_LASER_PORT->BSRR = (1u << LED_LASER_PIN);
    } else {
        laser.enabled = 0;
        laser.ramp_active = 0;
        laser.dac_current = 0;
        laser.dac_target = 0;
        dac_write(0);
        LED_LASER_PORT->BSRR = (1u << (LED_LASER_PIN + 16));
    }
    return 0;
}

void laser_set_power(uint8_t pct) {
    /* Set target power as percentage of max (0–100) */
    if (pct > 100) pct = 100;
    laser.dac_target = (uint16_t)((uint32_t)LASER_DAC_30MW * pct / 100);
    if (!laser.enabled) return;
    laser.ramp_active = 1;
    laser.ramp_start_ms = system_ms;
}

void laser_trigger(void) {
    laser.last_trigger_ms = system_ms;
}

void laser_tick(void) {
    /* Called every 1 ms from the main tick handler */

    /* 1. Safety checks — disable laser if interlock/key opens */
    if (laser.enabled) {
        if (!(INTERLOCK_PORT->IDR & (1u << INTERLOCK_PIN)) ||
            !(KEY_SW_PORT->IDR & (1u << KEY_SW_PIN))) {
            laser_enable(0);
            return;
        }

        /* Auto-shutoff after 30 s of no trigger */
        if ((system_ms - laser.last_trigger_ms) > LASER_AUTO_OFF_MS) {
            laser_enable(0);
            return;
        }
    }

    /* 2. Soft-start ramp (5 s linear ramp from 0 to target) */
    if (laser.ramp_active && laser.enabled) {
        uint32_t elapsed = system_ms - laser.ramp_start_ms;
        if (elapsed >= LASER_RAMP_MS) {
            laser.dac_current = laser.dac_target;
            laser.ramp_active = 0;
        } else {
            laser.dac_current = (uint16_t)(
                (uint32_t)laser.dac_target * elapsed / LASER_RAMP_MS);
        }
        dac_write(laser.dac_current);
    }

    /* 3. TEC PID update (every 10 ms) */
    static uint32_t last_tec_ms = 0;
    if (system_ms - last_tec_ms >= 10) {
        last_tec_ms = system_ms;
        tec_pid_update();
    }
}

uint16_t laser_get_current_sense(void) {
    return adc_read(ADC_LASER_ISENSE_CH);
}

uint16_t laser_get_thermistor(void) {
    return adc_read(ADC_THERM_CH);
}

int laser_is_enabled(void) {
    return laser.enabled;
}

int laser_is_ramping(void) {
    return laser.ramp_active;
}

void laser_set_tec_setpoint(uint16_t raw) {
    laser.setpoint_raw = raw;
}