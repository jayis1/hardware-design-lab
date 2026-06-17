/**
 * @file    registers.h
 * @brief   SonicSight — Register-level bit definitions for all on-board
 *          peripherals (ADC, muxes, VGA, filter, BLE module, IMU, LCD).
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ========================================================================== */
/*  ADS5281 Quad 12-bit 50 MSPS ADC Register Map                             */
/*  Base address is chip-select controlled; define offset within SPI frame.   */
/*  ADS5281 uses a 16-bit SPI: [R/W(1) | A0(1) | A1(1) | A2(1) | A3(1) |    */
/*                               reserved(3) | DATA(8)]                       */
/* ========================================================================== */

/* --- Register addresses (4-bit A[3:0]) --- */
#define ADS5281_REG_CHIP_ID       (0x00U)
#define ADS5281_REG_POWER         (0x01U)
#define ADS5281_REG_CLOCK         (0x02U)
#define ADS5281_REG_TEST          (0x03U)
#define ADS5281_REG_GAIN_CH1      (0x04U)
#define ADS5281_REG_GAIN_CH2      (0x05U)
#define ADS5281_REG_GAIN_CH3      (0x06U)
#define ADS5281_REG_GAIN_CH4      (0x07U)
#define ADS5281_REG_OFFSET_CH1    (0x08U)
#define ADS5281_REG_OFFSET_CH2    (0x09U)
#define ADS5281_REG_OFFSET_CH3    (0x0AU)
#define ADS5281_REG_OFFSET_CH4    (0x0BU)
#define ADS5281_REG_DATA_FORMAT   (0x0CU)
#define ADS5281_REG_SYNC          (0x0DU)
#define ADS5281_REG_PLL           (0x0EU)
#define ADS5281_REG_STATUS        (0x0FU)

/* --- Bit definitions --- */
#define ADS5281_POWER_PD          (1U << 0)   /* Power-down all channels */
#define ADS5281_POWER_STBY        (1U << 1)   /* Standby mode */
#define ADS5281_CLOCK_LVDS_EN     (1U << 0)   /* Enable LVDS clock output */
#define ADS5281_CLOCK_DIV2        (1U << 1)   /* Divide clock by 2 */
#define ADS5281_DATA_2S_COMP      (0U << 0)   /* Two's complement output */
#define ADS5281_DATA_OFFSET_BIN   (1U << 0)   /* Offset binary output */
#define ADS5281_DATA_LVDS_TERM    (1U << 1)   /* Enable LVDS internal termination */
#define ADS5281_SYNC_RESET        (1U << 0)   /* Reset frame-sync counter */
#define ADS5281_PLL_EN            (1U << 0)   /* Enable PLL clock multiplier */
#define ADS5281_PLL_BYPASS        (0U << 0)   /* Bypass PLL */
#define ADS5281_STATUS_PLL_LOCK   (1U << 0)   /* PLL locked indicator */
#define ADS5281_STATUS_REF_ALARM  (1U << 1)   /* Reference voltage alarm */
#define ADS5281_CHIP_ID_MASK      (0xFFU)     /* Should read 0x81 */

/* --- SPI command construction --- */
#define ADS5281_SPI_WRITE         (0U)         /* Write: A3=0, R/W=0, data = bits[7:0] */
#define ADS5281_SPI_READ          (1U)         /* Read:  A3=0, R/W=1, data = 0x00 */

/**
 * Build a 16-bit SPI command word for ADS5281.
 * @param read  0 = write, 1 = read
 * @param addr  Register address (0–15)
 * @param data  8-bit data value
 * @return      16-bit SPI word
 */
static inline uint16_t ADS5281_SPI_CMD(uint8_t read, uint8_t addr, uint8_t data)
{
    return (uint16_t)((read ? 0x8000U : 0x0000U) | ((addr & 0x0FU) << 8) | data);
}

/* ========================================================================== */
/*  ADG732 32:1 Analog Multiplexer Control Register                          */
/*  Controlled via 6 GPIOs: A0–A4 (address), EN (enable active-low).         */
/*  No SPI; control is parallel GPIO.                                         */
/* ========================================================================== */

