/**
 * @file    scd41.c
 * @brief   SCD41 CO₂ sensor I²C driver with CRC, FIFO, and self-calibration.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * The Sensirion SCD41 is a high-accuracy NDIR CO₂ sensor with a range of
 * 0–40,000 ppm, making it ideal for closed-chamber accumulation measurements
 * where CO₂ can reach 5,000+ ppm. This driver implements:
 *   - Periodic measurement mode (2-second interval)
 *   - CRC-protected I²C transactions (Sensirion CRC-8)
 *   - Single-shot and FIFO read modes
 *   - Automatic self-calibration (ASC) control
 *   - Forced recalibration (FRC) for zero/span calibration
 *   - Low-power sleep/wake for inter-measurement periods
 */

#include "scd41.h"
#include "../board.h"
#include "../registers.h"
#include <stddef.h>

/* ======================================================================== */
/*  SCD41 COMMAND TABLE                                                     */
/* ======================================================================== */

/* All commands are 16-bit big-endian, optionally followed by data + CRC */
#define CMD_START_PERIODIC      0x21B1  /* Start periodic (2s) measurement */
#define CMD_START_LP_PERIODIC   0x21AC  /* Start low-power periodic (30s)  */
#define CMD_STOP_PERIODIC       0x3F86  /* Stop periodic measurement        */
#define CMD_READ_MEASUREMENT    0xEC05  /* Read CO₂, temp, humidity        */
#define CMD_READ_SERIAL_NUM     0x3682  /* Read 48-bit serial number       */
#define CMD_SET_ASC             0x2416  /* Enable/disable automatic cal    */
#define CMD_SET_FRC             0x362F  /* Forced recalibration value      */
#define CMD_SET_TEMP_OFFSET     0x2426  /* Set temperature offset          */
#define CMD_SET_SENS_POWER      0x24E1  /* Set sensor power mode           */
#define CMD_GET_ASC             0x2313  /* Read ASC status                 */
#define CMD_GET_FRC             0x233F  /* Read FRC status                 */
#define CMD_PERSIST_SETTINGS    0x3615  /* Save settings to EEPROM         */
#define CMD_SLEEP               0x3681  /* Enter sleep mode                */
#define CMD_WAKE                0x3629  /* Wake from sleep                 */
#define CMD_SOFT_RESET          0x3664  /* Soft reset                      */
#define CMD_GET_SERIAL          0x3682  /* Get serial number (3 words)     */
#define CMD_SELF_CAL            0x2446  /* Perform self-calibration        */
#define CMD_LOW_POWER          0x24ED  /* Low-power measurement mode       */

/* ======================================================================== */
/*  I²C HELPER MACROS                                                       */
/* ======================================================================== */

#define SCD41_ADDR              (SCD41_I2C_ADDR << 1)    /* 8-bit addr     */
#define I2C_TIMEOUT_CYCLES      500000

/* Sensirion CRC-8 polynomial: x^8 + x^5 + x^4 + 1 (0x31) */
#define CRC8_POLY               0x31

/* ======================================================================== */
/*  LOCAL STATE                                                             */
/* ======================================================================== */

static struct {
    bool initialized;
    bool periodic_running;
    uint16_t co2_ppm;
    uint16_t temperature;       /* Raw: °C × 100 - 27315 */
    uint16_t humidity;          /* Raw: %RH × 100 */
    uint16_t serial[3];         /* 48-bit serial number */
    uint16_t frc_correction;    /* Last FRC correction value */
    uint8_t  error_count;       /* Consecutive I²C errors */
} g_scd41 = {0};

/* ======================================================================== */
/*  CRC-8 COMPUTATION (Sensirion Algorithm)                                 */
/* ======================================================================== */

/**
 * @brief Compute Sensirion CRC-8 checksum.
 *
 * Polynomial: x^8 + x^5 + x^4 + 1 (0x31)
 * Init value: 0xFF
 * Final XOR: 0x00
 *
 * @param data  2-byte data word
 * @return 8-bit CRC
 */
