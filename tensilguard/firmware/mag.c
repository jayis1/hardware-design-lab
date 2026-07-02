/*
 * mag.c — Magnetoelastic (Villari-effect) tension sensing
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Drives the excitation coil at a swept set of frequencies, demodulates the
 * coil voltage and shunt current into I/Q components at each step, extracts
 * the effective inductance (proxy for permeability), and converts it to a
 * tension estimate via the per-node calibration curve.
 *
 * The I/Q demodulation inner loop uses the FMAC streaming multiply-accumulate
 * path (conceptually); this file uses a portable software implementation so
 * it compiles and runs without the vendor's CMSIS-DSP dependency. The math
 * is identical: multiply each sample by sin/cos of the demodulation frequency
 * and accumulate. We then compute |Z| and phase from the I/Q of voltage and
 * current.
 */
#include <math.h>
#include <string.h>
#include "tensilguard.h"
#include "board.h"
#include "registers.h"
#include "hal.h"

/* ----------------------------- Local state -------------------------------- */
static int16_t  s_vcoil[MAG_NSAMP_PER_STEP];  /* coil voltage samples       */
static int16_t  s_vshunt[MAG_NSAMP_PER_STEP];  /* shunt voltage samples      */
static float    s_zmag[MAG_NFREQ];             /* |Z| per frequency step      */
static float    s_zphase[MAG_NFREQ];            /* phase angle per step        */

/* demodulation NCO tables (precomputed sine/cosine) */
static float s_sin_tab[MAG_NSAMP_PER_STEP];
static float s_cos_tab[MAG_NSAMP_PER_STEP];
static bool  s_nco_ready = false;

/* learned resonance peak index */
static int   s_resonance_idx = -1;
static float s_mu_eff_baseline = 1.0f;

/* ----------------------------- Prototypes --------------------------------- */
static void nco_init(float f_demod_hz, float fs_hz);
static void demod_iq(const int16_t *v, float fs, float *I, float *Q);
static float interp_tension(float mu_eff, const cal_page_t *cal);
static void find_resonance(void);

/* ----------------------------- Public API --------------------------------- */

/*
 * mag_sweep() — run the full frequency sweep, return mu_eff and tension.
 * Returns true on success (valid data), false on hardware fault.
 */
bool mag_sweep(float *mu_eff_out, float *t_mag_out)
{
    if (!s_nco_ready) {
        nco_init(MAG_FMIN_HZ, (float)MAG_ADC_RATE_HZ);
    }

    /* enable 24 V boost for the coil rail */
    gpio_set(PIN_MAG_BOOST_EN, 1);
    delay_ms(TG_BOOST_SETTLE_MS);

    bool ok = true;
    for (int step = 0; step < MAG_NFREQ; step++) {
        /* log-spaced frequency */
        float flog = logf(MAG_FMIN_HZ) +
                      (logf(MAG_FMAX_HZ) - logf(MAG_FMIN_HZ)) *
                      (float)step / (float)(MAG_NFREQ - 1);
        float f_hz = expf(flog);

        /* configure TIM1 PWM for the coil H-bridge at f_hz, 50 % duty */
        tim1_set_pwm_freq(f_hz);

        /* configure NCO tables for demodulation at f_hz */
        nco_init(f_hz, (float)MAG_ADC_RATE_HZ);

        /* settle, then trigger simultaneous ADC capture of vcoil + vshunt */
        delay_ms(MAG_SETTLE_MS);
        adc_capture_dual(ADC_VCOIL_CH, ADC_VSHUNT_CH,
                         s_vcoil, s_vshunt, MAG_NSAMP_PER_STEP);

        /* demodulate both channels */
        float Iv, Qv, Ii, Qi;
        demod_iq(s_vcoil, (float)MAG_ADC_RATE_HZ, &Iv, &Qv);
        demod_iq(s_vshunt, (float)MAG_ADC_RATE_HZ, &Ii, &Qi);

        /* compute |Z| = |V| / |I| and phase = phase(V) - phase(I) */
        float mag_v = sqrtf(Iv * Iv + Qv * Qv);
        float mag_i = sqrtf(Ii * Ii + Qi * Qi);
        if (mag_i < 1e-3f) {
            ok = false;
            s_zmag[step] = 0.0f;
            s_zphase[step] = 0.0f;
            continue;
        }
        s_zmag[step] = mag_v / mag_i;
        float ph_v = atan2f(Qv, Iv);
        float ph_i = atan2f(Qi, Ii);
        s_zphase[step] = (ph_v - ph_i) * 57.2957795f; /* rad → deg */
        if (s_zphase[step] < 0.0f) s_zphase[step] += 360.0f;
    }

    /* disable the boost immediately — energy budget matters */
    tim1_pwm_disable();
    gpio_set(PIN_MAG_BOOST_EN, 0);

    if (!ok) {
        *mu_eff_out = 0.0f;
        *t_mag_out = 0.0f;
        return false;
    }

    /* identify the resonance step (max |Z|) for sensitivity */
    find_resonance();

    /* extract effective permeability from the inductance at resonance.
     * L = (|Z| / (2*pi*f)) - R_dc, but for relative comparison we use a
     * normalized mu_eff = L_meas / L_ref.  L_ref is the baseline taken
     * at calibration (or the first valid sweep). */
    int idx = (s_resonance_idx >= 0) ? s_resonance_idx : (MAG_NFREQ / 2);
    float f_at = MAG_FMIN_HZ *
                 powf(MAG_FMAX_HZ / MAG_FMIN_HZ, (float)idx / (float)(MAG_NFREQ - 1));
    float L_meas = s_zmag[idx] / (2.0f * 3.14159265f * f_at);
    float mu_eff = L_meas / s_mu_eff_baseline;

    /* temperature compensation — done by caller via cal page tempcoef */
    *mu_eff_out = mu_eff;
    *t_mag_out = interp_tension(mu_eff, &g_cal);
    return true;
}