/* Address bits (mask for 5-bit channel select 0–31) */
#define ADG732_ADDR_MASK         (0x1FU)
#define ADG732_ENABLE            (0U)         /* EN pin LOW = mux enabled */
#define ADG732_DISABLE           (1U)         /* EN pin HIGH = all off */

/* ========================================================================== */
/*  AD8331 Variable-Gain Amplifier — Gain controlled via analog voltage       */
/*  (GAIN pin, 0–1.5 V maps to 0–40 dB). We use an external DAC (MCP4821)    */
/*  over SPI to set the GAIN voltage.                                         */
/* ========================================================================== */

/* MCP4821 DAC commands */
#define MCP4821_WRITE            (0x0000U)   /* Bit 15 = 0 (write to DAC) */
#define MCP4821_GAIN_1X          (0x0000U)   /* Bit 13 = 0 (Vout = 2× Vref × D/4096) */
#define MCP4821_GAIN_2X          (0x2000U)   /* Bit 13 = 1 (Vout = 4× Vref × D/4096) */
#define MCP4821_SHDN             (0x0000U)   /* Bit 12 = 0 (output normal) */
#define MCP4821_ACTIVE           (0x1000U)   /* Bit 12 = 1 (output disabled / high-Z) */

/**
 * Convert desired gain (dB) to 12-bit DAC code.
 * GAIN pin transfer function: Gain(dB) = V_GAIN / 0.0375
 * V_GAIN = 0–1.5 V for 0–40 dB.
 * DAC output: Vout = 2 × Vref × D/4096, Vref = 2.048 V (internal).
 * => D = (Gain_dB × 0.0375) × 4096 / (2 × 2.048)
 * Simplified: D = (Gain_dB × 37.5)  — check: 40 dB → 1500 (1.5 V) ✓
 */
#define AD8331_GAIN_TO_DAC(gain_db) ((uint16_t)((gain_db) * 37U))
#define AD8331_DAC_MAX_CODE        (3750U)    /* 40 dB max */

/* ========================================================================== */
/*  LTC1563-2 Programmable 4th-Order Filter — SPI Configuration              */
/*  Sets Fc via Rratio. Register map:                                        */
/*    Byte 0: Control (write 0x01 to program filter)                         */
/*    Byte 1: Fsel (divider ratio)                                           */
/* ========================================================================== */

#define LTC1563_CTRL_WRITE       (0x01U)     /* Latch frequency setting */
#define LTC1563_CTRL_SLEEP       (0x02U)     /* Power-down */
#define LTC1563_FSEL_MIN         (0x00U)     /* Fc = 200 kHz (Rratio = 0) */
#define LTC1563_FSEL_200K        (0x00U)
#define LTC1563_FSEL_100K        (0x01U)
#define LTC1563_FSEL_50K         (0x02U)
#define LTC1563_FSEL_20K         (0x03U)
#define LTC1563_FSEL_10K         (0x04U)
#define LTC1563_FSEL_5K          (0x05U)
#define LTC1563_FSEL_2K          (0x06U)
#define LTC1563_FSEL_1K          (0x07U)

/* ========================================================================== */
/*  ICM-20948 9-Axis IMU — SPI Register Map (partial)                        */
/* ========================================================================== */

#define ICM20948_WHO_AM_I        (0x00U)     /* Should read 0xEA */
#define ICM20948_USER_CTRL       (0x03U)     /* User control */
#define ICM20948_PWR_MGMT_1      (0x06U)     /* Power management */
#define ICM20948_PWR_MGMT_2      (0x07U)     /* Power management 2 */
#define ICM20948_INT_PIN_CFG     (0x0FU)     /* Interrupt pin config */
#define ICM20948_INT_ENABLE      (0x10U)     /* Interrupt enable */
#define ICM20948_ACCEL_XOUT_H    (0x2DU)     /* Accel X high byte */
#define ICM20948_ACCEL_XOUT_L    (0x2EU)
#define ICM20948_ACCEL_YOUT_H    (0x2FU)
#define ICM20948_ACCEL_YOUT_L    (0x30U)
#define ICM20948_ACCEL_ZOUT_H    (0x31U)
#define ICM20948_ACCEL_ZOUT_L    (0x32U)
#define ICM20948_GYRO_XOUT_H     (0x33U)
#define ICM20948_GYRO_XOUT_L     (0x34U)
#define ICM20948_GYRO_YOUT_H     (0x35U)
#define ICM20948_GYRO_YOUT_L     (0x36U)
#define ICM20948_GYRO_ZOUT_H     (0x37U)
#define ICM20948_GYRO_ZOUT_L     (0x38U)
#define ICM20948_TEMP_OUT_H      (0x39U)
#define ICM20948_TEMP_OUT_L      (0x3AU)
#define ICM20948_ACCEL_CONFIG    (0x14U)     /* ±2/4/8/16 g range */
#define ICM20948_GYRO_CONFIG     (0x1BU)     /* ±250/500/1000/2000 °/s */
#define ICM20948_ODR_ALIGN       (0x19U)     /* Output data rate */

