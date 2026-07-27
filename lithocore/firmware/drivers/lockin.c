/*
 * lockin.c — Digital lock-in detection for EIS impedance measurement.
 *
 * This is the heart of the impedance measurement. For each frequency
 * point in the sweep, we:
 *
 *   1. Set the DDS to frequency f and record the phase accumulator.
 *   2. Simultaneously sample V(t) and I(t) at rate ≥ 10×f.
 *   3. Multiply each sample by sin(2πf·n·Δt + φ₀) and cos(...), where
 *      φ₀ is the DDS phase at the acquisition start.
 *   4. Average (low-pass filter) the products to get the in-phase (I)
 *      and quadrature (Q) components of V and I.
 *   5. Compute Z = V_complex / I_complex using CORDIC complex division.
 *
 * The CORDIC hardware accelerator on the STM32G474 performs the sin/cos
 * generation and the complex magnitude/phase in 6 clock cycles each —
 * 20× faster than software float, and deterministic in cycle count.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "lockin.h"
#include "../board.h"
#include "../registers.h"
#include "dds.h"

/* -------------------------------------------------------------------------
 * Q1.31 fixed-point arithmetic
 *
 * Q1.31: 1 sign bit, 31 fractional bits. Range: [-1.0, +1.0)
 * Multiply: (a * b) >> 31, but use 64-bit intermediate to avoid overflow.
 * Divide: (a << 31) / b
 * ------------------------------------------------------------------------- */

q31_t q31_mul(q31_t a, q31_t b)
{
    return (q31_t)(((int64_t)a * (int64_t)b) >> 31);
}

q31_t q31_div(q31_t a, q31_t b)
{
    if (b == 0) return 0;
    return (q31_t)(((int64_t)a << 31) / (int64_t)b);
}

/* -------------------------------------------------------------------------
 * CORDIC hardware accelerator wrappers
 *
 * The STM32G474 CORDIC performs:
 *   - Cosine: input = angle in Q1.31 ([-1, +1) maps to [-π, +π))
 *   - Sine:   input = angle, output = sin(angle) in Q1.31
 *   - Phase:  input = (Y, X) in Q1.31, output = atan2(Y,X) in Q1.31
 *   - Modulus: input = (X, Y), output = sqrt(X²+Y²) in Q1.31
 *
 * Each operation takes n+3 cycles where n is the precision (default 16
 * iterations → 19 cycles at 163 MHz ≈ 116 ns). We set precision to 16
 * for a good speed/accuracy tradeoff.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */

static void cordic_set_function(uint32_t func, uint32_t precision)
{
    /* CSR: FUNC[3:0], PRECISION[7:4], NARGS (bit 1), NRES (bit 4) */
    CORDIC->CSR = (func & 0xF) | ((precision & 0xF) << 4) | CORDIC_CSR_NRES;
}

q31_t cordic_cos(q31_t angle)
{
    cordic_set_function(CORDIC_CSR_FUNC_COSINE, 16);
    CORDIC->WDATA = angle;
    while (!(CORDIC->CSR & CORDIC_CSR_RRDY)) { }
    return (q31_t)CORDIC->RDATA;
}

q31_t cordic_sin(q31_t angle)
{
    cordic_set_function(CORDIC_CSR_FUNC_SINE, 16);
    CORDIC->WDATA = angle;
    while (!(CORDIC->CSR & CORDIC_CSR_RRDY)) { }
    return (q31_t)CORDIC->RDATA;
}

q31_t cordic_atan2(q31_t y, q31_t x)
{
    cordic_set_function(CORDIC_CSR_FUNC_PHASE, 16);
    /* Phase function takes Y first, then X (NARGS=1 → 2 arguments) */
    CORDIC->CSR |= CORDIC_CSR_NARGS;
    CORDIC->WDATA = y;
    CORDIC->WDATA = x;
    while (!(CORDIC->CSR & CORDIC_CSR_RRDY)) { }
    return (q31_t)CORDIC->RDATA;
}

q31_t cordic_magnitude(q31_t x, q31_t y)
{
    cordic_set_function(CORDIC_CSR_FUNC_MODULUS, 16);
    CORDIC->CSR |= CORDIC_CSR_NARGS;
    CORDIC->WDATA = x;
    CORDIC->WDATA = y;
    while (!(CORDIC->CSR & CORDIC_CSR_RRDY)) { }
    return (q31_t)CORDIC->RDATA;
}

