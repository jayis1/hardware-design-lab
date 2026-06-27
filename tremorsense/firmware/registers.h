/*
 * registers.h — Hardware register definitions for TremorSense
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Register maps for: nRF5340 SoC, ICM-42688-P IMU, GC9A01 OLED,
 * MX25R6435F flash, MAX17048 fuel gauge, SHT40, LPS22HB.
 */

#ifndef TREMORSENSE_REGISTERS_H
#define TREMORSENSE_REGISTERS_H

#include <stdint.h>

/* ============================================================
 * nRF5340 Application Core Register Map (selected)
 * ============================================================ */

#define NRF53_BASE            0x40000000UL
#define NRF53_P0_BASE          (NRF53_BASE + 0x00800)  /* GPIO port 0 */
#define NRF53_P1_BASE          (NRF53_BASE + 0x00900)  /* GPIO port 1 */

/* GPIO */
#define GPIO_OUT               0x504
#define GPIO_OUTSET            0x508
#define GPIO_OUTCLR            0x50C
#define GPIO_IN                0x510
#define GPIO_DIR               0x514
#define GPIO_PIN_CNF(n)        (0x700 + (n) * 4)

/* GPIOTE */
#define GPIOTE_BASE            0x40006000UL
#define GPIOTE_INTENSET        0x304
#define GPIOTE_EVENTS_IN(n)    (0x100 + (n) * 4)
#define GPIOTE_EVENTS_PORT    0x13C
#define GPIOTE_PSEL(n)         (0x500 + (n) * 4)
#define GPIOTE_CONFIG(n)       (0x510 + (n) * 4)

/* SPI (SPIM) */
#define SPIM_BASE(n)           (0x40008000UL + (n) * 0x1000)
#define SPIM_TASKS_START       0x010
#define SPIM_TASKS_STOP        0x014
#define SPIM_EVENTS_ENDRX      0x110
#define SPIM_EVENTS_END        0x118
#define SPIM_EVENTS_ENDTX      0x120
#define SPIM_ENABLE            0x500
#define SPIM_PSEL_SCK          0x508
#define SPIM_PSEL_MOSI         0x50C
#define SPIM_PSEL_MISO         0x510
#define SPIM_PSEL_CS           0x514
#define SPIM_FREQUENCY         0x524
#define SPIM_TXD_PTR           0x544
#define SPIM_RXD_PTR           0x548
#define SPIM_TXD_MAXCNT        0x54C
#define SPIM_RXD_MAXCNT        0x550
#define SPIM_CONFIG            0x554
#define SPIM_ORC               0x5C0

#define SPIM_FREQ_8M   0x08000000
#define SPIM_FREQ_16M  0x10000000
#define SPIM_FREQ_24M 0x16000000
#define SPIM_FREQ_32M 0x20000000

#define SPIM_ENABLE_ON  1
#define SPIM_ENABLE_OFF 0
#define SPIM_CONFIG_CPHA 0x01
#define SPIM_CONFIG_CPOL 0x02

/* TWI / I²C */
#define TWIM_BASE(n)           (0x40003000UL + (n) * 0x1000)
#define TWIM_TASKS_STARTRX      0x014
#define TWIM_TASKS_STARTTX      0x018
#define TWIM_TASKS_STOP         0x01C
#define TWIM_EVENTS_RXDREADY    0x10C
#define TWIM_EVENTS_TXDSENT     0x124
#define TWIM_EVENTS_STOPPED     0x104
#define TWIM_ENABLE             0x500
#define TWIM_PSEL_SCL           0x508
#define TWIM_PSEL_SDA            0x50C
#define TWIM_FREQUENCY          0x524
#define TWIM_TXD_PTR            0x544
#define TWIM_RXD_PTR            0x548
#define TWIM_TXD_MAXCNT         0x54C
#define TWIM_RXD_MAXCNT         0x550
#define TWIM_ADDRESS            0x554

#define TWIM_FREQ_400K 0x01980000
#define TWIM_FREQ_100K 0x03000000

/* UART */
#define UARTE_BASE(n)          (0x40002000UL + (n) * 0x1000)

