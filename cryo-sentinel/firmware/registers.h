/*
 * registers.h — Register definitions for the external ICs on Cryo-Sentinel.
 *
 * Covers: TI FDC2214 (capacitive level), Maxim MAX31865 (RTD),
 *         Bosch BME280 (ambient), TDK ICM-45686 (IMU),
 *         Fujitsu MB85RC04 (FRAM), TCA9548A (I2C mux),
 *         TI BQ25895 (charger), TI MAX17055 (fuel gauge).
 *
 * Author: jayis1
 * License: MIT
 */
#ifndef CRYO_SENTINEL_REGISTERS_H
#define CRYO_SENTINEL_REGISTERS_H

#include <stdint.h>

/* ===================================================================== *
 *  TI FDC2214 — 4-channel capacitance-to-digital converter
 *  I2C addr 0x2A. We use CH0 (the level strip) and CH1 (a reference
 *  electrode for drift compensation).
 * ===================================================================== */
#define FDC2214_I2C_ADDR            0x2A

#define FDC2214_REG_RCOUNT_CH0      0x08
#define FDC2214_REG_RCOUNT_CH1      0x09
#define FDC2214_REG_OFFSET_CH0      0x0C
#define FDC2214_REG_OFFSET_CH1      0x0D
#define FDC2214_REG_SETTLECOUNT_CH0 0x10
#define FDC2214_REG_SETTLECOUNT_CH1 0x11
#define FDC2214_REG_FDIV_CH0        0x14
#define FDC2214_REG_FDIV_CH1        0x15
#define FDC2214_REG_CONFIG          0x1A  /* sleep / mode / ref / int */
#define FDC2214_REG_MUX_CONFIG      0x1B  /* autosc / deglitch / ch seq */
#define FDC2214_REG_STATUS          0x1C  /* DRDY / chan / err flags */
#define FDC2214_REG_RERROR_CONFIG   0x1E
#define FDC2214_REG_LC_CONFIG_CH0   0x1F
#define FDC2214_REG_LC_CONFIG_CH1   0x20
#define FDC2214_REG_WAIT_CH0        0x30
#define FDC2214_REG_WAIT_CH1        0x31
#define FDC2214_REG_RD_CH0_MSB      0x00  /* read result MSB+LSB */
#define FDC2214_REG_RD_CH0_LSB      0x01
#define FDC2214_REG_RD_CH1_MSB      0x02
#define FDC2214_REG_RD_CH1_LSB      0x03

#define FDC2214_CONFIG_ACTIVE_CHAN0 (0u << 14)
#define FDC2214_CONFIG_SLEEP_EN     (1u << 13)
#define FDC2214_CONFIG_MODE_ACTIVE  (0u << 10)  /* 00 = active, 11 = sleep */
#define FDC2214_CONFIG_HIGH_CURRENT (1u << 9)
#define FDC2214_CONFIG_INTB_DIS     (1u << 6)
#define FDC2214_CONFIG_DRDY_INT     (1u << 0)

/* Sensitivity: RCOUNT sets conversion time. 0xFFFF ≈ 4 ms / ch @ 10 MHz. */
#define FDC2214_RCOUNT_DEFAULT      0xFFFF
#define FDC2214_SETTLE_DEFAULT      0x0400

/* ===================================================================== *
 *  MAX31865 — PT1000 RTD-to-digital, SPI
 *  3 devices on the SPI bus, each with its own CS.
 * ===================================================================== */
#define MAX31865_REG_CONFIG         0x00
#define MAX31865_REG_RTD_MSB        0x01
#define MAX31865_REG_RTD_LSB        0x02
#define MAX31865_REG_HFT_MSB        0x03  /* high fault threshold */
#define MAX31865_REG_HFT_LSB        0x04
#define MAX31865_REG_LFT_MSB        0x05  /* low fault threshold */
#define MAX31865_REG_LFT_LSB        0x06
#define MAX31865_REG_FAULT_STATUS   0x07

