/*
 * registers.h — Register definitions for ThermoGlyph peripherals
 *
 * Covers: nRF5340 system, ICM-42688-P, SX1262, TMP117, TLC59711,
 *         Si7051, LTR-390UV, MAX17048
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* =========================================================================
 *  nRF5340 — selected register map (app core, base 0x40000000)
 * ========================================================================= */
#define NRF53_PERIPH_BASE       0x40000000U
#define NRF53_SPU_BASE          0x50000000U

/* GPIO port base addresses */
#define NRF_P0_BASE             0x4008C000U
#define NRF_P1_BASE             0x4008D000U
#define GPIO_PIN_CNF(n)         (0x200U + ((uint32_t)(n) * 0x08U))
#define GPIO_PIN_IN             0x510U
#define GPIO_PIN_OUT            0x504U
#define GPIO_PIN_OUTSET         0x508U
#define GPIO_PIN_OUTCLR         0x50CU

/* SPIM (SPI master) */
#define NRF_SPIM0_BASE          0x4002F000U
#define NRF_SPIM1_BASE          0x4002E000U
#define SPIM_TASKS_START        0x008U
#define SPIM_TASKS_STOP         0x00CU
#define SPIM_EVENTS_END         0x108U
#define SPIM_INTENSET           0x304U
#define SPIM_ENABLE             0x500U
#define SPIM_PSEL_SCK           0x508U
#define SPIM_PSEL_MOSI          0x50CU
#define SPIM_PSEL_MISO          0x510U
#define SPIM_FREQUENCY          0x524U
#define SPIM_CONFIG             0x554U
#define SPIM_TXDPTR             0x544U
#define SPIM_TXDMAXCNT          0x548U
#define SPIM_RXDPTR             0x54CU
#define SPIM_RXDMAXCNT          0x550U
#define SPIM_FREQUENCY_8M       0x8000000U

/* TWIM (I2C master) */
#define NRF_TWIM0_BASE          0x4003F000U
#define NRF_TWIM1_BASE          0x40003000U
#define TWIM_TASKS_STARTTX      0x008U
#define TWIM_TASKS_STARTRX      0x00CU
#define TWIM_TASKS_STOP         0x014U
#define TWIM_EVENTS_STOPPED     0x104U
#define TWIM_EVENTS_RXDRDY      0x124U
#define TWIM_EVENTS_TXDSENT     0x11CU
#define TWIM_EVENTS_ERROR       0x144U
#define TWIM_INTENSET           0x304U
#define TWIM_ENABLE             0x500U
#define TWIM_PSEL_SCL           0x508U
#define TWIM_PSEL_SDA           0x50CU
#define TWIM_FREQUENCY          0x524U
#define TWIM_ADDRESS            0x544U

/* RTC2 (watchdog for thermal task) */
#define NRF_RTC2_BASE           0x40009000U
#define RTC_TASKS_START         0x000U
#define RTC_TASKS_STOP          0x004U
#define RTC_TASKS_CLEAR         0x008U
#define RTC_EVENTS_TICK         0x100U
#define RTC_EVENTS_COMPARE(n)   (0x140U + ((uint32_t)(n) * 0x04U))
#define RTC_INTENSET            0x304U
#define RTC_PRESCALER           0x508U
#define RTC_CC(n)               (0x540U + ((uint32_t)(n) * 0x04U))

/* SAADC (for solar voltage divider) */
#define NRF_SAADC_BASE          0x4000A000U
#define SAADC_TASKS_START       0x000U
#define SAADC_TASKS_SAMPLE      0x004U
#define SAADC_EVENTS_END        0x100U
#define SAADC_INTENSET          0x304U
#define SAADC_ENABLE            0x500U

/* =========================================================================
 *  ICM-42688-P IMU registers (SPI, max 1 MHz)
 * ========================================================================= */
#define ICM_REG_BANK_SEL        0x76U
#define ICM_REG_WHO_AM_I        0x00U
#define ICM_VAL_WHO_AM_I        0x47U

#define ICM_REG_DEVICE_CONFIG   0x11U
#define ICM_REG_INT_CONFIG      0x14U
#define ICM_REG_INT_CONFIG0     0x63U
#define ICM_REG_INT_SOURCE0     0x65U
#define ICM_REG_INT_STATUS_DRDY 0x74U

#define ICM_REG_PWR_MGMT0       0x4EU
#define ICM_PWR_TEMP_DIS_OFF    0x00U
#define ICM_PWR_LOW_NOISE       0x10U
#define ICM_PWR_GYRO_LN         0x15U
#define ICM_PWR_ACCEL_LN        0x0AU

