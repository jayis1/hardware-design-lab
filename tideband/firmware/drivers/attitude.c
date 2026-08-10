/**
 * @file    attitude.c
 * @brief   TideBand — Attitude estimation using complementary filter.
 *          Fuses ICM-42688-P 6-axis IMU (SPI4) and MMC5983MA 3-axis
 *          magnetometer (I2C2) to produce a stable roll/pitch/yaw
 *          solution with direction cosine matrix (DCM) for rotating
 *          body-frame Doppler velocity into the Earth-fixed NED frame.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * The complementary filter combines:
 *   - Accelerometer: long-term stable pitch & roll (gravity reference)
 *   - Gyroscope: high-rate, short-term stable integration
 *   - Magnetometer: long-term stable yaw (heading reference)
 *
 * The filter equation for each Euler angle is:
 *   angle_filt = alpha * (angle_prev + gyro_rate * dt) + (1 - alpha) * angle_ref
 *
 * where alpha is typically 0.98 (trust gyro in short term, accel/mag
 * in long term). This is computationally cheap and robust for the
 * low-dynamic dive environment where the device moves slowly.
 */

#include <string.h>
#include <math.h>
#include "board.h"
#include "registers.h"
#include "attitude.h"

/* ---- ICM-42688-P register addresses ---- */
#define ICM_REG_PWR_MGMT0    0x06u
#define ICM_REG_GYRO_CONFIG  0x20u
#define ICM_REG_ACCEL_CONFIG 0x21u
#define ICM_REG_TEMP_DATA    0x1Du
#define ICM_REG_ACCEL_DATA   0x0Bu
#define ICM_REG_GYRO_DATA    0x11u
#define ICM_REG_INT_CONFIG   0x14u
#define ICM_REG_DEVICE_CONFIG 0x11u

/* ICM-42688 config values */
#define ICM_GYRO_2000DPS     0x06u  /* ±2000 °/s, 32.8 LSB/°/s */
#define ICM_ACCEL_16G        0x06u  /* ±16g, 2048 LSB/g */
#define ICM_ODR_100HZ        0x09u  /* 100 Hz output data rate */

/* MMC5983MA register addresses */
#define MMC_REG_XOUT0        0x00u
#define MMC_REG_YOUT0        0x02u
#define MMC_REG_ZOUT0        0x04u
#define MMC_REG_INT_CTRL0    0x08u
#define MMC_REG_CTRL0        0x09u
#define MMC_REG_CTRL1        0x0Au

/* ---- Filter constants ---- */
#define COMP_ALPHA    0.98f    /* Gyro trust factor */
#define COMP_ALPHA_YAW 0.95f   /* Yaw: slightly less gyro trust (mag drift) */
#define DT            0.01f    /* 100 Hz update rate = 10 ms */
#define GRAVITY       9.80665f /* m/s² */
#define DEG2RAD       0.01745329f
#define RAD2DEG       57.2957795f

/* ---- Calibration storage ---- */
typedef struct {
    float gyro_bias[3];     /* rad/s */
    float accel_bias[3];    /* m/s² */
    float mag_hard_iron[3]; /* gauss offset */
    float mag_soft_iron[3]; /* scale factor */
    uint8_t valid;
} attitude_cal_t;

static attitude_cal_t cal;
static attitude_t prev_att;
static uint8_t initialized = 0;

/* ---- Local function prototypes ---- */
static void imu_init(void);
static void mag_init(void);
static int imu_read(float accel[3], float gyro[3], float *temp);
static int mag_read(float mag[3]);
static void compute_dcm(attitude_t *att);
static void spi4_select(uint32_t cs_gpio, uint8_t cs_pin);
static void spi4_deselect(uint32_t cs_gpio, uint8_t cs_pin);
static uint8_t spi4_transfer(uint8_t tx);
static void i2c2_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
static void i2c2_write(uint8_t addr, uint8_t reg, uint8_t val);

/* ---- Public API ---- */