/*
 * mag_calibrate_step() — record one (T, mu_eff) calibration point.
 * Called from the calibration wizard; the third call finalises the curve.
 */
void mag_calibrate_step(float t_kn, float mu_eff)
{
    static int step = 0;
    if (step < 3) {
        g_cal.t_cal_c[step] = t_kn;
        g_cal.mu_cal[step]  = mu_eff;
        step++;
    }
    if (step == 3) {
        /* sort by tension ascending (simple 3-element sort) */
        for (int i = 0; i < 3; i++) {
            int m = i;
            for (int j = i + 1; j < 3; j++) {
                if (g_cal.t_cal_c[j] < g_cal.t_cal_c[m]) m = j;
            }
            if (m != i) {
                float tt = g_cal.t_cal_c[i];
                g_cal.t_cal_c[i] = g_cal.t_cal_c[m];
                g_cal.t_cal_c[m] = tt;
                float tm = g_cal.mu_cal[i];
                g_cal.mu_cal[i] = g_cal.mu_cal[m];
                g_cal.mu_cal[m] = tm;
            }
        }
        /* baseline mu_eff0 = mu at lowest tension */
        s_mu_eff_baseline = g_cal.mu_cal[0];
        g_cal.mu_eff0 = s_mu_eff_baseline;
        /* piecewise-linear coefficients: slope1, slope2 */
        float dT1 = g_cal.t_cal_c[1] - g_cal.t_cal_c[0];
        float dT2 = g_cal.t_cal_c[2] - g_cal.t_cal_c[1];
        g_cal.a1 = (dT1 != 0.0f) ?
                   (g_cal.mu_cal[1] - g_cal.mu_cal[0]) / dT1 : 0.0f;
        g_cal.a2 = (dT2 != 0.0f) ?
                   (g_cal.mu_cal[2] - g_cal.mu_cal[1]) / dT2 : 0.0f;
        g_cal.a3 = 0.0f;
        step = 0; /* ready for next calibration */
    }
}

/*
 * mag_set_baseline() — record the current mu_eff as the reference (for
 * vibration-reference self-calibration over the first week).
 */
void mag_set_baseline(float mu_eff)
{
    s_mu_eff_baseline = mu_eff;
    g_cal.mu_eff0 = mu_eff;
}

/* ----------------------------- Internals ---------------------------------- */

static void nco_init(float f_demod_hz, float fs_hz)
{
    float phase_inc = 2.0f * 3.14159265f * f_demod_hz / fs_hz;
    for (int i = 0; i < MAG_NSAMP_PER_STEP; i++) {
        s_sin_tab[i] = sinf(phase_inc * (float)i);
        s_cos_tab[i] = cosf(phase_inc * (float)i);
    }
    s_nco_ready = true;
}