#define ICM_REG_GYRO_CONFIG0    0x4FU
#define ICM_REG_ACCEL_CONFIG0   0x50U
#define ICM_GYRO_FS_2000DPS     0x06U
#define ICM_ACCEL_FS_4G         0x04U
#define ICM_ODR_200HZ           0x06U

#define ICM_REG_TEMP_DATA1      0x1DU
#define ICM_REG_TEMP_DATA0      0x1EU
#define ICM_REG_ACCEL_DATA_X1   0x1FU
#define ICM_REG_ACCEL_DATA_X0   0x20U
#define ICM_REG_ACCEL_DATA_Y1   0x21U
#define ICM_REG_ACCEL_DATA_Y0   0x22U
#define ICM_REG_ACCEL_DATA_Z1   0x23U
#define ICM_REG_ACCEL_DATA_Z0   0x24U
#define ICM_REG_GYRO_DATA_X1    0x25U
#define ICM_REG_GYRO_DATA_X0    0x26U
#define ICM_REG_GYRO_DATA_Y1    0x27U
#define ICM_REG_GYRO_DATA_Y0    0x28U
#define ICM_REG_GYRO_DATA_Z1    0x29U
#define ICM_REG_GYRO_DATA_Z0    0x2AU

#define ICM_REG_FIFO_CONFIG     0x35U
#define ICM_REG_FIFO_COUNT      0x46U
#define ICM_REG_FIFO_DATA       0x48U

/* LSB sensitivity: 4g range → 2048 LSB/g; 2000 dps → 16.4 LSB/dps */
#define ICM_ACCEL_SENS_4G       2048.0f
#define ICM_GYRO_SENS_2000DPS   16.4f

/* =========================================================================
 *  SX1262 LoRa radio registers (SPI, max 8 MHz)
 * ========================================================================= */
#define SX1262_REG_WHITENING_MSB    0x02F9U
#define SX1262_REG_WHITENING_LSB    0x02FAU
#define SX1262_REG_CRC_MSB          0x02FBU
#define SX1262_REG_CRC_LSB          0x02FCU
#define SX1262_REG_SYNC_WORD_MSB    0x0740U
#define SX1262_REG_SYNC_WORD_LSB    0x0741U

/* SX1262 commands (sent via SPI with NSS low) */
#define SX1262_CMD_SET_STANDBY      0x80U
#define SX1262_CMD_SET_RX           0x82U
#define SX1262_CMD_SET_TX           0x83U
#define SX1262_CMD_SET_SLEEP        0x84U
#define SX1262_CMD_WRITE_BUFFER     0x0DU
#define SX1262_CMD_READ_BUFFER      0x0DU
#define SX1262_CMD_SET_DIO_IRQ      0x08U
#define SX1262_CMD_CLEAR_IRQ        0x02U
#define SX1262_CMD_GET_IRQ_STATUS   0x12U
#define SX1262_CMD_SET_RF_FREQ      0x86U
#define SX1262_CMD_SET_MOD_PARAMS   0x8BU
#define SX1262_CMD_SET_PACKET_PARAMS 0x8CU
#define SX1262_CMD_SET_PA_CONFIG    0x95U
#define SX1262_CMD_SET_TX_PARAMS    0x8EU
#define SX1262_CMD_SET_BUFFER_BASE  0x8FU
#define SX1262_CMD_SET_CAD          0xC0U
#define SX1262_CMD_RESET_STATS      0x01U

/* IRQ masks */
#define SX1262_IRQ_TX_DONE          0x0001U
#define SX1262_IRQ_RX_DONE          0x0002U
#define SX1262_IRQ_PREAMBLE_DET     0x0004U
#define SX1262_IRQ_SYNCWORD_DET     0x0008U
#define SX1262_IRQ_HEADER_VALID     0x0010U
#define SX1262_IRQ_HEADER_ERR       0x0020U
#define SX1262_IRQ_CRC_ERR          0x0040U
#define SX1262_IRQ_CAD_DONE         0x0080U
#define SX1262_IRQ_CAD_DET          0x0100U
#define SX1262_IRQ_TIMEOUT          0x0200U
#define SX1262_IRQ_ALL              0x03FFU

/* Standby configurations */
#define SX1262_STDBY_RC             0x00U
#define SX1262_STDBY_XOSC           0x01U

/* Packet types */
#define SX1262_PKT_LORA             0x01U

/* =========================================================================
 *  TMP117 digital temperature sensor registers (I2C)
 * ========================================================================= */
