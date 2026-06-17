/**
 * @file    registers.h
 * @brief   HemoWave — Register map, memory-mapped I/O, and device registers
 * @author  jayis1
 * @copyright  Copyright © 2026 jayis1. All rights reserved.
 * @license  CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)
 *
 * Defines all custom peripheral registers, buffer addresses, calibration
 * storage offsets, and shared memory regions for the HemoWave system.
 */

#ifndef HEMOWAVE_REGISTERS_H
#define HEMOWAVE_REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* =========================================================================
 *  Memory Map — Internal SRAM Regions
 *  ========================================================================= */
#define SRAM1_BASE          0x30000000UL
#define SRAM2_BASE          0x30030000UL
#define SRAM3_BASE          0x30060000UL
#define BACKUP_SRAM_BASE    0x40016000UL
#define SRAM1_SIZE          0x00030000UL   /* 192 KB */
#define SRAM2_SIZE          0x00030000UL   /* 192 KB */
#define SRAM3_SIZE          0x00010000UL   /* 64 KB */
#define BACKUP_SRAM_SIZE    0x00001000UL   /* 4 KB */

/* =========================================================================
 *  Memory-Mapped Peripheral Register Structs
 *  ========================================================================= */

/**
 * PPG Engine Registers — mapped to SRAM1 at a fixed offset.
 * These control the AFE4900 data acquisition pipeline.
 */
typedef struct __attribute__((packed)) {
    /* 0x00: LED current configuration (8 channels × 1 byte) */
    uint8_t  led_current[8];

    /* 0x08: Sample rate configuration */
    uint32_t sample_rate_hz;       /* Per-wavelength target (default 100) */
    uint32_t oversample_ratio;     /* Raw/muxed rate (default 100: 10 kHz) */

    /* 0x10: Active LED mask (bit 0-7 for CH1-CH8) */
    uint8_t  active_led_mask;

    /* 0x11: LED settling time in microseconds */
    uint8_t  led_settle_us;

    /* 0x12: LED integration time in microseconds */
    uint8_t  led_integration_us;

    /* 0x13: Padding */
    uint8_t  _pad0[1];

    /* 0x14: DC offset cancellation (per channel, uV) */
    uint32_t dc_offset[8];

    /* 0x34: Gain settings (per photodiode, 0-7) */
    uint8_t  pd_gain[4];

    /* 0x38: Filter cutoff for decimation filter (Hz) */
    uint32_t filter_cutoff_hz;

    /* 0x3C: Status flags */
    volatile uint32_t flags;
    /*   bit [0]: data_ready */
    /*   bit [1]: fifo_overflow */
    /*   bit [2]: ambient_saturation */
    /*   bit [3]: led_fault */
    /*   bit [4]: dc_calibration_done */

    /* 0x40: PPG sample count (written by ISR, 32-bit) */
    volatile uint32_t sample_counter;

    /* 0x44: FIFO level */
    volatile uint32_t fifo_level;

    /* 0x48-0x5C: Reserved */
    uint32_t _reserved[5];
} ppg_regs_t;

static_assert(sizeof(ppg_regs_t) == 0x60, "PPG register struct size mismatch");

/**
 * BIS Engine Registers — mapped to SRAM1.
 */
typedef struct __attribute__((packed)) {
    /* 0x00: Frequency sweep configuration */
    uint32_t freq_start_hz;         /* 1000 */
    uint32_t freq_end_hz;           /* 1000000 */
    uint32_t num_steps;             /* 256 */
    uint32_t integration_periods;   /* 8 */

    /* 0x10: Excitation signal */
    uint32_t excitation_current_na; /* 50000 (50 µA RMS) */
    uint32_t settling_periods;      /* 2 */

    /* 0x18: Calibration coefficients */
    float    cal_magnitude[256];     /* Magnitude correction factors */
    float    cal_phase[256];         /* Phase correction (radians) */

    /* 0x818: Status flags */
    volatile uint32_t flags;
    /*   bit [0]: sweep_in_progress */
    /*   bit [1]: sweep_done */
    /*   bit [2]: electrode_contact_error */
    /*   bit [3]: calibration_dirty */
    /*   bit [4]: fit_converged */
    /*   bit [5]: fit_failed */

    /* 0x81C-0x82C: Latest measurement results */
    float    latest_magnitude[256];  /* |Z| at each frequency */
    float    latest_phase[256];      /* φ at each frequency */

    /* 0xE1C: Cole-Cole fit results */
    float    R0;                    /* Extracellular resistance (Ω) */
    float    Rinf;                  /* Intracellular + EC resistance (Ω) */
    float    Cm;                    /* Membrane capacitance (F) */
    float    alpha;                 /* Dispersion parameter */
    float    tau;                   /* Characteristic time constant (s) */
    float    fit_residual;          /* RSS of fit (Ω²) */
    uint32_t fit_iterations;        /* Number of LM iterations */

    /* 0xE40: Derived body composition */
    float    ecw_liters;            /* Extracellular water (L) */
    float    icw_liters;            /* Intracellular water (L) */
    float    tbw_liters;            /* Total body water (L) */
    float    ffm_kg;                /* Fat-free mass (kg) */
    float    body_fat_percent;      /* Body fat percentage */
    float    hydration_percent;     /* Relative hydration vs. normal */

    /* 0xE58-0xEFF: Reserved */
    uint8_t  _reserved[424];
} bis_regs_t;

