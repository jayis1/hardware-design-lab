/*
 * sapflow.c — Sap-flow heat-pulse measurement driver
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements the heat-pulse sap-flow measurement technique:
 *
 *  1. A resistive heater probe is inserted into the plant stem.
 *  2. Two thermistor probes are inserted at known distances above
 *     (downstream) and below (upstream) the heater.
 *  3. A short heat pulse (4 seconds) is applied.
 *  4. Temperature is sampled at 10 Hz for 60 seconds post-pulse.
 *  5. The time-to-peak (t_max) at the downstream sensor gives the
 *     heat pulse velocity: v = sqrt(α / (π × t_max))
 *     where α is the thermal diffusivity of the stem.
 *
 * This velocity correlates with sap flow rate.  When sap flows upward,
 * the heat pulse arrives at the upper sensor faster (advective transport)
 * than at the lower sensor.  The ratio of temperature rises gives the
 * actual flow rate via the heat ratio method (HRM).
 *
 * Hardware:
 *  - TIM1_CH1 (PC0): PWM output → MOSFET gate driver → heater resistor
 *  - ADS1220 (SPI2): 24-bit ADC reading two NTC thermistors
 *  - PC1: heater enable (MOSFET gate driver enable)
 */

#include "sapflow.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  Measurement buffer                                                   */
/* ===================================================================== */

#define SAPFLOW_SAMPLES  600   /* 60 s × 10 Hz */

static float temp_upper[SAPFLOW_SAMPLES];
static float temp_lower[SAPFLOW_SAMPLES];
static uint16_t sample_idx = 0;

static float baseline_temp = 0.0f;
static float delta_t_upper = 0.0f;
static float delta_t_lower = 0.0f;
static float t_max_upper = 0.0f;
static float velocity = 0.0f;
static uint8_t measuring = 0;
static uint8_t async_trigger = 0;

/* ===================================================================== */
/*  SPI2 low-level (shared with SD card — CS multiplexed)                */
/* ===================================================================== */

static void spi2_wait_tx(void)
{
    while (!(SPI_REG(SPI2_BASE, SPI_SR) & SPI_SR_TXE))
        ;
}

static void spi2_wait_rx(void)
{
    while (!(SPI_REG(SPI2_BASE, SPI_SR) & SPI_SR_RXNE))
        ;
}

static uint8_t spi2_xfer(uint8_t tx)
{
    spi2_wait_tx();
    SPI_REG(SPI2_BASE, SPI_DR) = tx;
    spi2_wait_rx();
    return (uint8_t)SPI_REG(SPI2_BASE, SPI_DR);
}

static void ads1220_select(void)
{
    gpio_reset(ADS1220_CS_PORT, ADS1220_CS_PIN);
}

static void ads1220_deselect(void)
{
    gpio_set(ADS1220_CS_PORT, ADS1220_CS_PIN);
}

/* ===================================================================== */
/*  ADS1220 ADC driver (minimal)                                         */
/* ===================================================================== */

static void ads1220_wreg(uint8_t reg, uint8_t val)
{
    ads1220_select();
    spi2_xfer(ADS1220_CMD_WREG | (reg << 2));
    spi2_xfer(val);
    ads1220_deselect();
}

static uint8_t ads1220_rreg(uint8_t reg)
{
    ads1220_select();
    spi2_xfer(ADS1220_CMD_RREG | (reg << 2));
    uint8_t val = spi2_xfer(0x00U);
    ads1220_deselect();
    return val;
}

static int32_t ads1220_read_data(void)
{
    int32_t val;
    ads1220_select();
    spi2_xfer(ADS1220_CMD_RDATA);
    /* Wait for DRDY (simplified: poll pin) */
    uint32_t timeout = 1000000;
    while (gpio_read(ADS1220_DRDY_PORT, ADS1220_DRDY_PIN) && timeout--)
        ;

    /* Read 3 bytes (24-bit signed) */
    uint8_t b0 = spi2_xfer(0x00U);
    uint8_t b1 = spi2_xfer(0x00U);
    uint8_t b2 = spi2_xfer(0x00U);
    ads1220_deselect();

    val = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | b2;
    if (val & 0x800000U) val |= 0xFF000000U;
    return val;
}