/* SAADC */
#define SAADC_BASE             0x4000E000UL
#define SAADC_TASKS_START       0x000
#define SAADC_TASKS_SAMPLE      0x004
#define SAADC_EVENTS_END        0x10C
#define SAADC_ENABLE            0x500
#define SAADC_CH_PSEL(n)        (0x510 + (n) * 0x40)
#define SAADC_CH_CONFIG(n)      (0x514 + (n) * 0x40)

/* CLOCK */
#define CLOCK_BASE              0x40000000UL
#define CLOCK_TASKS_HFCLKSTART  0x000
#define CLOCK_TASKS_LFCLKSTART  0x008
#define CLOCK_EVENTS_HFCLKSTARTED 0x100
#define CLOCK_EVENTS_LFCLKSTARTED 0x104
#define CLOCK_HFCLKSTARTED     0x100
#define CLOCK_LFCLKSRC          0x518

/* RTC (low power timer) */
#define RTC2_BASE               0x40024000UL
#define RTC2_TASKS_START        0x000
#define RTC2_TASKS_STOP         0x008
#define RTC2_EVENTS_TICK        0x100
#define RTC2_PRESCALER          0x508
#define RTC2_CC(n)              (0x540 + (n) * 4)

/* WDT (watchdog) */
#define WDT_BASE                0x4000B000UL
#define WDT_TASKS_START          0x000
#define WDT_CONFIG               0x504
#define WDT_TIMEOUT              0x508
#define WDT_RREN                 0x50C

/* FICR (Factory Information Configuration Registers) */
#define FICR_BASE                0x00FF0000UL
#define FICR_DEVICEID0           0x024
#define FICR_DEVICEID1           0x028

/* CryptoCell-310 */
#define CRYPTOCELL_BASE          0x40080000UL

/* ============================================================
 * ICM-42688-P IMU Register Map
 * ============================================================ */

#define IMU_REG_WHO_AM_I         0x75
#define IMU_WHO_AM_I_VAL         0x47
#define IMU_REG_DEVICE_CONFIG    0x11
#define IMU_REG_DRIVE_CONFIG     0x13
#define IMU_REG_INT_CONFIG       0x14
#define IMU_REG_FIFO_CONFIG      0x16
#define IMU_REG_TEMP_DATA1       0x49
#define IMU_REG_ACCEL_DATA_X1    0x1F
#define IMU_REG_ACCEL_DATA_X2    0x20
#define IMU_REG_ACCEL_DATA_Y1    0x21
#define IMU_REG_ACCEL_DATA_Y2    0x22
#define IMU_REG_ACCEL_DATA_Z1    0x23
#define IMU_REG_ACCEL_DATA_Z2    0x24
#define IMU_REG_GYRO_DATA_X1     0x25
#define IMU_REG_GYRO_DATA_X2     0x26
#define IMU_REG_GYRO_DATA_Y1     0x27
#define IMU_REG_GYRO_DATA_Y2     0x28
#define IMU_REG_GYRO_DATA_Z1     0x29
#define IMU_REG_GYRO_DATA_Z2     0x2A
#define IMU_REG_INT_STATUS       0x2D
#define IMU_REG_FIFO_COUNTH      0x2E
#define IMU_REG_FIFO_COUNTL      0x2F
#define IMU_REG_FIFO_DATA        0x30
#define IMU_REG_SIGNAL_PATH_RESET 0x4B
#define IMU_REG_INT_SOURCE0     0x4F
#define IMU_REG_INT_SOURCE1     0x50
#define IMU_REG_ACCEL_CONFIG0   0x50  /* (alternate: 0x53 in some datasheets) */
#define IMU_REG_ACCEL_CONFIG1   0x53
#define IMU_REG_GYRO_CONFIG0    0x54
#define IMU_REG_GYRO_CONFIG1    0x55
#define IMU_REG_ACCEL_ACCEL_CONFIG0 0x50
#define IMU_REG_PWR_MGMT0       0x4E
#define IMU_REG_GYRO_ACCEL_CONFIG0 0x52
#define IMU_REG_INT_CONFIG0     0x63
#define IMU_REG_INT_CONFIG1     0x64
#define IMU_REG_INT_SOURCE6     0x6D
#define IMU_REG_INT_SOURCE7     0x6E
#define IMU_REG_FIFO_CONFIG1     0x4F
#define IMU_REG_TMST_CONFIG     0x4A

