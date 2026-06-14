/*
 * NeuroLink — LSM6DSOX 6-axis IMU Driver
 * I2C1 interface (7-bit address: 0x6A)
 */

#ifndef LSM6DSOX_H
#define LSM6DSOX_H

#include <stdint.h>

/* LSM6DSOX I2C Address */
#define LSM6DSOX_I2C_ADDR       0x6A

/* Register Map */
#define LSM6DSOX_WHO_AM_I       0x0F
#define LSM6DSOX_CTRL1_XL       0x10
#define LSM6DSOX_CTRL2_G        0x11
#define LSM6DSOX_CTRL3_C        0x12
#define LSM6DSOX_CTRL4_C        0x13
#define LSM6DSOX_CTRL5_C        0x14
#define LSM6DSOX_CTRL6_C        0x15
#define LSM6DSOX_CTRL7_G        0x16
#define LSM6DSOX_CTRL8_XL       0x17
#define LSM6DSOX_CTRL9_XL       0x18
#define LSM6DSOX_CTRL10_C       0x19
#define LSM6DSOX_STATUS_REG     0x1E
#define LSM6DSOX_OUT_TEMP_L     0x20
#define LSM6DSOX_OUT_TEMP_H     0x21
#define LSM6DSOX_OUTX_L_G       0x22
#define LSM6DSOX_OUTX_H_G       0x23
#define LSM6DSOX_OUTY_L_G       0x24
#define LSM6DSOX_OUTY_H_G       0x25
#define LSM6DSOX_OUTZ_L_G       0x26
#define LSM6DSOX_OUTZ_H_G       0x27
#define LSM6DSOX_OUTX_L_XL      0x28
#define LSM6DSOX_OUTX_H_XL      0x29
#define LSM6DSOX_OUTY_L_XL      0x2A
#define LSM6DSOX_OUTY_H_XL      0x2B
#define LSM6DSOX_OUTZ_L_XL      0x2C
#define LSM6DSOX_OUTZ_H_XL      0x2D
#define LSM6DSOX_FIFO_CTRL1      0x06
#define LSM6DSOX_FIFO_CTRL2      0x07
#define LSM6DSOX_FIFO_CTRL3      0x08
#define LSM6DSOX_FIFO_STATUS1    0x3A

/* WHO_AM_I expected value */
#define LSM6DSOX_WHO_AM_I_VAL   0x6C

/* Accelerometer ODR and FS settings */
typedef enum {
    LSM6DSOX_XL_ODR_OFF      = 0,
    LSM6DSOX_XL_ODR_12_5Hz   = 1,
    LSM6DSOX_XL_ODR_26Hz     = 2,
    LSM6DSOX_XL_ODR_52Hz     = 3,
    LSM6DSOX_XL_ODR_104Hz    = 4,
    LSM6DSOX_XL_ODR_208Hz    = 5,
    LSM6DSOX_XL_ODR_416Hz    = 6,
    LSM6DSOX_XL_ODR_833Hz    = 7,
    LSM6DSOX_XL_ODR_1_66kHz  = 8,
    LSM6DSOX_XL_ODR_3_33kHz  = 9,
    LSM6DSOX_XL_ODR_6_66kHz  = 10,
} lsm6dsox_xl_odr_t;

typedef enum {
    LSM6DSOX_XL_FS_2g   = 0,
    LSM6DSOX_XL_FS_16g  = 1,
    LSM6DSOX_XL_FS_4g   = 2,
    LSM6DSOX_XL_FS_8g   = 3,
} lsm6dsox_xl_fs_t;

/* Gyroscope ODR and FS settings */
typedef enum {
    LSM6DSOX_G_ODR_OFF     = 0,
    LSM6DSOX_G_ODR_12_5Hz  = 1,
    LSM6DSOX_G_ODR_26Hz    = 2,
    LSM6DSOX_G_ODR_52Hz    = 3,
    LSM6DSOX_G_ODR_104Hz   = 4,
    LSM6DSOX_G_ODR_208Hz   = 5,
    LSM6DSOX_G_ODR_416Hz   = 6,
    LSM6DSOX_G_ODR_833Hz   = 7,
    LSM6DSOX_G_ODR_1_66kHz = 8,
    LSM6DSOX_G_ODR_3_33kHz = 9,
    LSM6DSOX_G_ODR_6_66kHz = 10,
} lsm6dsox_g_odr_t;

typedef enum {
    LSM6DSOX_G_FS_125dps  = 0,
    LSM6DSOX_G_FS_250dps  = 1,
    LSM6DSOX_G_FS_500dps  = 2,
    LSM6DSOX_G_FS_1000dps = 3,
    LSM6DSOX_G_FS_2000dps = 4,
} lsm6dsox_g_fs_t;

/* IMU data structure */
typedef struct {
    int16_t accel_x;     /* mg (milli-g) */
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;      /* mdps (milli-degrees/sec) */
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temperature;  /* Raw 16-bit temp */
} lsm6dsox_data_t;

/* Public API */
void     lsm6dsox_init(void);
uint8_t  lsm6dsox_who_am_i(void);
void     lsm6dsox_set_xl_odr(lsm6dsox_xl_odr_t odr);
void     lsm6dsox_set_xl_fs(lsm6dsox_xl_fs_t fs);
void     lsm6dsox_set_g_odr(lsm6dsox_g_odr_t odr);
void     lsm6dsox_set_g_fs(lsm6dsox_g_fs_t fs);
void     lsm6dsox_read_accel(int16_t *x, int16_t *y, int16_t *z);
void     lsm6dsox_read_gyro(int16_t *x, int16_t *y, int16_t *z);
void     lsm6dsox_read_all(lsm6dsox_data_t *data);
int16_t  lsm6dsox_read_temperature(void);
uint8_t  lsm6dsox_data_available(void);
void     lsm6dsox_soft_reset(void);
void     lsm6dsox_config_fifo(uint8_t watermark);

#endif /* LSM6DSOX_H */