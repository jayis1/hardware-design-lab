/*
 * imu.c — ICM-42688-P 6-channel IMU SPI driver implementation.
 *
 * Manages the shared SPI0 bus with 6 ICM-42688-P chips (5 fingers + wrist).
 * Uses round-robin DMA reads at 500 Hz with per-chip CS lines.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/imu.h"

/* -------------------------------------------------------------------------
 * SPI0 DMA buffers
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static uint8_t spi_tx_buf[16];
static uint8_t spi_rx_buf[16];

/* Chip select pin array (indices into P0) */
static const uint32_t cs_pins[NUM_IMUS] = {
    PIN_IMU_CS0, PIN_IMU_CS1, PIN_IMU_CS2,
    PIN_IMU_CS3, PIN_IMU_CS4, PIN_IMU_CS5
};

/* IMU enable state */
static int imu_enabled = 0;

/* -------------------------------------------------------------------------
 * Low-level SPI0 transfer (blocking, no DMA for simplicity in init)
 * Transfers tx_buf → rx_buf, len bytes.
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void spi0_transfer(uint8_t *tx, uint8_t *rx, uint16_t len)
{
    SPIM0->TXD_PTR = (uint32_t)tx;
    SPIM0->TXD_MAXCNT = len;
    SPIM0->RXD_PTR = (uint32_t)rx;
    SPIM0->RXD_MAXCNT = len;
    SPIM0->EVENTS_END = 0;
    SPIM0->TASKS_START = 1;
    while (SPIM0->EVENTS_END == 0)
        ;
    SPIM0->EVENTS_END = 0;
}

/* -------------------------------------------------------------------------
 * Assert/deassert chip select for a specific IMU
 * ------------------------------------------------------------------------- */
static void imu_cs_low(uint8_t chip)
{
    P0->OUTCLR = (1U << cs_pins[chip]);
}

static void imu_cs_high(uint8_t chip)
{
    P0->OUTSET = (1U << cs_pins[chip]);
}

/* -------------------------------------------------------------------------
 * Write a register to a specific ICM-42688-P chip
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void imu_write_reg(uint8_t chip, uint8_t reg, uint8_t value)
{
    imu_cs_low(chip);
    spi_tx_buf[0] = reg & 0x7F;  /* bit7=0 for write */
    spi_tx_buf[1] = value;
    spi0_transfer(spi_tx_buf, spi_rx_buf, 2);
    imu_cs_high(chip);
}

/* -------------------------------------------------------------------------
 * Read a register from a specific ICM-42688-P chip
 * ------------------------------------------------------------------------- */
static uint8_t imu_read_reg(uint8_t chip, uint8_t reg)
{
    imu_cs_low(chip);
    spi_tx_buf[0] = reg | 0x80;  /* bit7=1 for read */
    spi_tx_buf[1] = 0x00;        /* dummy */
    spi0_transfer(spi_tx_buf, spi_rx_buf, 2);
    imu_cs_high(chip);
    return spi_rx_buf[1];
}

/* -------------------------------------------------------------------------
 * Read a burst of registers (for accel/gyro data, 12 bytes + temp)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void imu_read_burst(uint8_t chip, uint8_t start_reg,
                            uint8_t *buf, uint16_t len)
{
    imu_cs_low(chip);
    spi_tx_buf[0] = start_reg | 0x80;  /* read */
    /* Fill TX with dummy bytes */
    for (uint16_t i = 1; i <= len; i++) {
        spi_tx_buf[i] = 0x00;
    }
    spi0_transfer(spi_tx_buf, spi_rx_buf, len + 1);
    imu_cs_high(chip);
    /* Copy data (skip the first byte which is dummy) */
    memcpy(buf, &spi_rx_buf[1], len);
}

/* -------------------------------------------------------------------------
 * Initialize SPI0 bus
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static int spi0_init(void)
{
    /* Enable SPIM0 */
    SPIM0->ENABLE = 0;
    SPIM0->PSEL_SCK = PIN_SPIM0_SCK;
    SPIM0->PSEL_MOSI = PIN_SPIM0_MOSI;
    SPIM0->PSEL_MISO = PIN_SPIM0_MISO;
    /* Configure GPIO pins for SPI AF */
    P0->PIN_CNF[PIN_SPIM0_SCK]  = GPIO_CNF_DIR_OUTPUT | GPIO_CNF_S0S1;
    P0->PIN_CNF[PIN_SPIM0_MOSI] = GPIO_CNF_DIR_OUTPUT | GPIO_CNF_S0S1;
    P0->PIN_CNF[PIN_SPIM0_MISO] = GPIO_CNF_DIR_INPUT | GPIO_CNF_PULL_DOWN;
    /* SPI mode 0, MSB first, 8 MHz */
    SPIM0->CONFIG = SPIM_CONFIG_ORDER_MSB | SPIM_CONFIG_CPHA_LEAD |
                     SPIM_CONFIG_CPOL_LOW;
    SPIM0->FREQUENCY = SPIM_FREQ_8M;
    SPIM0->ENABLE = SPIM_ENABLE_ENABLE;
    return 0;
}