/* -------------------------------------------------------------------------
 * Lock-in init
 * ------------------------------------------------------------------------- */
int lockin_init(void)
{
    /* Enable CORDIC clock (already done in gpio_init, but ensure) */
    volatile uint32_t *ahb1enr = (volatile uint32_t *)(RCC_BASE + 0x48);
    *ahb1enr |= (1U << 20);

    /* Reset CORDIC */
    CORDIC->CSR = 0;

    return 0;
}

/* -------------------------------------------------------------------------
 * Core lock-in computation
 *
 * Given V and I sample arrays, the reference frequency and phase, compute
 * the complex impedance Z = V/I.
 *
 * The reference phase φ₀ is the DDS phase accumulator value at the start
 * of the acquisition. The phase increment per sample is:
 *   Δφ = 2π × f / fs
 * In Q1.31 radians: Δφ_q31 = (2^31 × 2 × f) / fs  (since Q1.31 maps [-1,+1) to [-π,+π),
 *   2π corresponds to +1.0 in Q1.31 → so 1 Hz at 1 sample/sec = full 2π per sample = 1.0 Q31)
 *   Δφ_q31 = (f × 2^31) / fs  ... but this maps [0, fs/2] to [0, π] which is wrong.
 *   Actually Q1.31 angle: value 1.0 = π, value -1.0 = -π. Full 2π wraps.
 *   Phase per sample in radians = 2π × f / fs
 *   In Q1.31: angle_q31 = radians / π × 2^31 = (2π × f / fs) / π × 2^31 = (2f/fs) × 2^31
 *   So Δφ_q31 = (int64_t)(2 * f) * 2^31 / fs
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int lockin_compute(const ads1256_capture_t *v_cap,
                   const ads1256_capture_t *i_cap,
                   uint32_t ref_freq_hz,
                   uint32_t ref_phase_accum,
                   lockin_result_t *result)
{
    uint16_t n = v_cap->count;
    uint32_t fs = v_cap->sample_rate_hz;

    if (n == 0 || fs == 0 || n != i_cap->count) {
        result->valid = 0;
        return -1;
    }

    /* Phase increment per sample in Q1.31 radians */
    q31_t dphi = (q31_t)(((int64_t)(2 * (int64_t)ref_freq_hz) << 31) / (int64_t)fs);

    /* Initial phase: derived from the DDS phase accumulator.
     * The DDS phase accumulator is 28 bits; we scale it to Q1.31:
     *   phi0_q31 = (ref_phase_accum << 3) — shift left by 3 (28→31 bits)
     * This maps the DDS 28-bit phase [0, 2^28) to Q1.31 [0, 2^31) which
     * wraps to [0, 2π) → [0, +1.0) in Q1.31. Correct. */
    q31_t phi = (q31_t)(ref_phase_accum << 3);

    /* Accumulators for V_I, V_Q, I_I, I_Q (use 64-bit to prevent overflow) */
    int64_t v_i_acc = 0, v_q_acc = 0;
    int64_t i_i_acc = 0, i_q_acc = 0;

    /* Process samples */
    for (uint16_t k = 0; k < n; k++) {
        /* Generate reference cos/sin at current phase */
        q31_t cos_val = cordic_cos(phi);
        q31_t sin_val = cordic_sin(phi);

        /* Normalize raw ADC values to Q1.31.
         * ADS1256: 24-bit signed, full scale = ±2^23 → divide by 2^23 to
         * get ±1.0, then shift left by 8 to fill Q1.31 (24→31 bits). */
        q31_t v_norm = (q31_t)(v_cap->v_raw[k] << 8);
        q31_t i_norm = (q31_t)(i_cap->i_raw[k] << 8);

        /* Mix: V × cos, V × sin, I × cos, I × sin */
        v_i_acc += (int64_t)q31_mul(v_norm, cos_val);
        v_q_acc += (int64_t)q31_mul(v_norm, sin_val);
        i_i_acc += (int64_t)q31_mul(i_norm, cos_val);
        i_q_acc += (int64_t)q31_mul(i_norm, sin_val);

        /* Advance phase (mod 2^31 = mod 2π in Q1.31) */
        phi += dphi;
        /* Q1.31 wraps naturally at 2^31 (int32 overflow) */
    }

    /* Average (boxcar low-pass filter) */
    q31_t v_i = (q31_t)(v_i_acc / n);
    q31_t v_q = (q31_t)(v_q_acc / n);
    q31_t i_i = (q31_t)(i_i_acc / n);
    q31_t i_q = (q31_t)(i_q_acc / n);

    /* Store the complex V and I components */
    result->re_v = v_i;
    result->im_v = v_q;
    result->re_i = i_i;
    result->im_i = i_q;

    /* Complex divide: Z = V / I
     *   Z = (V_I + jV_Q) / (I_I + jI_Q)
     *   Z = (V_I + jV_Q) × (I_I - jI_Q) / |I|²
     *   Re(Z) = (V_I×I_I + V_Q×I_Q) / |I|²
     *   Im(Z) = (V_Q×I_I - V_I×I_Q) / |I|² */
    q31_t i_mag_sq = (q31_t)(((int64_t)i_i * (int64_t)i_i +
                              (int64_t)i_q * (int64_t)i_q) >> 31);

    if (i_mag_sq == 0) {
        result->valid = 0;
        return -1;
    }

    /* Re(Z) = (V_I×I_I + V_Q×I_Q) / |I|² */
    q31_t num_re = (q31_t)(((int64_t)q31_mul(v_i, i_i) +
                            (int64_t)q31_mul(v_q, i_q)));
    /* Im(Z) = (V_Q×I_I - V_I×I_Q) / |I|² */
    q31_t num_im = (q31_t)(((int64_t)q31_mul(v_q, i_i) -
                            (int64_t)q31_mul(v_i, i_q)));

    result->re_z = q31_div(num_re, i_mag_sq);
    result->im_z = q31_div(num_im, i_mag_sq);

    /* Magnitude and phase via CORDIC */
    result->mag_z = cordic_magnitude(result->re_z, result->im_z);
    result->phase_mdeg = cordic_atan2(result->im_z, result->re_z);

    /* Scale: the raw Z is in ADC-counts ratio. To convert to physical Ohms,
     * we need the transimpedance gain and sense resistor values.
     * The AFE has: V channel gain = 100×, I channel = TIA with R_tia = 1kΩ,
     * and the sense resistor is 0.1 Ω.
     * Z_physical = Z_raw × (R_sense / (V_gain × I_gain))
     * For now, the conversion to physical Ohms is done in the sweep manager
     * which knows the AFE calibration constants. Here we output the raw
     * complex ratio. */

    result->freq_hz = ref_freq_hz;
    result->samples = n;
    result->valid = 1;

    return 0;
}

