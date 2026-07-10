/*
 * registers.h — Peripheral register maps for Occlusograph sensors and PMIC.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 *
 * This header consolidates the register-level definitions for every external
 * IC on the board: the AD7746 CDC, ICM-42688-P IMU, TMP117 temperature sensor,
 * MAX30101 reflectance sensor, and the W25R128 QSPI flash. The nRF5340's own
 * SAADC and GPIO registers are covered by the Zephyr nRF HAL and are not
 * redefined here.
 */

#ifndef OCCLUSOGRAPH_REGISTERS_H
#define OCCLUSOGRAPH_REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* =====================================================================
 * AD7746 — 24-bit Capacitance-to-Digital Converter
 * =====================================================================
 * The AD7746 measures capacitance on two input channels (CIN1, CIN2) and an
 * internal voltage/temperature channel. We use CIN1 and CIN2 on each of two
 * devices. Each input is externally muxed 8:1, giving 32 total elements.
 */

#define AD7746_REG_STATUS       0x00u
#define AD7746_REG_CAP_SETUP_A   0x07u
#define AD7746_REG_CAP_SETUP_B   0x08u
#define AD7746_REG_CAP_GAIN_A    0x09u
#define AD7746_REG_CAP_GAIN_B    0x0Au
#define AD7746_REG_CAP_OFFSET_A  0x0Bu
#define AD7746_REG_CAP_OFFSET_B  0x0Cu
#define AD7746_REG_VOLT_SETUP    0x0Du
#define AD7746_REG_EXC_SETUP     0x0Eu
#define AD7746_REG_CONFIG       0x0Au  /* shared config / conversion mode */
#define AD7746_REG_CAP_DATA_HIG 0x01u
#define AD7746_REG_CAP_DATA_MID 0x02u
#define AD7746_REG_CAP_DATA_LOW 0x03u
#define AD7746_REG_VOLT_DATA_HIG 0x04u
#define AD7746_REG_VOLT_DATA_MID 0x05u
#define AD7746_REG_VOLT_DATA_LOW 0x06u

/* STATUS bits */
#define AD7746_STATUS_RDY       0x80u
#define AD7746_STATUS_RDYV     0x40u
#define AD7746_STATUS_RDYT     0x20u
#define AD7746_STATUS_EXCERR    0x10u
#define AD7746_STATUS_RDYCAPA  0x08u
#define AD7746_STATUS_RDYCAPB  0x04u

/* CAP_SETUP bits */
#define AD7746_CAPSET_EN       0x80u
#define AD7746_CAPSET_CIN2     0x40u   /* 1 = CIN2, 0 = CIN1 */
#define AD7746_CAPSET_CIN2_DIFF 0x20u
#define AD7746_CAPSET_CAPGAIN  0x0Fu  /* gain bits, masks */

/* CONFIG bits (conversion mode / update rate) */
#define AD7746_CONFIG_MD_CONT   0x00u   /* continuous */
#define AD7746_CONFIG_MD_SINGLE 0x02u
#define AD7746_CONFIG_MD_IDLE   0x04u
#define AD7746_CONFIG_FTSL_0   0x00u   /* 16.6 ms */
#define AD7746_CONFIG_FTSL_1   0x10u   /* 8 ms */
#define AD7746_CONFIG_FTSL_2   0x20u   /* 4 ms */
#define AD7746_CONFIG_FTSL_3   0x30u   /* 1 ms (lower resolution) */

/* EXC_SETUP bits — excitation output */
#define AD7746_EXCEN            0x0Cu   /* enable EXCA/EXCB */

/* Conversion: 24-bit signed value, full scale = ±4.096 pF */
#define AD7746_FS_PF            4.096f
#define AD7746_CODE_MAX         0x7FFFFFu

/* =====================================================================
 * ICM-42688-P — 6-axis IMU (accel + gyro)
 * =====================================================================
 */

#define ICM_REG_WHOAMI          0x75u
#define ICM_VAL_WHOAMI         0x47u
#define ICM_REG_PWR_MGMT0      0x4Fu
#define ICM_VAL_PWR_MGMT0_ON   0x0Fu   /* gyro + accel, LN mode */
#define ICM_VAL_PWR_MGMT0_OFF  0x00u
#define ICM_REG_GYRO_CONFIG0   0x4Fu
#define ICM_REG_ACCEL_CONFIG0  0x50u
#define ICM_REG_GYRO_DATAX0    0x1Fu
#define ICM_REG_ACCEL_DATAX0   0x1Bu
#define ICM_REG_INT_CONFIG    0x14u
#define ICM_REG_INT_STATUS    0x2Du
#define ICM_REG_INT_CONFIG0   0x63u
#define ICM_REG_INT_CONFIG1   0x64u
#define ICM_REG_FIFO_CONFIG   0x66u
#define ICM_REG_FIFO_COUNT     0x6Eu

/* Accelerometer full-scale ranges (CONFIG0 bits 5:3) */
#define ICM_ACCEL_FS_2G        0x00u
#define ICM_ACCEL_FS_4G        0x08u
#define ICM_ACCEL_FS_8G        0x10u
#define ICM_ACCEL_FS_16G       0x18u
/* Output data rate (CONFIG0 bits 2:0) */
#define ICM_ODR_1KHZ           0x08u
#define ICM_ODR_200HZ          0x05u
#define ICM_ODR_100HZ          0x04u