/* Configure ADS1220 for single-shot mode on AIN0 (upper thermistor) */
static void ads1220_config_channel(uint8_t channel)
{
    /* Config0: input multiplexer AIN_p = AIN_n
     * AIN0P-AIN1N (upper thermistor bridge): mux = 0x00
     * AIN2P-AIN3N (lower thermistor bridge): mux = 0x10
     * Gain = 1, PGA bypassed (use PGA for better CMRR)
     */
    uint8_t mux = (channel == 0) ? 0x00U : 0x10U;
    ads1220_wreg(0x00U, mux | 0x00U);  /* Gain 1, PGA enabled */

    /* Config1: data rate 20 SPS (good noise), normal mode, continuous */
    ads1220_wreg(0x01U, 0x04U);  /* 20 SPS, normal, continuous */

    /* Config2: internal Vref 2.048 V, 50 Hz rejection */
    ads1220_wreg(0x02U, 0x10U);  /* Vref internal, 50 Hz reject */

    /* Config3: IREF off, 50 µs warm-up */
    ads1220_wreg(0x03U, 0x00U);
}

/* ===================================================================== */
/*  Thermistor conversion                                                */
/* ===================================================================== */

/* NTC thermistor: 10 kΩ @ 25°C, Beta = 3977 K
 * Circuit: voltage divider with 10 kΩ reference resistor
 * ADC reads voltage across thermistor
 * R_therm = R_ref × (ADC / (2^23 - 1)) / (1 - ADC / (2^23 - 1))
 * Then: T = 1 / (1/T0 + (1/B) × ln(R/R0))
 */
static float adc_to_temp(int32_t adc)
{
    const float R_ref = 10000.0f;       /* 10 kΩ */
    const float R0    = 10000.0f;       /* @ 25°C */
    const float T0    = 298.15f;        /* 25°C in K */
    const float B     = 3977.0f;
    const float full_scale = 8388608.0f;  /* 2^23 */

    float ratio = (float)adc / full_scale;
    if (ratio <= 0.0f || ratio >= 1.0f) return -273.15f;

    float R_therm = R_ref * ratio / (1.0f - ratio);
    if (R_therm <= 0.0f) return -273.15f;

    float inv_T = (1.0f / T0) + (1.0f / B) * logf(R_therm / R0);
    float T_kelvin = 1.0f / inv_T;
    return T_kelvin - 273.15f;
}

/* ===================================================================== */
/*  Heater control                                                       */
/* ===================================================================== */

static void heater_init(void)
{
    /* Enable GPIOC and TIM1 clocks */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    RCC_APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* PC0 as TIM1_CH1 AF2, PC1 as GPIO output */
    volatile uint32_t *pc_moder  = (volatile uint32_t *)(GPIOC_BASE + GPIO_MODER);
    volatile uint32_t *pc_afrl   = (volatile uint32_t *)(GPIOC_BASE + GPIO_AFRL);
    volatile uint32_t *pc_ospeed = (volatile uint32_t *)(GPIOC_BASE + GPIO_OSPEEDR);

    /* PC0: AF2 (TIM1_CH1 on L4) */
    *pc_moder &= ~(0x3U << (HEATER_PWM_PIN * 2));
    *pc_moder |= (0x2U << (HEATER_PWM_PIN * 2));
    *pc_afrl &= ~(0xFU << (HEATER_PWM_PIN * 4));
    *pc_afrl |= ((uint32_t)0x2U << (HEATER_PWM_PIN * 4));  /* AF2 */
    *pc_ospeed |= (0x3U << (HEATER_PWM_PIN * 2));

    /* PC1: output (heater enable) */
    *pc_moder |= (0x1U << (HEATER_EN_PIN * 2));
    gpio_reset(HEATER_EN_PORT, HEATER_EN_PIN);

    /* TIM1: PWM mode, one-shot for heater pulse
     * 80 MHz / 1000 = 80 kHz → 1 ms resolution
     * ARR = 4000 → 4 s pulse
     */
    TIM1_PSC = 999;   /* 80 MHz / 1000 = 80 kHz */
    TIM1_ARR = 320000U;  /* 4 s at 80 kHz */
    TIM1_CCR1 = 160000U; /* 50% duty (safe for resistive heater) */

    TIM1_CCMR1 = 0;
    TIM1_CCMR1 |= TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM1_CCER = TIM_CCER_CC1E;
    TIM1_BDTR = TIM_BDTR_MOE;
    TIM1_CR1 = TIM_CR1_ARPE | TIM_CR1_OPM;  /* One-pulse mode */
    TIM1_EGR = 1;
}

