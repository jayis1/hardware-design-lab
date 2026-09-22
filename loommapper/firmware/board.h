/* LoomMapper board configuration
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#ifndef LOOMMAPPER_BOARD_H
#define LOOMMAPPER_BOARD_H
#include <stdint.h>
#define LM_CHANNELS 8u
#define LM_MAX_NODES 32u
#define LM_MAX_EDGES 96u
#define LM_SCAN_SAMPLES 64u
#define LM_EVENT_CAPACITY 64u
#define LM_PACKET_MAX 128u
#define LM_PROTOCOL_VERSION 1u
#define LM_SAMPLE_RATE_HZ 4000u
#define LM_EXCITATION_HZ 977u
#define LM_EXCITATION_MV 180u
#define LM_OPEN_THRESHOLD_UV 1800
#define LM_SHORT_THRESHOLD_UV 120000
#define LM_INTERMITTENT_DELTA_UV 8500
#define LM_BATTERY_LOW_MV 3350u
#define LM_SWDIO_PIN 18u
#define LM_SWCLK_PIN 19u
#define LM_STATUS_LED_PIN 22u
#define LM_MUX_A0_PIN 2u
#define LM_MUX_A1_PIN 3u
#define LM_MUX_A2_PIN 4u
#define LM_MUX_ENABLE_PIN 5u
#define LM_ADC_DRDY_PIN 7u
#define LM_ADC_CS_PIN 8u
#define LM_DAC_CS_PIN 9u
#define LM_CLIP_DETECT_BASE_PIN 10u
#define LM_I2C_SDA_PIN 26u
#define LM_I2C_SCL_PIN 27u
#define LM_SPI_HZ 8000000u
#define LM_I2C_HZ 400000u
#define LM_SCAN_PERIOD_MS 25u
#define LM_REPORT_PERIOD_MS 500u
#define LM_MAGIC0 0x4cu
#define LM_MAGIC1 0x4du
#endif
