/*
 * tensilguard.h — TensilGuard common types and configuration
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Shared definitions used across all TensilGuard firmware modules.
 * Keeps the driver files focused on their domain while letting the
 * scheduler and telemetry builder speak a common language.
 */
#ifndef TENSILGUARD_H
#define TENSILGUARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ----------------------------- Identity ---------------------------------- */
#define TG_FW_VERSION       "1.0.0"
#define TG_AUTHOR            "jayis1"
#define TG_BOARD_NAME        "tensilguard-v1"

/* ----------------------------- Geometry / cable -------------------------- */
#define CABLE_DIAM_MM        45.0f     /* nominal; overridable from cal page */
#define CABLE_LEN_M          55.0f     /* free length between anchorages */
#define CABLE_LIN_MASS       11.8f     /* kg/m for a typical Ø45 stay */
#define CABLE_SAG_M          0.10f     /* Irvine sag correction input */
#define CABLE_NWIRES         19        /* used by AE section-loss estimator  */

/* ----------------------------- Magnetoelastic sweep ---------------------- */
#define MAG_NFREQ            24
#define MAG_FMIN_HZ           500.0f
#define MAG_FMAX_HZ           10000.0f
#define MAG_DRIVE_MS          20       /* per frequency step            */
#define MAG_SETTLE_MS         2
#define MAG_ADC_RATE_HZ       500000UL
#define MAG_NSAMP_PER_STEP    256      /* I/Q demod window              */
#define MAG_COIL_NTURNS       32
#define MAG_RSHUNT_OHM        10.0f
#define MAG_PGA_GAIN          8.0f

/* ----------------------------- Accelerometer ----------------------------- */
#define ACC_RATE_HZ           100UL
#define ACC_WINDOW_S          32
#define ACC_NSAMP             (ACC_RATE_HZ * ACC_WINDOW_S)  /* 3200 */
#define ACC_FFT_SIZE          4096     /* zero-padded real FFT            */
#define ACC_FMIN_HZ           0.2f
#define ACC_FMAX_HZ           20.0f

/* ----------------------------- Acoustic emission ------------------------- */
#define AE_SAMPLE_RATE_HZ     200000UL
#define AE_PRETRIG_MS         8
#define AE_POSTTRIG_MS        24
#define AE_NSAMP              (((AE_PRETRIG_MS + AE_POSTTRIG_MS) * AE_SAMPLE_RATE_HZ) / 1000UL)
#define AE_COMP_THRESH_UV     6000.0f  /* 6 mV re 1 µV = 75 dB threshold  */
#define AE_RINGBUF_NSAMP      2048     /* pre-trigger ring (8 ms @200k)    */
#define AE_MIN_DUR_US         40
#define AE_MAX_DUR_US         4000
#define AE_MIN_RISE_US        20
#define AE_MAX_RISE_US        600
#define AE_LOBAND_HZ          20000.0f
#define AE_HIBAND_HZ          80000.0f

/* ----------------------------- Telemetry --------------------------------- */
#define TG_DEFAULT_INTERVAL_S  1800     /* 30 min */
#define TG_MIN_INTERVAL_S      300
#define TG_MAX_INTERVAL_S      86400
#define TG_URGENT_WINDOW_S     120
#define TG_UPLINK_MAX_BYTES    64

/* ----------------------------- Power ------------------------------------- */
#define TG_BATTERY_CRIT_PCT   15
#define TG_BATTERY_LOW_PCT     30
#define TG_BOOST_SETTLE_MS     50
#define TG_PANEL_VMPPT_MV      17000    /* 6-cell panel MPP              */

/* ----------------------------- Flash ------------------------------------- */
#define TG_FLASH_PAGES        4096
#define TG_FLASH_PAGE_BYTES   4096
#define TG_CAL_PAGE_IDX       0        /* calibration coefficients       */
#define TG_LOG_PAGE_IDX_START 1        /* ring buffer of measurements    */
#define TG_AE_PAGE_IDX_START  1024     /* AE events + waveforms          */