#define ICM20948_ACCEL_RANGE_2G  (0x00U << 1)
#define ICM20948_ACCEL_RANGE_4G  (0x01U << 1)
#define ICM20948_ACCEL_RANGE_8G  (0x02U << 1)
#define ICM20948_ACCEL_RANGE_16G (0x03U << 1)
#define ICM20948_GYRO_RANGE_250  (0x00U << 1)
#define ICM20948_GYRO_RANGE_500  (0x01U << 1)
#define ICM20948_GYRO_RANGE_1000 (0x02U << 1)
#define ICM20948_GYRO_RANGE_2000 (0x03U << 1)

/* SPI read/write bit (bit 7 of first byte) */
#define ICM20948_SPI_READ         (0x80U)
#define ICM20948_SPI_WRITE        (0x00U)

/* ========================================================================== */
/*  TMP117 Temperature Sensor — I2C Register Map                             */
/* ========================================================================== */

#define TMP117_REG_TEMP          (0x00U)     /* Temperature result (16-bit) */
#define TMP117_REG_CONFIG        (0x01U)     /* Configuration register */
#define TMP117_REG_THIGH         (0x02U)     /* High temperature limit */
#define TMP117_REG_TLOW          (0x03U)     /* Low temperature limit */
#define TMP117_REG_EEPROM_UL     (0x04U)     /* EEPROM unlock */
#define TMP117_REG_EEPROM1       (0x05U)     /* EEPROM 1 */
#define TMP117_REG_EEPROM2       (0x06U)     /* EEPROM 2 */
#define TMP117_REG_TEMP_OFFSET   (0x07U)     /* Temperature offset */
#define TMP117_REG_DEVICE_ID     (0x0FU)     /* Device ID (should read 0x117) */

/* Config register bits */
#define TMP117_CFG_RESET         (0x8000U)   /* Software reset */
#define TMP117_CFG_MODE_CONT     (0x0000U)   /* Continuous conversion */
#define TMP117_CFG_MODE_SHUT     (0x4000U)   /* Shutdown */
#define TMP117_CFG_MODE_ONESHOT  (0xC000U)   /* One-shot */
#define TMP117_CFG_CONV_1_8      (0x0000U)   /* 1/8 Hz conversion rate */
#define TMP117_CFG_CONV_1        (0x0400U)   /* 1 Hz */
#define TMP117_CFG_CONV_4        (0x0800U)   /* 4 Hz */
#define TMP117_CFG_CONV_8        (0x0C00U)   /* 8 Hz */
#define TMP117_CFG_AVG_1         (0x0000U)   /* No averaging */
#define TMP117_CFG_AVG_8         (0x0200U)   /* 8 averages */
#define TMP117_CFG_AVG_32        (0x0400U)   /* 32 averages */
#define TMP117_CFG_AVG_64        (0x0600U)   /* 64 averages */

/* Temperature conversion: raw × 0.0078125 = °C */
#define TMP117_LSB_DEGC          (0.0078125f)

/* ========================================================================== */
/*  GC9A01 LCD Controller — SPI Command Set (partial, for 240×240)           */
/* ========================================================================== */