/* -------------------------------------------------------------------------
 * Low-frequency measurement path
 *
 * Uses the ADS1256 at sample rates from 1 Hz (for 0.01 Hz excitation) to
 * 10 kHz (for 1 kHz excitation). Captures ≥ 5 cycles of the excitation
 * frequency for adequate low-pass averaging.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int lockin_measure_lf(uint32_t freq_hz, lockin_result_t *result)
{
    ads1256_capture_t v_cap, i_cap;
    uint16_t num_samples;
    uint32_t sample_rate;

    /* Determine sample rate and sample count:
     * - Sample at ≥ 10× the excitation frequency
     * - Capture ≥ 5 cycles
     * - At very low freq (0.01 Hz), 5 cycles = 500 seconds — too long.
     *   For ultra-low frequencies, we capture 2 cycles minimum. */
    uint32_t cycles;

    if (freq_hz < 1) {
        cycles = 2;
        sample_rate = freq_hz * 10;
        if (sample_rate < 1) sample_rate = 1;
    } else if (freq_hz < 10) {
        cycles = 5;
        sample_rate = freq_hz * 10;
    } else {
        cycles = 10;
        sample_rate = freq_hz * 10;
    }

    /* Number of samples = cycles × (sample_rate / freq_hz) */
    num_samples = (uint16_t)(cycles * sample_rate / freq_hz);
    if (num_samples > ADS1256_MAX_SAMPLES)
        num_samples = ADS1256_MAX_SAMPLES;
    if (num_samples < 10)
        num_samples = 10;

    /* Set DDS to the excitation frequency */
    dds_set_frequency_hz((double)freq_hz);
    dds_enable();

    /* Wait for settling (at least 2 cycles of the excitation) */
    if (freq_hz > 0) {
        uint32_t settle_ms = (uint32_t)(2000.0 / (double)freq_hz);
        if (settle_ms > 5000) settle_ms = 5000;
        /* delay_ms(settle_ms); — handled by sweep manager */
    }

    /* Capture V and I */
    if (ads1256_capture_dual(&v_cap, &i_cap, num_samples, sample_rate) != 0) {
        dds_disable();
        result->valid = 0;
        return -1;
    }

    /* Compute impedance */
    uint32_t phase = dds_get_phase_accumulator();
    int ret = lockin_compute(&v_cap, &i_cap, freq_hz, phase, result);

    dds_disable();
    return ret;
}

