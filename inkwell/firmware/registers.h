/*
 * registers.h — Peripheral register definitions for Inkwell
 *
 * Defines register addresses and bit fields for the off-chip peripherals
 * used by Inkwell: BMI270, BMM150, PMW3360, W25Q64, MAX17048, HX711.
 * The nRF52833's own register map is provided here as a thin convenience
 * layer; the full set lives in the Nordic MDK headers normally included
 * via `nrf.h`, but this file is self-contained so the tree compiles in
 * environments without the full SDK installed.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef INKWELL_REGISTERS_H
#define INKWELL_REGISTERS_H

#include <stdint.h>

/* ===================================================================== */
/* nRF52833 convenience base addresses (subset we use)                    */
/* ===================================================================== */
#define NRF_CLOCK_BASE      (0x40000000UL)
#define NRF_POWER_BASE      (0x40000000UL + 0x1000)
#define NRF_RADIO_BASE      (0x40001000UL)
#define NRF_UARTE0_BASE     (0x40002000UL)
#define NRF_SPIM0_BASE      (0x40003000UL)
#define NRF_SPIM1_BASE      (0x40004000UL)
#define NRF_SPIM2_BASE      (0x40023000UL)
#define NRF_TWIM0_BASE      (0x40003000UL)  /* shared with SPIM0 */
#define NRF_GPIO_BASE        (0x50000000UL)
#define NRF_RTC0_BASE        (0x4000B000UL)
#define NRF_TIMER0_BASE      (0x40008000UL)
#define NRF_SAADC_BASE       (0x40007000UL)

/* ===================================================================== */
/* BMI270 — Accelerometer + Gyroscope                                    */
/* ===================================================================== */
#define BMI270_REG_CHIPID      (0x00U)
#define BMI270_CHIPID_VAL      (0x24U)
#define BMI270_REG_ERR_REG     (0x02U)
#define BMI270_REG_STATUS      (0x03U)
#define BMI270_REG_ACC_X_LSB   (0x0CU)
#define BMI270_REG_GYR_X_LSB   (0x12U)
#define BMI270_REG_FIFO_LENGTH_LSB (0x22U)
#define BMI270_REG_FIFO_LENGTH_MSB (0x23U)
#define BMI270_REG_FIFO_DATA   (0x26U)
#define BMI270_REG_INT_STATUS_1 (0x1EU)
#define BMI270_REG_INT_CTRL    (0x1DU)
#define BMI270_REG_FIFO_CONFIG_0 (0x48U)
#define BMI270_REG_FIFO_CONFIG_1 (0x49U)
#define BMI270_REG_FIFO_WTM_0  (0x46U)
#define BMI270_REG_FIFO_WTM_1  (0x47U)
#define BMI270_REG_ACC_RANGE   (0x41U)
#define BMI270_REG_GYR_RANGE   (0x42U)
#define BMI270_REG_ACC_CONF   (0x40U)
#define BMI270_REG_GYR_CONF   (0x4DU)
#define BMI270_REG_PWR_CTRL   (0x4DU)
#define BMI270_REG_PWR_CONF   (0x4CU)
#define BMI270_REG_INT1_IO_CTRL (0x53U)
#define BMI270_REG_INT_MAP_1   (0x56U)
#define BMI270_REG_CMD        (0x7EU)
#define BMI270_CMD_FIFO_FLUSH  (0xB0U)
#define BMI270_CMD_SOFTRESET   (0xB6U)

#define BMI270_FIFO_WTM        (240U)  /* bytes; ~80 samples */
#define BMI270_ACC_RANGE_8G    (0x08U)
#define BMI270_GYR_RANGE_1000  (0x02U)
#define BMI270_ACC_BWP_OSR2   (0x08U)  /* osr2, 1kHz */
#define BMI270_GYR_BWP_OSR2   (0x08U)

/* FIFO header bits (BMI270 advanced format) */
#define BMI270_FIFO_HEADER_MSK    (0xFCU)
#define BMI270_FIFO_HEADER_SKIP   (0x40U)
#define BMI270_FIFO_HEADER_SENS   (0x80U)
#define BMI270_FIFO_HEADER_MGMT   (0x90U)
#define BMI270_FIFO_DATA_EN_BIT   (0x80U)

/* ===================================================================== */
/* BMM150 — Magnetometer                                                 */
/* ===================================================================== */
#define BMM150_REG_CHIPID      (0x40U)
#define BMM150_CHIPID_VAL      (0x32U)
#define BMM150_REG_DATA_X_LSB  (0x42U)
#define BMM150_REG_DATA_X_MSB  (0x43U)
#define BMM150_REG_DATA_Y_LSB  (0x44U)
#define BMM150_REG_DATA_Y_MSB  (0x45U)
#define BMM150_REG_DATA_Z_LSB  (0x46U)
#define BMM150_REG_DATA_Z_MSB  (0x47U)
#define BMM150_REG_DATA_READY  (0x48U)
#define BMM150_REG_PWR_CTRL   (0x4BU)
#define BMM150_REG_OP_MODE    (0x4CU)
#define BMM150_REG_REP_XY     (0x51U)
#define BMM150_REG_REP_Z      (0x52U)
#define BMM150_PWR_NORMAL     (0x01U)
#define BMM150_OP_MODE_NORMAL (0x00U)
#define BMM150_REP_XY_LOWPOWER (0x01U)  /* 3 Hz */
#define BMM150_REP_XY_REGULAR (0x0AU)  /* ~10 Hz */
#define BMM150_REP_Z_LOWPOWER (0x01U)

