/*
 * board.h — Pin and port definitions for the Occlusograph board.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 *
 * Target: nRF5340 (QFAA), custom Occlusograph intraoral flex PCB.
 * The board has two physical regions: the sensor flex tongue (analog
 * front-end + piezo/capacitive arrays) and the behind-molar electronics
 * module (MCU, radio, power). This header maps every fixed-function GPIO,
 * analog channel, peripheral bus, and power domain used by the firmware.
 */

#ifndef OCCLUSOGRAPH_BOARD_H
#define OCCLUSOGRAPH_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* ---- Board identity ---- */
#define BOARD_NAME          "occlusograph-v1"
#define BOARD_REV           0x10u
#define HW_MODEL_CODE       0x4F43u   /* "OC" */
#define SERIAL_DEFAULT      "OCG-00000"

/* ---- Clock tree ----
 * nRF5340 app core runs from the internal 64 MHz HFINTOSC; the net core is
 * clocked from the same source. The 32.768 kHz LFCLK drives the RTC and
 * the BLE sleep timing. For high-accuracy ADC we do NOT use an external
 * crystal — the internal RC is sufficient for 12-bit resolution after
 * software offset/gain calibration.
 */
#define HFCLK_MHZ           128u
#define LFCLK_HZ            32768u

/* ---- Power domains ---- */
#define VDD_MV              3300u   /* main rail from buck */
#define VDDA_MV             1800u   /* analog rail from LDO */
#define VBAT_MV_MIN         2700u   /* LiPo discharge floor */
#define VBAT_MV_MAX         4200u   /* LiPo full charge */
#define VBAT_ADC_GAIN       2u      /* divider: battery / 2 into ADC */
#define VBAT_CHARGE_THRESH  4100u
#define VBAT_LOW_THRESH     3100u

/* ---- LED / status (single 0201 SMD, visible through overmold) ---- */
#define LED_STATUS_PORT     0       /* P0 */
#define LED_STATUS_PIN      3
#define LED_STATUS_ON       1
#define LED_STATUS_OFF      0

/* ---- Battery divider into SAADC ---- */
#define VBAT_SENSE_CH       2u      /* SAADC channel 2 */

/* ---- Inductive charge supervisor ---- */
#define CHG_ENABLE_PORT    0
#define CHG_ENABLE_PIN     9
#define CHG_NIRQ_PORT      0
#define CHG_NIRQ_PIN       10
#define CHG_POWER_MW_MAX   5u

/* ---- Piezo charge-amp array: 16 charge amps, each fed by a 4:1 mux ----
 * The 16 AD8603 outputs go to SAADC inputs AN0..AN15. The mux select lines
 * (S0/S1) are driven from a single GPIO pair shared across all 16 amps, so
 * one mux-select pair selects element (0..3) within all banks simultaneously.
 * The charge-amp integrator reset switches are controlled by a single
 * global reset GPIO — all amps reset together at the end of each frame.
 */
#define PIEZO_MUX_S0_PORT   0
#define PIEZO_MUX_S0_PIN    4
#define PIEZO_MUX_S1_PORT   0
#define PIEZO_MUX_S1_PIN    5
#define PIEZO_RESET_PORT    0
#define PIEZO_RESET_PIN     6

/* SAADC input channel -> physical charge-amp index (0..15) */
#define PIEZO_SAADC_CH_BASE 0u  /* AN0..AN15 = charge amps 0..15 */
#define PIEZO_NUM_BANKS     16u
#define PIEZO_MUX_RATIO     4u
#define PIEZO_NUM_ELEMENTS  (PIEZO_NUM_BANKS * PIEZO_MUX_RATIO)  /* 64 */

/* ---- Capacitive pressure array: 2 × AD7746 on I²C ----
 * Each AD7746 has 2 channels (CIN1, CIN2); two devices × 2 channels = 4
 * effective channels. Each of those 4 channels is externally muxed 8:1 to
 * 32 capacitive elements. The mux selects are driven from a 74HC4051
 * (3 select lines) shared across the 4 CDC channels.
 */
#define CAP_MUX_A0_PORT     0
#define CAP_MUX_A0_PIN      11
#define CAP_MUX_A1_PORT     0
#define CAP_MUX_A1_PIN      12
#define CAP_MUX_A2_PORT     0
#define CAP_MUX_A2_PIN      13
#define CAP_NUM_MUX_OUTPUTS 8u
#define CAP_NUM_DEVICES     2u
#define CAP_CH_PER_DEVICE   2u
#define CAP_NUM_ELEMENTS    (CAP_NUM_MUX_OUTPUTS * CAP_NUM_DEVICES * CAP_CH_PER_DEVICE)  /* 32 */

/* ---- I²C buses ----
 * TWI0: capacitive CDC (AD7746 ×2)
 * TWI1: auxiliary sensors (IMU, TMP117, MAX30101)
 */
