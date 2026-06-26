/*
 * registers.h — Register maps for ADS1256, ICM-42688, BQ25895, SX1262
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

/* =========================================================================
 * TI ADS1256 — 24-bit, 8-channel, delta-sigma ADC
 * ========================================================================= */
#define ADS1256_CMD_WAKEUP      0x00
#define ADS1256_CMD_RDATA       0x01
#define ADS1256_CMD_RDATAC      0x03
#define ADS1256_CMD_SDATAC      0x0F
#define ADS1256_CMD_RREG        0x1A  /* + offset */
#define ADS1256_CMD_WREG        0x2A  /* + offset */
#define ADS1256_CMD_SELFCAL     0xF0
#define ADS1256_CMD_SYNC        0xFC
#define ADS1256_CMD_RESET       0xFE

/* Register addresses */
#define ADS1256_REG_STATUS      0x00
#define ADS1256_REG_MUX         0x01
#define ADS1256_REG_ADCON       0x02
#define ADS1256_REG_DRATE       0x03
#define ADS1256_REG_IO          0x04
#define ADS1256_REG_OFC0        0x05
#define ADS1256_REG_OFC1        0x06
#define ADS1256_REG_OFC2        0x07
#define ADS1256_REG_FSC0        0x08
#define ADS1256_REG_FSC1        0x09
#define ADS1256_REG_FSC2        0x0A

/* STATUS bits */
#define ADS1256_STATUS_ORDER    (1 << 3)
#define ADS1256_STATUS_ACAL     (1 << 2)
#define ADS1256_STATUS_BUFEN    (1 << 1)
#define ADS1256_STATUS_DRDY     (1 << 0)

/* ADCON: clock out rate + sensor detect + PGA gain */
#define ADS1256_ADCON_PGA_1     0x00
#define ADS1256_ADCON_PGA_2     0x01
#define ADS1256_ADCON_PGA_4     0x02
#define ADS1256_ADCON_PGA_8     0x03
#define ADS1256_ADCON_PGA_16    0x04
#define ADS1256_ADCON_PGA_32    0x05
#define ADS1256_ADCON_PGA_64    0x06
#define ADS1256_ADCON_COC_0     (0 << 5)
#define ADS1256_ADCON_COC_384   (1 << 5)
#define ADS1256_ADCON_SDC_OFF   0
#define ADS1256_ADCON_SDC_ON    (1 << 2)

/* DRATE codes (subset) */
#define ADS1256_DRATE_30000SPS  0xF0
#define ADS1256_DRATE_15000SPS  0xE0
#define ADS1256_DRATE_7500SPS   0xC0
#define ADS1256_DRATE_1000SPS   0xA0
#define ADS1256_DRATE_500SPS    0x92
#define ADS1256_DRATE_100SPS    0x72
#define ADS1256_DRATE_60SPS     0x64

/* MUX channel selects (AINn = MUX[3:0], AINp = MUX[7:4]) */
#define ADS1256_MUX_AIN0_AINCOM   0x08
#define ADS1256_MUX_AIN1_AINCOM   0x18
#define ADS1256_MUX_AIN2_AINCOM   0x28
#define ADS1256_MUX_AIN3_AINCOM   0x38
#define ADS1256_MUX_AIN4_AINCOM   0x48
#define ADS1256_MUX_AIN5_AINCOM   0x58
#define ADS1256_MUX_AIN6_AINCOM   0x68
#define ADS1256_MUX_AIN7_AINCOM   0x78

/* Conversion result: 24-bit signed, MSB first, scaled to ±Vref/PGA */

/* =========================================================================
 * TDK ICM-42688 — 6-axis IMU, SPI
 * ========================================================================= */