/* WHO_AM_I */
#define IMU_WHOAMI_ICM42688P     0x47

/* PWR_MGMT0 bits */
#define IMU_PWR_LOW_NOISE        0x03  /* Low-noise mode */
#define IMU_PWR_SLEEP            0x00

/* ACCEL_CONFIG0: ODR + FS (Full Scale) */
#define IMU_ACCEL_ODR_1K         (0x07 << 0)  /* 1 kHz ODR */
#define IMU_ACCEL_FS_16G         (0x00 << 5)  /* ±16g */
#define IMU_ACCEL_FS_8G          (0x01 << 5)  /* ±8g */
#define IMU_ACCEL_FS_4G          (0x02 << 5)  /* ±4g */
#define IMU_ACCEL_FS_2G          (0x03 << 5)  /* ±2g */
#define IMU_ACCEL_UI_FILTER_DIV  (0x00 << 7)  /* 1 (no division) */
#define IMU_ACCEL_UI_FILTER_BW   0x00         /* Bandwidth: default */

/* GYRO_CONFIG0: ODR + FS */
#define IMU_GYRO_ODR_1K          (0x07 << 0)
#define IMU_GYRO_FS_2000DPS      (0x00 << 5)
#define IMU_GYRO_FS_1000DPS      (0x01 << 5)
#define IMU_GYRO_FS_500DPS       (0x02 << 5)

/* FIFO_CONFIG: stream mode + watermark */
#define IMU_FIFO_MODE_STREAM     (1 << 0)
#define IMU_FIFO_FIFO_BYPASS     (0 << 0)
#define IMU_FIFO_WM_BITMASK      0x3F  /* 6-bit watermark */

/* INT_CONFIG0: interrupt polarity + push-pull */
#define IMU_INT_PUSH_PULL        (0 << 2)
#define IMU_INT_ACTIVE_LOW       (0 << 1)
#define IMU_INT_ACTIVE_HIGH      (1 << 1)
#define IMU_INT_LATCHED          (0 << 0)
#define IMU_INT_PULSE            (1 << 0)

/* INT_SOURCE0: enable FIFO watermark interrupt */
#define IMU_INT_FIFO_FULL_EN     (1 << 5)
#define IMU_INT_FIFO_WM_EN       (1 << 2)
#define IMU_INT_FIFO_WM_INT1     (1 << 6)

/* Sensitivity (LSB per unit) */
#define IMU_ACC_SENS_16G_LSB     2048.0f   /* LSB/g for ±16g */
#define IMU_GYRO_SENS_2000_LSB   16.4f     /* LSB/dps for ±2000 dps */
#define IMU_G_TO_MS2             9.80665f

/* ============================================================
 * MX25R6435F SPI Flash Register Map
 * ============================================================ */

#define FLASH_CMD_WREN           0x06   /* Write enable */
#define FLASH_CMD_WRDI           0x04   /* Write disable */
#define FLASH_CMD_RDSR           0x05   /* Read status register */
#define FLASH_CMD_RDCR           0x15   /* Read config register */
#define FLASH_CMD_READ           0x03   /* Read data (low power) */
#define FLASH_CMD_FAST_READ      0x0B   /* Fast read */
#define FLASH_CMD_PP             0x02   /* Page program (256 bytes) */
#define FLASH_CMD_SE              0x20   /* Sector erase (4 KB) */
#define FLASH_CMD_BE32            0x52   /* Block erase (32 KB) */
#define FLASH_CMD_BE64            0xD8   /* Block erase (64 KB) */
#define FLASH_CMD_CE               0xC7  /* Chip erase */
#define FLASH_CMD_RDID            0x9F   /* Read JEDEC ID */
#define FLASH_CMD_PD              0xB9   /* Power down */
#define FLASH_CMD_PU              0xAB   /* Power up / release PD */