static_assert(sizeof(bis_regs_t) == 0xF00, "BIS register struct size mismatch");

/**
 * Fusion Engine — combined hemodynamic results (SRAM2 base).
 */
typedef struct __attribute__((packed)) {
    /* 0x00: Hemodynamic readings (updated at 100 Hz) */
    volatile float systolic_mmhg;        /* SBP (mmHg) */
    volatile float diastolic_mmhg;       /* DBP (mmHg) */
    volatile float map_mmhg;             /* Mean arterial pressure (mmHg) */
    volatile float heart_rate_bpm;       /* Heart rate (BPM) */
    volatile float spo2_percent;         /* Peripheral oxygen saturation (%) */
    volatile float perfusion_index;      /* Perfusion index (%) */
    volatile float stroke_volume_ml;     /* Stroke volume (mL) */
    volatile float cardiac_output_lpm;   /* Cardiac output (L/min) */
    volatile float svr_dynes;            /* Systemic vascular resistance (dyn·s·cm⁻⁵) */

    /* 0x24: Pulse waveform features */
    volatile float ptt_ms;               /* Pulse transit time (ms) */
    volatile float pat_ms;               /* Pulse arrival time (ms) */
    volatile float pep_ms;               /* Pre-ejection period (ms) */
    volatile float augmentation_index;   /* AIx (%) */
    volatile float pp_amplification;     /* Pulse pressure amplification */

    /* 0x38: Skin temperature */
    volatile float skin_temp_c;

    /* 0x3C: Motion artifact quality metrics */
    volatile float motion_energy;        /* Normalized motion energy (0-1) */
    volatile float signal_quality;       /* PPG signal quality index (0-1) */
    volatile uint8_t contact_quality;    /* 0=no contact, 1=poor, 2=fair, 3=good */

    /* 0x44: Timestamp (seconds since epoch, synced via BLE SNTP) */
    volatile uint64_t timestamp_epoch_s;

    /* 0x4C: Reserved */
    uint8_t _reserved[436];
} fusion_regs_t;

static_assert(sizeof(fusion_regs_t) == 0x200, "Fusion register struct size mismatch");

/**
 * BLE Communication Registers — for UART bridge to DA14531.
 */
typedef struct __attribute__((packed)) {
    /* 0x00: TX circular buffer (MCU → BLE) */
    uint8_t  tx_fifo[512];
    volatile uint16_t tx_head;
    volatile uint16_t tx_tail;

    /* 0x204: RX circular buffer (BLE → MCU) */
    uint8_t  rx_fifo[256];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;

    /* 0x306: Flow control */
    volatile uint8_t cts_state;          /* Current CTS line state */
    volatile uint8_t rts_state;          /* Current RTS line state */
    volatile uint8_t ble_connected;
    volatile uint8_t ble_notifications_enabled;

    /* 0x30A: GATT notification queue */
    volatile uint32_t notify_flags;      /* Bitmask of pending notifications */

    /* 0x30E-0x31F: Reserved */
    uint8_t _reserved[18];
} ble_regs_t;

static_assert(sizeof(ble_regs_t) == 0x320, "BLE register struct size mismatch");

/* =========================================================================
 *  External Memory Mappings — QSPI PSRAM (SRAM extension)
 *  ========================================================================= */
#define PSRAM_BASE          0x90000000UL
#define PSRAM_SIZE          (8 * 1024 * 1024)   /* 8 MB */

/* PSRAM region partitioning */
#define PSRAM_PPG_BUFFER_OFF    0x000000UL      /* PPG raw data ring buffer */
#define PSRAM_PPG_BUFFER_SIZE   (4 * 1024 * 1024) /* 4 MB — ~3.6 hours of 8-ch data */
#define PSRAM_IMU_BUFFER_OFF    0x400000UL      /* IMU data buffer */
#define PSRAM_IMU_BUFFER_SIZE   (1 * 1024 * 1024) /* 1 MB */
#define PSRAM_BIS_SWEEP_OFF     0x500000UL      /* BIS sweep history */
#define PSRAM_BIS_SWEEP_SIZE    (1 * 1024 * 1024) /* 1 MB */
#define PSRAM_RESULT_OFF        0x600000UL      /* Processed results log */
#define PSRAM_RESULT_SIZE       (1 * 1024 * 1024) /* 1 MB */
#define PSRAM_SCRATCH_OFF       0x700000UL      /* General-purpose scratch */
#define PSRAM_SCRATCH_SIZE      (1 * 1024 * 1024) /* 1 MB */

