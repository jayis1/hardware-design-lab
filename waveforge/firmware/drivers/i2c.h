/* ============================================
 * i2c.h — WaveForge I2C Driver
 * STM32H743 I2C1 master mode, 400 kHz
 * ============================================ */

#ifndef WAVEFORGE_I2C_H
#define WAVEFORGE_I2C_H

#include <stdint.h>

/* Initialize I2C1 peripheral (400 kHz fast mode) */
void I2C1_Init(void);

/* Write data to I2C device
 * addr: 7-bit device address (e.g., 0x1A for WM8778)
 * reg: 8-bit register address
 * data: pointer to data bytes
 * len: number of bytes to write
 * Returns: 0 on success, -1 on error
 */
int I2C1_Write(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len);

/* Read data from I2C device
 * addr: 7-bit device address
 * reg: 8-bit register address
 * data: pointer to buffer for read data
 * len: number of bytes to read
 * Returns: 0 on success, -1 on error
 */
int I2C1_Read(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

/* Write a single byte to I2C device register */
int I2C1_WriteByte(uint8_t addr, uint8_t reg, uint8_t value);

/* Read a single byte from I2C device register */
int I2C1_ReadByte(uint8_t addr, uint8_t reg, uint8_t *value);

/* Check if device is present on the bus */
int I2C1_IsDeviceReady(uint8_t addr, uint32_t retries);

#endif /* WAVEFORGE_I2C_H */