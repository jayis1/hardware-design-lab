/*
 * registers.h — Register maps for ADS1255, SSD1306, NEO-M9N, NINA-B306
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * TI ADS1255 — 24-bit, delta-sigma ADC (for photodiode array multiplexing)
 * ========================================================================= */

/* Commands */
#define ADS1255_CMD_WAKEUP      0x00
#define ADS1255_CMD_RDATA       0x01
#define ADS1255_CMD_RDATAC      0x03
#define ADS1255_CMD_SDATAC      0x0F
#define ADS1255_CMD_RREG        0x1A  /* + offset */
#define ADS1255_CMD_WREG        0x2A  /* + offset */
#define ADS1255_CMD_SELFCAL     0xF0
#define ADS1255_CMD_SELFOCAL    0xF1
#define ADS1255_CMD_SELFGCAL    0xF2
#define ADS1255_CMD_SYSOCAL     0xF3
#define ADS1255_CMD_SYSGCAL     0xF4
#define ADS1255_CMD_SYNC        0xFC
#define ADS1255_CMD_STANDBY     0xFD
#define ADS1255_CMD_RESET       0xFE

/* Register addresses */
#define ADS1255_REG_STATUS      0x00
#define ADS1255_REG_MUX         0x01
#define ADS1255_REG_ADCON       0x02
#define ADS1255_REG_DRATE       0x03
#define ADS1255_REG_OFC0        0x04
#define ADS1255_REG_OFC1        0x05
#define ADS1255_REG_OFC2        0x06
#define ADS1255_REG_FSC0        0x07
#define ADS1255_REG_FSC1        0x08
#define ADS1255_REG_FSC2        0x09

/* STATUS bits */
#define ADS1255_STATUS_ORDER    0x01
#define ADS1255_STATUS_ACAL     0x02
#define ADS1255_STATUS_BUFEN    0x04
#define ADS1255_STATUS_DRDY     0x80

/* MUX channel selections (single-ended) */
#define ADS1255_MUX_AIN0        0x00  /* AIN0 = photodiode element select */
#define ADS1255_MUX_AIN1        0x10
#define ADS1255_MUX_AINCOM      0x08

/* ADCON: PGA gain */
#define ADS1255_GAIN_1          0x00
#define ADS1255_GAIN_2          0x01
#define ADS1255_GAIN_4          0x02
#define ADS1255_GAIN_8          0x03
#define ADS1255_GAIN_16         0x04
#define ADS1255_GAIN_32         0x05
#define ADS1255_GAIN_64         0x06

/* DRATE: data rate (SPS) */
#define ADS1255_DRATE_30000     0xF0
#define ADS1255_DRATE_15000     0xE0
#define ADS1255_DRATE_7500      0xD0
#define ADS1255_DRATE_3750      0xC0
#define ADS1255_DRATE_2000      0xB0
#define ADS1255_DRATE_1000      0xA1
#define ADS1255_DRATE_500       0x92
#define ADS1255_DRATE_100       0x72
#define ADS1255_DRATE_60        0x63
#define ADS1255_DRATE_30        0x50
#define ADS1255_DRATE_10        0x40

/* =========================================================================
 * SSD1306 — OLED display controller (128x64)
 * ========================================================================= */

#define SSD1306_SET_CONTRAST        0x81
#define SSD1306_ENTIRE_ON           0xA4
#define SSD1306_ENTIRE_ON_NORM      0xA5
#define SSD1306_NORMAL_DISPLAY      0xA6
#define SSD1306_INVERT_DISPLAY      0xA7
#define SSD1306_DISPLAY_OFF         0xAE
#define SSD1306_DISPLAY_ON          0xAF
#define SSD1306_SET_DISP_OFFSET     0xD3
#define SSD1306_SET_COMPINS         0xDA
#define SSD1306_SET_VCOM_DETECT     0xDB
#define SSD1306_SET_DISPCLK_DIV     0xD5
#define SSD1306_SET_PRECHARGE       0xD9
#define SSD1306_SET_MULTIPLEX       0xA8
#define SSD1306_SET_START_LINE      0x40
#define SSD1306_SET_LOW_COL         0x00
#define SSD1306_SET_HIGH_COL        0x10
#define SSD1306_SET_PAGE            0xB0
#define SSD1306_SEG_REMAP_0         0xA0
#define SSD1306_SEG_REMAP_127       0xA1
#define SSD1306_COM_SCAN_NORMAL     0xC0
#define SSD1306_COM_SCAN_REMAPPED   0xC8
#define SSD1306_CHARGE_PUMP         0x8D
#define SSD1306_EXTERNAL_VCC        0x01
#define SSD1306_INTERNAL_VCC        0x02

#define SSD1306_WIDTH               128
#define SSD1306_HEIGHT              64
#define SSD1306_PAGES               8

/* =========================================================================
 * u-blox NEO-M9N — GNSS receiver (I2C protocol)
 * ========================================================================= */

#define GPS_REG_TXBUF              0xFD  /* bytes available in TX buffer (2 bytes) */
#define GPS_REG_DATA               0xFF  /* read data stream */

/* UBX message class/ID (used for NAV-PVT) */
#define GPS_UBX_CLASS_NAV          0x01
#define GPS_UBX_ID_NAV_PVT         0x07
#define GPS_UBX_SYNC1              0xB5
#define GPS_UBX_SYNC2              0x62