/* ===================================================================== */
/* PMW3360 — Optical Flow                                                */
/* ===================================================================== */
#define PMW3360_REG_PRODUCT_ID   (0x00U)
#define PMW3360_PRODUCT_ID_VAL  (0x42U)
#define PMW3360_REG_REVISION_ID  (0x01U)
#define PMW3360_REG_MOTION       (0x02U)
#define PMW3360_REG_DELTA_X_LSB  (0x03U)
#define PMW3360_REG_DELTA_X_MSB  (0x04U)
#define PMW3360_REG_DELTA_Y_LSB  (0x05U)
#define PMW3360_REG_DELTA_Y_MSB  (0x06U)
#define PMW3360_REG_SQUAL        (0x07U)
#define PMW3360_REG_RAW_DATA_SUM (0x08U)
#define PMW3360_REG_MAXIMUM_RAW  (0x09U)
#define PMW3360_REG_MINIMUM_RAW  (0x0AU)
#define PMW3360_REG_SHUTTER_LOWER (0x0BU)
#define PMW3360_REG_SHUTTER_UPPER (0x0CU)
#define PMW3360_REG_CONFIG1      (0x0FU)
#define PMW3360_REG_CONFIG2      (0x10U)
#define PMW3360_REG_ANGLE_SNAP   (0x11U)
#define PMW3360_REG_LIFT_CONTROL  (0x12U)
#define PMW3360_REG_LIFT_DETECTION (0x13U)
#define PMW3360_REG_MOTION_BURST (0x16U)
#define PMW3360_REG_POWER_UP     (0x3AU)
#define PMW3360_REG_POWER_DOWN   (0x3BU)
#define PMW3360_REG_SHUTDOWN     (0x3CU)
#define PMW3360_CONFIG1_CPI_DIV1200 (0x00U)  /* 12000 CPI default */
#define PMW3360_MOTION_MOT_BIT     (0x80U)
#define PMW3360_SQUAL_THRESHOLD    (60U)

/* ===================================================================== */
/* W25Q64 — 8 MB SPI NOR flash                                          */
/* ===================================================================== */
#define W25Q64_REG_MANUFACTURER_ID (0x90U)
#define W25Q64_MFR_WINBOND          (0xEFU)
#define W25Q64_DEV_W25Q64           (0x4017U)
#define W25Q64_REG_JEDEC_ID         (0x9FU)
#define W25Q64_REG_STATUS1          (0x05U)
#define W25Q64_REG_STATUS2          (0x35U)
#define W25Q64_REG_STATUS3          (0x15U)
#define W25Q64_REG_FLAG_STATUS      (0x70U)
#define W25Q64_CMD_READ             (0x03U)
#define W25Q64_CMD_FAST_READ        (0x0BU)
#define W25Q64_CMD_PAGE_PROGRAM     (0x02U)
#define W25Q64_CMD_SECTOR_ERASE     (0x20U)
#define W25Q64_CMD_BLOCK_ERASE_32K  (0x52U)
#define W25Q64_CMD_CHIP_ERASE       (0xC7U)
#define W25Q64_CMD_POWER_DOWN       (0xB9U)
#define W25Q64_CMD_RELEASE_PD       (0xABU)
#define W25Q64_CMD_WRITE_ENABLE     (0x06U)
#define W25Q64_CMD_WRITE_DISABLE    (0x04U)
#define W25Q64_CMD_READ_SFDP        (0x5AU)
#define W25Q64_CMD_ENTER_4B_ADDR    (0xB7U)
#define W25Q64_STATUS_BUSY_BIT      (0x01U)
#define W25Q64_STATUS_WEL_BIT       (0x02U)
#define W25Q64_STATUS_BP_MASK       (0x3CU)

/* ===================================================================== */
/* MAX17048 — Fuel gauge                                                 */
/* ===================================================================== */
#define MAX17048_REG_VCELL      (0x02U)
#define MAX17048_REG_SOC        (0x04U)
#define MAX17048_REG_MODE       (0x06U)
#define MAX17048_REG_VERSION    (0x08U)
#define MAX17048_REG_HIBRT      (0x0AU)
#define MAX17048_REG_CONFIG     (0x0BU)
#define MAX17048_REG_VALRT      (0x0EU)
#define MAX17048_REG_CRATE      (0x0DU)
#define MAX17048_REG_VRESET     (0x18U)
#define MAX17048_REG_STATUS     (0x1AU)
#define MAX17048_REG_CMD        (0xFEU)
#define MAX17048_CMD_RESET      (0x5400U)
#define MAX17048_CMD_QUICKSTART (0x4000U)

/* ===================================================================== */
/* HX711 — 24-bit strain-gauge ADC                                      */
/* ===================================================================== */
#define HX711_GAIN_128         (1U)  /* ch A, gain 128 */
#define HX711_GAIN_32          (2U)  /* ch B, gain 32 */
#define HX711_GAIN_64          (3U)  /* ch A, gain 64 */
#define HX711_SETTLE_US        (500U)
#define HX711_BITS             (24U)
#define HX711_TIMEOUT_US       (200000U)

/* ===================================================================== */
/* Common utility macros                                                 */
/* ===================================================================== */
#define ARRAY_LEN(a)       (sizeof(a) / sizeof((a)[0]))
#define MIN(a,b)           ((a) < (b) ? (a) : (b))
#define MAX(a,b)           ((a) > (b) ? (a) : (b))
#define CLAMP(v,lo,hi)     MAX((lo), MIN((hi), (v)))
#define BIT(n)             (1UL << (n))
#define REG16(addr)       (*(volatile uint16_t *)(uintptr_t)(addr))
#define REG32(addr)       (*(volatile uint32_t *)(uintptr_t)(addr))

#endif /* INKWELL_REGISTERS_H */