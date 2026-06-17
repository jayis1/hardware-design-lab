/**
 * @file    acquisition.h
 * @brief   SonicSight — Acquisition module header.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef ACQUISITION_H
#define ACQUISITION_H

#include <stdint.h>
#include "board.h"
#include "registers.h"

/* ========================================================================== */
/*  Acquisition Context                                                       */
/* ========================================================================== */

typedef struct {
    /* Current transmitter index (0..TOMO_MAX_SENSORS-1) */
    uint8_t  current_tx;
    /* Total number of active transmitters in this scan */
    uint8_t  total_tx;
    /* Accumulated ray count (N×(N−1)/2) */
    uint32_t ray_count;

    /* ADC buffers: 8 channels × 1024 samples each (ping-pong using SDRAM) */
    int16_t *adc_buffers[ADC_NUM_CHANNELS];
    /* LVDS deserialization buffer (raw DCMI data) */
    int16_t *raw_lvds_buffer;

    /* Transmit pulse template (for cross-correlation) */
    int16_t *tx_template;
    uint32_t tx_template_len;

    /* Time-of-flight matrix: sender×receiver → ToF in µs */
    float tof_matrix[TOMO_MAX_SENSORS][TOMO_MAX_SENSORS];

    /* RX mux mapping: ADC channel index → sensor number */
    uint8_t rx_mux_map[ADC_NUM_CHANNELS];

    /* Per-channel SNR values (dB) */
    float channel_snr[ADC_NUM_CHANNELS];
    /* Per-channel applied gain (dB) */
    float channel_gain[ADC_NUM_CHANNELS];

    /* DMA/DCMI state */
    volatile uint8_t  capture_done;
    volatile uint32_t capture_error;
} acquisition_ctx_t;

/* ========================================================================== */
/*  Function Prototypes                                                       */
/* ========================================================================== */

/**
 * @brief Initialise the acquisition subsystem.
 * @param ctx Pointer to acquisition context.
 */
void acquisition_init(acquisition_ctx_t *ctx);

/**
 * @brief Check transducer coupling quality for all active sensors.
 * @param ctx       Acquisition context.
 * @param active    Bitmap of active sensors (1 = active).
 * @param num_sens  Number of sensors.
 * @return Bitmask: bit N = 1 if sensor N has good coupling, or negative error.
 */
int32_t acquisition_check_coupling(acquisition_ctx_t *ctx,
                                    const uint8_t *active,
                                    uint8_t num_sens);

/**
 * @brief Set per-channel gains (dB) from calibration data.
 * @param ctx       Acquisition context.
 * @param gain_db   Array of gain values per ADC channel.
 */
void acquisition_set_gains(acquisition_ctx_t *ctx, const float *gain_db);

/**
 * @brief Set programmable filter cutoff frequency.
 * @param ctx  Acquisition context.
 * @param fc_hz  Target cutoff frequency (Hz).
 */
void acquisition_set_filter_fc(acquisition_ctx_t *ctx, uint32_t fc_hz);

/**
 * @brief Prepare ADC for capture (configure sync, start clock).
 * @param ctx Acquisition context.
 */
void acquisition_prepare_adc(acquisition_ctx_t *ctx);

/**
 * @brief Select which transducer channel receives the HV pulse.
 * @param tx_index Transmitter index (0–31).
 */
void acquisition_select_tx_channel(uint8_t tx_index);

/**
 * @brief Configure receive multiplexers for a given transmitter.
 * @param tx_index   Current transmitter index.
 * @param active     Bitmap of active sensors.
 * @param num_sens   Number of sensors.
 */
void acquisition_configure_rx_muxes(uint8_t tx_index,
                                     const uint8_t *active,
                                     uint8_t num_sens);

/**
 * @brief Fire a burst on the selected transmitter channel.
 * @param tx_index   Transmitter index.
 * @param n_cycles   Number of cycles in the burst.
 * @param freq_hz    Burst frequency (Hz).
 */
void acquisition_fire_burst(uint8_t tx_index, uint8_t n_cycles, uint32_t freq_hz);

/**
 * @brief Start ADC capture via DCMI with DMA.
 * @param ctx Acquisition context.
 */
void acquisition_start_capture(acquisition_ctx_t *ctx);

/**
 * @brief Store extracted time-of-flight values for the current transmitter.
 * @param ctx           Acquisition context.
 * @param tx_index      Transmitter index.
 * @param arrival_times Array of arrival times per ADC channel.
 * @param num_channels  Number of ADC channels.
 */
void acquisition_store_tofs(acquisition_ctx_t *ctx, uint8_t tx_index,
                             const float *arrival_times, uint8_t num_channels);

#endif /* ACQUISITION_H */