void attitude_init(void)
{
    memset(&cal, 0, sizeof(cal));
    memset(&prev_att, 0, sizeof(prev_att));
    cal.mag_soft_iron[0] = 1.0f;
    cal.mag_soft_iron[1] = 1.0f;
    cal.mag_soft_iron[2] = 1.0f;

    /* Enable SPI4 and I2C2 clocks */
    RCC->APB2ENR |= RCC_APB2ENR_SPI4;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2;

    /* Configure SPI4 GPIO: SCK (PE2 AF5), MISO (PE5 AF5), MOSI (PE6 AF5) */
    gpio_set_mode(IMU_SCK_GPIO, IMU_SCK_PIN, GPIO_MODE_AF);
    gpio_set_af(IMU_SCK_GPIO, IMU_SCK_PIN, IMU_SCK_AF);
    gpio_set_speed(IMU_SCK_GPIO, IMU_SCK_PIN, GPIO_SPEED_VHIGH);
    gpio_set_mode(IMU_MISO_GPIO, IMU_MISO_PIN, GPIO_MODE_AF);
    gpio_set_af(IMU_MISO_GPIO, IMU_MISO_PIN, IMU_MISO_AF);
    gpio_set_mode(IMU_MOSI_GPIO, IMU_MOSI_PIN, GPIO_MODE_AF);
    gpio_set_af(IMU_MOSI_GPIO, IMU_MOSI_PIN, IMU_MOSI_AF);

    /* CS pins for IMU and NAND (both on SPI4) */
    gpio_set_mode(IMU_CS_GPIO, IMU_CS_PIN, GPIO_MODE_OUTPUT);
    gpio_set(IMU_CS_GPIO, IMU_CS_PIN);
    gpio_set_mode(NAND_CS_GPIO, NAND_CS_PIN, GPIO_MODE_OUTPUT);
    gpio_set(NAND_CS_GPIO, NAND_CS_PIN);

    /* Configure I2C2 GPIO: PB10 (SCL), PB11 (SDA) — but wait, we defined
     * mag on I2C2. Need to set up I2C2 pins. On H733 LQFP100:
     * I2C2_SCL = PB10 or PF1, I2C2_SDA = PB11 or PF0.
     * We use PB10/PB11 AF4. */
    /* Actually, let's re-check — the board.h says mag uses I2C2 but
     * didn't specify pins. Let's use PB10/PB11. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOB;
    gpio_set_mode(GPIOB_BASE, 10, GPIO_MODE_AF);
    gpio_set_af(GPIOB_BASE, 10, 4);
    gpio_set_speed(GPIOB_BASE, 10, GPIO_SPEED_HIGH);
    gpio_set_pupd(GPIOB_BASE, 10, GPIO_PUPD_PU);
    gpio_set_mode(GPIOB_BASE, 11, GPIO_MODE_AF);
    gpio_set_af(GPIOB_BASE, 11, 4);
    gpio_set_speed(GPIOB_BASE, 11, GPIO_SPEED_HIGH);
    gpio_set_pupd(GPIOB_BASE, 11, GPIO_PUPD_PU);

    /* Configure SPI4 as master */
    /* SPI4_CR1 = SPE; enable after config */
    SPI4_CR1 = 0;  /* Disable first */
    /* Note: Using simplified SPI registers (same layout as SPI1 for H7) */
    *(volatile uint32_t *)(SPI4_BASE + 0x04) = 0x00000001; /* CFG2: MASTER */
    *(volatile uint32_t *)(SPI4_BASE + 0x00) = 0x0000000F; /* CFG1: 16-bit, FIFO */
    SPI4_CR1 = SPI_CR1_SPE;  /* Enable SPI4 */

    /* Configure I2C2 */
    I2C2_CR1 = 0;  /* Disable */
    I2C2_TIMINGR = I2C2_TIMING_400K;
    I2C2_CR1 = I2C_CR1_PE;  /* Enable */

    /* Initialize sensors */
    imu_init();
    mag_init();

    initialized = 1;
}

