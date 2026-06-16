/**
 * @file    ads1120.c
 * @brief   Terramesh — TI ADS1120 16-bit ADC driver for pore pressure sensors
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1
 * @license GPL-2.0
 *
 * I²C driver for the ADS1120 16-bit, 2-channel, delta-sigma ADC with
 * integrated PGA. Used to read the MEMS piezoresistive pore pressure
 * sensors at two depths.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../board.h"
#include "../registers.h"

/* ADS1120 register map */
#define ADS1120_REG_CONFIG0      0x00
#define ADS1120_REG_CONFIG1      0x01
#define ADS1120_REG_CONFIG2      0x02
#define ADS1120_REG_CONFIG3      0x03
#define ADS1120_REG_DATA         0x00  /* Read-only, 2 bytes */

/* CONFIG0 bits */
#define ADS1120_CFG0_MUX_POS     0
#define ADS1120_CFG0_MUX_MASK    (0x07UL << ADS1120_CFG0_MUX_POS)
#define ADS1120_CFG0_MUX_AIN0_AIN1  0x00  /* Default differential */
#define ADS1120_CFG0_MUX_AIN0_AIN2  0x01
#define ADS1120_CFG0_MUX_AIN1_AIN3  0x02
#define ADS1120_CFG0_MUX_AIN2_AIN3  0x03
#define ADS1120_CFG0_MUX_AIN0_GND   0x04  /* Single-ended */
#define ADS1120_CFG0_MUX_AIN1_GND   0x05
#define ADS1120_CFG0_MUX_AIN2_GND   0x06
#define ADS1120_CFG0_MUX_AIN3_GND   0x07
#define ADS1120_CFG0_GAIN_POS    4
#define ADS1120_CFG0_GAIN_MASK   (0x07UL << ADS1120_CFG0_GAIN_POS)
#define ADS1120_CFG0_GAIN_1      0x00
#define ADS1120_CFG0_GAIN_2      0x01
#define ADS1120_CFG0_GAIN_4      0x02
#define ADS1120_CFG0_GAIN_8      0x03
#define ADS1120_CFG0_GAIN_16     0x04
#define ADS1120_CFG0_GAIN_32     0x05
#define ADS1120_CFG0_GAIN_64     0x06
#define ADS1120_CFG0_GAIN_128    0x07

/* CONFIG1 bits */
#define ADS1120_CFG1_DR_POS      0
#define ADS1120_CFG1_DR_MASK     (0x07UL << ADS1120_CFG1_DR_POS)
#define ADS1120_CFG1_DR_20SPS    0x00
#define ADS1120_CFG1_DR_45SPS    0x01
#define ADS1120_CFG1_DR_90SPS    0x02
#define ADS1120_CFG1_DR_175SPS   0x03
#define ADS1120_CFG1_DR_330SPS   0x04
#define ADS1120_CFG1_DR_600SPS   0x05
#define ADS1120_CFG1_DR_1000SPS  0x06
#define ADS1120_CFG1_DR_2000SPS  0x07
#define ADS1120_CFG1_MODE_POS    3
#define ADS1120_CFG1_MODE_MASK   (0x03UL << ADS1120_CFG1_MODE_POS)
#define ADS1120_CFG1_MODE_NORMAL     0x00
#define ADS1120_CFG1_MODE_DUTY_CYCLE 0x01
#define ADS1120_CFG1_MODE_TURBO      0x02
#define ADS1120_CFG1_CM_POS      7
#define ADS1120_CFG1_CM_CONTINUOUS   (0UL << ADS1120_CFG1_CM_POS)
#define ADS1120_CFG1_CM_SINGLE       (1UL << ADS1120_CFG1_CM_POS)

/* CONFIG2 bits */
#define ADS1120_CFG2_VREF_POS    4
#define ADS1120_CFG2_VREF_MASK   (0x03UL << ADS1120_CFG2_VREF_POS)
#define ADS1120_CFG2_VREF_INTERNAL  0x00  /* 2.048 V */
#define ADS1120_CFG2_VREF_REFP0_REFN0 0x01
#define ADS1120_CFG2_VREF_AVDD_AVSS  0x02
#define ADS1120_CFG2_VREF_REFP1_REFN1 0x03
#define ADS1120_CFG2_PSW          (1UL << 3)  /* Power switch */
#define ADS1120_CFG2_IDAC_POS    0
#define ADS1120_CFG2_IDAC_MASK   (0x07UL << ADS1120_CFG2_IDAC_POS)

/* CONFIG3 bits */
#define ADS1120_CFG3_BCS         (1UL << 6)  /* Burn-out current source */
#define ADS1120_CFG3_CRC         (1UL << 5)  /* CRC enable */

/* I²C command bytes */
#define ADS1120_CMD_RESET        0x06
#define ADS1120_CMD_START_SYNC   0x08
#define ADS1120_CMD_PWRDWN       0x02
#define ADS1120_CMD_RDATA        0x10
#define ADS1120_CMD_RREG         0x20
#define ADS1120_CMD_WREG         0x40

/* ======================================================================== *
 *  Private helpers                                                             *
 * ======================================================================== */

static bool ads1120_i2c_write(uint8_t dev_addr, uint8_t reg, uint8_t val) {
    uint8_t data[] = { reg, val };
    return i2c2_write(dev_addr, data, 2);
}

static bool ads1120_i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *val) {
    if (!i2c2_write(dev_addr, &reg, 1)) return false;
    return i2c2_read(dev_addr, val, 1);
}

/* ======================================================================== *
 *  Public API                                                                  *
 * ======================================================================== */