/* -------------------------------------------------------------------------
 * Initialize all 6 ICM-42688-P IMUs
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int imu_init(void)
{
    if (spi0_init() != 0)
        return -1;

    /* All CS lines high (deselected) */
    for (int i = 0; i < NUM_IMUS; i++) {
        imu_cs_high(i);
    }

    /* Small delay for SPI bus settle */
    for (volatile int i = 0; i < 1000; i++);

    /* Configure each IMU chip */
    for (int chip = 0; chip < NUM_IMUS; chip++) {
        /* Verify chip identity */
        uint8_t whoami = imu_read_reg(chip, ICM_REG_WHO_AM_I);
        if (whoami != 0x47) {
            /* ICM-42688-P WHO_AM_I should be 0x47 */
            return -(chip + 1);
        }

        /* Reset the chip */
        imu_write_reg(chip, ICM_REG_PWR_MGMT0, 0x00);
        for (volatile int i = 0; i < 10000; i++);

        /* Configure accelerometer: ±16g, 500 Hz ODR */
        imu_write_reg(chip, ICM_REG_ACCEL_CONFIG0,
                       ICM_ACCEL_CONFIG_16G_500HZ);

        /* Configure gyroscope: ±2000 dps, 500 Hz ODR */
        imu_write_reg(chip, ICM_REG_GYRO_CONFIG0,
                       ICM_GYRO_CONFIG_2000DPS_500HZ);

        /* Configure internal sample rate */
        imu_write_reg(chip, ICM_REG_CONFIG0, 0x06);  /* UI filter config */

        /* Enable low-noise mode (gyro + accel on) */
        imu_write_reg(chip, ICM_REG_PWR_MGMT0, ICM_PWR_LOW_NOISE);

        /* Small delay between chips */
        for (volatile int i = 0; i < 1000; i++);
    }

    imu_enabled = 1;
    return 0;
}

/* -------------------------------------------------------------------------
 * Enable/disable all IMUs (power management)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void imu_enable(int enable)
{
    if (enable && !imu_enabled) {
        for (int chip = 0; chip < NUM_IMUS; chip++) {
            imu_write_reg(chip, ICM_REG_PWR_MGMT0, ICM_PWR_LOW_NOISE);
        }
        imu_enabled = 1;
    } else if (!enable && imu_enabled) {
        for (int chip = 0; chip < NUM_IMUS; chip++) {
            imu_write_reg(chip, ICM_REG_PWR_MGMT0, ICM_PWR_OFF);
        }
        imu_enabled = 0;
    }
}

/* -------------------------------------------------------------------------
 * Read all 6 IMUs (round-robin SPI burst read)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int imu_read_all(imu_sample_t *samples)
{
    uint8_t raw[14];  /* 6 accel + 6 gyro + 2 temp = 14 bytes */

    for (int chip = 0; chip < NUM_IMUS; chip++) {
        /* Read accel + gyro + temp in one burst (0x1F to 0x2C = 14 bytes) */
        imu_read_burst(chip, ICM_REG_ACCEL_DATA_X1, raw, 14);

        /* Parse accelerometer (big-endian, 16-bit signed) */
        samples[chip].accel[0] = (int16_t)((raw[0] << 8) | raw[1]);
        samples[chip].accel[1] = (int16_t)((raw[2] << 8) | raw[3]);
        samples[chip].accel[2] = (int16_t)((raw[4] << 8) | raw[5]);

        /* Parse gyroscope (big-endian, 16-bit signed) */
        /* raw[6:7] is temp_data, gyro starts at offset 8 */
        samples[chip].temp = (int16_t)((raw[6] << 8) | raw[7]);
        samples[chip].gyro[0] = (int16_t)((raw[8] << 8) | raw[9]);
        samples[chip].gyro[1] = (int16_t)((raw[10] << 8) | raw[11]);
        samples[chip].gyro[2] = (int16_t)((raw[12] << 8) | raw[13]);

        samples[chip].timestamp = 0;  /* filled by caller */
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Gyroscope bias calibration (static hand, 512 samples ~1 second)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int imu_calibrate_gyro(int16_t bias_out[NUM_IMUS][3])
{
    int32_t accum[NUM_IMUS][3] = {{0}};
    imu_sample_t samples[NUM_IMUS];

    for (int n = 0; n < 512; n++) {
        if (imu_read_all(samples) != 0)
            return -1;
        for (int c = 0; c < NUM_IMUS; c++) {
            for (int axis = 0; axis < 3; axis++) {
                accum[c][axis] += samples[c].gyro[axis];
            }
        }
        /* Delay ~2 ms between samples */
        for (volatile int i = 0; i < 5000; i++);
    }

    for (int c = 0; c < NUM_IMUS; c++) {
        for (int axis = 0; axis < 3; axis++) {
            bias_out[c][axis] = (int16_t)(accum[c][axis] / 512);
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Accelerometer bias calibration (hand flat, palm down)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int imu_calibrate_accel(int16_t bias_out[NUM_IMUS][3])
{
    int32_t accum[NUM_IMUS][3] = {{0}};
    imu_sample_t samples[NUM_IMUS];

    /* Expected: Z = +1g, X = 0, Y = 0 (flat, palm down) */
    for (int n = 0; n < 512; n++) {
        if (imu_read_all(samples) != 0)
            return -1;
        for (int c = 0; c < NUM_IMUS; c++) {
            for (int axis = 0; axis < 3; axis++) {
                accum[c][axis] += samples[c].accel[axis];
            }
        }
        for (volatile int i = 0; i < 5000; i++);
    }

    /* 1g at ±16g range, 16-bit: 1g = 2048 LSB */
    for (int c = 0; c < NUM_IMUS; c++) {
        bias_out[c][0] = (int16_t)(accum[c][0] / 512);           /* X bias */
        bias_out[c][1] = (int16_t)(accum[c][1] / 512);           /* Y bias */
        bias_out[c][2] = (int16_t)((accum[c][2] / 512) - 2048);  /* Z - 1g */
    }

    return 0;
}

/*
 * Author: jayis1
 * End of imu.c
 */