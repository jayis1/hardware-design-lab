/**
 * @file    scl3300.c
 * @brief   Terramesh — Murata SCL3300 3-axis inclinometer driver
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1
 * @license GPL-2.0
 *
 * SPI mode 3 driver for the SCL3300 high-precision inclinometer.
 * Provides tilt angle readout with 0.001° resolution, self-test,
 * and filter configuration.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"
#include "../registers.h"

/* SCL3300 register addresses */
#define SCL3300_REG_ANGLE_X        0x04
#define SCL3300_REG_ANGLE_Y        0x06
#define SCL3300_REG_ANGLE_Z        0x08
#define SCL3300_REG_TEMP           0x0C
#define SCL3300_REG_STATUS         0x0E
#define SCL3300_REG_SELFTEST       0x10
#define SCL3300_REG_OPMODE         0x12
#define SCL3300_REG_SENSITIVITY    0x14
#define SCL3300_REG_FILTER         0x16
#define SCL3300_REG_OFFSET_X       0x18
#define SCL3300_REG_OFFSET_Y       0x1A
#define SCL3300_REG_OFFSET_Z       0x1C
#define SCL3300_REG_WHOAMI         0x40

/* SCL3300 command bits */
#define SCL3300_READ_CMD           0x00
#define SCL3300_WRITE_CMD          0x40
#define SCL3300_AUTO_INC           0x80

/* SCL3300 operating modes */
#define SCL3300_MODE_SLEEP         0x00
#define SCL3300_MODE_STANDBY       0x01
#define SCL3300_MODE_MEASURE       0x02

/* SCL3300 status bits */
#define SCL3300_STATUS_DRDY        (1UL << 0)
#define SCL3300_STATUS_ERR         (1UL << 2)
#define SCL3300_STATUS_SELFTEST_OK (1UL << 3)

/* SCL3300 WHOAMI value */
#define SCL3300_WHOAMI_VAL         0xC3

/* ======================================================================== *
 *  Private helpers                                                             *
 * ======================================================================== */

static uint16_t scl3300_spi_transfer16(uint16_t tx) {
    uint8_t tx_h = (uint8_t)(tx >> 8);
    uint8_t tx_l = (uint8_t)(tx);
    uint8_t rx_h, rx_l;

    while (!(REG_READ(SPI_SR(SPI2_BASE)) & SPI_SR_TXE)) { __asm__("nop"); }
    REG_WRITE(SPI_DR(SPI2_BASE), tx_h);
    while (!(REG_READ(SPI_SR(SPI2_BASE)) & SPI_SR_RXNE)) { __asm__("nop"); }
    rx_h = (uint8_t)REG_READ(SPI_DR(SPI2_BASE));

    while (!(REG_READ(SPI_SR(SPI2_BASE)) & SPI_SR_TXE)) { __asm__("nop"); }
    REG_WRITE(SPI_DR(SPI2_BASE), tx_l);
    while (!(REG_READ(SPI_SR(SPI2_BASE)) & SPI_SR_RXNE)) { __asm__("nop"); }
    rx_l = (uint8_t)REG_READ(SPI_DR(SPI2_BASE));

    return ((uint16_t)rx_h << 8) | rx_l;
}

static uint16_t scl3300_read_reg(uint8_t reg) {
    uint16_t cmd = (SCL3300_READ_CMD | reg) << 8;
    uint16_t rx;

    GPIO_CLR_PIN(GPIOB_BASE, PIN_SCL3300_CS);
    rx = scl3300_spi_transfer16(cmd);
    GPIO_SET_PIN(GPIOB_BASE, PIN_SCL3300_CS);

    return rx;
}

static bool scl3300_write_reg(uint8_t reg, uint16_t val) {
    uint16_t cmd = ((SCL3300_WRITE_CMD | reg) << 8) | (val & 0xFF);

    GPIO_CLR_PIN(GPIOB_BASE, PIN_SCL3300_CS);
    scl3300_spi_transfer16(cmd);
    GPIO_SET_PIN(GPIOB_BASE, PIN_SCL3300_CS);

    /* Verify by reading back */
    uint16_t check = scl3300_read_reg(reg);
    return (check == val);
}

/* ======================================================================== *
 *  Public API                                                                  *
 * ======================================================================== */

bool scl3300_init(void) {
    /* Verify WHOAMI */
    uint16_t whoami = scl3300_read_reg(SCL3300_REG_WHOAMI);
    if (whoami != SCL3300_WHOAMI_VAL) {
        return false;
    }

    /* Set operating mode to measurement */
    if (!scl3300_write_reg(SCL3300_REG_OPMODE, SCL3300_MODE_MEASURE)) {
        return false;
    }

    /* Set filter to 10 Hz low-pass */
    if (!scl3300_write_reg(SCL3300_REG_FILTER, 0x0002)) {
        return false;
    }

    /* Wait for first data ready */
    for (volatile uint32_t i = 0; i < 50000; i++) {
        uint16_t status = scl3300_read_reg(SCL3300_REG_STATUS);
        if (status & SCL3300_STATUS_DRDY) break;
        __asm__("nop");
    }

    return true;
}

