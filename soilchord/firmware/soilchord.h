/*
 * soilchord.h — public API of the Soilchord firmware modules
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Aggregated declarations for the drivers and DSP used by main.c.
 */
#ifndef SOILCHORD_H
#define SOILCHORD_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* ----------------------------- Types ------------------------------------- */

typedef enum {
    SC_OK = 0,
    SC_ERR_TIMEOUT = -1,
    SC_ERR_NACK = -2,
    SC_ERR_RANGE = -3,
    SC_ERR_BUSY = -4,
    SC_ERR_DEAD_PLUCK = -5
} sc_err_t;

typedef struct {
    uint8_t  chord;            /* 0..NCHORDS-1 */
    float    f0;               /* fundamental frequency (Hz), temperature-compensated */
    float    q_freq;           /* Q from -3 dB bandwidth */
    float    q_time;           /* Q from time-domain ring-down */
    float    rms;              /* RMS of the captured window (ADC counts) */
    float    crest;            /* crest factor (peak/RMS) */
    float    h2_h1;            /* 2nd-harmonic / 1st-harmonic power ratio */
    float    h3_h1;            /* 3rd-harmonic / 1st-harmonic power ratio */
    float    temp_c;           /* chord temperature (°C) */
    uint8_t  flags;            /* bit0: dead-pluck, bit1: suspect, bit2: alert */
} chord_features_t;

typedef struct {
    uint32_t seq;              /* monotonic sequence number */
    uint32_t unix_time;        /* RTC-derived epoch seconds */
    int16_t  battery_mv;       /* battery voltage */
    int16_t  solar_mv;         /* solar panel voltage */
    uint8_t  interval_s;       /* current measurement interval (seconds) */
    uint8_t  reset_cause;      /* last reset cause (RCC CSR) */
    uint8_t  urgent;           /* 1 if in adaptive fast mode */
    chord_features_t chord[NCHORDS];
} measurement_t;

/* ----------------------------- Drivers ---------------------------------- */
void        board_init(void);                         /* clocks, GPIO, RTC, NVIC */
sc_err_t    adc_init(void);
sc_err_t    adc_sample_block(int16_t *out, size_t n); /* blocking n-sample DMA-style read */

void        actuator_init(void);
sc_err_t    actuator_pluck(uint8_t chord);             /* fire solenoid for `chord` */

void        piezo_chain_on(void);
void        piezo_chain_off(void);
sc_err_t    piezo_capture(uint8_t chord, int16_t *buf, size_t n);
void        piezo_set_pga_gain(uint8_t gain_idx);     /* 1x,2x,4x,5x,8x,16x,32x */

sc_err_t    lora_init(void);
sc_err_t    lora_send(const uint8_t *payload, uint8_t len, uint8_t retries);
sc_err_t    lora_recv(uint8_t *buf, uint8_t *len, uint32_t timeout_ms);
int8_t      lora_last_rssi_dbm(void);

sc_err_t    temp_init(void);
sc_err_t    temp_read(uint8_t chord, float *out_c);   /* one-shot convert + read */

sc_err_t    power_init(void);
sc_err_t    power_read(int16_t *batt_mv, int16_t *solar_mv);
void        power_enter_stop2(uint32_t seconds);       /* sleep until next RTC alarm */
uint8_t     power_reset_cause(void);

sc_err_t    flash_log_init(void);
sc_err_t    flash_log_append(const measurement_t *m);
sc_err_t    flash_log_read(uint32_t idx, measurement_t *out);
uint32_t    flash_log_count(void);
sc_err_t    flash_log_erase(void);

/* ----------------------------- DSP -------------------------------------- */
void        dsp_init(void);                            /* CMSIS-DSP tables */
void        dsp_extract_features(const int16_t *samples,
                                size_t n,
                                uint8_t chord,
                                chord_features_t *out);

/* ----------------------------- Protocol --------------------------------- */
/* Pack one measurement into a LoRa uplink frame. Returns the byte length
 * (≤ 51 bytes to fit EU868 duty-cycle-friendly payloads). */
uint8_t     proto_pack_uplink(const measurement_t *m, uint8_t *out, uint8_t max_len);

/* Unpack a downlink command (interval set, calibrate, backfill request). */
typedef enum {
    DL_NOP = 0,
    DL_SET_INTERVAL = 1,
    DL_CALIBRATE_BASELINE = 2,
    DL_REQUEST_BACKFILL = 3,
    DL_FORCE_MEASURE = 4
} dl_cmd_t;
typedef struct {
    dl_cmd_t cmd;
    uint32_t arg;          /* interval seconds / from-seq / etc. */
} dl_frame_t;
sc_err_t    proto_unpack_downlink(const uint8_t *buf, uint8_t len, dl_frame_t *out);

/* ----------------------------- Scheduler -------------------------------- */
extern volatile uint32_t g_seq;
extern measurement_t g_last;
extern uint8_t g_interval_s;
extern uint8_t g_urgent_remaining;

void        scheduler_run(void);                       /* main loop, never returns */

#endif /* SOILCHORD_H */