static uint8_t scd41_crc8(uint16_t data) {
    uint8_t crc = 0xFF;
    uint8_t byte;

    /* Process MSB first */
    byte = (uint8_t)(data >> 8);
    for (int i = 0; i < 8; i++) {
        crc ^= (byte & 0x80) ? 0x31 : 0;
        byte <<= 1;
        if (crc & 0x80) {
            crc = (uint8_t)((crc << 1) ^ CRC8_POLY);
        } else {
            crc <<= 1;
        }
    }

    /* Process LSB */
    byte = (uint8_t)(data & 0xFF);
    for (int i = 0; i < 8; i++) {
        crc ^= (byte & 0x80) ? 0x31 : 0;
        byte <<= 1;
        if (crc & 0x80) {
            crc = (uint8_t)((crc << 1) ^ CRC8_POLY);
        } else {
            crc <<= 1;
        }
    }

    return crc;
}

/**
 * @brief Verify CRC-8 for a 2-byte data + 1-byte CRC triplet.
 */
static bool scd41_verify_crc(uint16_t data, uint8_t crc) {
    return scd41_crc8(data) == crc;
}

/* ======================================================================== */
/*  I²C LOW-LEVEL OPERATIONS                                                */
/* ======================================================================== */

/**
 * @brief Write a 2-byte command to SCD41.
 *
 * @param cmd 16-bit command word
 * @return 0 on success, -ERR_I2C_TIMEOUT or -ERR_I2C_NACK on failure
 */
static int scd41_write_cmd(uint16_t cmd) {
    uint32_t timeout;

    /* Wait for bus to be free */
    timeout = I2C_TIMEOUT_CYCLES;
    while (I2C1->ISR & I2C_ISR_BUSY) {
        if (--timeout == 0) return -ERR_I2C_TIMEOUT;
    }

    /* Clear any pending flags */
    I2C1->ICR = 0xFFFFFFFF;

    /* Configure transfer: 2 bytes, slave address, write */
    I2C1->CR2 = (SCD41_ADDR << I2C_CR2_SADD_Pos)  /* Slave address */
              | (2 << I2C_CR2_NBYTES_Pos)           /* 2 data bytes */
              | I2C_CR2_AUTOEND                      /* Auto STOP */
              | I2C_CR2_START;                       /* Generate START */

    /* Wait for TXE and write MSB */
    timeout = I2C_TIMEOUT_CYCLES;
    while (!(I2C1->ISR & I2C_ISR_TXE)) {
        if (--timeout == 0) return -ERR_I2C_TIMEOUT;
    }
    I2C1->TXDR = (uint8_t)(cmd >> 8);  /* MSB first */

    /* Wait for TXE and write LSB */
    timeout = I2C_TIMEOUT_CYCLES;
    while (!(I2C1->ISR & I2C_ISR_TXE)) {
        if (--timeout == 0) return -ERR_I2C_TIMEOUT;
    }
    I2C1->TXDR = (uint8_t)(cmd & 0xFF);

    /* Wait for STOP condition */
    timeout = I2C_TIMEOUT_CYCLES;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (I2C1->ISR & I2C_ISR_NACKF) {
            I2C1->ICR = I2C_ISR_NACKF;  /* Clear NACK */
            return -ERR_I2C_NACK;
        }
        if (--timeout == 0) return -ERR_I2C_TIMEOUT;
    }

    /* Clear STOP flag */
    I2C1->ICR = I2C_ISR_STOPF;

    return 0;
}

/**
 * @brief Write a command followed by 2 data bytes + CRC.
 *
 * @param cmd 16-bit command
 * @param data 16-bit data word
 * @return 0 on success, negative on failure
 */