#define ICM42688_REG_WHO_AM_I       0x75
#define ICM42688_VAL_WHO_AM_I       0x47
#define ICM42688_REG_DEVICE_CONFIG  0x11
#define ICM42688_REG_INT_CONFIG     0x14
#define ICM42688_REG_INT_CONFIG0    0x63
#define ICM42688_REG_INT_SOURCE0    0x65
#define ICM42688_REG_INT_SOURCE1    0x66
#define ICM42688_REG_PWR_MGMT0      0x4E
#define ICM42688_REG_GYRO_CONFIG0   0x4F
#define ICM42688_REG_ACCEL_CONFIG0  0x50
#define ICM42688_REG_GYRO_CONFIG1   0x51
#define ICM42688_REG_ACCEL_CONFIG1  0x52
#define ICM42688_REG_FIFO_CONFIG    0x54
#define ICM42688_REG_FIFO_COUNTH    0x46
#define ICM42688_REG_FIFO_COUNTL    0x47
#define ICM42688_REG_FIFO_DATA      0x48
#define ICM42688_REG_TEMP_DATA1     0x49
#define ICM42688_REG_TEMP_DATA0     0x4A
#define ICM42688_REG_INT_STATUS     0x2D
#define ICM42688_REG_ACCEL_DATA_X1  0x1F
#define ICM42688_REG_ACCEL_DATA_X0  0x20
#define ICM42688_REG_ACCEL_DATA_Y1  0x21
#define ICM42688_REG_ACCEL_DATA_Y0  0x22
#define ICM42688_REG_ACCEL_DATA_Z1  0x23
#define ICM42688_REG_ACCEL_DATA_Z0  0x24

/* PWR_MGMT0 */
#define ICM42688_PWR_ACCEL_LP       0x03
#define ICM42688_PWR_ACCEL_OFF      0x00
#define ICM42688_PWR_GYRO_OFF       0x00
#define ICM42688_PWR_TEMP_OFF       (1 << 5)
#define ICM42688_PWR_IDLE_BIT       (1 << 4)

/* ACCEL_CONFIG0: ODR + FS */
#define ICM42688_ACCEL_ODR_1KHZ     0x09
#define ICM42688_ACCEL_FS_2G        (0x00 << 5)
#define ICM42688_ACCEL_FS_4G        (0x01 << 5)
#define ICM42688_ACCEL_FS_8G        (0x02 << 5)
#define ICM42688_ACCEL_FS_16G       (0x03 << 5)

/* SPI: read = 0x80 | addr, write = addr */
#define ICM42688_SPI_READ(addr)   ((addr) | 0x80)
#define ICM42688_SPI_WRITE(addr)  ((addr) & 0x7F)

/* =========================================================================
 * TI BQ25895 — USB/solar buck-boost charger
 * ========================================================================= */
#define BQ25895_REG_INPUT_SRC     0x00
#define BQ25895_REG_CTRL0         0x01
#define BQ25895_REG_CTRL1         0x02
#define BQ25895_REG_ICHG          0x04
#define BQ25895_REG_PRECHG        0x05
#define BQ25895_REG_TERM          0x06
#define BQ25895_REG_VREG          0x06
#define BQ25895_REG_CHG_CTRL1     0x07
#define BQ25895_REG_CHG_CTRL2     0x08
#define BQ25895_REG_BAT_COMP      0x09
#define BQ25895_REG_SYS_CTRL      0x0A
#define BQ25895_REG_FAULT         0x0B
#define BQ25895_REG_STATUS        0x0C
#define BQ25895_REG_FG_CTRL       0x0D
#define BQ25895_REG_VBUS          0x0E
#define BQ25895_REG_VSYS          0x0F
#define BQ25895_REG_TS            0x10

/* INPUT_SRC: EN_HIZ, VINDPM (3.5-15.2V), IINLIM (100-3250mA) */
#define BQ25895_VINDPM_4500MV     0x0C  /* 4.5 V input regulation */
#define BQ25895_IINLIM_2000MA     0x40  /* 2 A input current limit */

/* ICHG: 0-5056mA in 64 mA steps (bits 0-6) */
#define BQ25895_ICHG_2000MA       (0x20) /* ~2 A charge current */