/* ----------------------------- Types ------------------------------------- */

typedef struct {
    float f0;            /* Hz, free-air (cal) */
    float mu_eff0;       /* permeability ref at cal */
    float a1, a2, a3;    /* T(mu_eff) piecewise-linear coefficients */
    float tempcoef_ppm;  /* d(mu_eff)/dT compensation, ppm/°C */
    float t_cal_c[3];    /* calibration tensions (kN) */
    float mu_cal[3];     /* measured mu_eff at each cal point */
    float cable_len_m;
    float cable_lin_mass;
    float cable_diam_mm;
    float sag_m;
    uint8_t n_wires;
    uint16_t crc16;
} cal_page_t;

typedef struct {
    float mu_eff;       /* effective permeability (dimensionless)  */
    float z_mag_ohm;    /* |Z| at resonance step (ohm)             */
    float z_phase_deg;  /* phase angle (deg)                        */
    float t_mag_kn;     /* tension from magnetoelastic (kN)        */
    float f1_hz;        /* cable fundamental vibration frequency    */
    float t_vib_kn;     /* tension from vibration (kN)              */
    float dt_kn;        /* |t_mag - t_vib| (kN)                     */
    float temp_c;       /* temperature (°C)                         */
    float battery_pct;  /* state of charge (%)                      */
    float battery_mv;
    float panel_mv;
    int16_t accel_rms_mg; /* ambient vibration RMS (mg)             */
    uint8_t ae_count;   /* new AE events since last uplink         */
    uint8_t flags;      /* bit0 urgent, bit1 dt_alarm, bit2 ae_alarm */
} measurement_t;

typedef struct {
    uint32_t unix_time;
    uint32_t seq;
    float    peak_uv;
    float    rise_us;
    float    dur_us;
    float    centroid_khz;
    float    hi_lo_ratio;   /* partial power hi/lo band          */
    uint8_t  score;         /* classifier score 0..100          */
    uint8_t  classification; /* 0 noise 1 fret 2 impact 3 break  */
    uint8_t  waveform[AE_NSAMP / 8]; /* downsampled 8:1 summary      */
} ae_event_t;

typedef struct {
    uint8_t  magic;
    uint8_t  ver;
    uint16_t crc;
    uint32_t seq;
    uint32_t unix_time;
    uint16_t battery_mv;
    uint16_t panel_mv;
    int16_t  accel_rms_mg;
    uint8_t  interval_s;     /* /10 s units */
    uint8_t  reset_cause;
    uint8_t  urgent;
    uint8_t  ae_count;
    uint8_t  flags;
    uint8_t  reserved;
    /* per-modality packed values */
    uint16_t t_mag_q4;       /* kN × 16 */
    uint16_t t_vib_q4;       /* kN × 16 */
    uint16_t dt_q4;          /* kN × 16 */
    uint16_t f1_mHz;         /* Hz × 1000 */
    int16_t  temp_c10;       /* °C × 10 */
    uint16_t mu_eff_q12;     /* × 4096 */
    uint8_t  battery_pct;
    uint8_t  soc_q4;         /* state of charge × 16 */
} ul_packet_t;

/* flag bits */
#define TG_FLAG_URGENT    0x01
#define TG_FLAG_DT_ALARM  0x02
#define TG_FLAG_AE_ALARM  0x04
#define TG_FLAG_LOW_BATT  0x08
#define TG_FLAG_CAL_LOST  0x10

/* AE classification codes */
#define AE_NOISE   0
#define AE_FRET    1
#define AE_IMPACT  2
#define AE_BREAK   3

/* Global state (defined in main.c) */
extern volatile uint32_t g_seq;
extern measurement_t     g_last;
extern cal_page_t        g_cal;
extern uint8_t           g_interval_s;
extern uint8_t           g_urgent_remaining;

#endif /* TENSILGUARD_H */