#define GC9A01_SWRESET          (0x01U)      /* Software reset */
#define GC9A01_SLPOUT           (0x11U)      /* Sleep out */
#define GC9A01_INVOFF           (0x20U)      /* Display inversion off */
#define GC9A01_DISPOFF          (0x28U)      /* Display off */
#define GC9A01_DISPON           (0x29U)      /* Display on */
#define GC9A01_CASET            (0x2AU)      /* Column address set */
#define GC9A01_RASET            (0x2BU)      /* Row address set */
#define GC9A01_RAMWR            (0x2CU)      /* Memory write */
#define GC9A01_RAMRD            (0x2EU)      /* Memory read */
#define GC9A01_MADCTL           (0x36U)      /* Memory data access control */
#define GC9A01_COLMOD           (0x3AU)      /* Interface pixel format */
#define GC9A01_SETPCR            (0xB2U)      /* Porch control */
#define GC9A01_SETDGC            (0xE8U)      /* Display gamma control */
#define GC9A01_FRCTRL2           (0xC6U)      /* Frame rate control */

/* MADCTL bits */
#define GC9A01_MAD_MY           (1U << 7)    /* Row order (bottom to top) */
#define GC9A01_MAD_MX           (1U << 6)    /* Column order (right to left) */
#define GC9A01_MAD_MV           (1U << 5)    /* Row/column exchange */
#define GC9A01_MAD_ML           (1U << 4)    /* Vertical refresh order (bottom to top) */
#define GC9A01_MAD_BGR          (1U << 3)    /* RGB → BGR order */
#define GC9A01_MAD_MH           (1U << 2)    /* Horizontal refresh order (right to left) */

/* COLMOD pixel formats */
#define GC9A01_COLMOD_12BIT     (0x03U)      /* 12-bit RGB444 */
#define GC9A01_COLMOD_16BIT     (0x05U)      /* 16-bit RGB565 */
#define GC9A01_COLMOD_18BIT     (0x06U)      /* 18-bit RGB666 (default) */

/* ========================================================================== */
/*  Color Lookup Table — Tomographic Velocity-to-Color Mapping                */
/*  Maps 8-bit velocity index (0–255) to 16-bit RGB565 colour.               */
/* ========================================================================== */

/**
 * Pre-computed 256-entry colour map: blue (fast) → cyan → green → yellow →
 * orange → red (slow) — the "turbo" or "jet" colour scheme adapted for
 * acoustic velocity visualisation.
 * Defined as external in display.c.
 */
extern const uint16_t tomo_colormap[256];

/* ========================================================================== */
/*  State Machine — Firmware Global States                                    */
/* ========================================================================== */

enum sonicsight_state {
    STATE_INIT          = 0,      /**< Power-on initialisation */
    STATE_IDLE          = 1,      /**< Low-power sleep, waiting for command */
    STATE_ARM_SENSORS   = 2,      /**< Check coupling, pre-charge HV */
    STATE_TX_CHANNEL    = 3,      /**< Fire one transducer, capture all RX */
    STATE_ALL_TX_DONE   = 4,      /**< All transmitters fired, matrix built */
    STATE_RECONSTRUCT   = 5,      /**< Run tomographic inversion */
    STATE_DISPLAY       = 6,      /**< Push image to LCD */
    STATE_STREAM        = 7,      /**< Stream tomogram + ToF data via BLE */
    STATE_CALIBRATE     = 8,      /**< Run calibration phantom scan */
    STATE_ERROR         = 9,      /**< Fault display — requires user reset */
    STATE_NUM_STATES
};

/* ========================================================================== */
/*  Error Codes                                                               */
/* ========================================================================== */

#define ERR_OK                   (0)
#define ERR_BAD_COUPLING         (-1)    /* One or more transducers poorly coupled */
#define ERR_LOW_BATTERY          (-2)    /* Battery voltage below safe threshold */
#define ERR_SD_FAIL              (-3)    /* SD card init or write failure */
#define ERR_ADC_SYNC             (-4)    /* ADC frame sync timeout */
#define ERR_TOMO_SOLVER          (-5)    /* Matrix inversion failed (singular) */
#define ERR_CALIBRATION          (-6)    /* Calibration data out of range */
#define ERR_BLE_DISCONNECTED     (-7)    /* BLE link lost during streaming */
#define ERR_OVER_TEMPERATURE     (-8)    /* Internal temperature exceeded 70 °C */
#define ERR_HV_UNDERVOLTAGE     (-9)    /* HV rail below 120 V */
#define ERR_MUX_FAILED          (-10)    /* Mux address did not settle */

#ifdef __cplusplus
}
#endif

#endif /* REGISTERS_H */