static void heater_pulse(uint32_t duration_ms)
{
    /* Configure ARR for desired pulse duration */
    TIM1_ARR = (duration_ms * 80U);  /* 80 kHz tick */
    TIM1_CCR1 = TIM1_ARR / 2;

    /* Enable heater gate driver */
    gpio_set(HEATER_EN_PORT, HEATER_EN_PIN);

    /* Start one-shot pulse */
    TIM1_CR1 = TIM_CR1_ARPE | TIM_CR1_OPM | TIM_CR1_CEN;
    TIM1_BDTR |= TIM_BDTR_MOE;

    /* Wait for pulse to complete (UIF set in one-shot mode) */
    while (!(TIM1_SR & TIM_SR_UIF))
        ;

    /* Disable heater gate driver */
    gpio_reset(HEATER_EN_PORT, HEATER_EN_PIN);

    /* Clear flag */
    TIM1_SR = 0;
}

/* ===================================================================== */
/*  Public API                                                           */
/* ===================================================================== */

void sapflow_init(void)
{
    /* Initialize heater */
    heater_init();

    /* Initialize SPI2 for ADS1220 */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    RCC_APB1ENR1 |= RCC_APB1ENR1_SPI2EN;

    /* PB13 (SCK), PB14 (MISO), PB15 (MOSI) as AF5 */
    volatile uint32_t *pb_moder  = (volatile uint32_t *)(GPIOB_BASE + GPIO_MODER);
    volatile uint32_t *pb_afrh   = (volatile uint32_t *)(GPIOB_BASE + GPIO_AFRH);
    volatile uint32_t *pb_ospeed = (volatile uint32_t *)(GPIOB_BASE + GPIO_OSPEEDR);

    for (uint8_t pin = 13; pin <= 15; pin++) {
        *pb_moder &= ~(0x3U << (pin * 2));
        *pb_moder |= (0x2U << (pin * 2));
        *pb_afrh &= ~(0xFU << ((pin - 8) * 4));
        *pb_afrh |= ((uint32_t)GPIO_AF5 << ((pin - 8) * 4));
        *pb_ospeed |= (0x3U << (pin * 2));
    }

    /* PB4 (ADS1220_CS), PB5 (DRDY input) */
    volatile uint32_t *pb_moder2 = (volatile uint32_t *)(GPIOB_BASE + GPIO_MODER);
    *pb_moder2 |= (0x1U << (ADS1220_CS_PIN * 2));   /* CS output */
    *pb_moder2 &= ~(0x3U << (ADS1220_DRDY_PIN * 2)); /* DRDY input */
    gpio_set(ADS1220_CS_PORT, ADS1220_CS_PIN);

    /* SPI2: master, CPOL=1, CPHA=1, 2.5 MHz */
    SPI_REG(SPI2_BASE, SPI_CR1) = 0;
    SPI_REG(SPI2_BASE, SPI_CR1) = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                                   SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_BR_DIV32;
    SPI_REG(SPI2_BASE, SPI_CR2) = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI_REG(SPI2_BASE, SPI_CR1) |= SPI_CR1_SPE;

    /* Reset ADS1220 */
    ads1220_select();
    spi2_xfer(ADS1220_CMD_RESET);
    ads1220_deselect();
    for (volatile int i = 0; i < 10000; i++)
        ;

    measuring = 0;
    velocity = 0.0f;
}

