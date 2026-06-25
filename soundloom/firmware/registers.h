/*
 * registers.h — ADS131M08 register definitions and peripheral constants
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * SoundLoom acoustic acquisition front-end register map.
 * Covers the TI ADS131M08 8-channel simultaneous-sampling
 * 24-bit delta-sigma ADC, daisy-chained 4-deep.
 */

#ifndef SOUNDLOOM_REGISTERS_H
#define SOUNDLOOM_REGISTERS_H

#include "board.h"

/* ---- ADS131M08 command set ---- */
#define ADS_CMD_RESET      0x0011u
#define ADS_CMD_STANDBY    0x0022u
#define ADS_CMD_WAKEUP     0x0033u
#define ADS_CMD_LOCK       0x0555u
#define ADS_CMD_UNLOCK     0x0655u
#define ADS_CMD_RREG       0x8000u   /* RREG(reg, count): 0x80|reg<<8|count-1 */
#define ADS_CMD_WREG       0x4000u   /* WREG(reg, count): 0x40|reg<<8|count-1 */
#define ADS_CMD_NULLCMD    0x0000u

/* ---- ADS131M08 register addresses ---- */
#define ADS_REG_ID         0x00u   /* Device ID */
#define ADS_REG_STATUS     0x01u   /* Status */
#define ADS_REG_INTR_MASK  0x02u   /* Interrupt mask */
#define ADS_REG_CONFIG     0x03u   /* Default config */
#define ADS_REG_CLOCK      0x04u   /* Clock */
#define ADS_REG_THRESHOLD  0x05u   /* Threshold for DRL */
#define ADS_REG_OSR        0x06u   /* Oversampling ratio */
#define ADS_REG_DCGAIN     0x07u   /* Digital gain */
#define ADS_REG_CH0SET     0x08u   /* Channel 0 setup */
#define ADS_REG_CH1SET     0x09u
#define ADS_REG_CH2SET     0x0Au
#define ADS_REG_CH3SET     0x0Bu
#define ADS_REG_CH4SET     0x0Cu
#define ADS_REG_CH5SET     0x0D
#define ADS_REG_CH6SET     0x0Eu
#define ADS_REG_CH7SET     0x0Fu
#define ADS_REG_CRC        0x10u

/* STATUS bit fields */
#define ADS_STATUS_DRDY    (1u << 15)
#define ADS_STATUS_RST     (1u << 14)
#define ADS_STATUS_SYNC    (1u << 13)
#define ADS_STATUS_CRC_ERR (1u << 12)
#define ADS_STATUS_WDT_ERR (1u << 11)
#define ADS_STATUS_ASYM_ERR (1u << 10)

/* CLOCK bit fields */
#define ADS_CLOCK_PWR_HD   (1u << 2)
#define ADS_CLOCK_PWR_REF  (1u << 1)
#define ADS_CLOCK_PWR_ACV  (1u << 0)

/* OSR settings */
#define ADS_OSR_2048       0x0000u  /* default: 8 kSPS */
#define ADS_OSR_4096       0x0001u
#define ADS_OSR_8192       0x0002u
#define ADS_OSR_16384      0x0003u

/* CHnSET fields: input mux, PGA gain */
#define ADS_CHSET_MUX_SHUNT 0x00u
#define ADS_CHSET_MUX_INPUT 0x01u
#define ADS_CHSET_MUX_SHORT 0x02u
#define ADS_CHSET_MUX_RBIAS 0x03u

#define ADS_GAIN_1         0x00u
#define ADS_GAIN_2         0x01u
#define ADS_GAIN_4         0x02u
#define ADS_GAIN_8         0x03u
#define ADS_GAIN_16        0x04u
#define ADS_GAIN_32        0x05u
#define ADS_GAIN_64        0x06u
#define ADS_GAIN_128       0x07u

