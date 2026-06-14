/*
 * SkyPilot — ICM-42688-P IMU Driver Header
 * See phase4_software_stack.md for full documentation
 * This is the canonical copy for firmware build
 */

#ifndef SKYPILOT_IMU_ICM42688_H
#define SKYPILOT_IMU_ICM42688_H

#include <stdint.h>
#include <stdbool.h>

/* ICM-42688-P Register Map */
#define ICM42688_WHO_AM_I          0x75
#define ICM42688_DEVICE_CONFIG     0x11
#define ICM42688_DRIVE_CONFIG      0x13
#define ICM42688_INT_CONFIG        0x14
#define ICM42688_FIFO_CONFIG       0x16
#define ICM42688_TEMP_DATA1        0x1D
#define ICM42688_TEMP_DATA0        0x1E
#define ICM42688_ACCEL_DATA_X1    0x1F
#define ICM42688_ACCEL_DATA_X0    0x20
#define ICM42688_ACCEL_DATA_Y1    0x21
#define ICM42688_ACCEL_DATA_Y0    0x22
#define ICM42688_ACCEL_DATA_Z1    0x23
#define ICM42688_ACCEL_DATA_Z0    0x24
#define ICM42688_GYRO_DATA_X1     0x25
#define ICM42688_GYRO_DATA_X0     0x26
#define ICM42688_GYRO_DATA_Y1     0x27
#define ICM42688_GYRO_DATA_Y0     0x28
#define ICM42688_GYRO_DATA_Z1     0x29
#define ICM42688_GYRO_DATA_Z0     0x2A
#define ICM42688_INT_STATUS        0x2D
#define ICM42688_SIGNAL_PATH_RESET 0x4B
#define ICM42688_INTF_CONFIG0      0x4C
#define ICM42688_INTF_CONFIG1      0x4D
#define ICM42688_PWR_MGMT0         0x4E
#define ICM42688_GYRO_SMPLRT_DIV  0x4F
#define ICM42688_GYRO_CONFIG0      0x50
#define ICM42688_ACCEL_SMPLRT_DIV 0x52
#define ICM42688_ACCEL_CONFIG0     0x53
#define ICM42688_GYRO_CONFIG1      0x54
#define ICM42688_ACCEL_CONFIG1      0x55
#define ICM42688_INT_SOURCE0       0x5C
#define ICM42688_INT_SOURCE1       0x5D
#define ICM42688_USER_BANK         0x76

/* Gyroscope FS selections */
#define ICM42688_GYRO_FS_2000DPS   0x00
#define ICM42688_GYRO_FS_1000DPS   0x01
#define ICM42688_GYRO_FS_500DPS    0x02
#define ICM42688_GYRO_FS_250DPS    0x03
#define ICM42688_GYRO_FS_125DPS    0x04

/* Accelerometer FS selections */
#define ICM42688_ACCEL_FS_16G      0x00
#define ICM42688_ACCEL_FS_8G       0x01
#define ICM42688_ACCEL_FS_4G       0x02
#define ICM42688_ACCEL_FS_2G       0x03

/* ODR selections */
#define ICM42688_ODR_32KHZ   0x01
#define ICM42688_ODR_16KHZ   0x02
#define ICM42688_ODR_8KHZ    0x03
#define ICM42688_ODR_4KHZ    0x04
#define ICM42688_ODR_2KHZ    0x05
#define ICM42688_ODR_1KHZ    0x06

/* Low noise mode */
#define ICM42688_GYRO_MODE_LN   0x03
#define ICM42688_ACCEL_MODE_LN  0x03

/* Data structures */
typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temperature;
    uint32_t timestamp;
} imu_data_t;

typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float temperature;
} imu_scaled_t;

/* Function prototypes */
int  icm42688_init(void);
int  icm42688_who_am_i(void);
int  icm42688_reset(void);
int  icm42688_config_gyro(uint8_t fs, uint8_t odr);
int  icm42688_config_accel(uint8_t fs, uint8_t odr);
int  icm42688_read_raw(imu_data_t *data);
int  icm42688_read_scaled(imu_scaled_t *data);
void icm42688_scale_data(const imu_data_t *raw, imu_scaled_t *scaled);
int  icm42688_set_bank(uint8_t bank);
int  icm42688_read_reg(uint8_t reg, uint8_t *buf, uint16_t len);
int  icm42688_write_reg(uint8_t reg, const uint8_t *buf, uint16_t len);
int  icm42688_start_dma_read(uint8_t *rx_buf, uint16_t len);
int  icm42688_dma_complete(void);

/* SPI CS macros */
#define ICM42688_CS_LOW()   (GPIOA->BSRR = (1UL << (IMU1_CS_PIN + 16)))
#define ICM42688_CS_HIGH()  (GPIOA->BSRR = (1UL << IMU1_CS_PIN))

#endif /* SKYPILOT_IMU_ICM42688_H */