/*
 * EAG-7000 — I2C Driver for Cortex-M4F
 *
 * I2C1: PMIC (PCA9450) + RTC (PCF2129)
 * I2C2: Sensor mux (TCA9548A) → downstream sensors
 * I2C3: Hailo-8 M.2 module
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#include "i2c.h"
#include "registers.h"

/* ============================================================================
 * I2C Timing Calculations
 *
 * i.MX8MP I2C frequency divider (IFDR register):
 *   I2C clock source = PERCLK (66 MHz)
 *   For 400 kHz fast mode: divide ratio ≈ 66MHz / (400kHz * 2) ≈ 82
 *   Closest IFDR value: 0x1E (divide by 96) → ~344 kHz
 *   Closest IFDR value: 0x24 (divide by 80) → ~412 kHz
 *   Use 0x1E for safe 400 kHz operation
 * ============================================================================ */

#define I2C_IFDR_400KHZ    0x1E    /* IFDR divider for ~344 kHz at 66 MHz PERCLK */

/* ============================================================================
 * I2C Bus Operations
 * ============================================================================ */

int i2c_init(i2c_dev_t *dev, uintptr_t base_addr, uint32_t speed_hz)
{
    dev->base = base_addr;
    dev->speed_hz = speed_hz;

    /* Disable I2C during configuration */
    mmio_write32(dev->base + I2C_I2CR, 0);

    /* Set frequency divider for 400 kHz */
    mmio_write32(dev->base + I2C_IFDR, I2C_IFDR_400KHZ);

    /* Set slave address to 0 (we're always master) */
    mmio_write32(dev->base + I2C_IADR, 0);

    /* Enable I2C */
    mmio_write32(dev->base + I2C_I2CR, I2C_I2CR_EN);

    return 0;
}

/**
 * Wait for I2C bus to become idle.
 */
static int i2c_wait_bus_idle(i2c_dev_t *dev)
{
    uint32_t timeout = 100000;
    while (mmio_read32(dev->base + I2C_I2SR) & I2C_I2SR_IBB) {
        if (--timeout == 0) return -1;
    }
    return 0;
}

/**
 * Wait for I2C transfer complete interrupt flag.
 */
static int i2c_wait_transfer_complete(i2c_dev_t *dev)
{
    uint32_t timeout = 100000;
    while (!(mmio_read32(dev->base + I2C_I2SR) & I2C_I2SR_ICF)) {
        if (--timeout == 0) return -1;
    }

    /* Check for arbitration lost */
    if (mmio_read32(dev->base + I2C_I2SR) & I2C_I2SR_IAL) {
        /* Clear arbitration lost flag */
        mmio_write32(dev->base + I2C_I2SR,
                      mmio_read32(dev->base + I2C_I2SR) & ~I2C_I2SR_IAL);
        return -2;
    }

    return 0;
}

/**
 * Generate I2C start condition and send slave address.
 * @param dev     I2C device handle
 * @param addr    7-bit slave address
 * @param read    1 for read, 0 for write
 * @return 0 on success, negative on error
 */
static int i2c_start(i2c_dev_t *dev, uint8_t addr, int read)
{
    int ret;

    /* Wait for bus idle */
    ret = i2c_wait_bus_idle(dev);
    if (ret) return ret;

    /* Enable I2C, set master mode, transmit mode */
    uint32_t cr = I2C_I2CR_EN | I2C_I2CR_MSTA;

    if (!read) {
        cr |= I2C_I2CR_MTX;   /* Transmit mode for write */
    } else {
        cr &= ~I2C_I2CR_MTX;  /* Receive mode for read */
    }

    mmio_write32(dev->base + I2C_I2CR, cr);

    /* Clear status flags */
    mmio_write32(dev->base + I2C_I2SR, 0);

    /* Send slave address + R/W bit */
    uint8_t addr_byte = (addr << 1) | (read ? 1 : 0);
    mmio_write32(dev->base + I2C_I2DR, addr_byte);

    /* Wait for address transfer complete */
    ret = i2c_wait_transfer_complete(dev);
    if (ret) return ret;

    /* Check for ACK (RXAK should be 0) */
    if (mmio_read32(dev->base + I2C_I2SR) & I2C_I2SR_RXAK) {
        /* No ACK from slave — generate stop and return error */
        i2c_stop(dev);
        return -3;
    }

    return 0;
}

/**
 * Generate I2C stop condition.
 */