#define MAX31865_CFG_VBIAS          (1u << 7)  /* enable bias voltage */
#define MAX31865_CFG_CONV_AUTO      (1u << 6)  /* auto conversion mode */
#define MAX31865_CFG_CONV_1SHOT     (1u << 5)  /* 1-shot conversion */
#define MAX31865_CFG_3WIRE          (1u << 4)  /* 1 = 3-wire, 0 = 2/4-wire */
#define MAX31865_CFG_FAULT_DETECT   (1u << 3)
#define MAX31865_CFG_FAULT_STATUSCLR (1u << 1)
#define MAX31865_CFG_50HZ_FILTER    (1u << 0)  /* 1 = 50Hz, 0 = 60Hz */

#define MAX31865_FAULT_OVUV         (1u << 7)  /* over/undervoltage */
#define MAX31865_FAULT_RTDIN_LOW    (1u << 6)  /* RTDIN- < low threshold */
#define MAX31865_FAULT_REF_LOW      (1u << 5)  /* REF- < low threshold */
#define MAX31865_FAULT_REF_HIGH     (1u << 4)  /* REF- > high threshold */
#define MAX31865_FAULT_RTDIN_HIGH   (1u << 3)  /* RTDIN- > high threshold */
#define MAX31865_FAULT_LOW_THRESH   (1u << 2)
#define MAX31865_FAULT_HIGH_THRESH  (1u << 1)
#define MAX31865_FAULT_OVUV2        (1u << 0)

/* RTD math for PT1000: rRref = 4300 ohm; RTD = (code/32768) * rRef. */
#define MAX31865_RREF_PT1000        4300.0f
#define MAX31865_RTD_NOMINAL_PT1000 1000.0f
#define MAX31865_ALPHA_PT1000       0.003851f  /* Pt class B */

/* ===================================================================== *
 *  Bosch BME280 — ambient T/RH/P, I2C 0x76
 * ===================================================================== */
#define BME280_I2C_ADDR             0x76
#define BME280_REG_ID               0xD0  /* should read 0x60 */
#define BME280_REG_RESET            0xE0  /* write 0xB6 */
#define BME280_REG_CTRL_HUM         0xF2
#define BME280_REG_STATUS           0xF3
#define BME280_REG_CTRL_MEAS        0xF4
#define BME280_REG_CONFIG           0xF5
#define BME280_REG_PRESS_MSB        0xF7
#define BME280_REG_TEMP_MSB         0xFA
#define BME280_REG_HUM_MSB          0xFD
#define BME280_REG_CALIB00          0x88  /* calibration block */
#define BME280_REG_CALIB26          0xA1
#define BME280_REG_CALIB41          0xE1

#define BME280_MODE_FORCED          (1u << 0)  /* 1 = forced, others = sleep */
#define BME280_MODE_SLEEP           (0u << 0)
#define BME280_OSRS_T_X1            (1u << 5)
#define BME280_OSRS_P_X1            (1u << 2)
#define BME280_OSRS_H_X1            0x01
#define BME280_STBY_500MS           (4u << 5)  /* t_sb */
#define BME280_FILTER_OFF           (0u << 2)

/* ===================================================================== *
 *  TDK ICM-45686 — 6-axis IMU, I2C 0x69 (AD0=1) / 0x68 (AD0=0)
 * ===================================================================== */
#define ICM45686_I2C_ADDR_AD0_0     0x68
#define ICM45686_I2C_ADDR_AD0_1     0x69
#define ICM45686_REG_WHOAMI         0x72  /* should read 0xE9 */
#define ICM45686_REG_PWR_MGMT0      0x4F
#define ICM45686_REG_GYRO_CONFIG0   0x50
#define ICM45686_REG_ACCEL_CONFIG0  0x51
#define ICM45686_REG_INT_CONFIG     0x63
#define ICM45686_REG_INT_STATUS     0x2D
#define ICM45686_REG_ACCEL_DATA_X1  0x1F
#define ICM45686_REG_TEMP_DATA0     0x3D

#define ICM45686_PWR_TEMP_DIS       (1u << 5)
#define ICM45686_PWR_IDLE           (1u << 2)
#define ICM45686_PWR_GYRO_MODE_LN   (3u << 0)  /* low-noise */
#define ICM45686_PWR_ACCEL_MODE_LN  (3u << 2)

#define ICM45686_GYR_FS_2000DPS     (0u << 5)  /* 2000 dps full scale */
#define ICM45686_GYR_ODR_50HZ       (6u << 0)
#define ICM45686_ACC_FS_4G          (1u << 5)  /* 4g full scale */
#define ICM45686_ACC_ODR_50HZ       (6u << 0)

