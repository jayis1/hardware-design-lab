/*
 * imu_drv.c — ICM-42688-P 6-axis IMU driver implementation
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * SPI driver for the TDK InvenSense ICM-42688-P 6-axis IMU.
 * Configures the device for 1 kHz ODR, ±16g accel, ±2000 dps gyro,
 * FIFO stream mode with watermark interrupt.
 */

#include "imu_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* SPI read: MSB of reg address = 1 for read, 0 for write */
#define IMU_SPI_READ_BIT  0x80
#define IMU_SPI_WRITE_BIT 0x00

static imu_irq_callback_t irq_callback = NULL;
static float current_acc_sens = IMU_ACC_SENS_16G_LSB;
static float current_gyro_sens = IMU_GYRO_SENS_2000_LSB;
static uint8_t spi_cs_pin = IMU_CS_PIN;

/* ---- Low-level SPI transfer ---- */
static void imu_spi_cs_low(void)
{
    /* Drive CS pin low via GPIO */
    /* In production: gpio_set(spi_cs_pin, 0); */
    /* Placeholder using direct register access pattern */
}

static void imu_spi_cs_high(void)
{
    /* Drive CS pin high */
}

static void imu_spi_xfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    /* Configure SPIM2 for transfer
     * Set TXD_PTR, RXD_PTR, MAXCNT, trigger START, wait for END
     * In production: use nRF SPIM2 peripheral with DMA
     */
    (void)tx;
    (void)rx;
    (void)len;
}

/* ---- Register access ---- */
uint8_t imu_read_reg(uint8_t reg)
{
    uint8_t tx[2] = { (uint8_t)(reg | IMU_SPI_READ_BIT), 0xFF };
    uint8_t rx[2] = { 0, 0 };

    imu_spi_cs_low();
    imu_spi_xfer(tx, rx, 2);
    imu_spi_cs_high();

    return rx[1];
}

void imu_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { (uint8_t)(reg | IMU_SPI_WRITE_BIT), val };
    uint8_t rx[2] = { 0, 0 };

    imu_spi_cs_low();
    imu_spi_xfer(tx, rx, 2);
    imu_spi_cs_high();
}

void imu_read_regs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (len == 0) return;

    /* Burst read: send reg | 0x80, then clock out len bytes */
    uint8_t tx[257];
    uint8_t rx[257];

    tx[0] = (uint8_t)(reg | IMU_SPI_READ_BIT);
    for (uint16_t i = 1; i <= len; i++) {
        tx[i] = 0xFF;  /* Dummy bytes for read */
    }

    imu_spi_cs_low();
    imu_spi_xfer(tx, rx, len + 1);
    imu_spi_cs_high();

    memcpy(buf, &rx[1], len);
}

/* ---- Initialization ---- */
int imu_init(void)
{
    /* Configure SPI pins: SCLK, MOSI, MISO, CS */
    /* In production: configure SPIM2 peripheral via nRF registers */

    /* Hardware reset via software: set DEVICE_CONFIG.SOFT_RESET_CONFIG = 1 */
    imu_write_reg(IMU_REG_DEVICE_CONFIG, 0x01);
    /* Wait 1 ms for reset to complete */
    /* board_delay_ms(1); */

    /* Check WHO_AM_I */
    uint8_t whoami = imu_read_reg(IMU_REG_WHO_AM_I);
    if (whoami != IMU_WHOAMI_ICM42688P) {
        return -1;  /* IMU not detected */
    }

    /* Configure SPI interface: 4-wire, MSB first, Mode 0 */
    /* Set DRIVE_CONFIG for optimal SPI timing */
    imu_write_reg(IMU_REG_DRIVE_CONFIG, 0x0F);

    /* Clear any pending interrupts */
    (void)imu_read_reg(IMU_REG_INT_STATUS);

    return 0;
}