#define FLASH_MX25R6435F_JEDEC    0xC2253A  /* Manufacturer=C2h, Type=25h, Cap=3Ah(8MB) */
#define FLASH_PAGE_SIZE           256
#define FLASH_SECTOR_SIZE         4096
#define FLASH_BLOCK_SIZE          65536
#define FLASH_TOTAL_SIZE          0x800000UL  /* 8 MB */

/* Status register bits */
#define FLASH_SR_WIP              0x01   /* Write in progress */
#define FLASH_SR_WEL              0x02   /* Write enable latch */
#define FLASH_SR_BP0              0x04
#define FLASH_SR_BP1              0x08
#define FLASH_SR_BP2              0x10
#define FLASH_SR_BP3              0x20
#define FLASH_SR_QE                0x40
#define FLASH_SR_SRWD              0x80

/* ============================================================
 * MAX17048 Fuel Gauge Register Map
 * ============================================================ */

#define FG_REG_VCELL             0x02   /* Battery voltage (78.125 µV/cell) */
#define FG_REG_SOC                0x04   /* State of charge (% / 256) */
#define FG_REG_MODE               0x06
#define FG_REG_VERSION             0x08
#define FG_REG_HIBRT               0x0A
#define FG_REG_CONFIG             0x0C
#define FG_REG_VALRT               0x14
#define FG_REG_CRATE              0x16   /* Charge/discharge rate (%/hr / 256) */
#define FG_REG_VRESET              0x18
#define FG_REG_STATUS              0x1A
#define FG_REG_TABLE               0x42
#define FG_REG_CMD                 0xFE

#define FG_VCELL_LSB_MV           1.25f   /* 78.125 µV × 16 = 1.25 mV */
#define FG_SOC_LSB_PCT            0.00390625f  /* 1/256 % */

/* Status bits */
#define FG_STATUS_RI              0x0001
#define FG_STATUS_VH               0x0100
#define FG_STATUS_VR               0x8000

/* ============================================================
 * SHT40 Temperature/Humidity Sensor
 * ============================================================ */

#define SHT40_CMD_SOFTRESET       0x94
#define SHT40_CMD_MEAS_HIRES      0xFD   /* Measure T & RH, high repeatability */
#define SHT40_CMD_MEAS_MEDRES      0xF6
#define SHT40_CMD_MEAS_LOWRES      0xE0
#define SHT40_CMD_HEATER_200MS_1V 0x39   /* Heater 200ms, 1.0V */
#define SHT40_CMD_READ_SN          0x89

#define SHT40_MEAS_DELAY_MS       8      /* High repeatability measurement time */

/* Conversion constants */
#define SHT40_T_OFFSET            -45.0f
#define SHT40_T_COEFF              175.0f
#define SHT40_RH_COEFF             125.0f
#define SHT40_RH_OFFSET            -6.0f

/* ============================================================
 * LPS22HB Barometric Pressure Sensor
 * ============================================================ */

#define LPS22_REG_INT_CFG          0x0B
#define LPS22_REG_THS_P_L          0x0C
#define LPS22_REG_THS_P_H          0x0D
#define LPS22_REG_WHO_AM_I         0x0F
#define LPS22_REG_CTRL_REG1        0x10
#define LPS22_REG_CTRL_REG2        0x11
#define LPS22_REG_CTRL_REG3        0x12
#define LPS22_REG_FIFO_CTRL        0x14
#define LPS22_REG_REF_P_XL         0x15
#define LPS22_REG_REF_P_L          0x16
#define LPS22_REG_REF_P_H          0x17
#define LPS22_REG_RPDS_L           0x18
#define LPS22_REG_RPDS_H           0x19
#define LPS22_REG_RES_CONF          0x1A
#define LPS22_REG_PRESS_OUT_XL     0x28
#define LPS22_REG_PRESS_OUT_L      0x29
#define LPS22_REG_PRESS_OUT_H      0x2A
#define LPS22_REG_TEMP_OUT_L       0x2B
#define LPS22_REG_TEMP_OUT_H       0x2C
#define LPS22_REG_FIFO_STATUS      0x26

