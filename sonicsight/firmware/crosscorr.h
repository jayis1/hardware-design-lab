/**
 * @file    crosscorr.h
 * @brief   SonicSight — Cross-correlation time-of-flight detector.
 *          Computes ToF between transmit pulse template and received waveform
 *          using hardware-accelerated CORDIC and CMSIS-DSP.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef CROSSCORR_H
#define CROSSCORR_H

#include <stdint.h>

/**
 * @brief Compute time-of-flight using cross-correlation peak detection.
 *
 * Cross-correlates the received waveform against the known transmit pulse
 * template. The peak of the cross-correlation (interpolated to sub-sample
 * precision) gives the arrival time in microseconds.
 *
 * @param[in]  waveform    Received ADC samples (12-bit, signed).
 * @param[in]  wf_len      Number of samples in waveform.
 * @param[in]  template    Transmit pulse template (same sample rate).
 * @param[in]  tmpl_len    Template length in samples.
 * @param[out] peak_time_us Arrival time in microseconds (sub-sample).
 * @param[out] snr_db      Estimated SNR at peak (dB).
 * @return 0 on success, negative on error.
 */
int32_t crosscorr_compute(const int16_t *waveform, uint32_t wf_len,
                           const int16_t *template, uint32_t tmpl_len,
                           float *peak_time_us, float *snr_db);

/**
 * @brief AIC (Akaike Information Criterion) first-arrival picker.
 *
 * Fallback method for noisy channels. Finds the point that minimises the AIC
 * cost function, representing the transition from noise to signal.
 *
 * @param[in]  waveform    Received ADC samples.
 * @param[in]  wf_len      Number of samples.
 * @param[in]  sample_rate Sample rate in Hz.
 * @return Arrival time in microseconds.
 */
float crosscorr_aic_pick(const int16_t *waveform, uint32_t wf_len,
                          uint32_t sample_rate);

/**
 * @brief Parabolic interpolation of cross-correlation peak for sub-sample
 *        time resolution.
 *
 * @param y0  Cross-correlation value at (peak_idx - 1).
 * @param y1  Cross-correlation value at peak_idx.
 * @param y2  Cross-correlation value at (peak_idx + 1).
 * @return Fractional offset from peak_idx (−0.5 to +0.5).
 */
float crosscorr_parabolic_interp(float y0, float y1, float y2);

#endif /* CROSSCORR_H */