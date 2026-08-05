/*
 * drivers/psychro.c — Forced-ventilation psychrometer (wet-bulb) driver
 *
 * Two matched PT100 class-A RTDs are read in a 4-wire configuration.
 * The "dry" RTD is exposed to ambient air; the "wet" RTD is wrapped in
 * a cotton wick fed from a distilled-water reservoir.  A micro fan
 * forces air over both at ~3 m/s for 8 seconds, after which the wet
 * RTD reads the true thermodynamic wet-bulb temperature.
 *
 * The RTDs are excited through a low-side 1 kΩ reference resistor.
 * The voltage across the reference resistor gives the RTD current;
 * the voltage across the RTD (4-wire) gives the RTD voltage.  The
 * ratio gives the RTD resistance, from which temperature is computed
 * via the Callendar-Van Dusen equation (ITS-90, simplified for T > 0 °C).
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "psychro.h"
#include "../board.h"

/* ------------------------------------------------------------------ */
/*  ADC sampling of the two RTD channels                               */
/* ------------------------------------------------------------------ */
#define RTD_SAMPLE_COUNT   64      /* samples averaged per reading */
#define RTD_SAMPLE_HZ      1000    /* 1 kSPS */

static uint16_t adc_read_channel(uint32_t channel)
{
    /* Select channel via ADC CFGR2 / SQR (simplified: single-channel) */
    ADC1->CFGR2 = (channel & 0x1F);
    delay_ms(1);

    /* Start conversion */
    ADC1->CR |= ADC_CR_ADSTART;
    uint32_t to = 0x1000;
    while (!(ADC1->ISR & ADC_ISR_EOC) && to--) ;
    if (to == 0) return 0;
    return (uint16_t)(ADC1->DR1 & 0x0FFF);
}

static uint16_t adc_sample_average(uint32_t channel, uint32_t n)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < n; i++) {
        sum += adc_read_channel(channel);
    }
    return (uint16_t)(sum / n);
}

/* ------------------------------------------------------------------ */
/*  PT100 resistance → temperature (Callendar-Van Dusen, T > 0 °C)     */
/*                                                                    */
/*  R(t) = R0 * (1 + A*t + B*t^2)                                     */
/*  A = 3.9083e-3, B = -5.775e-7, R0 = 100 Ω                          */
/*                                                                    */
/*  Inverted:  t = (-A + sqrt(A^2 - 4B(1 - R/R0))) / (2B)             */
/*  We use fixed-point: resistance in milliohms, temperature in 0.01°C */
/* ------------------------------------------------------------------ */
#define R0_MOHM          100000     /* 100.000 Ω in mΩ */
#define A_COEFF_X1E6     3908300    /* 3.9083e-3 * 1e9 */
#define B_COEFF_X1E6     (-577500)  /* -5.775e-7 * 1e9 */

static int32_t rtd_resistance_to_temp_cx100(uint32_t r_mohm)
{
    /* r_ratio_x1e9 = (R / R0) * 1e9 */
    int64_t r_ratio = ((int64_t)r_mohm * 1000000000LL) / R0_MOHM;
    int64_t one_minus_r = 1000000000LL - r_ratio;  /* (1 - R/R0) * 1e9 */

    /* disc = A^2 - 4B(1 - R/R0)   (all ×1e18) */
    int64_t a2 = (int64_t)A_COEFF_X1E6 * A_COEFF_X1E6;           /* ×1e12 */
    int64_t four_b = 4LL * B_COEFF_X1E6;                         /* ×1e6 */
    int64_t disc = a2 - four_b * one_minus_r;                    /* ×1e18 */

    /* Integer square root of disc (×1e18) → sqrt ×1e9 */
    if (disc < 0) disc = 0;
    int64_t s = disc;
    /* Newton-Raphson integer sqrt, 6 iterations from a good guess */
    int64_t x0 = 1000000000LL;  /* 1e9 */
    for (int i = 0; i < 8; i++) {
        if (x0 == 0) break;
        x0 = (x0 + s / x0) / 2;
    }
    int64_t sqrt_disc = x0;   /* ×1e9 */

    /* t = (-A + sqrt(disc)) / (2B)   ; A ×1e6, B ×1e6, sqrt ×1e9 */
    /* numerator ×1e9 = -A*1e3 + sqrt_disc  (A ×1e6 → ×1e9 by *1e3) */
    int64_t num = -((int64_t)A_COEFF_X1E6 * 1000LL) + sqrt_disc;
    int64_t den = 2LL * B_COEFF_X1E6 * 1000LL;  /* ×1e9 */
    if (den == 0) return 0;

    /* t in °C = num/den ; convert to 0.01 °C by ×100 */
    int32_t temp_cx100 = (int32_t)((num * 100LL) / den);
    return temp_cx100;
}

/* ------------------------------------------------------------------ */
/*  Compute RTD resistance from ADC readings                           */
/*                                                                    */
/*  V_ref = I_exc * R_ref  (across 1 kΩ reference)                    */
/*  V_rtd = I_exc * R_rtd  (across RTD, 4-wire)                       */
/*  R_rtd = R_ref * (V_rtd / V_ref) = R_ref * (adc_rtd / adc_ref)     */
/* ------------------------------------------------------------------ */
#define R_REF_MOHM    1000000   /* 1.000 kΩ = 1,000,000 mΩ */

static uint32_t adc_to_resistance_mohm(uint16_t adc_rtd, uint16_t adc_ref)
{
    if (adc_ref == 0) return 0;
    return (uint32_t)(((uint64_t)adc_rtd * R_REF_MOHM) / adc_ref);
}