/* =========================================================================
 *  Calibration & Configuration Storage — QSPI Flash
 *  ========================================================================= */
#define FLASH_CAL_BASE       0x70000000UL
#define FLASH_CONFIG_OFF     0x000000UL      /* 64 KB — system configuration */
#define FLASH_CONFIG_SIZE    (64 * 1024)
#define FLASH_USER_MODEL_OFF 0x010000UL      /* 256 KB — per-user TinyML model */
#define FLASH_USER_MODEL_SIZE (256 * 1024)
#define FLASH_CAL_DATA_OFF   0x050000UL      /* 64 KB — factory calibration */
#define FLASH_CAL_DATA_SIZE  (64 * 1024)
#define FLASH_FILESYSTEM_OFF 0x060000UL      /* 7.5 MB — LittleFS partition */
#define FLASH_FILESYSTEM_SIZE (7500 * 1024)

/**
 * Persistent system configuration struct (stored at FLASH_CONFIG_OFF).
 */
typedef struct __attribute__((packed)) {
    uint8_t  magic[4];              /* "HMWC" — configuration magic */
    uint32_t version;               /* Config struct version */
    uint32_t serial_number;         /* Unique device serial number */

    /* User profile */
    float    user_height_cm;
    float    user_weight_kg;
    uint8_t  user_age;
    uint8_t  user_sex;              /* 0=unspecified, 1=male, 2=female */
    uint8_t  user_activity_level;   /* 0-3: sedentary, light, moderate, heavy */

    /* Measurement intervals */
    uint32_t ppg_interval_sec;      /* PPG measurement interval (default: 0 = continuous) */
    uint32_t bis_interval_sec;      /* BIS sweep interval (default: 900 = 15 min) */
    uint32_t ble_adv_interval_ms;   /* BLE advertising interval (default: 250) */

    /* Alert thresholds */
    float    sbp_high_threshold;    /* SBP high alert (mmHg) */
    float    sbp_low_threshold;     /* SBP low alert (mmHg) */
    float    dbp_high_threshold;    /* DBP high alert (mmHg) */
    float    hr_high_threshold;     /* HR high alert (BPM) */
    float    hr_low_threshold;      /* HR low alert (BPM) */
    float    fluid_gain_threshold;  /* ECW gain threshold (L over 24h) */

    /* Calibration state */
    uint8_t  user_calibrated;       /* 0=no, 1=yes */
    uint32_t last_calibration_time; /* Unix timestamp of last calibration */
    uint8_t  calibration_samples;   /* Number of calibration points collected */

    /* Feature flags */
    uint8_t  enable_continuous_ppg;
    uint8_t  enable_bis;
    uint8_t  enable_motion_cancel;
    uint8_t  enable_cloud_sync;
    uint8_t  enable_haptic_alerts;

    uint8_t  _pad[24];             /* Padding to 256 bytes */
    uint32_t crc32;                /* CRC-32 of all preceding bytes */
} __attribute__((packed, aligned(4))) sysconfig_t;

static_assert(sizeof(sysconfig_t) == 128, "sysconfig_t size mismatch");

/* =========================================================================
 *  LED Multiplexer — Channel-to-Wavelength Mapping
 *  ========================================================================= */
#define LED_COUNT           8
#define LED_WAVELENGTHS \
    { 520, 590, 625, 660, 740, 810, 870, 940 }

/* Channel index constants */
#define LED_CH_GREEN_520    0
#define LED_CH_AMBER_590    1
#define LED_CH_RED_625      2
#define LED_CH_RED_660      3
#define LED_CH_IR_740       4
#define LED_CH_IR_810       5   /* Isobestic — total hemoglobin */
#define LED_CH_IR_870       6
#define LED_CH_IR_940       7

/* =========================================================================
 *  GATT Service / Characteristic UUIDs (custom 128-bit, truncated to 32-bit tags)
 *  ========================================================================= */
#define HMWV_SVC_UUID           0xAA01
#define HMWV_CHAR_PPG_WAVEFORM  0xAA10
#define HMWV_CHAR_BP_READING    0xAA11
#define HMWV_CHAR_BIS_RESULT    0xAA12
#define HMWV_CHAR_BODY_COMP     0xAA13
#define HMWV_CHAR_IMU_DATA      0xAA14
#define HMWV_CHAR_COMMAND       0xAA20
#define HMWV_CHAR_SETTINGS      0xAA21
#define HMWV_CHAR_BATTERY       0xAA22