#define TWI0_SCL_PORT       0
#define TWI0_SCL_PIN        26
#define TWI0_SDA_PORT       0
#define TWI0_SDA_PIN        27
#define TWI0_FREQ_HZ        400000u

#define TWI1_SCL_PORT       0
#define TWI1_SCL_PIN        30
#define TWI1_SDA_PIN        31
#define TWI1_SCL_PORT1      0
#define TWI1_SDA_PORT       0
#define TWI1_FREQ_HZ        400000u

/* ---- I²C device addresses ---- */
#define AD7746_ADDR_A       0x48u   /* AD7746 #1, A0=GND */
#define AD7746_ADDR_B       0x4Au   /* AD7746 #2, A0=VDD */
#define IMU_I2C_ADDR        0x69u   /* ICM-42688-P, AD0=1 */
#define TMP117_ADDR         0x48u   /* TMP117 default */
#define MAX30101_ADDR       0x57u

/* ---- QSPI flash for event buffering ----
 * W25R128WEIG 32 Mb, on QSPI bus P0.16..P0.19
 */
#define QSPI_CS_PORT        0
#define QSPI_CS_PIN         17
#define QSPI_SCK_PORT       0
#define QSPI_SCK_PIN        18
#define QSPI_D0_PORT        0
#define QSPI_D0_PIN         20
#define QSPI_D1_PORT        0
#define QSPI_D1_PIN         21
#define QSPI_D2_PORT        0
#define QSPI_D2_PIN         22
#define QSPI_D3_PORT        0
#define QSPI_D3_PIN         23
#define FLASH_PAGE_SIZE     256u
#define FLASH_SECTOR_SIZE   4096u
#define FLASH_TOTAL_BYTES   (16u * 1024u * 1024u)  /* 16 MB */

/* ---- NFC NTAG 215 ----
 * The NTAG is wired to the NFC2 antenna pads on the nRF5340; firmware does
 * not actively drive it — it is powered parasitically. We only read the
 * serial via the NFC tag reader in the app.
 */
#define NFC_FIELD_DETECT_PORT 0
#define NFC_FIELD_DETECT_PIN  24

/* ---- GPIO interrupt lines from peripherals ---- */
#define IMU_NIRQ_PORT       0
#define IMU_NIRQ_PIN        14
#define TMP117_NIRQ_PORT    0
#define TMP117_NIRQ_PIN     15
#define MAX30101_NIRQ_PORT  0
#define MAX30101_NIRQ_PIN   16

/* ---- Watchdog ---- */
#define WDT_TIMEOUT_MS      8000u   /* 8 s — generous; sensor loop is 1 ms */

/* ---- Sampling configuration ---- */
#define SAMPLE_RATE_HZ      1000u
#define SAMPLE_PERIOD_US    (1000000u / SAMPLE_RATE_HZ)
#define CAP_SAMPLE_RATE_HZ  100u
#define CAP_SAMPLE_PERIOD_MS 10u

/* ---- Feature extraction ---- */
#define FEATURE_WINDOW_MS   250u
#define FEATURE_HOP_MS       50u
#define FEATURE_WINDOW_SAMPS (FEATURE_WINDOW_MS * SAMPLE_RATE_HZ / 1000u)
#define FEATURE_HOP_SAMPS   (FEATURE_HOP_MS * SAMPLE_RATE_HZ / 1000u)

/* ---- BLE GATT characteristic UUIDs (custom 128-bit, base prefix) ---- */
/* Base: 6e400001-b5a3-f393-e0a9-e50e24dcca9e (UART-like, reused for our profile) */
#define UUID_SERVICE_OCCLUSAL    0x9A01u
#define UUID_CHR_FORCE_STREAM    0x9A02u
#define UUID_CHR_FORCE_CONFIG     0x9A03u
#define UUID_CHR_CALIBRATION     0x9A04u
#define UUID_SERVICE_EVENTLOG    0x9A05u
#define UUID_CHR_EVENTS          0x9A06u
#define UUID_CHR_EVENT_RANGE     0x9A07u
#define UUID_CHR_SYNC_STATUS     0x9A08u

/* ---- Thermistors / thermal limits ---- */
#define TEMP_MV_DETACH_HIGH     4200  /* if temp rises above this, device detached (m°C) */
#define TEMP_MV_DETACH_LOW      2000  /* intraoral is ~34-37°C; below 20°C means out */

/* ---- Runtime limits ---- */
#define MAX_FORCE_N             300u   /* per-element clamp for display */
#define BRUX_THRESH_N           20u    /* RMS force above this = bruxism candidate */
#define BRUX_MIN_EPISODE_MS     500u
#define MAX_EVENT_RECORDS       4096u

/* ---- Convenience macros ---- */
#define ARRAY_SIZE(a)  (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b)      ((a) < (b) ? (a) : (b))
#define MAX(a, b)      ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) (MAX((lo), MIN((x), (hi))))

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSOGRAPH_BOARD_H */