/* StitchScope peripheral register map
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef STITCHSCOPE_REGISTERS_H
#define STITCHSCOPE_REGISTERS_H

#include <stdint.h>

#define ADS131_REG_ID          0x00u
#define ADS131_REG_STATUS      0x01u
#define ADS131_REG_MODE        0x02u
#define ADS131_REG_CLOCK       0x03u
#define ADS131_REG_GAIN1       0x04u
#define ADS131_REG_GAIN2       0x05u
#define ADS131_CMD_RESET       0x0011u
#define ADS131_CMD_STANDBY     0x0022u
#define ADS131_CMD_WAKEUP      0x0033u
#define ADS131_CMD_LOCK        0x0555u
#define ADS131_CMD_UNLOCK      0x0655u

#define IIS3DWB_REG_WHO_AM_I   0x0Fu
#define IIS3DWB_REG_CTRL1_XL   0x10u
#define IIS3DWB_REG_CTRL3_C    0x12u
#define IIS3DWB_REG_FIFO_CTRL1 0x07u
#define IIS3DWB_REG_FIFO_CTRL3 0x09u
#define IIS3DWB_REG_FIFO_CTRL4 0x0Au
#define IIS3DWB_REG_STATUS     0x1Eu
#define IIS3DWB_REG_FIFO_DATA  0x78u
#define IIS3DWB_WHO_AM_I       0x7Bu

#define MMC5603_REG_XOUT0      0x00u
#define MMC5603_REG_STATUS1    0x18u
#define MMC5603_REG_ODR        0x1Au
#define MMC5603_REG_CTRL0      0x1Bu
#define MMC5603_REG_CTRL1      0x1Cu
#define MMC5603_REG_CTRL2      0x1Du
#define MMC5603_REG_PRODUCT_ID 0x39u
#define MMC5603_PRODUCT_ID     0x10u

#define TMF8821_REG_APPID      0x00u
#define TMF8821_REG_ENABLE     0xE0u
#define TMF8821_REG_INT_STATUS 0xE1u
#define TMF8821_REG_CMD_STAT   0x08u
#define TMF8821_REG_RESULTS    0x20u

#define W25Q_CMD_READ_ID       0x9Fu
#define W25Q_CMD_READ_STATUS1  0x05u
#define W25Q_CMD_WRITE_ENABLE  0x06u
#define W25Q_CMD_PAGE_PROGRAM  0x02u
#define W25Q_CMD_SECTOR_ERASE  0x20u
#define W25Q_CMD_FAST_READ     0x0Bu

#define MMIO32(address) (*(volatile uint32_t *)(uintptr_t)(address))
#ifdef STITCHSCOPE_TARGET_NRF5340
#define NRF_P0_OUTSET MMIO32(0x50000508u)
#define NRF_P0_OUTCLR MMIO32(0x5000050Cu)
#define NRF_TIMER2_CC0 MMIO32(0x5000A540u)
#define NRF_WDT_RR0 MMIO32(0x50018500u)
#endif

static inline uint16_t be16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static inline int32_t sign_extend24(uint32_t value) {
    value &= 0x00FFFFFFu;
    return (value & 0x00800000u) ? (int32_t)(value | 0xFF000000u) : (int32_t)value;
}

#endif