static void demod_iq(const int16_t *v, float fs, float *I, float *Q)
{
    (void)fs;
    float accI = 0.0f, accQ = 0.0f;
    /* Apply a Hann window to reduce spectral leakage from edge discontinuities.
     * The window is implicit in the cosine taper applied below. */
    for (int i = 0; i < MAG_NSAMP_PER_STEP; i++) {
        float s = (float)v[i];
        /* taper first and last 5 % of samples (Hann-like) */
        float w = 1.0f;
        int taper = MAG_NSAMP_PER_STEP / 20;
        if (i < taper) w = 0.5f * (1.0f - cosf(3.14159265f * (float)i / (float)taper));
        if (i > MAG_NSAMP_PER_STEP - taper)
            w = 0.5f * (1.0f - cosf(3.14159265f * (float)(MAG_NSAMP_PER_STEP - i) / (float)taper));
        accI += s * s_cos_tab[i] * w;
        accQ += s * s_sin_tab[i] * w;
    }
    /* scale to amplitude */
    float scale = 2.0f / (float)MAG_NSAMP_PER_STEP;
    *I = accI * scale;
    *Q = accQ * scale;
}

static float interp_tension(float mu_eff, const cal_page_t *cal)
{
    /* Invert the (T -> mu) curve to get T from mu. The calibration stores
     * three (T, mu) points sorted by T ascending. We invert piecewise. */
    if (cal->t_cal_c[0] == 0.0f && cal->t_cal_c[1] == 0.0f &&
        cal->t_cal_c[2] == 0.0f) {
        /* no calibration — return raw mu scaled to kN as a fallback */
        return mu_eff * 100.0f;
    }
    /* mu is expected to decrease as T increases (typical bridge steel) */
    if (mu_eff >= cal->mu_cal[0]) {
        /* below lowest cal point — extrapolate from first segment */
        if (cal->a1 != 0.0f)
            return cal->t_cal_c[0] + (mu_eff - cal->mu_cal[0]) / cal->a1;
        return cal->t_cal_c[0];
    }
    if (mu_eff <= cal->mu_cal[2]) {
        if (cal->a2 != 0.0f)
            return cal->t_cal_c[2] + (mu_eff - cal->mu_cal[2]) / cal->a2;
        return cal->t_cal_c[2];
    }
    /* middle segment */
    if (mu_eff >= cal->mu_cal[1]) {
        if (cal->a1 != 0.0f)
            return cal->t_cal_c[0] + (mu_eff - cal->mu_cal[0]) / cal->a1;
        return cal->t_cal_c[1];
    } else {
        if (cal->a2 != 0.0f)
            return cal->t_cal_c[1] + (mu_eff - cal->mu_cal[1]) / cal->a2;
        return cal->t_cal_c[1];
    }
}

static void find_resonance(void)
{
    float max_z = 0.0f;
    int   max_i = 0;
    for (int i = 0; i < MAG_NFREQ; i++) {
        if (s_zmag[i] > max_z) {
            max_z = s_zmag[i];
            max_i = i;
        }
    }
    /* require a clear peak (10 % above neighbours) to lock resonance */
    if (max_i > 0 && max_i < MAG_NFREQ - 1) {
        float neighbours = 0.5f * (s_zmag[max_i - 1] + s_zmag[max_i + 1]);
        if (s_zmag[max_i] > 1.10f * neighbours) {
            s_resonance_idx = max_i;
            return;
        }
    }
    /* otherwise keep previous or default */
    if (s_resonance_idx < 0) s_resonance_idx = MAG_NFREQ / 2;
}

/* ---- low-level stubs the real HAL would provide; declared here for link ---- */
void delay_ms(uint32_t ms)
{
    /* placeholder: real firmware loops on a TIM counter or uses the WDT;
     * a portable build supplies a weak version. */
    volatile uint32_t i = ms * 8000;
    while (i--) __asm__ volatile ("nop");
}

void gpio_set(pin_t p, int level)
{
    (void)p;
    (void)level;
    /* real firmware: GPIO_BSRR(gpio_base(p.port)) = ... */
}

void tim1_set_pwm_freq(float f_hz)
{
    (void)f_hz;
    /* real firmware: TIM1_ARR = BOARD_PCLK2_HZ / f_hz; TIM1_CCR1/2 = ARR/2 */
}

void tim1_pwm_disable(void)
{
    TIM1_CR1 &= ~BIT(0);
}

void adc_capture_dual(uint8_t ch1, uint8_t ch2,
                      int16_t *buf1, int16_t *buf2, uint16_t n)
{
    (void)ch1; (void)ch2; (void)buf1; (void)buf2; (void)n;
    /* real firmware: configure ADC1 dual regular simultaneous mode,
     * DMA the n samples into buf1/buf2, block on DMA-complete flag. */
}