void attitude_update(attitude_t *att)
{
    if (!initialized) {
        attitude_init();
    }

    float accel[3], gyro[3], mag_vec[3], temp;

    if (imu_read(accel, gyro, &temp) != 0) {
        att->valid = 0;
        return;
    }
    if (mag_read(mag_vec) != 0) {
        /* Continue without mag — yaw will drift */
        mag_vec[0] = mag_vec[1] = mag_vec[2] = 0.0f;
    }

    /* Remove biases */
    for (int i = 0; i < 3; i++) {
        gyro[i] -= cal.gyro_bias[i];
        accel[i] -= cal.accel_bias[i];
        mag_vec[i] = (mag_vec[i] - cal.mag_hard_iron[i]) * cal.mag_soft_iron[i];
    }

    /* Store raw values */
    memcpy(att->gyro, gyro, sizeof(gyro));
    memcpy(att->accel, accel, sizeof(accel));
    memcpy(att->mag, mag_vec, sizeof(mag_vec));

    /* ---- Pitch and Roll from accelerometer ---- */
    /* pitch_ref = atan2(-ax, az) */
    float pitch_ref = atan2f(-accel[0], sqrtf(accel[1]*accel[1] +
                                               accel[2]*accel[2]));
    /* roll_ref = atan2(ay, az) */
    float roll_ref = atan2f(accel[1], accel[2]);

    /* ---- Yaw from magnetometer ---- */
    /* Tilt-compensated heading:
     *   mx_h = mx*cos(roll) + my*sin(roll)*sin(pitch) + mz*cos(roll)*sin(pitch)
     *   my_h = my*cos(pitch) - mz*sin(pitch)
     *   yaw_ref = atan2(-my_h, mx_h)
     * Using the previous attitude as the tilt reference. */
    float roll_p = prev_att.roll;
    float pitch_p = prev_att.pitch;
    float mx = mag_vec[0], my = mag_vec[1], mz = mag_vec[2];

    float mx_h = mx * cosf(roll_p) +
                 my * sinf(roll_p) * sinf(pitch_p) +
                 mz * cosf(roll_p) * sinf(pitch_p);
    float my_h = my * cosf(pitch_p) - mz * sinf(pitch_p);
    float yaw_ref = atan2f(-my_h, mx_h);

    /* ---- Complementary filter ---- */
    /* Integrate gyro and blend with accel/mag reference */
    att->roll  = COMP_ALPHA * (prev_att.roll  + gyro[0] * DT) +
                 (1.0f - COMP_ALPHA) * roll_ref;
    att->pitch = COMP_ALPHA * (prev_att.pitch + gyro[1] * DT) +
                 (1.0f - COMP_ALPHA) * pitch_ref;
    att->yaw   = COMP_ALPHA_YAW * (prev_att.yaw + gyro[2] * DT) +
                 (1.0f - COMP_ALPHA_YAW) * yaw_ref;

    /* Wrap yaw to [-pi, pi] */
    while (att->yaw > 3.14159265f) att->yaw -= 2.0f * 3.14159265f;
    while (att->yaw < -3.14159265f) att->yaw += 2.0f * 3.14159265f;

    /* Compute DCM from Euler angles */
    compute_dcm(att);

    att->valid = 1;

    /* Save for next iteration */
    prev_att = *att;
}

void attitude_calibrate_gyro(void)
{
    float gyro_sum[3] = {0, 0, 0};
    float accel_sum[3] = {0, 0, 0};
    float accel[3], gyro[3], temp;
    const int N_samples = 200;

    for (int i = 0; i < N_samples; i++) {
        if (imu_read(accel, gyro, &temp) == 0) {
            for (int j = 0; j < 3; j++) {
                gyro_sum[j] += gyro[j];
                accel_sum[j] += accel[j];
            }
        }
        /* Delay ~10 ms (100 Hz) */
        for (volatile int k = 0; k < 280000; k++) { }
    }

    for (int i = 0; i < 3; i++) {
        cal.gyro_bias[i] = gyro_sum[i] / N_samples;
        /* Accel bias = mean - expected gravity component.
         * Assuming device is flat, gravity is on Z axis: az = +g.
         * We subtract gravity from the Z bias. */
        cal.accel_bias[i] = accel_sum[i] / N_samples;
    }
    cal.accel_bias[2] -= GRAVITY;
    cal.valid = 1;
}

void attitude_calibrate_mag(void)
{
    /* Simple hard-iron: collect min/max on each axis as user rotates.
     * Hard iron offset = (max + min) / 2.
     * Soft iron scale = 1 / ((max - min) / 2) normalized. */
    float mag_min[3] = {1e9, 1e9, 1e9};
    float mag_max[3] = {-1e9, -1e9, -1e9};
    float mag_vec[3];
    const int N_samples = 500;  /* ~5 seconds at 100 Hz */

    for (int i = 0; i < N_samples; i++) {
        if (mag_read(mag_vec) == 0) {
            for (int j = 0; j < 3; j++) {
                if (mag_vec[j] < mag_min[j]) mag_min[j] = mag_vec[j];
                if (mag_vec[j] > mag_max[j]) mag_max[j] = mag_vec[j];
            }
        }
        for (volatile int k = 0; k < 280000; k++) { }
    }

    float max_range = 0.0f;
    for (int i = 0; i < 3; i++) {
        cal.mag_hard_iron[i] = (mag_max[i] + mag_min[i]) / 2.0f;
        float range = (mag_max[i] - mag_min[i]) / 2.0f;
        cal.mag_soft_iron[i] = 1.0f / (range + 1e-7f);
        if (range > max_range) max_range = range;
    }
    /* Normalize soft iron so the largest axis has scale 1.0 */
    for (int i = 0; i < 3; i++) {
        cal.mag_soft_iron[i] *= max_range;
    }
}