void sapflow_measure(void)
{
    measuring = 1;
    sample_idx = 0;

    /* --- Phase 1: Baseline temperature (5 samples at 10 Hz) --- */
    float t_upper_sum = 0, t_lower_sum = 0;
    for (int i = 0; i < 5; i++) {
        ads1220_config_channel(0);  /* Upper thermistor */
        int32_t adc_u = ads1220_read_data();
        float t_u = adc_to_temp(adc_u);

        ads1220_config_channel(1);  /* Lower thermistor */
        int32_t adc_l = ads1220_read_data();
        float t_l = adc_to_temp(adc_l);

        t_upper_sum += t_u;
        t_lower_sum += t_l;

        /* 100 ms delay (10 Hz) */
        for (volatile uint32_t j = 0; j < 800000; j++)
            ;
    }
    baseline_temp = (t_upper_sum + t_lower_sum) / 10.0f;

    /* --- Phase 2: Fire heat pulse (4 seconds) --- */
    heater_pulse(BOARD_HEATER_PULSE_MS);

    /* --- Phase 3: Post-pulse sampling (60 s at 10 Hz) --- */
    float max_upper = -273.15f;
    uint16_t max_upper_idx = 0;

    for (sample_idx = 0; sample_idx < SAPFLOW_SAMPLES; sample_idx++) {
        ads1220_config_channel(0);
        int32_t adc_u = ads1220_read_data();
        temp_upper[sample_idx] = adc_to_temp(adc_u);

        ads1220_config_channel(1);
        int32_t adc_l = ads1220_read_data();
        temp_lower[sample_idx] = adc_to_temp(adc_l);

        /* Track peak at upper sensor */
        if (temp_upper[sample_idx] > max_upper) {
            max_upper = temp_upper[sample_idx];
            max_upper_idx = sample_idx;
        }

        /* 100 ms delay */
        for (volatile uint32_t j = 0; j < 800000; j++)
            ;
    }

    /* --- Phase 4: Compute results --- */
    delta_t_upper = max_upper - baseline_temp;
    t_max_upper = (float)max_upper_idx / 10.0f;  /* Convert to seconds */

    /* Heat pulse velocity (T-max method):
     * v = sqrt(α / (π × t_max))
     * α = thermal diffusivity of wood ≈ 2.5×10⁻⁷ m²/s
     * Result in m/s, convert to cm/h
     */
    if (t_max_upper > 0.1f) {
        float v_ms = sqrtf(BOARD_WOOD_THERMAL_DIFFUSIVITY /
                           (3.14159265f * t_max_upper));
        velocity = v_ms * 100.0f * 3600.0f;  /* m/s → cm/h */
    } else {
        velocity = 0.0f;
    }

    /* Also compute lower sensor delta */
    float max_lower = -273.15f;
    for (uint16_t i = 0; i < SAPFLOW_SAMPLES; i++) {
        if (temp_lower[i] > max_lower)
            max_lower = temp_lower[i];
    }
    delta_t_lower = max_lower - baseline_temp;

    measuring = 0;
}

float sapflow_get_velocity(void)        { return velocity; }
float sapflow_get_baseline_temp(void)   { return baseline_temp; }
float sapflow_get_delta_t(void)         { return delta_t_upper; }
float sapflow_get_tmax(void)            { return t_max_upper; }
uint8_t sapflow_is_measuring(void)      { return measuring; }

void sapflow_get_temps(float *upper, float *lower)
{
    *upper = delta_t_upper;
    *lower = delta_t_lower;
}

void sapflow_trigger_async(void)
{
    async_trigger = 1;
}

uint8_t sapflow_poll(void)
{
    if (async_trigger) {
        async_trigger = 0;
        sapflow_measure();
        return 1;
    }
    return 0;
}

/* Math functions */
extern double sqrtf(double x);
extern double logf(double x);