/* ---- Start continuous sampling ---- */
void imu_start_sampling(uint16_t odr_hz, uint8_t accel_fs, uint8_t gyro_fs,
                         uint16_t fifo_watermark)
{
    /* Configure accelerometer: ODR + full-scale range */
    /* ACCEL_CONFIG0 = [UI_FILT_BW(2) | FS(2) | ODR(4)] */
    uint8_t acc_config = 0;
    switch (accel_fs) {
    case 16: acc_config |= IMU_ACCEL_FS_16G; break;
    case 8:  acc_config |= IMU_ACCEL_FS_8G;  break;
    case 4:  acc_config |= IMU_ACCEL_FS_4G;  break;
    case 2:  acc_config |= IMU_ACCEL_FS_2G;  break;
    default: acc_config |= IMU_ACCEL_FS_16G; break;
    }

    /* ODR selection (lower nibble) */
    switch (odr_hz) {
    case 1000: acc_config |= IMU_ACCEL_ODR_1K; break;
    /* Add other rates as needed */
    default:   acc_config |= IMU_ACCEL_ODR_1K; break;
    }

    /* Configure gyroscope similarly */
    uint8_t gyro_config = 0;
    switch (gyro_fs) {
    case 2000: gyro_config |= IMU_GYRO_FS_2000DPS; break;
    case 1000: gyro_config |= IMU_GYRO_FS_1000DPS; break;
    case 500:  gyro_config |= IMU_GYRO_FS_500DPS;  break;
    default:   gyro_config |= IMU_GYRO_FS_2000DPS; break;
    }
    gyro_config |= IMU_GYRO_ODR_1K;

    /* Enable FIFO in stream mode with both accel + gyro */
    /* FIFO_CONFIG1: stream mode, watermark = 50 samples */
    uint8_t fifo_cfg = IMU_FIFO_MODE_STREAM | (fifo_watermark & 0x3F);
    imu_write_reg(IMU_REG_FIFO_CONFIG, fifo_cfg);

    /* Configure interrupt: FIFO watermark on INT1, push-pull, active high */
    imu_write_reg(IMU_REG_INT_CONFIG,
                  IMU_INT_PUSH_PULL | IMU_INT_ACTIVE_HIGH | IMU_INT_LATCHED);

    /* Enable FIFO watermark interrupt in INT_SOURCE0 */
    imu_write_reg(IMU_REG_INT_SOURCE0, IMU_INT_FIFO_WM_EN | IMU_INT_FIFO_WM_INT1);

    /* Set watermark in FIFO_CONFIG1 (lower 6 bits) */
    imu_write_reg(IMU_REG_FIFO_CONFIG1, fifo_watermark & 0x3F);

    /* Power management: enable both accel and gyro in low-noise mode */
    imu_write_reg(IMU_REG_PWR_MGMT0, IMU_PWR_LOW_NOISE);

    /* Write accel and gyro configs */
    imu_write_reg(IMU_REG_ACCEL_CONFIG0, acc_config);
    imu_write_reg(IMU_REG_GYRO_CONFIG0, gyro_config);

    /* Update sensitivity factors */
    switch (accel_fs) {
    case 16: current_acc_sens = IMU_ACC_SENS_16G_LSB; break;
    case 8:  current_acc_sens = 4096.0f; break;
    case 4:  current_acc_sens = 8192.0f; break;
    case 2:  current_acc_sens = 16384.0f; break;
    default: current_acc_sens = IMU_ACC_SENS_16G_LSB; break;
    }

    switch (gyro_fs) {
    case 2000: current_gyro_sens = IMU_GYRO_SENS_2000_LSB; break;
    case 1000: current_gyro_sens = 32.8f; break;
    case 500:  current_gyro_sens = 65.5f; break;
    default:   current_gyro_sens = IMU_GYRO_SENS_2000_LSB; break;
    }
}

void imu_stop_sampling(void)
{
    /* Put accel and gyro in sleep mode */
    imu_write_reg(IMU_REG_PWR_MGMT0, IMU_PWR_SLEEP);
}

/* ---- FIFO read ---- */
uint16_t imu_read_fifo(uint8_t *buf, uint16_t max_len)
{
    /* Read FIFO count */
    uint8_t count_h = imu_read_reg(IMU_REG_FIFO_COUNTH);
    uint8_t count_l = imu_read_reg(IMU_REG_FIFO_COUNTL);
    uint16_t fifo_count = ((uint16_t)(count_h & 0x0F) << 8) | count_l;

    if (fifo_count == 0) return 0;
    if (fifo_count > max_len) fifo_count = max_len;

    /* Burst read from FIFO_DATA register */
    imu_read_regs(IMU_REG_FIFO_DATA, buf, fifo_count);

    return fifo_count;
}

/* ---- Interrupt callback ---- */
void imu_register_irq_callback(imu_irq_callback_t cb)
{
    irq_callback = cb;

    /* Configure IMU_IRQ_PIN as input with rising edge detection
     * via GPIOTE. In production: configure GPIOTE channel.
     */
}

/* ---- Self-test ---- */
int imu_self_test(void)
{
    /* Read factory self-test result from SIGNAL_PATH_RESET */
    /* For ICM-42688-P: set bit 2 of SIGNAL_PATH_RESET, wait, read result */
    imu_write_reg(IMU_REG_SIGNAL_PATH_RESET, 0x04);
    /* Wait 200 ms for self-test to complete */
    /* board_delay_ms(200); */
    uint8_t result = imu_read_reg(IMU_REG_SIGNAL_PATH_RESET);
    /* If bit 2 is cleared, self-test passed */
    return ((result & 0x04) == 0) ? 0 : -1;
}

uint8_t imu_who_am_i(void)
{
    return imu_read_reg(IMU_REG_WHO_AM_I);
}

float imu_get_accel_sensitivity(void)
{
    return current_acc_sens;
}

float imu_get_gyro_sensitivity(void)
{
    return current_gyro_sens;
}

void imu_enter_sleep(void)
{
    imu_write_reg(IMU_REG_PWR_MGMT0, IMU_PWR_SLEEP);
}

void imu_wake_from_sleep(void)
{
    imu_write_reg(IMU_REG_PWR_MGMT0, IMU_PWR_LOW_NOISE);
}

void imu_configure_wake_on_motion(uint16_t threshold_mg)
{
    /* Enable wake-on-motion with specified threshold
     * This puts the IMU in ultra-low-power mode (~0.6 µA)
     * and generates an interrupt when acceleration exceeds threshold.
     */
    (void)threshold_mg;  /* Configure WOM registers in production */
}

/* ---- End of imu_drv.c ---- */