#define TMP117_REG_TEMP_RESULT      0x00U
#define TMP117_REG_CONFIG           0x01U
#define TMP117_REG_THIGH_LIMIT      0x02U
#define TMP117_REG_TLOW_LIMIT       0x03U
#define TMP117_REG_EEPROM1          0x04U
#define TMP117_REG_EEPROM2          0x05U
#define TMP117_REG_EEPROM3          0x06U
#define TMP117_REG_TEMP_OFFSET      0x07U
#define TMP117_REG_EEPROM4          0x08U
#define TMP117_REG_DEVICE_ID        0x0FU

#define TMP117_DEVICE_ID_VAL        0x0117U
#define TMP117_CONFIG_CC(n)         ((uint16_t)(n) << 10)
#define TMP117_CONFIG_AVG(n)        ((uint16_t)(n) << 5)
#define TMP117_CONFIG_CONTINUOUS    0x0000U
#define TMP117_CONFIG_SHUTDOWN      0x2000U
#define TMP117_CONFIG_64_AVG        0x00E0U

/* Temperature conversion: raw 16-bit is °C × 78.125 mGa/LSB →
 * TEMP_C = raw * 0.0078125 (signed)
 * For fixed-point (°C × 100): raw * 100 / 128 */
#define TMP117_LSB_TO_C100(raw)     ((int32_t)((int16_t)(raw) * 100) / 128)

/* =========================================================================
 *  TLC59711 12-channel PWM LED driver (bit-banged SPI)
 * ========================================================================= */
#define TLC59711_CHIP_COUNT         4U  /* 4 chips × 12 channels = 48 */
#define TLC59711_CHANNELS_PER_CHIP  12U
#define TLC59711_TOTAL_CHANNELS     (TLC59711_CHIP_COUNT * TLC59711_CHANNELS_PER_CHIP)

/* Write command: 6-bit function + 6-bit control + 12×16-bit PWM */
#define TLC59711_CMD_WRT            0x25U  /* write command + blank, etc. */
#define TLC59711_OUTTMG             0x0100U
#define TLC59711_EXTGCK             0x0000U
#define TLC59711_TMGRST             0x0040U
#define TLC59711_DSPRPT             0x0020U
#define TLC59711_BLANK_CONTROL       0x0000U

/* =========================================================================
 *  Si7051 ambient temperature sensor (I2C 0x40)
 * ========================================================================= */
#define SI7051_CMD_MEASURE_HOLD     0xF3U
#define SI7051_CMD_MEASURE_NOHOLD   0xF3U
#define SI7051_CMD_READ_ID1         0xFA0FU
#define SI7051_CMD_READ_ID2         0xFC0FU
#define SI7051_CMD_FIRMWARE_REV     0x84B8U

/* Si7051 conversion: TEMP_C = 175.72 + (raw * 175.72) / 65536 − 46.85
 * Fixed-point °C × 100: */
#define SI7051_RAW_TO_C100(raw) \
    (int32_t)(((17572 * (int32_t)(raw)) / 65536) - 4685)

/* =========================================================================
 *  LTR-390UV ambient light / UV sensor (I2C 0x53)
 * ========================================================================= */
#define LTR390_REG_MAIN_CTRL        0x00U
#define LTR390_REG_MEAS_RATE        0x04U
#define LTR390_REG_GAIN             0x05U
#define LTR390_REG_PART_ID          0x06U
#define LTR390_REG_STATUS           0x07U
#define LTR390_REG_ALS_DATA_0       0x0DU
#define LTR390_REG_ALS_DATA_1       0x0EU
#define LTR390_REG_ALS_DATA_2       0x0FU
#define LTR390_REG_INT_CFG          0x19U
#define LTR390_REG_INT_PST          0x1AU
#define LTR390_REG_THRESH_UP        0x21U

#define LTR390_PART_ID_VAL          0xB2U
#define LTR390_CTRL_ALS_MODE        0x02U
#define LTR390_CTRL_UVS_MODE        0x0AU
#define LTR390_CTRL_ALS_ENABLE      0x02U

/* =========================================================================
 *  MAX17048 fuel gauge registers (I2C 0x36)
 * ========================================================================= */
#define MAX17048_REG_VCELL          0x02U
#define MAX17048_REG_SOC            0x04U
#define MAX17048_REG_MODE           0x06U
#define MAX17048_REG_VERSION        0x08U
#define MAX17048_REG_HIBRT          0x0AU
#define MAX17048_REG_CONFIG         0x0CU
#define MAX17048_REG_VALERT         0x14U
#define MAX17048_REG_CRATE          0x16U
#define MAX17048_REG_VRESET         0x18U
#define MAX17048_REG_STATUS         0x1AU
#define MAX17048_REG_CMD            0xFEU

