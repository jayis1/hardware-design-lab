/*
 * flow.c — AeroCast sample-flow controller
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Closed-loop control of the intake blower to hold the cuvette at a
 * constant 15 L/min using a Sensirion SDP810 differential-pressure
 * sensor and a PI controller. The SDP810 reports differential pressure
 * across a calibrated critical orifice; mass-flow is derived from the
 * Bernoulli relation pre-calibrated in factory.
 *
 * The blower is a brushless radial micro-fan driven by TIM2_CH1 PWM
 * at 20 kHz. PWM duty 0–4095 maps to 0–100 % fan speed.
 */
#ifndef AEROCAST_FLOW_C
#define AEROCAST_FLOW_C
#endif

#include <stdint.h>
#include <math.h>
#include "board.h"
#include "registers.h"
#include "flow.h"

/* ---- Module state ---- */
static struct {
    float    target_lpm;
    float    measured_lpm;
    float    integral;
    uint16_t pwm;
    uint8_t  initialized;
} g_flow;

/* SDP810 I2C address (0x25 << 1) */
#define SDP810_ADDR     0x25
#define SDP810_RAW_READ 0x3624u   /* continuous read, mass-flow compensated */

/* ---- Low-level I2C transaction (blocking, polled) ---- */
static int sdp810_read_raw(int16_t *dp_raw, int16_t *temp_raw)
{
    i2c_t *i = I2C(I2C4);
    /* Issue continuous measurement command */
    i->CR2 = (SDP810_ADDR << 1) | (2u << 16u) | (1u << 25u) /* AUTOEND */;
    i->TXDR = (SDP810_RAW_READ >> 8) & 0xFFu;
    while (!(i->ISR & (1u << 6u))) ;   /* TXIS */
    i->TXDR = SDP810_RAW_READ & 0xFFu;
    while (!(i->ISR & (1u << 5u))) ;   /* STOPF */
    i->ICR = (1u << 5u);

    /* Wait ~20 ms for first conversion */
    delay_ms(20);

    /* Read 9 bytes: 3× (value + CRC). We only need the first two values. */
    uint8_t buf[9];
    i->CR2 = ((SDP810_ADDR << 1) | 1u) | (9u << 16u) | (1u << 25u);
    for (int k = 0; k < 9; ++k) {
        while (!(i->ISR & (1u << 2u))) ;  /* RXNE */
        buf[k] = (uint8_t)i->RXDR;
    }
    i->ICR = (1u << 5u);

    /* Basic CRC-8 check on each pair (Sensirion polynomial 0x31) */
    for (int pair = 0; pair < 3; ++pair) {
        uint8_t crc = 0x00u;
        for (int b = 0; b < 2; ++b) {
            crc ^= buf[pair * 3 + b];
            for (int bit = 0; bit < 8; ++bit)
                crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x31u) : (uint8_t)(crc << 1);
        }
        if (crc != buf[pair * 3 + 2]) return -1;  /* CRC fail */
    }

    *dp_raw    = (int16_t)((buf[0] << 8) | buf[1]);
    *temp_raw  = (int16_t)((buf[3] << 8) | buf[4]);
    return 0;
}

/* Convert SDP810 raw differential pressure to L/min using the
 * factory calibration for the AeroCast orifice:
 *   flow_lpm = scale * sqrt(dp_pa)  (Bernoulli, turbulent)
 * where dp_pa = dp_raw * 0.04 (SDP810 scale factor for 500 Pa range).
 * The scale constant was derived against a Gilibrator primary standard. */
#define SDP810_DP_SCALE_PA   0.04f
#define ORIFICE_FLOW_SCALE   3.354f    /* L/min per sqrt(Pa) — calibrated */

static float dp_to_lpm(int16_t dp_raw)
{
    float dp_pa = (float)dp_raw * SDP810_DP_SCALE_PA;
    if (dp_pa < 0.0f) dp_pa = 0.0f;
    return ORIFICE_FLOW_SCALE * sqrtf(dp_pa);
}

/* ---- Public API ---- */
void flow_init(void)
{
    /* TIM2_CH1: 20 kHz PWM, 0–4095 duty (12-bit-ish). */
    tim_init_pwm(TIM2_BASE, /* PSC */ 280000000u / (20000u * 4096u),
                 /* ARR */ 4095u, /* CCR */ 0u);
    /* I2C4 at 100 kHz (approx TIMINGR for 280 MHz / 100k) */
    i2c_init(I2C4_BASE, 0x10909CECu);

    g_flow.target_lpm = 0.0f;
    g_flow.measured_lpm = 0.0f;
    g_flow.integral = 0.0f;
    g_flow.pwm = 0;
    g_flow.initialized = 1;
}

void flow_set_target(float lpm)
{
    if (lpm < 0.0f) lpm = 0.0f;
    if (lpm > 25.0f) lpm = 25.0f;
    g_flow.target_lpm = lpm;
}

float flow_measured(void)
{
    return g_flow.measured_lpm;
}

float flow_target(void)
{
    return g_flow.target_lpm;
}

uint16_t flow_pwm(void)
{
    return g_flow.pwm;
}

/* Called every 100 ms from main loop. Reads sensor, runs PI, writes PWM. */
void flow_pi_step(void)
{
    if (!g_flow.initialized) return;

    int16_t dp_raw = 0, t_raw = 0;
    if (sdp810_read_raw(&dp_raw, &t_raw) == 0) {
        g_flow.measured_lpm = dp_to_lpm(dp_raw);
    }
    /* If sensor read fails, hold last measurement; PI will still
     * servo on the stale value, which is safer than zeroing. */

    float err = g_flow.target_lpm - g_flow.measured_lpm;
    g_flow.integral += err * 0.1f;            /* 100 ms dt */
    if (g_flow.integral >  FLOW_I_LIMIT) g_flow.integral =  FLOW_I_LIMIT;
    if (g_flow.integral < -FLOW_I_LIMIT) g_flow.integral = -FLOW_I_LIMIT;

    float out = FLOW_KP * err + FLOW_KI * g_flow.integral;
    if (out < 0.0f) out = 0.0f;
    if (out > 4095.0f) out = 4095.0f;

    g_flow.pwm = (uint16_t)out;
    tim_set_duty(TIM2_BASE, g_flow.pwm);
}

/* Diagnostic: returns the most recent raw differential pressure in Pa. */
float flow_dp_pa(void)
{
    int16_t dp_raw = 0, t_raw = 0;
    if (sdp810_read_raw(&dp_raw, &t_raw) != 0) return -1.0f;
    return (float)dp_raw * SDP810_DP_SCALE_PA;
}