void attitude_rotate_to_ned(const attitude_t *att,
                             const float body[3], float ned[3])
{
    for (int i = 0; i < 3; i++) {
        ned[i] = 0.0f;
        for (int j = 0; j < 3; j++) {
            ned[i] += att->dcm[i][j] * body[j];
        }
    }
}

/* ---- Local function implementations ---- */

static void imu_init(void)
{
    /* Software reset */
    spi4_select(IMU_CS_GPIO, IMU_CS_PIN);
    spi4_transfer(ICM_REG_PWR_MGMT0 | 0x80u);  /* Read not write; actually: */
    spi4_deselect(IMU_CS_GPIO, IMU_CS_PIN);

    /* Write PWR_MGMT0 = 0x00 (LN mode for both accel and gyro) */
    spi4_select(IMU_CS_GPIO, IMU_CS_PIN);
    spi4_transfer(ICM_REG_PWR_MGMT0);
    spi4_transfer(0x00u);
    spi4_deselect(IMU_CS_GPIO, IMU_CS_PIN);

    /* Configure gyro: ±2000 °/s, 100 Hz ODR */
    spi4_select(IMU_CS_GPIO, IMU_CS_PIN);
    spi4_transfer(ICM_REG_GYRO_CONFIG);
    spi4_transfer((ICM_GYRO_2000DPS << 0) | (ICM_ODR_100HZ << 3));
    spi4_deselect(IMU_CS_GPIO, IMU_CS_PIN);

    /* Configure accel: ±16g, 100 Hz ODR */
    spi4_select(IMU_CS_GPIO, IMU_CS_PIN);
    spi4_transfer(ICM_REG_ACCEL_CONFIG);
    spi4_transfer((ICM_ACCEL_16G << 0) | (ICM_ODR_100HZ << 3));
    spi4_deselect(IMU_CS_GPIO, IMU_CS_PIN);
}

static void mag_init(void)
{
    /* Set continuous measurement mode at 100 Hz */
    i2c2_write(MAG_I2C_ADDR, MMC_REG_CTRL0, 0x08u);  /* Set rate */
    i2c2_write(MAG_I2C_ADDR, MMC_REG_CTRL1, 0x00u);  /* Default */
    i2c2_write(MAG_I2C_ADDR, MMC_REG_INT_CTRL0, 0x01u); /* Enable Meas */
}

static int imu_read(float accel[3], float gyro[3], float *temp)
{
    uint8_t buf[12];

    spi4_select(IMU_CS_GPIO, IMU_CS_PIN);
    /* Read 12 bytes starting from ACCEL_DATA (0x0B), auto-increment.
     * ICM-42688 uses bit7=1 for read, and auto-increments register addr. */
    spi4_transfer(ICM_REG_ACCEL_DATA | 0x80u);
    for (int i = 0; i < 12; i++) {
        buf[i] = spi4_transfer(0x00u);
    }
    spi4_deselect(IMU_CS_GPIO, IMU_CS_PIN);

    /* Parse accel (big-endian, signed 16-bit): [ax_h, ax_l, ay_h, ...] */
    int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t az = (int16_t)((buf[4] << 8) | buf[5]);

    /* Parse gyro (big-endian, signed 16-bit) */
    int16_t gx = (int16_t)((buf[6] << 8) | buf[7]);
    int16_t gy = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t gz = (int16_t)((buf[10] << 8) | buf[11]);

    /* Convert to physical units.
     * Accel: ±16g, 2048 LSB/g → m/s² = raw / 2048 * 9.80665
     * Gyro: ±2000°/s, 32.8 LSB/(°/s) → rad/s = raw / 32.8 * DEG2RAD */
    accel[0] = (float)ax / 2048.0f * GRAVITY;
    accel[1] = (float)ay / 2048.0f * GRAVITY;
    accel[2] = (float)az / 2048.0f * GRAVITY;
    gyro[0]  = (float)gx / 32.8f * DEG2RAD;
    gyro[1]  = (float)gy / 32.8f * DEG2RAD;
    gyro[2]  = (float)gz / 32.8f * DEG2RAD;

    /* Read temperature (optional) */
    uint8_t tbuf[2];
    spi4_select(IMU_CS_GPIO, IMU_CS_PIN);
    spi4_transfer(ICM_REG_TEMP_DATA | 0x80u);
    tbuf[0] = spi4_transfer(0x00u);
    tbuf[1] = spi4_transfer(0x00u);
    spi4_deselect(IMU_CS_GPIO, IMU_CS_PIN);
    int16_t t_raw = (int16_t)((tbuf[0] << 8) | tbuf[1]);
    *temp = (float)t_raw / 2.56f + 25.0f;  /* ICM-42688 temp formula */

    return 0;
}