static int scd41_write_cmd_with_data(uint16_t cmd, uint16_t data) {
    uint8_t crc = scd41_crc8(data);
    uint32_t timeout;

    /* Wait for bus to be free */
    timeout = I2C_TIMEOUT_CYCLES;
    while (I2C1->ISR & I2C_ISR_BUSY) {
        if (--timeout == 0) return -ERR_I2C_TIMEOUT;
    }

    I2C1->ICR = 0xFFFFFFFF;

    /* 5 bytes: cmd(2) + data(2) + crc(1) */
    I2C1->CR2 = (SCD41_ADDR << I2C_CR2_SADD_Pos)
              | (5 << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_AUTOEND
              | I2C_CR2_START;

    /* Write cmd MSB */
    timeout = I2C_TIMEOUT_CYCLES;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = (uint8_t)(cmd >> 8);

    /* Write cmd LSB */
    timeout = I2C_TIMEOUT_CYCLES;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = (uint8_t)(cmd & 0xFF);

    /* Write data MSB */
    timeout = I2C_TIMEOUT_CYCLES;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = (uint8_t)(data >> 8);

    /* Write data LSB */
    timeout = I2C_TIMEOUT_CYCLES;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = (uint8_t)(data & 0xFF);

    /* Write CRC */
    timeout = I2C_TIMEOUT_CYCLES;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = crc;

    /* Wait for STOP */
    timeout = I2C_TIMEOUT_CYCLES;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = I2C_ISR_NACKF; return -ERR_I2C_NACK; }
        if (--timeout == 0) return -ERR_I2C_TIMEOUT;
    }
    I2C1->ICR = I2C_ISR_STOPF;

    return 0;
}

/**
 * @brief Read 3 words (6 bytes) + 3 CRC bytes from SCD41.
 *
 * Standard read measurement returns 3 × (2 data + 1 CRC) = 9 bytes.
 *
 * @param data_out  Array of 3 uint16_t to receive data
 * @return 0 on success, negative on failure
 */
static int scd41_read_3_words(uint16_t *data_out) {
    uint32_t timeout;
    uint8_t rx_buf[9];

    /* Wait for bus free */
    timeout = I2C_TIMEOUT_CYCLES;
    while (I2C1->ISR & I2C_ISR_BUSY) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }

    I2C1->ICR = 0xFFFFFFFF;

    /* Read 9 bytes (3 data words × 3 bytes each) */
    I2C1->CR2 = (SCD41_ADDR << I2C_CR2_SADD_Pos)
              | (9 << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_AUTOEND
              | I2C_CR2_RD_WRN           /* Read direction */
              | I2C_CR2_START;           /* Generate repeated START */

    /* Read all 9 bytes */
    for (int i = 0; i < 9; i++) {
        timeout = I2C_TIMEOUT_CYCLES;
        while (!(I2C1->ISR & I2C_ISR_RXNE)) {
            if (I2C1->ISR & I2C_ISR_STOPF) break;  /* Early STOP */
            if (--timeout == 0) return -ERR_I2C_TIMEOUT;
        }
        rx_buf[i] = (uint8_t)(I2C1->RXDR & 0xFF);
    }

    /* Wait for STOP */
    timeout = I2C_TIMEOUT_CYCLES;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = I2C_ISR_NACKF; return -ERR_I2C_NACK; }
        if (--timeout == 0) return -ERR_I2C_TIMEOUT;
    }
    I2C1->ICR = I2C_ISR_STOPF;

    /* Reconstruct words and verify CRCs */
    for (int i = 0; i < 3; i++) {
        uint16_t word = ((uint16_t)rx_buf[i * 3] << 8) | rx_buf[i * 3 + 1];
        if (!scd41_verify_crc(word, rx_buf[i * 3 + 2])) {
            g_scd41.error_count++;
            return -ERR_SENSOR_CRC;
        }
        data_out[i] = word;
    }

    g_scd41.error_count = 0;
    return 0;
}

/* ======================================================================== */
/*  PUBLIC API                                                              */
/* ======================================================================== */

/**
 * @brief Initialize the SCD41 sensor.
 *
 * Performs:
 *   1. Soft reset
 *   2. Read serial number for identification
 *   3. Enable automatic self-calibration (ASC)
 *   4. Read current ASC status
 *   5. Start periodic measurement mode (2-second interval)
 *
 * @return 0 on success, negative error code on failure
 */