/* NAV-PVT field offsets */
#define GPS_PVT_ITOW_OFFSET        4
#define GPS_PVT_YEAR_OFFSET        8
#define GPS_PVT_MONTH_OFFSET       10
#define GPS_PVT_DAY_OFFSET         11
#define GPS_PVT_HOUR_OFFSET        12
#define GPS_PVT_MIN_OFFSET         13
#define GPS_PVT_SEC_OFFSET         14
#define GPS_PVT_FIX_TYPE_OFFSET    20
#define GPS_PVT_LAT_OFFSET         28   /* 1e-7 deg */
#define GPS_PVT_LON_OFFSET         24   /* 1e-7 deg */
#define GPS_PVT_HACC_OFFSET        40   /* mm */
#define GPS_PVT_SATS_OFFSET        23
#define GPS_PVT_PDOP_OFFSET        20
#define GPS_PVT_LEN                92

/* Fix types */
#define GPS_FIX_NONE               0
#define GPS_FIX_2D                 2
#define GPS_FIX_3D                 3
#define GPS_FIX_GNSS_DR            4

/* =========================================================================
 * u-blox NINA-B306 — BLE 5.0 module (UART NCP protocol)
 * ========================================================================= */

/* NCP (Network Co-Processor) UART protocol framing */
#define NINA_SYNC                  0x01
#define NINA_EOF                   0x03
#define NINA_CMD_GAP               0x0C
#define NINA_CMD_GATT              0x0D

/* BLE custom service UUIDs */
#define BLE_SERVICE_CHLOROMAP      "0000C701-1212-EFDE-1523-785FEABCD123"
#define BLE_CHAR_MEASUREMENT       "0000C702-1212-EFDE-1523-785FEABCD124"
#define BLE_CHAR_COMMAND           "0000C703-1212-EFDE-1523-785FEABCD125"
#define BLE_CHAR_STATUS            "0000C704-1212-EFDE-1523-785FEABCD126"
#define BLE_CHAR_CALIBRATION       "0000C705-1212-EFDE-1523-785FEABCD127"

/* BLE state machine */
#define BLE_STATE_RESET            0
#define BLE_STATE_BOOTING          1
#define BLE_STATE_ADVERTISING      2
#define BLE_STATE_CONNECTED        3
#define BLE_STATE_TX_PENDING       4

/* BLE measurement packet format (48 bytes) */
#define BLE_PKT_MAGIC              0xCF
#define BLE_PKT_VER                0x01
#define BLE_PKT_LEN                48
#define BLE_PKT_MAGIC_OFF          0
#define BLE_PKT_VER_OFF            1
#define BLE_PKT_SPAD_OFF           2    /* int16 */
#define BLE_PKT_NDVI_OFF           4    /* int16 (×1000) */
#define BLE_PKT_NSI_OFF            6    /* int16 (×1000) */
#define BLE_PKT_LWBI_OFF           8    /* int16 (×1000) */
#define BLE_PKT_REDEDGE_OFF        10   /* int16 (×1000) */
#define BLE_PKT_LAT_OFF            12   /* int32 (1e-7 deg) */
#define BLE_PKT_LON_OFF            16   /* int32 (1e-7 deg) */
#define BLE_PKT_TS_OFF             20   /* uint32 ms */
#define BLE_PKT_BANDS_OFF          24   /* 16 × int16 reflectance ×1000 */
#define BLE_PKT_BATT_OFF           56   /* uint16 mV — wait, packet is 48 bytes */

/* Corrected: bands are 16 × int16 = 32 bytes; total = 24 header + 32 = 56; we use 48 */
/* Optimized: 8 key bands + indices = 48 bytes */
#define BLE_PKT_BAND450_OFF        24
#define BLE_PKT_BAND531_OFF        26
#define BLE_PKT_BAND660_OFF        28
#define BLE_PKT_BAND680_OFF        30
#define BLE_PKT_BAND700_OFF        32
#define BLE_PKT_BAND800_OFF        34
#define BLE_PKT_BAND900_OFF        36
#define BLE_PKT_BAND970_OFF        38
#define BLE_PKT_BATT_OFF           40
#define BLE_PKT_TEMP_OFF           42
#define BLE_PKT_SAT_OFF            44
#define BLE_PKT_CRC_OFF            46

/* USB CDC commands */
#define USB_CMD_MEASURE            "MEASURE"
#define USB_CMD_CAL_WHITE          "CAL WHITE"
#define USB_CMD_GET_SPECTRUM       "GET SPECTRUM"
#define USB_CMD_GET_STATUS         "GET STATUS"
#define USB_CMD_SET_BANDS          "SET BANDS"
#define USB_CMD_SET_INTTIME        "SET INTTIME"
#define USB_CMD_HELP               "HELP"

/* =========================================================================
 * STM32L432 flash (for calibration storage)
 * ========================================================================= */
#define FLASH_PAGE_SIZE            2048
#define FLASH_KEY1                 0x45670123
#define FLASH_KEY2                 0xCDEF89AB
#define FLASH_SR_BSY               (1 << 0)
#define FLASH_SR_PGERR             (1 << 2)
#define FLASH_SR_EOP               (1 << 1)
#define FLASH_CR_LOCK              (1 << 7)
#define FLASH_CR_PER               (1 << 1)
#define FLASH_CR_PG                (1 << 0)
#define FLASH_CR_STRT              (1 << 3)

#endif /* REGISTERS_H */