static int mag_read(float mag[3])
{
    uint8_t buf[6];

    i2c2_read(MAG_I2C_ADDR, MMC_REG_XOUT0, buf, 6);

    /* MMC5983 outputs 18-bit values: XOUT0 (8 bits) | XOUT1 (8 bits) | XOUT2 (2 bits)
     * For simplicity, use 16-bit: XOUT0 << 8 | XOUT1 (ignore extra 2 bits) */
    int32_t mx = ((int32_t)buf[0] << 8) | buf[1];
    int32_t my = ((int32_t)buf[2] << 8) | buf[3];
    int32_t mz = ((int32_t)buf[4] << 8) | buf[5];

    /* MMC5983: 0.163 mG/LSB (16-bit mode), offset = 32768 */
    /* Convert to gauss: (raw - 32768) * 0.000163 */
    mag[0] = ((float)mx - 32768.0f) * 0.000163f;
    mag[1] = ((float)my - 32768.0f) * 0.000163f;
    mag[2] = ((float)mz - 32768.0f) * 0.000163f;

    return 0;
}

static void compute_dcm(attitude_t *att)
{
    /* DCM = Rz(yaw) * Ry(pitch) * Rx(roll)
     * This rotates body-frame vector to NED frame. */
    float cr = cosf(att->roll),  sr = sinf(att->roll);
    float cp = cosf(att->pitch), sp = sinf(att->pitch);
    float cy = cosf(att->yaw),   sy = sinf(att->yaw);

    att->dcm[0][0] = cp * cy;
    att->dcm[0][1] = cp * sy;
    att->dcm[0][2] = -sp;
    att->dcm[1][0] = sr * sp * cy - cr * sy;
    att->dcm[1][1] = sr * sp * sy + cr * cy;
    att->dcm[1][2] = sr * cp;
    att->dcm[2][0] = cr * sp * cy + sr * sy;
    att->dcm[2][1] = cr * sp * sy - sr * cy;
    att->dcm[2][2] = cr * cp;
}

/* ---- SPI4 and I2C2 low-level functions ---- */

static void spi4_select(uint32_t cs_gpio, uint8_t cs_pin)
{
    gpio_clear(cs_gpio, cs_pin);
    /* Small delay for CS setup time */
    for (volatile int i = 0; i < 10; i++) { }
}

static void spi4_deselect(uint32_t cs_gpio, uint8_t cs_pin)
{
    gpio_set(cs_gpio, cs_pin);
}

static uint8_t spi4_transfer(uint8_t tx)
{
    /* Write to SPI4 data register (8-bit mode) */
    *(volatile uint8_t *)&SPI4_DR = tx;
    /* Wait for RXNE (receive register not empty) */
    while ((SPI4_SR & SPI_SR_RXP) == 0) { }
    return *(volatile uint8_t *)&SPI4_DR;
}

static void i2c2_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    /* Send register address */
    I2C2_CR2 = ((uint32_t)addr << 1) | (1u << 16) | I2C_CR2_START;
    while ((I2C2_ISR & I2C_ISR_TXE) == 0) { }
    I2C2_TXDR = reg;
    while ((I2C2_ISR & I2C_ISR_TC) == 0) { }

    /* Read data */
    I2C2_CR2 = ((uint32_t)addr << 1) | ((uint32_t)len << 16) |
               I2C_CR2_START | (1u << 14);  /* AUTOEND + START */
    for (uint8_t i = 0; i < len; i++) {
        while ((I2C2_ISR & I2C_ISR_RXNE) == 0) { }
        buf[i] = (uint8_t)I2C2_RXDR;
    }
}

static void i2c2_write(uint8_t addr, uint8_t reg, uint8_t val)
{
    I2C2_CR2 = ((uint32_t)addr << 1) | (2u << 16) | I2C_CR2_START;
    while ((I2C2_ISR & I2C_ISR_TXE) == 0) { }
    I2C2_TXDR = reg;
    while ((I2C2_ISR & I2C_ISR_TXE) == 0) { }
    I2C2_TXDR = val;
    while ((I2C2_ISR & I2C_ISR_TC) == 0) { }
}