/* =========================================================================
 *  BLE Command Opcodes
 *  ========================================================================= */
#define CMD_START_MEAS          0x01
#define CMD_STOP_MEAS           0x02
#define CMD_START_BIS_SWEEP      0x03
#define CMD_CALIBRATE_BP        0x04
#define CMD_SET_INTERVAL        0x05
#define CMD_ENTER_DFU           0x06
#define CMD_SYNC_TIME           0x07
#define CMD_FACTORY_RESET       0x08
#define CMD_GET_STATUS          0x09
#define CMD_SET_THRESHOLD       0x0A

/* =========================================================================
 *  System State Machine
 *  ========================================================================= */
typedef enum {
    HMWV_STATE_INIT              = 0,
    HMWV_STATE_IDLE              = 1,
    HMWV_STATE_MEASURING         = 2,
    HMWV_STATE_BIS_SWEEP         = 3,
    HMWV_STATE_CALIBRATING       = 4,
    HMWV_STATE_DFU               = 5,
    HMWV_STATE_SLEEP             = 6,
    HMWV_STATE_ERROR             = 7
} hmwv_state_t;

/* =========================================================================
 *  Error Codes
 *  ========================================================================= */
enum {
    HMWV_ERR_OK                  = 0,
    HMWV_ERR_SPI                 = -1,
    HMWV_ERR_I2C                 = -2,
    HMWV_ERR_AFE4900             = -3,
    HMWV_ERR_AD5941              = -4,
    HMWV_ERR_BMI270              = -5,
    HMWV_ERR_TMP117              = -6,
    HMWV_ERR_PSRAM               = -7,
    HMWV_ERR_FLASH               = -8,
    HMWV_ERR_BLE                 = -9,
    HMWV_ERR_PMIC                = -10,
    HMWV_ERR_NO_CONTACT          = -11,
    HMWV_ERR_MOTION_TOO_HIGH     = -12,
    HMWV_ERR_MODEL_FAIL          = -13,
    HMWV_ERR_CALIBRATION         = -14,
};

/* =========================================================================
 *  ADC Reference & Constants
 *  ========================================================================= */
#define VREF_INT                1.21f       /* Internal VREF (V) */
#define VREF_EXT                3.00f       /* External VREF (V) */
#define BATTERY_DIVIDER_RATIO   2.0f        /* R1=R2=100kΩ, ratio = 2 */
#define BATT_FULL_VOLTAGE       4.2f        /* LiPo full charge */
#define BATT_EMPTY_VOLTAGE      3.2f        /* LiPo cutoff */
#define ADC_RESOLUTION          4096        /* 12-bit ADC */

/* =========================================================================
 *  Timing Constants
 *  ========================================================================= */
#define SYSTICK_HZ              1000        /* 1 ms tick */
#define PPG_RAW_SAMPLE_HZ       10000       /* 10 kHz per-wavelength raw */
#define PPG_OUTPUT_HZ           100         /* 100 Hz decimated output */
#define IMU_ODR_HZ              1600        /* 1.6 kHz IMU data rate */
#define BLE_TX_INTERVAL_MS      100         /* 100 ms BLE notification window */
#define LOG_INTERVAL_MS         1000        /* 1 second flash log interval */
#define BIS_SWEEP_DEFAULT_S     900         /* 15 min default BIS interval */
#define WATCHDOG_TIMEOUT_MS     8000        /* 8 second IWDG timeout */

/* =========================================================================
 *  PPG Buffer Sizes
 *  ========================================================================= */
#define PPG_RAW_FIFO_SIZE       2048        /* Entries in raw FIFO (each = 8 samples) */
#define PPG_DECIMATED_SIZE      256         /* Decimated window size (2.56 s) */
#define BP_ESTIMATE_WINDOW      256         /* Samples per BP estimation window */

/* =========================================================================
 *  Motion Cancellation (LMS) Parameters
 *  ========================================================================= */
#define LMS_TAP_COUNT           16          /* Taps per axis */
#define LMS_AXES                3           /* X, Y, Z */
#define LMS_MU                  0.01f       /* Adaptation step size */
#define LMS_CONVERGENCE_MS      200         /* Typical convergence time */

/* =========================================================================
 *  Cole-Cole Fitting Parameters
 *  ========================================================================= */
#define COLE_MAX_ITERATIONS     20          /* Max LM iterations */
#define COLE_CONVERGENCE_TOL    1e-6f       /* Convergence tolerance */
#define COLE_PARAM_COUNT        4           /* R₀, R∞, τ, α */
#define COLE_RESIDUAL_THRESHOLD 0.5f        /* Max acceptable RSS (Ω²) */

#ifdef __cplusplus
}
#endif

#endif /* HEMOWAVE_REGISTERS_H */