/* ------------------------------------------------------------------ */
/*  Public: run the psychrometer cycle                                 */
/*                                                                    */
/*  1. Turn on the fan.                                                */
/*  2. Wait for wet-bulb stabilization (8 s).                          */
/*  3. Sample both RTDs at 1 kSPS for 2 s, average.                    */
/*  4. Turn off the fan.                                               */
/*  5. Convert to temperatures and compute depression.                 */
/* ------------------------------------------------------------------ */
int psychro_measure(int32_t *tdry_cx100, int32_t *twet_cx100,
                    int32_t *depression_cx100, uint8_t *wick_dry)
{
    *wick_dry = 0;

    /* Fan on */
    FAN_ON();
    delay_ms(FAN_ON_DURATION_MS);

    /* Sample dry-bulb (ADC channel 0) and wet-bulb (ADC channel 1) */
    uint16_t adc_dry_rtd = adc_sample_average(0, RTD_SAMPLE_COUNT);
    uint16_t adc_dry_ref = adc_sample_average(2, RTD_SAMPLE_COUNT);
    uint16_t adc_wet_rtd = adc_sample_average(1, RTD_SAMPLE_COUNT);
    uint16_t adc_wet_ref = adc_sample_average(3, RTD_SAMPLE_COUNT);

    /* Fan off */
    FAN_OFF();

    /* Convert to resistance */
    uint32_t r_dry = adc_to_resistance_mohm(adc_dry_rtd, adc_dry_ref);
    uint32_t r_wet = adc_to_resistance_mohm(adc_wet_rtd, adc_wet_ref);

    /* Convert to temperature */
    *tdry_cx100 = rtd_resistance_to_temp_cx100(r_dry);
    *twet_cx100 = rtd_resistance_to_temp_cx100(r_wet);

    /* Wet-bulb depression */
    *depression_cx100 = *tdry_cx100 - *twet_cx100;

    /* Wick-dry detection: if depression < 0.20 °C, wick is likely dry */
    if (*depression_cx100 < 20) {
        *wick_dry = 1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Psychrometric computation: from dry-bulb and wet-bulb to           */
/*  dew point and vapor pressure (simplified Magnus formula).          */
/*                                                                    */
/*  e = e_w - A * p * (T - Tw)          (psychrometric equation)       */
/*  where e_w = 6.112 * exp(17.62*Tw/(243.12+Tw))  (hPa)               */
/*  A = 0.00066(1 + 0.00115*Tw)  for ventilated psychrometer           */
/*  p = station pressure in hPa                                        */
/*                                                                    */
/*  Dew point: Td = 243.12 * ln(e/6.112) / (17.62 - ln(e/6.112))       */
/*  All in 0.01 °C units.                                              */
/* ------------------------------------------------------------------ */
int32_t psychro_dewpoint_cx100(int32_t tdry_cx100, int32_t twet_cx100,
                               int32_t pressure_hpa)
{
    /* Convert inputs to float-equivalent (we use Q16.16 fixed-point) */
    /* For simplicity and speed, we use a Q1000000 representation internally */

    int64_t T  = (int64_t)tdry_cx100;        /* 0.01 °C */
    int64_t Tw = (int64_t)twet_cx100;
    int64_t p  = (int64_t)pressure_hpa;

    /* e_w = 6.112 * exp(17.62 * Tw / (243.12 + Tw))   (hPa, Tw in °C) */
    /* Compute exponent in fixed-point with Taylor approximation */
    /* x = 17.62 * Tw / (243.12 + Tw) */
    int64_t Tw_c = Tw;   /* 0.01 °C */
    int64_t denom = 24312 + Tw_c / 100;  /* (243.12 + Tw) in 0.01 */
    if (denom == 0) return tdry_cx100;
    /* x in milli-units: 17.62 * Tw_c / denom  →  ×1000 scaling */
    int64_t x_milli = (17620LL * Tw_c) / denom;  /* x × 1000000 */

    /* exp(x) via Taylor series (x typically 0.1 to 0.5) */
    /* We compute e_w in 0.001 hPa */
    int64_t x = x_milli;  /* ×1e6 */
    int64_t exp_val = 1000000LL + x + (x*x)/(2000000LL) +
                      (x*x*x)/(6000000000000LL);
    int64_t e_w = (6112LL * exp_val) / 1000000LL;  /* hPa × 1000 */

    /* e = e_w - A * p * (T - Tw) */
    /* A = 0.00066 * (1 + 0.00115 * Tw)   (dimensionless) */
    int64_t A_milli = 660LL + (660LL * 115LL * Tw_c) / (100000LL * 100LL);
    /* e = e_w - A * p * (T - Tw)  ;  all in consistent units */
    int64_t e = e_w - (A_milli * p * (T - Tw)) / 1000000LL;
    if (e < 0) e = 0;

    /* Td = 243.12 * ln(e/6.112) / (17.62 - ln(e/6.112)) */
    /* ln(e/6.112) — approximate ln via series for y = e/6.112 */
    int64_t y_milli = e / 6112;   /* e / 6.112, in 0.001 hPa → ratio × 1000 */
    if (y_milli <= 0) return -10000;  /* -100.00 °C sentinel */
    /* ln(y) ≈ 2 * [(y-1)/(y+1) + 1/3 ((y-1)/(y+1))^3]  for y near 1 */
    int64_t num_ln = y_milli - 1000;
    int64_t den_ln = y_milli + 1000;
    if (den_ln == 0) return -10000;
    int64_t z = (num_ln * 1000000LL) / den_ln;  /* (y-1)/(y+1) × 1e6 */
    int64_t ln_y = 2LL * (z + (z*z*z)/(3LL*1000000000000LL));
    /* ln_y in ×1e6 units */

    int64_t td = (24312LL * ln_y) / (17620LL - ln_y / 100LL);
    /* td in 0.01 °C */
    return (int32_t)td;
}