/* -------------------------------------------------------------------------
 * High-frequency measurement path (1 kHz – 100 kHz)
 *
 * Uses the MCU's built-in 12-bit ADC at 500 kSPS (with 16× hardware
 * oversampling → effective 16-bit at 31.25 kSPS). For frequencies up to
 * 100 kHz, we need sample rate ≥ 500 kHz — the MCU ADC at full speed
 * (no oversampling) provides this.
 *
 * At 100 kHz, 50 cycles = 0.5 ms = 250 samples at 500 kSPS.
 * The signal amplitude is larger at high freq (lower Z), so 12-bit
 * resolution is adequate.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int lockin_measure_hf(uint32_t freq_hz, lockin_result_t *result)
{
    /* For the high-frequency path, we'd use the MCU ADC with DMA.
     * The implementation is similar to lockin_measure_lf but uses:
     *   - ADC1 at 500 kSPS, 12-bit, hardware trigger from TIM6
     *   - DMA double-buffer for V and I channels
     *   - Same lock-in computation but with 12-bit → Q1.31 scaling (shift by 19)
     *
     * This is a simplified implementation that reuses the ADS1256 path
     * for frequencies up to 1 kHz (ADS1256 max 30 kSPS = 3 kHz at 10×
     * oversampling). For 3–100 kHz, the MCU ADC path is required.
     */

    /* For frequencies the ADS1256 can handle (≤ 3 kHz), use it */
    if (freq_hz <= 3000) {
        return lockin_measure_lf(freq_hz, result);
    }

    /* For > 3 kHz, use MCU ADC — this requires DMA setup.
     * The capture is done via the MCU ADC reading AN_VAC_HI (PA2) and
     * AN_ISENSE (PA1) in sequence at 500 kSPS. */
    ads1256_capture_t v_cap, i_cap;
    uint32_t sample_rate = 500000;
    uint32_t cycles = 50;
    uint16_t num_samples = (uint16_t)(cycles * sample_rate / freq_hz);
    if (num_samples > ADS1256_MAX_SAMPLES)
        num_samples = ADS1256_MAX_SAMPLES;
    if (num_samples < 50)
        num_samples = 50;

    /* Set DDS */
    dds_set_frequency_hz((double)freq_hz);
    dds_enable();

    /* MCU ADC capture — simplified: fill with simulated ADC reads.
     * In production, this triggers a DMA transfer from ADC1 to the
     * capture buffers. Each sample is 12-bit (0-4095), centered at 2048
     * for AC signals. */
    for (uint16_t k = 0; k < num_samples; k++) {
        /* Read ADC1 channel 3 (PA2, V_ac) and channel 2 (PA1, I_sense)
         * In real firmware: trigger ADC, read DR register.
         * Here we read the ADC data register directly. */
        ADC1->SQR1 = (3U << 6);  /* first channel = IN3 (PA2) */
        ADC1->CR |= ADC_CR_ADSTART;
        while (!(ADC1->ISR & ADC_ISR_EOC)) { }
        v_cap.v_raw[k] = (int32_t)(ADC1->DR) - 2048;  /* 12-bit, AC-coupled */

        ADC1->SQR1 = (2U << 6);  /* first channel = IN2 (PA1) */
        ADC1->CR |= ADC_CR_ADSTART;
        while (!(ADC1->ISR & ADC_ISR_EOC)) { }
        i_cap.i_raw[k] = (int32_t)(ADC1->DR) - 2048;
    }

    v_cap.count = num_samples;
    i_cap.count = num_samples;
    v_cap.sample_rate_hz = sample_rate;
    i_cap.sample_rate_hz = sample_rate;

    /* Scale 12-bit to 24-bit equivalent for the lock-in computation
     * (shift left by 12 to match ADS1256 24-bit format) */
    for (uint16_t k = 0; k < num_samples; k++) {
        v_cap.v_raw[k] <<= 12;
        i_cap.i_raw[k] <<= 12;
    }

    uint32_t phase = dds_get_phase_accumulator();
    int ret = lockin_compute(&v_cap, &i_cap, freq_hz, phase, result);

    dds_disable();
    return ret;
}