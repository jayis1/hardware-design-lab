/*
 * drivers/imu_icm42688.h — ICM-42688-P 6-axis IMU driver for PicoRadar
 * SPI1 interface with DMA on STM32H743
 */

#ifndef IMU_ICM42688_H
#define IMU_ICM42688_H

#include <stdint.h>
#include <stddef.h>

/* IMU configuration */
#define IMU_SPI_TIMEOUT_MS       100
#define IMU_GYRO_RANGE_DPS       2000   // ±2000 °/s
#define IMU_ACCEL_RANGE_G        16     // ±16 g
#define IMU_GYRO_ODR             6      // 0=12.5Hz..6=800Hz (encoded)
#define IMU_ACCEL_ODR            6      // Same
#define IMU_FIFO_WATERMARK       16     // 16 samples

/* Scaling factors */
#define IMU_GYRO_SCALE           (2000.0f / 32768.0f)  // DPS/LSB
#define IMU_ACCEL_SCALE          (16.0f / 32768.0f)    // g/LSB

/* Register bank 0 (default) */
#define ICM42688_WHO_AM_I        0x75
#define ICM42688_DEVICE_CONFIG   0x11
#define ICM42688_PWR_MGMT0       0x4E
#define ICM42688_GYRO_CONFIG0    0x4F
#define ICM42688_ACCEL_CONFIG0   0x50
#define ICM42688_GYRO_CONFIG1    0x51
#define ICM42688_ACCEL_CONFIG1   0x53
#define ICM42688_FIFO_CONFIG     0x16
#define ICM42688_FIFO_COUNTH     0x3A
#define ICM42688_FIFO_DATA       0x3C
#define ICM42688_INT_STATUS      0x3A
#define ICM42688_INT_CONFIG      0x14

/* SPI read/write bit */
#define ICM42688_SPI_READ        0x80
#define ICM42688_SPI_WRITE       0x00

/* Data structures */
typedef struct {
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;
    int16_t temperature;
    uint32_t timestamp;
} imu_sample_t;

typedef struct {
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    float temperature;
} imu_scaled_t;

/* Public API */
int  imu_init(void);
int  imu_read_raw(imu_sample_t *sample);
void imu_scale(const imu_sample_t *raw, imu_scaled_t *scaled);
int  imu_fifo_read(imu_sample_t *samples, uint16_t max_count, uint16_t *out_count);
int  imu_self_test(void);
void imu_power_down(void);
int  imu_configure_fifo(uint16_t watermark);
int  imu_read_register(uint8_t reg, uint8_t *val);
int  imu_write_register(uint8_t reg, uint8_t val);

/* DMA callback type */
typedef void (*imu_dma_cb_t)(void);
void imu_set_dma_callback(imu_dma_cb_t cb);

#endif /* IMU_ICM42688_H */