bool ads1120_init(uint8_t dev_addr, uint8_t gain, uint8_t data_rate) {
    /* Reset device */
    uint8_t reset_cmd[] = { ADS1120_CMD_RESET };
    if (!i2c2_write(dev_addr, reset_cmd, 1)) return false;

    /* Wait for power-up */
    for (volatile uint32_t i = 0; i < 10000; i++) { __asm__("nop"); }

    /* Configure CONFIG0: MUX = AIN0/AIN1 (differential), gain */
    uint8_t cfg0 = (ADS1120_CFG0_MUX_AIN0_AIN1) |
                   ((gain << ADS1120_CFG0_GAIN_POS) & ADS1120_CFG0_GAIN_MASK);
    if (!ads1120_i2c_write(dev_addr, ADS1120_REG_CONFIG0, cfg0)) return false;

    /* Configure CONFIG1: data rate, normal mode, continuous conversion */
    uint8_t cfg1 = (data_rate & ADS1120_CFG1_DR_MASK) |
                   ADS1120_CFG1_MODE_NORMAL |
                   ADS1120_CFG1_CM_CONTINUOUS;
    if (!ads1120_i2c_write(dev_addr, ADS1120_REG_CONFIG1, cfg1)) return false;

    /* Configure CONFIG2: internal VREF (2.048 V), power switch enabled */
    uint8_t cfg2 = ADS1120_CFG2_VREF_INTERNAL | ADS1120_CFG2_PSW;
    if (!ads1120_i2c_write(dev_addr, ADS1120_REG_CONFIG2, cfg2)) return false;

    /* Configure CONFIG3: no burn-out, no CRC */
    if (!ads1120_i2c_write(dev_addr, ADS1120_REG_CONFIG3, 0x00)) return false;

    /* Start continuous conversion */
    uint8_t start_cmd[] = { ADS1120_CMD_START_SYNC };
    if (!i2c2_write(dev_addr, start_cmd, 1)) return false;

    return true;
}

bool ads1120_read_raw(uint8_t dev_addr, int16_t *raw) {
    uint8_t rx[2];

    /* Read data register */
    uint8_t rdata_cmd[] = { ADS1120_CMD_RDATA };
    if (!i2c2_write(dev_addr, rdata_cmd, 1)) return false;
    if (!i2c2_read(dev_addr, rx, 2)) return false;

    *raw = (int16_t)((rx[0] << 8) | rx[1]);
    return true;
}

bool ads1120_read_voltage(uint8_t dev_addr, float *voltage, uint8_t gain) {
    int16_t raw;
    if (!ads1120_read_raw(dev_addr, &raw)) return false;

    /* VREF = 2.048 V, 16-bit, PGA gain */
    float vref = 2.048f;
    float lsb = vref / 32768.0f;  /* 16-bit, bipolar */
    *voltage = (float)raw * lsb / (float)(1UL << gain);

    return true;
}

bool ads1120_read_pressure(uint8_t dev_addr, float *pressure_kpa, uint8_t gain) {
    float voltage;
    if (!ads1120_read_voltage(dev_addr, &voltage, gain)) return false;

    /* MEMS piezoresistive sensor: 0–500 kPa, output 0–2.048 V at PGA=1 */
    /* With PGA gain, the effective range scales */
    float vref = 2.048f;
    float max_voltage = vref / (float)(1UL << gain);
    *pressure_kpa = (voltage / max_voltage) * 500.0f;

    return true;
}

void ads1120_set_mux(uint8_t dev_addr, uint8_t mux_config) {
    uint8_t cfg0;
    if (ads1120_i2c_read(dev_addr, ADS1120_REG_CONFIG0, &cfg0)) {
        cfg0 = (cfg0 & ~ADS1120_CFG0_MUX_MASK) |
               ((mux_config << ADS1120_CFG0_MUX_POS) & ADS1120_CFG0_MUX_MASK);
        ads1120_i2c_write(dev_addr, ADS1120_REG_CONFIG0, cfg0);
    }
}

void ads1120_set_gain(uint8_t dev_addr, uint8_t gain) {
    uint8_t cfg0;
    if (ads1120_i2c_read(dev_addr, ADS1120_REG_CONFIG0, &cfg0)) {
        cfg0 = (cfg0 & ~ADS1120_CFG0_GAIN_MASK) |
               ((gain << ADS1120_CFG0_GAIN_POS) & ADS1120_CFG0_GAIN_MASK);
        ads1120_i2c_write(dev_addr, ADS1120_REG_CONFIG0, cfg0);
    }
}

void ads1120_power_down(uint8_t dev_addr) {
    uint8_t cmd[] = { ADS1120_CMD_PWRDWN };
    i2c2_write(dev_addr, cmd, 1);
}

bool ads1120_self_test(uint8_t dev_addr) {
    /* Read CONFIG registers to verify communication */
    uint8_t cfg0, cfg1, cfg2, cfg3;
    if (!ads1120_i2c_read(dev_addr, ADS1120_REG_CONFIG0, &cfg0)) return false;
    if (!ads1120_i2c_read(dev_addr, ADS1120_REG_CONFIG1, &cfg1)) return false;
    if (!ads1120_i2c_read(dev_addr, ADS1120_REG_CONFIG2, &cfg2)) return false;
    if (!ads1120_i2c_read(dev_addr, ADS1120_REG_CONFIG3, &cfg3)) return false;

    /* Verify default values (after reset) */
    return (cfg0 == 0x00 && cfg1 == 0x00 && cfg2 == 0x10 && cfg3 == 0x00);
}