#define LPS22_WHO_AM_I_VAL          0xB1
#define LPS22_CTRL1_ODR_10HZ       (0x20)
#define LPS22_CTRL1_EN_LPFP        (0x08)
#define LPS22_CTRL1_BDU            (0x02)
#define LPS22_CTRL1_PD_EN          (0x04)  /* Power-down → active */

/* ============================================================
 * GC9A01 OLED Display Controller
 * ============================================================ */

#define GC9A01_CMD_NOP             0x00
#define GC9A01_CMD_SWRESET          0x01
#define GC9A01_CMD_RDDID            0x04
#define GC9A01_CMD_SLPIN           0x10
#define GC9A01_CMD_SLPOUT          0x11
#define GC9A01_CMD_PTLON           0x12
#define GC9A01_CMD_NORON           0x13
#define GC9A01_CMD_INVOFF          0x20
#define GC9A01_CMD_INVON           0x21
#define GC9A01_CMD_GAMSET          0x26
#define GC9A01_CMD_DISPOFF         0x28
#define GC9A01_CMD_DISPON          0x29
#define GC9A01_CMD_CASET           0x2A
#define GC9A01_CMD_RASET           0x2B
#define GC9A01_CMD_RAMWR           0x2C
#define GC9A01_CMD_RAMRD           0x2E
#define GC9A01_CMD_PTLAR           0x30
#define GC9A01_CMD_VSCRDEF         0x33
#define GC9A01_CMD_MADCTL          0x36
#define GC9A01_CMD_VSCRSADD        0x37
#define GC9A01_CMD_PIXFMT          0x3A
#define GC9A01_CMD_FRMCTR1         0xB1
#define GC9A01_CMD_FRMCTR2         0xB2
#define GC9A01_CMD_FRMCTR3         0xB3
#define GC9A01_CMD_DDBCTR1         0xB4
#define GC9A01_CMD_DFUNC           0xB6
#define GC9A01_CMD_PWCTR1          0xC0
#define GC9A01_CMD_PWCTR2          0xC1
#define GC9A01_CMD_PWCTR3          0xC2
#define GC9A01_CMD_PWCTR4          0xC3
#define GC9A01_CMD_PWCTR5          0xC4
#define GC9A01_CMD_VMCTR1          0xC5
#define GC9A01_CMD_GAMCTRP1        0xE0
#define GC9A01_CMD_GAMCTRN1        0xE1

/* MADCTL bits */
#define GC9A01_MADCTL_MY           0x80
#define GC9A01_MADCTL_MX           0x40
#define GC9A01_MADCTL_MV           0x20
#define GC9A01_MADCTL_ML           0x10
#define GC9A01_MADCTL_BGR          0x08
#define GC9A01_MADCTL_MH           0x04
#define GC9A01_MADCTL_RGB          0x00

/* Pixel format */
#define GC9A01_PIXFMT_16BIT         0x55   /* 16-bit RGB565 */
#define GC9A01_PIXFMT_18BIT         0x66

/* Color macros (RGB565) */
#define RGB565(r,g,b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
#define COLOR_BLACK    RGB565(0, 0, 0)
#define COLOR_WHITE    RGB565(255, 255, 255)
#define COLOR_RED      RGB565(255, 0, 0)
#define COLOR_GREEN    RGB565(0, 255, 0)
#define COLOR_BLUE     RGB565(0, 0, 255)
#define COLOR_YELLOW   RGB565(255, 255, 0)
#define COLOR_CYAN     RGB565(0, 255, 255)
#define COLOR_MAGENTA  RGB565(255, 0, 255)
#define COLOR_GRAY     RGB565(128, 128, 128)
#define COLOR_DARKBLUE RGB565(10, 20, 40)

/* ============================================================
 * CRC-CCITT (for episode record integrity)
 * ============================================================ */

#define CRC16_POLY_CCITT  0x1021
#define CRC16_INIT_VAL     0xFFFF

/* ============================================================
 * BLE UUID base (full 128-bit, abbreviated in board.h)
 * ============================================================ */

#define BLE_UUID_BASE_LSB 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
                           0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E

#endif /* TREMORSENSE_REGISTERS_H */