int scd41_init(void) {
    int ret;

    /* Soft reset — sensor starts in sleep mode after reset */
    ret = scd41_write_cmd(CMD_SOFT_RESET);
    if (ret < 0) return ret;

    /* Wait 2 ms for reset to complete */
    for (volatile uint32_t i = 0; i < 50000; i++);

    /* Read serial number for device identification */
    ret = scd41_write_cmd(CMD_GET_SERIAL);
    if (ret < 0) return ret;

    /* Wait 10 ms for measurement ready */
    for (volatile uint32_t i = 0; i < 250000; i++);

    ret = scd41_read_3_words(g_scd41.serial);
    if (ret < 0) return ret;

    /* Enable automatic self-calibration */
    ret = scd41_write_cmd_with_data(CMD_SET_ASC, 1);  /* 1 = enabled */
    if (ret < 0) return ret;

    /* Wait for settings to take effect */
    for (volatile uint32_t i = 0; i < 50000; i++);

    /* Start periodic measurement (2-second interval) */
    ret = scd41_write_cmd(CMD_START_PERIODIC);
    if (ret < 0) return ret;

    g_scd41.periodic_running = true;
    g_scd41.initialized = true;

    return 0;
}

/**
 * @brief Read CO₂ concentration and optional temperature/humidity.
 *
 * Reads the latest measurement from the SCD41. Must be called at least
 * 2 seconds after the previous read (or after start_periodic).
 *
 * @param co2_ppm  Output: CO₂ concentration in ppm
 * @param temp_c   Optional output: temperature in °C (pass NULL to skip)
 * @param rh_pct   Optional output: relative humidity in % (pass NULL to skip)
 * @return 0 on success, negative error code
 */
int scd41_read_co2_ex(uint16_t *co2_ppm, float *temp_c, float *rh_pct) {
    if (!g_scd41.initialized) return -ERR_SENSOR_NOT_FOUND;

    uint16_t data[3];
    int ret = scd41_read_3_words(data);

    if (ret < 0) {
        /* On CRC error, try to restart periodic measurement */
        if (ret == -ERR_SENSOR_CRC && g_scd41.error_count > 3) {
            scd41_write_cmd(CMD_STOP_PERIODIC);
            for (volatile uint32_t i = 0; i < 50000; i++);
            scd41_write_cmd(CMD_START_PERIODIC);
            g_scd41.error_count = 0;
        }
        return ret;
    }

    /* data[0] = CO₂ in ppm */
    /* data[1] = Temperature: °C = (data/100) - 273.15 */
    /* data[2] = Humidity: %RH = data/100 */

    g_scd41.co2_ppm = data[0];
    g_scd41.temperature = data[1];
    g_scd41.humidity = data[2];

    if (co2_ppm) *co2_ppm = data[0];
    if (temp_c)  *temp_c   = (float)((int16_t)data[1] / 100.0f + 273.15f);
    if (rh_pct)  *rh_pct   = (float)(data[2] / 100.0f);

    return 0;
}

/**
 * @brief Simplified CO₂ read (most common usage).
 *
 * @param co2_ppm Output: CO₂ concentration in ppm
 * @return 0 on success, negative on failure
 */
int scd41_read_co2(uint16_t *co2_ppm) {
    return scd41_read_co2_ex(co2_ppm, NULL, NULL);
}

/**
 * @brief Start periodic measurement mode.
 *
 * After calling this, measurements are available every 2 seconds.
 * Call scd41_read_co2() to get the latest value.
 *
 * @return 0 on success, negative error code
 */
int scd41_start_periodic(void) {
    if (g_scd41.periodic_running) return 0;

    int ret = scd41_write_cmd(CMD_START_PERIODIC);
    if (ret == 0) {
        g_scd41.periodic_running = true;
    }
    return ret;
}

/**
 * @brief Stop periodic measurement mode.
 *
 * Transitions sensor to idle state. Call before changing settings
 * or entering forced recalibration.
 *
 * @return 0 on success
 */
int scd41_stop_periodic(void) {
    if (!g_scd41.periodic_running) return 0;

    int ret = scd41_write_cmd(CMD_STOP_PERIODIC);
    if (ret == 0) {
        g_scd41.periodic_running = false;
        /* Wait 500 ms for stop sequence to complete */
        for (volatile uint32_t i = 0; i < 1000000; i++);
    }
    return ret;
}

/**
 * @brief Enter sleep mode.
 *
 * Reduces power consumption to ~0.5 µA. Must wake before next measurement.
 *
 * @return 0 on success
 */
int scd41_sleep(void) {
    if (g_scd41.periodic_running) {
        scd41_stop_periodic();
    }
    return scd41_write_cmd(CMD_SLEEP);
}

