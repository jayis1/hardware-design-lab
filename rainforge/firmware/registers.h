/*
 * registers.h — Register definitions for RainForge external ICs
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Covers the ADS131M04 ADC, CY15B104Q FRAM, SX1262 LoRa radio,
 * SHT45, BMP390 and TSL2591. The ESP32-C3's own register set is
 * provided by the ESP-IDF headers; here we define only the
 * external-device registers used by the drivers.
 */
#ifndef RAINFORGE_REGISTERS_H
#define RAINFORGE_REGISTERS_H

#include <stdint.h>

/* ================================================================
 * ADS131M04 — 4-channel 24-bit simultaneous delta-sigma ADC
 * ================================================================ */

/* Command words (16-bit, MSB first) */
#define ADS131_CMD_RESET        0x0011
#define ADS131_CMD_STANDBY      0x0022
#define ADS131_CMD_WAKEUP       0x0033
#define ADS131_CMD_LOCK         0x0555
#define ADS131_CMD_UNLOCK       0x0655
#define ADS131_CMD_RREG         0xA000  /* + 8-bit reg addr */
#define ADS131_CMD_WREG         0x4000  /* + 8-bit reg addr */
#define ADS131_CMD_RREGC        0x2000  /* read continuous */

/* Register addresses */
#define ADS131_REG_ID           0x00
#define ADS131_REG_STATUS       0x01
#define ADS131_REG_MODE        0x04
#define ADS131_REG_CLOCK        0x05
#define ADS131_REG_GAIN         0x06
#define ADS131_REG_CFG          0x07
#define ADS131_REG_THRESHOLD    0x08
#define ADS131_REG_OFFSET       0x09
#define ADS131_REG_CRC          0x0A

/* STATUS bit masks */
#define ADS131_STATUS_DRDY      (1U << 0)
#define ADS131_STATUS_RESET     (1U << 4)
#define ADS131_STATUS_CRC_ERR   (1U << 6)
#define ADS131_STATUS_WDT       (1U << 14)

/* MODE bits */
#define ADS131_MODE_DRDY_HIZ    (1U << 8)
#define ADS131_MODE_CRC_EN      (1U << 12)
#define ADS131_MODE_WDT_EN      (1U << 14)

/* Frame format: each SPI read returns 6 words = 12 bytes:
 *   word0: status (16-bit)
 *   word1..4: ch0..ch3 sample (24-bit, left-justified in 32-bit word)
 *   word5: CRC (16-bit)
 */
#define ADS131_FRAME_WORDS  6
#define ADS131_FRAME_BYTES  (ADS131_FRAME_WORDS * 4)

/* ================================================================
 * CY15B104Q — 4 Mbit FRAM (SPI, up to 40 MHz)
 * ================================================================ */
#define FRAM_CMD_WREN      0x06
#define FRAM_CMD_WRDI      0x04
#define FRAM_CMD_RDSR      0x05
#define FRAM_CMD_WRSR      0x01
#define FRAM_CMD_READ      0x03
#define FRAM_CMD_WRITE     0x02
#define FRAM_CMD_RDID      0x9F
#define FRAM_CMD_SLEEP     0xB9
#define FRAM_CMD_WAKE      0xAB

/* Status register bits */
#define FRAM_SR_WEL        (1U << 1)
#define FRAM_SR_BP0        (1U << 2)
#define FRAM_SR_BP1        (1U << 3)
#define FRAM_SR_WPEN       (1U << 7)

/* Expected RDID for CY15B104Q: 0x7F7F7F7F 0x7F7F7F7F 0x7F2C2C (9 bytes) */
#define FRAM_ID_LEN        9

/* ================================================================
 * SX1262 — Sub-GHz LoRa transceiver
 * ================================================================ */

/* SPI opcode set (first byte of every transaction) */
#define SX1262_SET_SLEEP       0x84
#define SX1262_SET_STDBY       0x80
#define SX1262_SET_FS          0xC1
#define SX1262_SET_TX          0x83
#define SX1262_SET_RX          0x82
#define SX1262_STOP_TIMER      0x9A
#define SX1262_SET_RXFREQ      0x86
#define SX1262_SET_PKT_TYPE    0x8A
#define SX1262_GET_PKT_TYPE    0x11
#define SX1262_SET_TX_PARAMS   0x8E
#define SX1262_SET_MOD_PARAMS  0x8B
#define SX1262_SET_CAD_PARAMS  0x88
#define SX1262_SET_BUFFER_BASE 0x8F
#define SX1262_WRITE_BUFFER    0x0E
#define SX1262_READ_BUFFER     0x1D
#define SX1262_SET_DIO_IRQ     0x8D
#define SX1262_GET_IRQ_STATUS  0x12
#define SX1262_CLEAR_IRQ       0x02
#define SX1262_SET_DIO2_AS_RF  0x9D
#define SX1262_SET_DIO3_AS_TC  0x97
#define SX1262_SET_REGULATOR   0x96
#define SX1262_CALIBRATE       0x89
#define SX1262_SET_PA_CONFIG   0x95
#define SX1262_GET_STATUS      0xC0
#define SX1262_GET_RX_BUF_STATUS 0x13
#define SX1262_GET_TX_BUF_STATUS  0x14
#define SX1262_GET_DEVICE_ERRORS 0x17
#define SX1262_RESET_STATS     0x94