#define MAX17048_CMD QUICKSTART      0x4000U
#define MAX17048_VERSION_VAL         0x0003U

/* VCELL: raw × 78.125 µV per LSB → V_mV = raw × 0.078125
 * SOC: raw / 256.0 (percent) */

/* =========================================================================
 *  BQ25570 solar harvester (no I2C; uses GPIO + ADC)
 * ========================================================================= */
#define BQ25570_VBAT_OK_DEFAULT_MV  3200U
#define BQ25570_VBAT_OK_HYST_MV     4900U
#define BQ25570_MPPT_RATIO           0x80U   /* 80% of open-circuit voltage */

/* =========================================================================
 *  TLV3691 comparator (no registers; hardware-only safety)
 * ========================================================================= */
/* The THERM_FAULT GPIO is pulled high externally.
 * When any of the 8 TLV3691 comparators trips (row temp > 44°C),
 * the shared fault line goes low, triggering a GPIO interrupt.
 * The interrupt handler immediately deasserts PIN_TEC_PWR_EN. */

/* =========================================================================
 *  Glyph protocol (BLE/LoRa binary packet format)
 * ========================================================================= */
#define GLYPH_CMD_PIXEL      0x01U
#define GLYPH_CMD_ARROW      0x02U
#define GLYPH_CMD_TEXT       0x03U
#define GLYPH_CMD_SHAPE      0x04U
#define GLYPH_CMD_ANIM       0x05U
#define GLYPH_CMD_BAR        0x06U
#define GLYPH_CMD_CLEAR      0x07U
#define GLYPH_CMD_REPEAT     0x08U

#define GLYPH_DIR_NORTH      0x00U
#define GLYPH_DIR_NORTHEAST  0x01U
#define GLYPH_DIR_EAST       0x02U
#define GLYPH_DIR_SOUTHEAST  0x03U
#define GLYPH_DIR_SOUTH      0x04U
#define GLYPH_DIR_SOUTHWEST  0x05U
#define GLYPH_DIR_WEST       0x06U
#define GLYPH_DIR_NORTHWEST  0x07U

#define GLYPH_SHAPE_CIRCLE   0x00U
#define GLYPH_SHAPE_RING     0x01U
#define GLYPH_SHAPE_CROSS    0x02U
#define GLYPH_SHAPE_WAVE     0x03U
#define GLYPH_SHAPE_CHECK    0x04U
#define GLYPH_SHAPE_X        0x05U

#define GLYPH_WARM           0x01U
#define GLYPH_COOL           0x02U
#define GLYPH_NEUTRAL        0x00U

/* LoRa packet format:
 * [0xTG=0x54 0x47] [cmd] [len] [payload...] [crc16_hi] [crc16_lo]
 */
#define LORA_PROTO_MAGIC0    0x54U
#define LORA_PROTO_MAGIC1    0x47U
#define LORA_CMD_GLYPH       0x01U
#define LORA_CMD_POSITION    0x02U
#define LORA_CMD_SOS         0x03U
#define LORA_CMD_HEARTBEAT   0x04U
#define LORA_MAX_PAYLOAD     64U

/* AES-128 key schedule (placeholder — real key loaded from flash OTP) */
#define LORA_AES_KEY_LEN     16U

/* =========================================================================
 *  Gesture event codes
 * ========================================================================= */
#define GESTURE_NONE         0x00U
#define GESTURE_TAP          0x01U
#define GESTURE_DOUBLE_TAP   0x02U
#define GESTURE_FLIP         0x03U
#define GESTURE_SHAKE        0x04U

/* =========================================================================
 *  Telemetry packet (BLE notify)
 * ========================================================================= */
typedef struct {
    uint8_t  battery_pct;       /* 0–100 */
    uint8_t  solar_mv_lo;       /* solar voltage (mV) low byte */
    uint8_t  solar_mv_hi;
    uint8_t  ambient_temp_c;    /* °C */
    uint8_t  skin_temp_avg_c;   /* average of 12 row temps */
    uint8_t  skin_temp_max_c;   /* max row temp */
    uint8_t  current_glyph_cmd; /* current glyph being rendered */
    uint8_t  gesture_last;      /* last detected gesture */
    uint8_t  lora_rssi;         /* signed, offset 128 */
    uint8_t  state;             /* 0=sleep, 1=idle, 2=active, 3=fault */
} telemetry_packet_t;

#endif /* REGISTERS_H */