bool scl3300_self_test(void) {
    /* Initiate self-test */
    if (!scl3300_write_reg(SCL3300_REG_SELFTEST, 0x0001)) {
        return false;
    }

    /* Wait for completion */
    for (volatile uint32_t i = 0; i < 100000; i++) {
        uint16_t status = scl3300_read_reg(SCL3300_REG_STATUS);
        if (status & SCL3300_STATUS_SELFTEST_OK) return true;
        if (status & SCL3300_STATUS_ERR) return false;
        __asm__("nop");
    }

    return false;
}

bool scl3300_read_angle_x(float *angle_deg) {
    uint16_t raw = scl3300_read_reg(SCL3300_REG_ANGLE_X);

    /* Check status nibble (bits 14:12) */
    if ((raw & 0x7000) != 0x1000) return false;

    /* Sign-extend from 13-bit */
    int16_t val = (int16_t)(raw & 0x1FFF);
    if (val & 0x1000) val |= 0xF000;

    *angle_deg = (float)val * 0.001f;
    return true;
}

bool scl3300_read_angle_y(float *angle_deg) {
    uint16_t raw = scl3300_read_reg(SCL3300_REG_ANGLE_Y);

    if ((raw & 0x7000) != 0x1000) return false;

    int16_t val = (int16_t)(raw & 0x1FFF);
    if (val & 0x1000) val |= 0xF000;

    *angle_deg = (float)val * 0.001f;
    return true;
}

bool scl3300_read_angle_z(float *angle_deg) {
    uint16_t raw = scl3300_read_reg(SCL3300_REG_ANGLE_Z);

    if ((raw & 0x7000) != 0x1000) return false;

    int16_t val = (int16_t)(raw & 0x1FFF);
    if (val & 0x1000) val |= 0xF000;

    *angle_deg = (float)val * 0.001f;
    return true;
}

bool scl3300_read_all(float *x, float *y, float *z) {
    /* Read all three axes with auto-increment from ANGLE_X */
    uint16_t cmd = ((SCL3300_READ_CMD | SCL3300_REG_ANGLE_X | SCL3300_AUTO_INC) << 8);
    uint16_t rx[3];

    GPIO_CLR_PIN(GPIOB_BASE, PIN_SCL3300_CS);
    scl3300_spi_transfer16(cmd);
    rx[0] = scl3300_spi_transfer16(0x0000);
    rx[1] = scl3300_spi_transfer16(0x0000);
    rx[2] = scl3300_spi_transfer16(0x0000);
    GPIO_SET_PIN(GPIOB_BASE, PIN_SCL3300_CS);

    /* Check status nibble for each */
    for (int i = 0; i < 3; i++) {
        if ((rx[i] & 0x7000) != 0x1000) return false;
    }

    int16_t raw_x = (int16_t)(rx[0] & 0x1FFF);
    int16_t raw_y = (int16_t)(rx[1] & 0x1FFF);
    int16_t raw_z = (int16_t)(rx[2] & 0x1FFF);

    if (raw_x & 0x1000) raw_x |= 0xF000;
    if (raw_y & 0x1000) raw_y |= 0xF000;
    if (raw_z & 0x1000) raw_z |= 0xF000;

    *x = (float)raw_x * 0.001f;
    *y = (float)raw_y * 0.001f;
    *z = (float)raw_z * 0.001f;

    return true;
}

bool scl3300_read_temperature(float *temp_c) {
    uint16_t raw = scl3300_read_reg(SCL3300_REG_TEMP);

    /* Temperature: 0.1°C/LSB, offset 0 at 25°C */
    int16_t val = (int16_t)(raw & 0x1FFF);
    if (val & 0x1000) val |= 0xF000;

    *temp_c = 25.0f + (float)val * 0.1f;
    return true;
}

void scl3300_set_offset_x(float offset_deg) {
    int16_t raw = (int16_t)(offset_deg * 1000.0f);
    scl3300_write_reg(SCL3300_REG_OFFSET_X, (uint16_t)raw);
}

void scl3300_set_offset_y(float offset_deg) {
    int16_t raw = (int16_t)(offset_deg * 1000.0f);
    scl3300_write_reg(SCL3300_REG_OFFSET_Y, (uint16_t)raw);
}

void scl3300_set_filter(uint8_t bw_code) {
    /* bw_code: 0=50Hz, 1=25Hz, 2=10Hz, 3=5Hz, 4=2Hz, 5=1Hz */
    scl3300_write_reg(SCL3300_REG_FILTER, (uint16_t)bw_code);
}

void scl3300_sleep(void) {
    scl3300_write_reg(SCL3300_REG_OPMODE, SCL3300_MODE_SLEEP);
}

void scl3300_wake(void) {
    scl3300_write_reg(SCL3300_REG_OPMODE, SCL3300_MODE_MEASURE);
}
