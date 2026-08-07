/*
 * registers.h — Sensor & Peripheral Register Definitions
 *
 * Register maps for all sensors and peripherals on the FermenTiq board.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_REGISTERS_H
#define FERMENTIQ_REGISTERS_H

#include <stdint.h>

/* ========================================================================
 * AD5933 Impedance-to-Digital Converter (I2C addr 0x0D)
 * ======================================================================== */

/* Register addresses */
#define AD5933_REG_CTRL_H       0x80    /* Control byte high */
#define AD5933_REG_CTRL_L       0x81    /* Control byte low  */
#define AD5933_REG_START_FREQ_H 0x82    /* Start freq bits [27:24] */
#define AD5933_REG_START_FREQ_M 0x83    /* Start freq bits [23:16] */
#define AD5933_REG_START_FREQ_L 0x84    /* Start freq bits [15:8]  */
#define AD5933_REG_FREQ_INCR_H  0x85    /* Freq increment [27:24]  */
#define AD5933_REG_FREQ_INCR_M  0x86    /* Freq increment [23:16]  */
#define AD5933_REG_FREQ_INCR_L  0x87    /* Freq increment [15:8]   */
#define AD5933_REG_NUM_INCR_H   0x88    /* Num increments [8:1]    */
#define AD5933_REG_NUM_INCR_L   0x89    /* Num increments [0]      */
#define AD5933_REG_NUM_SETTLE_H 0x8A    /* Settling cycles [9:8]   */
#define AD5933_REG_NUM_SETTLE_L 0x8B    /* Settling cycles [7:0]   */
#define AD5933_REG_STATUS       0x8F    /* Status register         */
#define AD5933_REG_REAL_H       0x94    /* Real data [15:8]        */
#define AD5933_REG_REAL_L       0x95    /* Real data [7:0]         */
#define AD5933_REG_IMAG_H       0x96    /* Imag data [15:8]        */
#define AD5933_REG_IMAG_L       0x97    /* Imag data [7:0]         */
#define AD5933_REG_GAIN_H       0x95    /* Magnitude gain [15:8]   */
#define AD5933_REG_GAIN_L       0x96    /* Magnitude gain [7:0]    */
#define AD5933_REG_TEMP_H       0x92    /* Temperature [13:6]      */
#define AD5933_REG_TEMP_L       0x93    /* Temperature [5:0]       */

/* Control register high bits (D15-D12) */
#define AD5933_CTRL_MODE_STANDBY    0x30
#define AD5933_CTRL_MODE_INIT_FREQ  0x10
#define AD5933_CTRL_MODE_START_SWEEP 0x20
#define AD5933_CTRL_MODE_INC_FREQ   0x30
#define AD5933_CTRL_MODE_REPEAT     0x40
#define AD5933_CTRL_MODE_POWER_DOWN 0xA0
#define AD5933_CTRL_MODE_TEMP       0x90

/* Control register low bits (D11-D8) */
#define AD5933_CTRL_RANGE_1V        0x00  /* 1.0 Vpp output */
#define AD5933_CTRL_RANGE_4V        0x01  /* 0.4 Vpp (range 1) */
#define AD5933_CTRL_RANGE_2V        0x02  /* 0.2 Vpp (range 2) */
#define AD5933_CTRL_RANGE_400MV     0x03  /* 0.4 Vpp (range 3) */
#define AD5933_CTRL_PGA_1X          0x00  /* PGA gain x1 */
#define AD5933_CTRL_PGA_5X          0x01  /* PGA gain x5 */
#define AD5933_CTRL_RESET           0x10  /* Reset command */

/* Status register bits */
#define AD5933_STATUS_VALID_TEMP    0x01
#define AD5933_STATUS_VALID_DATA    0x02
#define AD5933_STATUS_SWEEP_DONE    0x04
#define AD5933_STATUS_ADDR_PTR      0x08  /* Address pointer D8 */
#define AD5933_STATUS_POR           0x10  /* Power-on reset */

/* AD5933 internal clock frequency */
#define AD5933_MCLK_HZ              16776000UL

/* ADG715 analog switch channel assignments for 4-wire Kelvin */
#define ADG715_CHAN_I_PLUS          0x01  /* CH1: I+ excitation   */
#define ADG715_CHAN_I_MINUS         0x02  /* CH2: I- excitation   */
#define ADG715_CHAN_V_PLUS          0x04  /* CH3: V+ sense        */
#define ADG715_CHAN_V_MINUS         0x08  /* CH4: V- sense        */
#define ADG715_CHAN_CALIBRATION     0x10  /* CH5: Calibration resistor */
#define ADG715_CHAN_ALL_OFF         0x00

/* ========================================================================
 * LMP91200 ISFET pH Front-End (I2C addr 0x09)
 * ======================================================================== */
#define LMP91200_REG_CONFIG     0x00
#define LMP91200_REG_STATUS     0x01
#define LMP91200_REG_CALIB      0x02