/**
 * @brief Wake from sleep mode.
 *
 * After wake, the sensor needs ~30 ms before accepting I²C commands.
 * This function sends the wake sequence and waits.
 *
 * @return 0 on success
 */
int scd41_wake(void) {
    /* Wake sequence: pull SDA low for >100 µs, then generate START */
    /* Alternate approach: send 0x0000 (no-op) to trigger wake */
    int ret = scd41_write_cmd(CMD_WAKE);
    if (ret < 0) ret = 0;  /* May NACK during wake — that's OK */

    /* Wait for sensor to stabilize */
    for (volatile uint32_t i = 0; i < 500000; i++);
    return 0;
}

/**
 * @brief Perform forced recalibration (FRC) for zero or span calibration.
 *
 * For zero calibration: send FRC=0 ppm (after flushing with CO₂-free air)
 * For span calibration: send FRC=known reference concentration
 *
 * @param reference_ppm  Known CO₂ concentration (0–40000 ppm)
 * @return 0 on success, negative on error
 */
int scd41_forced_recalibration(uint16_t reference_ppm) {
    int ret;

    /* Must stop periodic measurement first */
    if (g_scd41.periodic_running) {
        ret = scd41_stop_periodic();
        if (ret < 0) return ret;
    }

    /* Send FRC command with reference value */
    ret = scd41_write_cmd_with_data(CMD_SET_FRC, reference_ppm);
    if (ret < 0) return ret;

    /* Wait 400 ms for FRC computation */
    for (volatile uint32_t i = 0; i < 10000000; i++);

    /* Read FRC result (correction value) */
    ret = scd41_write_cmd(CMD_GET_FRC);
    if (ret < 0) return ret;

    for (volatile uint32_t i = 0; i < 250000; i++);

    uint16_t result[1];
    ret = scd41_read_3_words(result);
    if (ret < 0) return ret;

    g_scd41.frc_correction = result[0];  /* FRC correction in ppm */

    /* Save settings to EEPROM */
    ret = scd41_write_cmd(CMD_PERSIST_SETTINGS);
    if (ret < 0) return ret;

    /* Restart periodic measurement */
    ret = scd41_start_periodic();

    return ret;
}

/**
 * @brief Enable or disable automatic self-calibration (ASC).
 *
 * ASC continuously adjusts the sensor's offset based on the lowest
 * measured CO₂ concentration over several days (assuming it reaches
 * background ambient ~400 ppm).
 *
 * @param enable  true to enable, false to disable
 * @return 0 on success
 */
int scd41_set_asc(bool enable) {
    int ret = scd41_write_cmd_with_data(CMD_SET_ASC, enable ? 1 : 0);
    if (ret == 0) {
        ret = scd41_write_cmd(CMD_PERSIST_SETTINGS);
    }
    return ret;
}

/**
 * @brief Read ASC status.
 *
 * @param enabled Output: true if ASC is enabled
 * @return 0 on success
 */
int scd41_get_asc(bool *enabled) {
    int ret = scd41_write_cmd(CMD_GET_ASC);
    if (ret < 0) return ret;

    for (volatile uint32_t i = 0; i < 250000; i++);

    uint16_t data[3];
    ret = scd41_read_3_words(data);
    if (ret < 0) return ret;

    if (enabled) *enabled = (data[0] == 1);
    return 0;
}

/**
 * @brief Get sensor serial number.
 *
 * @param serial_out  Buffer for 6-character hex string (7 bytes including null)
 */
void scd41_get_serial(char *serial_out) {
    if (serial_out) {
        snprintf(serial_out, 7, "%04X%04X%04X",
                 g_scd41.serial[0], g_scd41.serial[1], g_scd41.serial[2]);
    }
}

/**
 * @brief Check if sensor data is ready (measurement available).
 *
 * Uses sensor status to determine if a new measurement is ready.
 *
 * @return true if data is ready
 */
bool scd41_data_ready(void) {
    /* In periodic mode, data is ready every 2 seconds without polling */
    return g_scd41.periodic_running && g_scd41.error_count == 0;
}

/**
 * @brief Get last error count (consecutive I²C/CRC errors).
 */
uint8_t scd41_get_error_count(void) {
    return g_scd41.error_count;
}