/* Packet types */
#define SX1262_PKT_LORA        0x01
#define SX1262_PKT_FSK         0x02

/* IRQ masks */
#define SX1262_IRQ_TX_DONE     (1U << 0)
#define SX1262_IRQ_RX_DONE     (1U << 1)
#define SX1262_IRQ_PREAMBLE    (1U << 2)
#define SX1262_IRQ_SYNCWORD    (1U << 3)
#define SX1262_IRQ_HEADER      (1U << 4)
#define SX1262_IRQ_CRC_ERR     (1U << 5)
#define SX1262_IRQ_CAD_DONE    (1U << 6)
#define SX1262_IRQ_CAD_DETECTED (1U << 7)
#define SX1262_IRQ_TIMEOUT     (1U << 9)
#define SX1262_IRQ_ALL         0x03FF

/* Status byte fields */
#define SX1262_STATUS_MODE_MASK    0x70
#define SX1262_STATUS_MODE_SHIFT   4
#define SX1262_STATUS_CMD_MASK     0x0E
#define SX1262_STATUS_CMD_SHIFT    1

/* ================================================================
 * SHT45 — Temperature & Humidity sensor (I2C 0x44)
 * ================================================================ */
#define SHT45_CMD_MEASURE_HPM     0xFD   /* clock stretching, high repeatability */
#define SHT45_CMD_MEASURE_MPM     0xF6   /* no clock stretching, medium */
#define SHT45_CMD_MEASURE_LPM     0xE0   /* low repeatability */
#define SHT45_CMD_SOFT_RESET      0x94
#define SHT45_CMD_READ_SERIAL     0x89
#define SHT45_CMD_HEATER_200MW_1S 0x2D   /* heater for condensation removal */

/* Conversion time: max 8.3 ms for high-repeatability */
#define SHT45_CONV_MS             9

/* ================================================================
 * BMP390 — Barometric pressure sensor (I2C 0x77)
 * ================================================================ */
#define BMP390_REG_CHIP_ID        0x00
#define BMP390_CHIP_ID_VAL        0x60
#define BMP390_REG_REV_ID         0x01
#define BMP390_REG_ERR_REG        0x02
#define BMP390_REG_STATUS         0x03
#define BMP390_REG_DATA           0x04
#define BMP390_REG_FIFO_CONFIG    0x09
#define BMP390_REG_FIFO_DATA      0x0B
#define BMP390_REG_INT_CTRL       0x0C
#define BMP390_REG_FIFO_COUNT     0x0E
#define BMP390_REG_FIFO_CONFIG_1  0x0F
#define BMP390_REG_DSP_CONFIG     0x1F
#define BMP390_REG_DSP_IIR        0x21
#define BMP390_REG_CALIB          0x30   /* 21 bytes */
#define BMP390_REG_CMD            0x7E

#define BMP390_CMD_RESET          0xB6
#define BMP390_CMD_FIFO_FLUSH     0xB0
#define BMP390_OSR_PRESS_8X       0x05   /* 8x oversampling */
#define BMP390_OSR_TEMP_1X        0x00
#define BMP390_ODR_50HZ           0x09
#define BMP390_MODE_NORMAL        0x30
#define BMP390_IIR_COEF_3         0x02

/* ================================================================
 * TSL2591 — Ambient light sensor (I2C 0x29)
 * ================================================================ */
#define TSL2591_REG_ENABLE        0x00   /* via command byte 0xA0 */
#define TSL2591_REG_CONTROL       0x01
#define TSL2591_REG_CHAN0_LOW     0x14
#define TSL2591_REG_CHAN1_LOW     0x18
#define TSL2591_REG_ID            0x12
#define TSL2591_ID_VAL            0x50

#define TSL2591_ENABLE_PON        0x01
#define TSL2591_ENABLE_AEN        0x02
#define TSL2591_ENABLE_SAI        0x40
#define TSL2591_CMD_CMD           0xA0    /* command bit + auto-increment */

#define TSL2591_GAIN_MED          (0x01 << 2)
#define TSL2591_INT_100MS         0x00
#define TSL2591_INT_200MS         0x01
#define TSL2591_INT_300MS         0x02
#define TSL2591_INT_400MS         0x03
#define TSL2591_INT_500MS         0x04
#define TSL2591_INT_600MS         0x05

/* ================================================================
 * LTC3588-1 — Piezo energy harvesting rectifier (no registers;
 * hardware-only. We expose the status via GPIO on the SE1010.)
 * ================================================================ */

/* ================================================================
 * SE1010 — Piezo/TEG MPPT harvester (no SPI; GPIO status only)
 * ================================================================ */
/* Two status lines, both active-high:
 *   V_STORE_OK  (PIN_HARVEST_VOK): supercap > 3.3 V
 *   V_STORE_LOW (PIN_HARVEST_LOW): supercap < 2.5 V (IRQ, active-low IRQ)
 */

/* ================================================================
 * LTC5500-1 — RF peak detector
 * ================================================================ */
/* No registers. The peak-hold capacitor voltage is compared against
 * a reference by an internal comparator; the IRQ line pulses low
 * for ~10 µs when the threshold is exceeded. The threshold is set
 * by an external resistor divider (R7/R8 on the schematic). */

#endif /* RAINFORGE_REGISTERS_H */