static void i2c_stop(i2c_dev_t *dev)
{
    uint32_t cr = mmio_read32(dev->base + I2C_I2CR);

    /* Clear MSTA to generate stop condition */
    cr &= ~I2C_I2CR_MSTA;

    /* Keep I2C enabled */
    cr |= I2C_I2CR_EN;

    mmio_write32(dev->base + I2C_I2CR, cr);

    /* Wait for bus to become idle */
    i2c_wait_bus_idle(dev);
}

/**
 * Write a single byte to I2C bus.
 */
static int i2c_write_byte(i2c_dev_t *dev, uint8_t data)
{
    int ret;

    mmio_write32(dev->base + I2C_I2DR, data);

    ret = i2c_wait_transfer_complete(dev);
    if (ret) return ret;

    /* Check for ACK */
    if (mmio_read32(dev->base + I2C_I2SR) & I2C_I2SR_RXAK) {
        return -1;  /* NACK */
    }

    return 0;
}

/**
 * Read a single byte from I2C bus.
 */
static int i2c_read_byte(i2c_dev_t *dev, int last_byte)
{
    int ret;
    uint32_t cr = mmio_read32(dev->base + I2C_I2CR);

    /* For the last byte, set TXAK to NACK (no ACK) to signal end */
    if (last_byte) {
        cr |= I2C_I2CR_TXAK;    /* NACK next byte */
    } else {
        cr &= ~I2C_I2CR_TXAK;   /* ACK next byte */
    }

    /* Switch to receive mode */
    cr &= ~I2C_I2CR_MTX;
    mmio_write32(dev->base + I2C_I2CR, cr);

    /* Dummy read to trigger next byte reception */
    mmio_read32(dev->base + I2C_I2DR);

    ret = i2c_wait_transfer_complete(dev);
    if (ret) return ret;

    return mmio_read32(dev->base + I2C_I2DR) & 0xFF;
}

/* ============================================================================
 * Public API
 * ============================================================================ */

int i2c_write_reg(i2c_dev_t *dev, uint8_t slave_addr,
                   uint8_t reg, const uint8_t *data, uint16_t len)
{
    int ret;

    /* Start + slave address (write) */
    ret = i2c_start(dev, slave_addr, 0);
    if (ret) return ret;

    /* Write register address */
    ret = i2c_write_byte(dev, reg);
    if (ret) goto stop;

    /* Write data bytes */
    for (uint16_t i = 0; i < len; i++) {
        ret = i2c_write_byte(dev, data[i]);
        if (ret) goto stop;
    }

stop:
    i2c_stop(dev);
    return ret;
}

int i2c_read_reg(i2c_dev_t *dev, uint8_t slave_addr,
                  uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret;
    int byte;

    /* Start + slave address (write) for register pointer */
    ret = i2c_start(dev, slave_addr, 0);
    if (ret) return ret;

    /* Write register address */
    ret = i2c_write_byte(dev, reg);
    if (ret) goto stop;

    /* Repeated start + slave address (read) */
    /* Generate repeated start */
    uint32_t cr = mmio_read32(dev->base + I2C_I2CR);
    cr |= I2C_I2CR_RSTA;
    mmio_write32(dev->base + I2C_I2CR, cr);

    /* Clear status */
    mmio_write32(dev->base + I2C_I2SR, 0);

    /* Send slave address + R bit */
    mmio_write32(dev->base + I2C_I2DR, (slave_addr << 1) | 1);

    ret = i2c_wait_transfer_complete(dev);
    if (ret) goto stop;

    /* Read data bytes */
    for (uint16_t i = 0; i < len; i++) {
        byte = i2c_read_byte(dev, (i == len - 1));
        if (byte < 0) {
            ret = byte;
            goto stop;
        }
        buf[i] = (uint8_t)byte;
    }

stop:
    i2c_stop(dev);
    return ret;
}

int i2c_write_byte_reg(i2c_dev_t *dev, uint8_t slave_addr,
                        uint8_t reg, uint8_t value)
{
    return i2c_write_reg(dev, slave_addr, reg, &value, 1);
}

int i2c_read_byte_reg(i2c_dev_t *dev, uint8_t slave_addr,
                       uint8_t reg)
{
    uint8_t value;
    int ret = i2c_read_reg(dev, slave_addr, reg, &value, 1);
    if (ret < 0) return ret;
    return value;
}