/* VREG: 3.856-4.624 V in 16 mV steps, offset 0x16 */
#define BQ25895_VREG_LIFEPO4_3600 (0x0E) /* 3.6 V float for LiFePO4 */

/* STATUS bits */
#define BQ25895_STAT_CHARGE_DONE  (0x03 << 4)
#define BQ25895_STAT_CHARGING     (0x01 << 4)
#define BQ25895_STAT_NOT_CHARGING (0x00 << 4)
#define BQ25895_PG_STAT           (1 << 2)
#define BQ25895_VBUS_STAT         (0x07 << 5)

/* =========================================================================
 * Semtech SX1262 (internal to STM32WL55) — LoRa radio register subset
 * ========================================================================= */
#define SX1262_CMD_SET_STANDBY        0x80
#define SX1262_CMD_SET_PACKET_TYPE    0x8A
#define SX1262_CMD_SET_RF_FREQ        0x86
#define SX1262_CMD_SET_TX_PARAMS      0x8E
#define SX1262_CMD_SET_MOD_PARAMS     0x8B
#define SX1262_CMD_SET_PACKET_PARAMS  0x8C
#define SX1262_CMD_SET_CAD_PARAMS     0x88
#define SX1262_CMD_SET_BUFFER_BASE    0x8F
#define SX1262_CMD_WRITE_BUFFER       0x0E
#define SX1262_CMD_READ_BUFFER        0x1E
#define SX1262_CMD_SET_TX             0x83
#define SX1262_CMD_SET_RX             0x82
#define SX1262_CMD_SET_DIO_IRQ        0x08
#define SX1262_CMD_GET_IRQ_STATUS     0x12
#define SX1262_CMD_CLEAR_IRQ          0x02
#define SX1262_CMD_GET_PACKET_STATUS  0x1D
#define SX1262_CMD_SET_TX_FALLBACK    0x93

#define SX1262_STDBY_RC               0x40
#define SX1262_STDBY_XOSC             0x41
#define SX1262_PKT_TYPE_LORA          0x01
#define SX1262_IRQ_TX_DONE            (1 << 0)
#define SX1262_IRQ_RX_DONE            (1 << 1)
#define SX1262_IRQ_PREAMBLE_DET       (1 << 2)
#define SX1262_IRQ_SYNCWORD_DET       (1 << 3)
#define SX1262_IRQ_HEADER_VALID       (1 << 4)
#define SX1262_IRQ_HEADER_ERR         (1 << 5)
#define SX1262_IRQ_CRC_ERR            (1 << 6)
#define SX1262_IRQ_CAD_DONE           (1 << 7)
#define SX1262_IRQ_CAD_DETECTED       (1 << 8)
#define SX1262_IRQ_TIMEOUT            (1 << 9)
#define SX1262_IRQ_ALL                0x03FF

/* LoRa modulation params: SF, BW, CR, LDRO */
#define SX1262_LORA_SF7               0x07
#define SX1262_LORA_SF9               0x09
#define SX1262_LORA_SF12              0x0C
#define SX1262_LORA_BW_125            0x04  /* 125 kHz */
#define SX1262_LORA_BW_250            0x05
#define SX1262_LORA_CR_4_5            0x01
#define SX1262_LORA_CR_4_8            0x04
#define SX1262_LORA_LDRO_ON           0x01
#define SX1262_LORA_LDRO_OFF          0x00

/* Packet params: preamble, header type, payload len, CRC, invertIQ */
#define SX1262_LORA_PREAMBLE_8        8
#define SX1262_LORA_HEADER_EXPLICIT   0x00
#define SX1262_LORA_HEADER_IMPLICIT   0x01
#define SX1262_LORA_CRC_ON            0x01
#define SX1262_LORA_CRC_OFF           0x00
#define SX1262_LORA_IQ_STD            0x00
#define SX1262_LORA_IQ_INVERTED       0x01

#endif /* REGISTERS_H */