/* Config register bits */
#define LMP91200_CFG_PH_MODE    0x01    /* pH measurement mode  */
#define LMP91200_CFG_TEMP_EN    0x02    /* Temperature compensation enable */
#define LMP91200_CFG_VBIAS_EN   0x04    /* Voltage bias enable  */
#define LMP91200_CFG_GND_REF    0x08    /* Ground reference     */
#define LMP91200_CFG_HIGH_Z     0x10    /* High-impedance buffer */
#define LMP91200_CFG_CAL_4_00   0x20    /* pH 4.00 calibration  */
#define LMP91200_CFG_CAL_7_00   0x40    /* pH 7.00 calibration  */

/* ========================================================================
 * Senseair S8 (LP8) NDIR CO2 Sensor (UART 9600 8N1)
 * ======================================================================== */

/* S8 uses a simple request-response protocol over UART */
#define S8_FRAME_START         0xFE    /* Start byte */
#define S8_ACK                 0x79    /* Ack byte   */

/* Commands */
#define S8_CMD_CO2_READ        0x8404  /* Read CO2 concentration */
#define S8_CMD_SERIAL_READ     0xB0D0  /* Read serial number     */
#define S8_CMD_VERSION_READ    0xB0F0  /* Read firmware version  */
#define S8_CMD_ABC_PERIOD      0xB0E0  /* Read ABC period        */
#define S8_CMD_CALIBRATION     0x7306  /* Manual calibration     */

#define S8_CO2_MIN             400     /* Minimum CO2 reading (ppm) */
#define S8_CO2_MAX             10000   /* Maximum CO2 reading       */
#define S8_RESPONSE_TIMEOUT_MS 1000

/* ========================================================================
 * MAX31865 RTD-to-Digital Converter (SPI)
 * ======================================================================== */
#define MAX31865_REG_CONFIG    0x00
#define MAX31865_REG_RTD_MSB   0x01
#define MAX31865_REG_RTD_LSB   0x02
#define MAX31865_REG_HFT_MSB   0x03
#define MAX31865_REG_HFT_LSB   0x04
#define MAX31865_REG_LFT_MSB   0x05
#define MAX31865_REG_LFT_LSB   0x06
#define MAX31865_REG_FAULT     0x07
#define MAX31865_REG_HFT_CLEAR 0x03
#define MAX31865_REG_LFT_CLEAR 0x05

/* Config register bits */
#define MAX31865_CFG_VBIAS     0x80    /* Bias voltage enable   */
#define MAX31865_CFG_CONV_AUTO 0x40    /* Auto conversion mode  */
#define MAX31865_CFG_CONV_1SHOT 0x20   /* 1-shot conversion     */
#define MAX31865_CFG_CONV_FAULT 0x0C   /* Fault detection cycle */
#define MAX31865_CFG_CONV_FAULT_CLR 0x02 /* Fault status clear  */
#define MAX31865_CFG_50HZ      0x01    /* 50 Hz filter (vs 60Hz) */
#define MAX31865_CFG_60HZ      0x00    /* 60 Hz filter          */

/* RTD parameters (PT100) */
#define RTD_RREF_OHMS          430.0f  /* Reference resistor    */
#define RTD_RNOM_OHMS          100.0f  /* Nominal resistance 0°C */
#define RTD_ALPHA              0.003851f /* PT100 alpha coefficient */

/* ========================================================================
 * SHT41 Temperature & Humidity Sensor (I2C addr 0x44)
 * ======================================================================== */
#define SHT41_CMD_MEAS_HIGHREP  0xFD    /* High-repeatability, clock-stretch */
#define SHT41_CMD_MEAS_MEDREP   0xF6    /* Medium repeatability              */
#define SHT41_CMD_MEAS_LOWREP   0xE0    /* Low repeatability                 */
#define SHT41_CMD_SOFT_RESET    0x94
#define SHT41_CMD_READ_SERIAL   0x89
#define SHT41_MEAS_TIMEOUT_MS   10

/* ========================================================================
 * MAX17048 Fuel Gauge (I2C addr 0x36)
 * ======================================================================== */
#define MAX17048_REG_VCELL      0x02
#define MAX17048_REG_SOC        0x04
#define MAX17048_REG_MODE       0x06
#define MAX17048_REG_VERSION    0x08
#define MAX17048_REG_HIBRT      0x0A
#define MAX17048_REG_CONFIG     0x0C
#define MAX17048_REG_VALERT     0x14
#define MAX17048_REG_CRATE      0x16
#define MAX17048_REG_VRESET     0x18
#define MAX17048_REG_STATUS     0x1A
#define MAX17048_REG_CMD        0xFE    /* Quick-start command */

#define MAX17048_STATUS_POR     0x0010  /* Power-on reset flag */

/* ========================================================================
 * TP4056 Charger Status (GPIO)
 * ======================================================================== */
#define TP4056_STAT_CHARGING    0       /* GPIO LOW = charging  */
#define TP4056_STAT_FULL        1       /* GPIO HIGH = charged  */
#define TP4056_STAT_STANDBY     1       /* GPIO HIGH = no battery / standby */

#endif /* FERMENTIQ_REGISTERS_H */