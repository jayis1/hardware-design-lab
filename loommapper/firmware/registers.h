/* LoomMapper peripheral register constants
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 * Device command values are from ADS131M04 and AD5686R data sheets.
 */
#ifndef LOOMMAPPER_REGISTERS_H
#define LOOMMAPPER_REGISTERS_H
#include <stdint.h>
#define ADS131_CMD_NULL 0x0000u
#define ADS131_CMD_RESET 0x0011u
#define ADS131_CMD_STANDBY 0x0022u
#define ADS131_CMD_WAKEUP 0x0033u
#define ADS131_CMD_LOCK 0x0555u
#define ADS131_CMD_UNLOCK 0x0655u
#define ADS131_REG_ID 0x00u
#define ADS131_REG_STATUS 0x01u
#define ADS131_REG_MODE 0x02u
#define ADS131_REG_CLOCK 0x03u
#define ADS131_REG_GAIN1 0x04u
#define ADS131_REG_GAIN2 0x05u
#define ADS131_REG_CFG 0x06u
#define ADS131_STATUS_LOCKED (1u << 15)
#define ADS131_STATUS_RESET (1u << 10)
#define ADS131_MODE_CRC_EN (1u << 1)
#define AD5686_CMD_WRITE_INPUT 0x1u
#define AD5686_CMD_UPDATE_DAC 0x2u
#define AD5686_CMD_WRITE_UPDATE 0x3u
#define AD5686_CMD_POWER 0x4u
#define AD5686_CMD_RESET 0x6u
#define AD5686_CMD_REF_SETUP 0x7u
#define TCA9548A_ADDRESS 0x70u
#define TCA9548A_CHANNEL_MASK 0xffu
#define MAX17048_ADDRESS 0x36u
#define MAX17048_VCELL_REG 0x02u
#define MAX17048_SOC_REG 0x04u
#define MAX17048_MODE_REG 0x06u
#define MAX17048_VERSION_REG 0x08u
#endif