#define ICM45686_ACCEL_SENS_4G      (2048.0f)  /* LSB/g at 4g FS */
#define ICM45686_GYRO_SENS_2000DPS  (16.4f)    /* LSB/dps at 2000 dps FS */

/* ===================================================================== *
 *  TCA9548A — I2C multiplexer, addr 0x70
 *  We route the shared I2C to one of the 3 MAX31865 control-register
 *  banks (the MAX31865 data path is SPI; its control/status is also
 *  mirrored on I2C on the breakout we use).
 * ===================================================================== */
#define TCA9548A_I2C_ADDR           0x70
#define TCA9548A_CH_RTD_TOP         (1u << 0)
#define TCA9548A_CH_RTD_MID         (1u << 1)
#define TCA9548A_CH_RTD_BOT         (1u << 2)

/* ===================================================================== *
 *  Fujitsu MB85RC04 — 4 Kbit I2C FRAM, addr 0x50 (shared with calibration
 *  region; the device occupies 0x50..0x53 and we partition by offset).
 * ===================================================================== */
/* (address constants in board.h; here the device identity only) */
#define MB85RC04_WHOAMI             0xF8  /* read at 0x07 gives 0x04 .. */

/* ===================================================================== *
 *  TI BQ25895 — USB charger, I2C 0x6B
 * ===================================================================== */
#define BQ25895_I2C_ADDR            0x6B
#define BQ25895_REG_ICHARGE         0x04  /* [7:3] = ICHG, 64 mA steps */
#define BQ25895_REG_VCHARGE         0x06  /* [7:3] = VREG, 16 mV steps */
#define BQ25895_REG_CTRL0           0x00  /* EN_HIZ / EN_ILIM */
#define BQ25895_REG_SYS_CTRL        0x07  /* WATCHDOG / ICO */
#define BQ25895_REG_FAULT           0x0C
#define BQ25895_REG_VBUS_STAT       0x0B  /* VBUS status / VSYS_MIN */
#define BQ25895_REG_CHG_STAT        0x0B  /* [5:4] = charging state */

#define BQ25895_STAT_NOT_CHARGING   (0u << 4)
#define BQ25895_STAT_PRECHARGE      (1u << 4)
#define BQ25895_STAT_FAST_CHARGE    (2u << 4)
#define BQ25895_STAT_CHARGE_DONE    (3u << 4)

/* ===================================================================== *
 *  TI MAX17055 — fuel gauge, I2C 0x36
 * ===================================================================== */
#define MAX17055_I2C_ADDR           0x36
#define MAX17055_REG_STATUS         0x00
#define MAX17055_REG_REPCAP         0x05
#define MAX17055_REG_REPSOC         0x06  /* reported SOC % */
#define MAX17055_REG_VCELL          0x09
#define MAX17055_REG_CURRENT        0x0A
#define MAX17055_REG_AVG_CURRENT    0x0B
#define MAX17055_REG_TEMP           0x08
#define MAX17055_REG_CYCLES         0x17
#define MAX17055_REG_CONFIG         0x1D
#define MAX17055_REG_CONFIG2        0x1F  /* dQACC / dAcc */

#define MAX17055_STATUS_POR         (1u << 1)  /* power-on reset */
#define MAX17055_STATUS_VMN         (1u << 9)  /* min voltage alert */
#define MAX17055_STATUS_TMN         (1u << 8)  /* min temp alert */
#define MAX17055_STATUS_SMN         (1u << 7)  /* min SOC alert */
#define MAX17055_STATUS_BST         (1u << 6)  /* battery status */
#define MAX17055_VSCALE             78.125f    /* LSB = 78.125 µV */
#define MAX17055_ISCALE             1.5625f    /* LSB = 1.5625 µV */

/* ===================================================================== *
 *  Quectel BG770A — LTE-M / NB-IoT modem, UART
 * ===================================================================== */
#define BG770A_BAUD                 115200
#define BG770A_AT_TIMEOUT_MS        8000
#define BG770A_RDY_TIMEOUT_MS       15000

#endif /* CRYO_SENTINEL_REGISTERS_H */