/* Gyro ranges (CONFIG0 bits 5:3) */
#define ICM_GYRO_FS_2000DPS    0x18u
#define ICM_GYRO_FS_1000DPS    0x10u
#define ICM_GYRO_FS_500DPS     0x08u
#define ICM_GYRO_FS_250DPS     0x00u

/* Sensitivity (LSB / physical unit) at selected FS + scale */
#define ICM_ACCEL_LSB_PER_G_2G  16384.0f   /* 16-bit; we use 8-bit FIFO */
#define ICM_GYRO_LSB_PER_DPS_2000  16.4f

/* =====================================================================
 * TMP117 — High-accuracy digital temperature sensor
 * =====================================================================
 */

#define TMP117_REG_TEMP        0x00u
#define TMP117_REG_CONFIG      0x01u
#define TMP117_REG_THIGH_LIMIT 0x02u
#define TMP117_REG_TLOW_LIMIT  0x03u
#define TMP117_REG_ID          0x0Fu

/* Temperature register: 16-bit signed, 7.8125 m°C / LSB */
#define TMP117_RES_MC          7.8125f

/* =====================================================================
 * MAX30101 — Reflectance pulse-ox / proximity (used for tissue-contact detect)
 * =====================================================================
 */

#define MAX_REG_INT_STATUS     0x00u
#define MAX_REG_INT_ENABLE     0x02u
#define MAX_REG_FIFO_WR_PTR    0x04u
#define MAX_REG_OVF_COUNTER    0x05u
#define MAX_REG_FIFO_RD_PTR    0x06u
#define MAX_REG_FIFO_DATA      0x07u
#define MAX_REG_FIFO_CONFIG    0x08u
#define MAX_REG_MODE_CONFIG    0x09u
#define MAX_REG_SPO2_CONFIG    0x0Au
#define MAX_REG_LED1_PA        0x0Cu
#define MAX_REG_LED2_PA        0x0Du
#define MAX_REG_LED3_PA        0x10u
#define MAX_REG_PROX_INT_THRESH 0x10u
#define MAX_REG_MULTI_LED_CTRL1 0x11u
#define MAX_REG_MULTI_LED_CTRL2 0x12u
#define MAX_REG_DIE_TEMP_INT   0x1Fu
#define MAX_REG_DIE_TEMP_FRAC  0x20u
#define MAX_REG_DIE_TEMP_EN    0x21u
#define MAX_REG_PART_ID        0xFFu
#define MAX_VAL_PART_ID        0x15u

/* Mode configuration */
#define MAX_MODE_HR            0x02u   /* heart rate (red + IR) */
#define MAX_MODE_SPO2          0x03u
#define MAX_MODE_MULTILED      0x07u
#define MAX_MODE_SHDN          0x80u
#define MAX_MODE_RESET         0x40u

/* =====================================================================
 * W25R128 — 16 MB QSPI NOR flash (event buffer)
 * =====================================================================
 */

#define W25_CMD_READ           0x03u
#define W25_CMD_FAST_READ      0x0Bu
#define W25_CMD_PAGE_PROGRAM   0x02u
#define W25_CMD_SECTOR_ERASE    0x20u
#define W25_CMD_CHIP_ERASE      0x60u
#define W25_CMD_READ_STATUS    0x05u
#define W25_CMD_READ_STATUS2   0x35u
#define W25_CMD_WRITE_ENABLE   0x06u
#define W25_CMD_READ_ID        0x9Fu
#define W25_CMD_RELEASE_DEEP   0xABu
#define W25_CMD_ENTER_DEEP     0xB9u

/* Status register bits */
#define W25_STATUS_WIP         0x01u   /* write-in-progress */
#define W25_STATUS_WEL         0x02u   /* write-enable latch */
#define W25_STATUS_BP_SHIFT    2u
#define W25_STATUS_SRP         0x80u

/* Expected JEDEC ID for W25R128 */
#define W25_JEDEC_MANUF        0xEFu   /* Winbond */
#define W25_JEDEC_TYPE         0x40u
#define W25_JEDEC_CAPACITY     0x18u   /* 16 MB = 2^24 */

/* =====================================================================
 * nRF5340 SAADC — quick register summary for the charge-amp path
 * (Zephyr HAL normally covers this; we expose a few constants the
 * driver uses directly for speed-critical paths.)
 * =====================================================================
 */

#define SAADC_RES_12BIT        0x03u   /* 12-bit resolution */
#define SAADC_GAIN_1_6         0x00u   /* 1/6 gain (for 3.3V range) */
#define SAADC_REF_INTERNAL     0x00u
#define SAADC_TACQ_10US        0x02u
#define SAADC_MODE_SINGLE      0x00u
#define SAADC_BURST_ENABLED    0x01u

/* =====================================================================
 * PMIC / charger (on-board micro-buck + LDO; charge controller is a
 * discrete LTC4124-compatible via I²C, address 0x40)
 * =====================================================================
 */

#define LTC4124_ADDR           0x40u
#define LTC_REG_STATUS         0x00u
#define LTC_REG_CHARGE         0x01u
#define LTC_REG_CONFIG         0x02u
#define LTC_REG_VBAT           0x03u
#define LTC_REG_TEMP           0x04u

#define LTC_STATUS_CHARGING     0x01u
#define LTC_STATUS_FAULT       0x08u
#define LTC_STATUS_PRESENT     0x20u

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSOGRAPH_REGISTERS_H */