/* SPI frame size: 3 bytes status + 8 ch × 3 bytes + 3 bytes CRC = 30 bytes per device */
#define ADS_FRAME_BYTES    30u
/* 4 devices daisy-chained */
#define ADS_TOTAL_FRAME    (ADS_FRAME_BYTES * ADC_NUM_DEVICES)  /* 120 bytes */

/* ---- SX1262 register addresses (subset) ---- */
#define SX1262_REG_WHITENING   0x06B1u
#define SX1262_REG_PKT_PARAMS   0x0704u
#define SX1262_REG_SYNC_ADDR    0x0740u
#define SX1262_REG_IRQ_MASK     0x0817u
#define SX1262_REG_LNA_REG      0x0899u

#define SX1262_CMD_SET_SLEEP    0x84u
#define SX1262_CMD_SET_STDBY    0x80u
#define SX1262_CMD_SET_RX       0x82u
#define SX1262_CMD_SET_TX       0x83u
#define SX1262_CMD_WRITE_REG    0x0Du
#define SX1262_CMD_READ_REG     0x1Du
#define SX1262_CMD_WRITE_BUF    0x0Eu
#define SX1262_CMD_READ_BUF     0x1Eu
#define SX1262_CMD_SET_DIO_IRQ  0x08u
#define SX1262_CMD_CLEAR_IRQ    0x02u
#define SX1262_CMD_SET_MOD_PARAMS 0x8Bu
#define SX1262_CMD_SET_PACKET_TYPE 0x8Au
#define SX1262_CMD_SET_RF_FREQ  0x86u
#define SX1262_CMD_SET_TX_PARAMS 0x8Eu
#define SX1262_CMD_CALIBRATE    0x89u

#define SX1262_PKT_TYPE_LORA    0x01u
#define SX1262_IRQ_TX_DONE      (1u << 0)
#define SX1262_IRQ_RX_DONE      (1u << 1)
#define SX1262_IRQ_PREAMBLE_DET (1u << 2)
#define SX1262_IRQ_SYNC_DET     (1u << 3)
#define SX1262_IRQ_HEADER_DET   (1u << 4)
#define SX1262_IRQ_CRC_ERR      (1u << 5)
#define SX1262_IRQ_CAD_DONE     (1u << 6)
#define SX1262_IRQ_CAD_DET      (1u << 7)
#define SX1262_IRQ_TIMEOUT      (1u << 8)

/* ---- LSM6DSO32 register addresses ---- */
#define LSM_WHO_AM_I        0x0Fu
#define LSM_CTRL1_XL        0x10u
#define LSM_CTRL2_G         0x11u
#define LSM_CTRL3_C         0x12u
#define LSM_STATUS_REG      0x1Eu
#define LSM_OUTX_L_XL       0x28u
#define LSM_OUTY_L_XL       0x2Au
#define LSM_OUTZ_L_XL       0x2Cu
#define LSM_WHO_AM_I_VAL    0x6Cu

/* ---- GMP343 CO2 commands ---- */
#define CO2_CMD_START_MEAS  0x10u
#define CO2_CMD_READ_RESULT 0x11u
#define CO2_CMD_SET_FILTER  0x20u

/* ---- SHT45 commands ---- */
#define SHT45_CMD_MEAS_HIGHREP  0xFDu

/* ---- Teros-12 SDI-12 commands ---- */
#define SDI12_CMD_INFO     'I'
#define SDI12_CMD_MEASURE  'M'
#define SDI12_CMD_READ     'D'
#define SDI12_CMD_INFO_EXT 'I!'

/* ---- Flash / OTA model region ---- */
#define MODEL_FLASH_ADDR   0x90000000u  /* QSPI bank 1 */
#define MODEL_SIZE_MAX      (96u * 1024u)  /* 96 KB int8 weights */
#define MODEL_MAGIC        0x534C4D31u  /* "SLM1" */

#endif /* SOUNDLOOM_REGISTERS_H */