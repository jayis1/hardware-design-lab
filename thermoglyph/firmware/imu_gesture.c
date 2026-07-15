/*
 * imu_gesture.c — ICM-42688-P IMU driver and gesture recognition
 *
 * Implements:
 *   - SPI register-level communication with ICM-42688-P
 *   - 200 Hz accel + gyro sampling
 *   - Tap / double-tap / flip / shake gesture detection
 *   - Pitch/roll orientation estimation (complementary filter)
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#include "imu_gesture.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ------------------------------------------------------------------------- */
/* SPI (SPIM1) for ICM-42688 — register-level */
/* ------------------------------------------------------------------------- */

static volatile uint32_t *spim1 = (volatile uint32_t *)NRF_SPIM1_BASE;
static uint8_t spi_tx_buf[16];
static uint8_t spi_rx_buf[16];

static void spi_init_imu(void)
{
    /* Configure SPIM1 pins */
    spim1[SPIM_PSEL_SCK / 4]  = (1U << 8) | 1U;   /* P1.01 */
    spim1[SPIM_PSEL_MOSI / 4] = (1U << 8) | 2U;   /* P1.02 */
    spim1[SPIM_PSEL_MISO / 4] = (1U << 8) | 3U;   /* P1.03 */
    spim1[SPIM_FREQUENCY / 4] = 0x1000000U;       /* 1 MHz */
    spim1[SPIM_CONFIG / 4] = 0U;                  /* mode 0, MSB first */
    spim1[SPIM_ENABLE / 4] = 7U;                  /* enable SPIM */

    /* Configure CS pin as GPIO output (active high selection) */
    volatile uint32_t *p1 = (volatile uint32_t *)NRF_P1_BASE;
    p1[GPIO_PIN_CNF(4) / 4] = 3UL;  /* P1.04 output */
    p1[GPIO_PIN_OUTSET / 4] = (1UL << 4);  /* CS high (deselected) */
}

static void spi_cs_select(void)
{
    volatile uint32_t *p1 = (volatile uint32_t *)NRF_P1_BASE;
    p1[GPIO_PIN_OUTCLR / 4] = (1UL << 4);  /* CS low */
}

static void spi_cs_deselect(void)
{
    volatile uint32_t *p1 = (volatile uint32_t *)NRF_P1_BASE;
    p1[GPIO_PIN_OUTSET / 4] = (1UL << 4);  /* CS high */
}

static bool spi_xfer(uint8_t *tx, uint8_t *rx, uint8_t len)
{
    spim1[SPIM_TXDPTR / 4] = (uint32_t)(uintptr_t)tx;
    spim1[SPIM_TXDMAXCNT / 4] = len;
    spim1[SPIM_RXDPTR / 4] = (uint32_t)(uintptr_t)rx;
    spim1[SPIM_RXDMAXCNT / 4] = len;

    spi_cs_select();
    spim1[SPIM_TASKS_START / 4] = 1U;

    uint32_t timeout = 100000U;
    while (!(spim1[SPIM_EVENTS_END / 4] & 1U) && timeout-- > 0) {}
    spim1[SPIM_EVENTS_END / 4] = 0U;
    spi_cs_deselect();

    return timeout > 0;
}

/* ------------------------------------------------------------------------- */
/* ICM-42688 register access */
/* ------------------------------------------------------------------------- */

static bool icm_write_reg(uint8_t reg, uint8_t val)
{
    spi_tx_buf[0] = reg & 0x7F;  /* bit7=0 for write */
    spi_tx_buf[1] = val;
    return spi_xfer(spi_tx_buf, spi_rx_buf, 2);
}

static uint8_t icm_read_reg(uint8_t reg)
{
    spi_tx_buf[0] = reg | 0x80;  /* bit7=1 for read */
    spi_tx_buf[1] = 0x00;        /* dummy */
    if (spi_xfer(spi_tx_buf, spi_rx_buf, 2)) {
        return spi_rx_buf[1];
    }
    return 0xFF;
}

static bool icm_read_burst(uint8_t reg, uint8_t *buf, uint8_t len)
{
    spi_tx_buf[0] = reg | 0x80;
    memset(&spi_tx_buf[1], 0, len);
    bool ok = spi_xfer(spi_tx_buf, spi_rx_buf, len + 1);
    if (ok) memcpy(buf, &spi_rx_buf[1], len);
    return ok;
}

/* ------------------------------------------------------------------------- */
/* IMU initialization */
/* ------------------------------------------------------------------------- */

bool imu_init(void)
{
    spi_init_imu();

    /* Reset device */
    icm_write_reg(ICM_REG_DEVICE_CONFIG, 0x01);  /* soft reset */
    /* Wait for reset to complete (simplified: busy-wait) */
    for (volatile uint32_t i = 0; i < 10000; i++) {}

    /* Verify WHO_AM_I */
    uint8_t whoami = icm_read_reg(ICM_REG_WHO_AM_I);
    if (whoami != ICM_VAL_WHO_AM_I) return false;

    /* Power management: enable accel + gyro in low-noise mode */
    icm_write_reg(ICM_REG_PWR_MGMT0, ICM_PWR_ACCEL_LN | ICM_PWR_GYRO_LN);

    /* Configure accel: ±4g, 200 Hz ODR */
    icm_write_reg(ICM_REG_ACCEL_CONFIG0,
                  (ICM_ACCEL_FS_4G << 5) | ICM_ODR_200HZ);

    /* Configure gyro: ±2000 dps, 200 Hz ODR */
    icm_write_reg(ICM_REG_GYRO_CONFIG0,
                  (ICM_GYRO_FS_2000DPS << 5) | ICM_ODR_200HZ);

    /* Enable data-ready interrupt on INT1 */
    icm_write_reg(ICM_REG_INT_CONFIG, 0x03);  /* pushed-pull, active high */
    icm_write_reg(ICM_REG_INT_CONFIG0, 0x00);  /* latched mode */
    icm_write_reg(ICM_REG_INT_SOURCE0, 0x08);  /* DRDY interrupt */

    return true;
}

/* ------------------------------------------------------------------------- */
/* IMU sample reading */
/* ------------------------------------------------------------------------- */

bool imu_read(imu_sample_t *sample)
{
    uint8_t buf[16];

    /* Read accel + gyro + temp (20 bytes total in ICM, but we read key regs) */
    /* Accel: X1, X0, Y1, Y0, Z1, Z0 (0x1F–0x24) */
    /* Gyro:  X1, X0, Y1, Y0, Z1, Z0 (0x25–0x2A) */
    /* Temp:  1, 0 (0x1D–0x1E) */
    /* Read all from 0x1D (temp) through 0x2A (gyro Z0) = 14 bytes */
    if (!icm_read_burst(ICM_REG_TEMP_DATA1, buf, 14)) {
        return false;
    }

    /* Parse temperature (buf[0], buf[1]) */
    int16_t temp_raw = (int16_t)((buf[0] << 8) | buf[1]);
    /* ICM temp: °C = (raw / 132.48) + 25, in fixed-point × 100: */
    sample->temp_c100 = (int16_t)((int32_t)temp_raw * 100 / 13248 + 2500);

    /* Parse accel (buf[2]–buf[7]) */
    sample->accel_x = (int16_t)((buf[2] << 8) | buf[3]);
    sample->accel_y = (int16_t)((buf[4] << 8) | buf[5]);
    sample->accel_z = (int16_t)((buf[6] << 8) | buf[7]);

    /* Convert accel from LSB to mg (4g range: 2048 LSB/g → 0.488 mg/LSB) */
    /* We keep raw and convert in gesture logic; or convert here: */
    /* accel_mg = raw * 1000 / 2048 */
    sample->accel_x = (int16_t)((int32_t)sample->accel_x * 1000 / 2048);
    sample->accel_y = (int16_t)((int32_t)sample->accel_y * 1000 / 2048);
    sample->accel_z = (int16_t)((int32_t)sample->accel_z * 1000 / 2048);

    /* Parse gyro (buf[8]–buf[13]) */
    sample->gyro_x = (int16_t)((buf[8] << 8) | buf[9]);
    sample->gyro_y = (int16_t)((buf[10] << 8) | buf[11]);
    sample->gyro_z = (int16_t)((buf[12] << 8) | buf[13]);

    /* Convert gyro from LSB to mdps (2000 dps: 16.4 LSB/dps) */
    sample->gyro_x = (int16_t)((int32_t)sample->gyro_x * 1000 / 16400);
    sample->gyro_y = (int16_t)((int32_t)sample->gyro_y * 1000 / 16400);
    sample->gyro_z = (int16_t)((int32_t)sample->gyro_z * 1000 / 16400);

    /* Timestamp (simplified: use a global counter) */
    static uint32_t time_ms = 0;
    time_ms += 5;  /* 200 Hz → 5 ms per sample */
    sample->timestamp_ms = time_ms;

    return true;
}

/* ------------------------------------------------------------------------- */
/* Gesture detection state machine */
/* ------------------------------------------------------------------------- */

typedef enum {
    GEST_STATE_IDLE,
    GEST_STATE_TAP_DETECTED,
    GEST_STATE_WAIT_DOUBLE,
} gest_state_t;

static gest_state_t gest_state = GEST_STATE_IDLE;
static uint32_t gest_timer_ms = 0;
static int16_t pitch_estimate = 0;  /* degrees × 1 */
static int16_t roll_estimate = 0;

/* Shake tracking */
static uint32_t shake_start_ms = 0;
static bool shake_active = false;

/* Compute acceleration magnitude (in mg) */
static int32_t accel_magnitude(const imu_sample_t *s)
{
    int32_t ax = s->accel_x, ay = s->accel_y, az = s->accel_z;
    /* Approximate magnitude without sqrt: use max component + 0.5 * others */
    int32_t abs_x = ax < 0 ? -ax : ax;
    int32_t abs_y = ay < 0 ? -ay : ay;
    int32_t abs_z = az < 0 ? -az : az;
    int32_t max_val = abs_x > abs_y ? (abs_x > abs_z ? abs_x : abs_z)
                                    : (abs_y > abs_z ? abs_y : abs_z);
    return max_val + ((abs_x + abs_y + abs_z - max_val) / 4);
}

uint8_t imu_detect_gesture(const imu_sample_t *sample)
{
    uint8_t result = GESTURE_NONE;
    int32_t mag = accel_magnitude(sample);

    /* Update orientation estimate (complementary filter, simplified) */
    /* Pitch from accel: atan2(ay, az) ≈ ay * 57.3 / az (for small angles) */
    if (sample->accel_z != 0) {
        int16_t accel_pitch = (int16_t)((int32_t)sample->accel_y * 5730 /
                                        (int32_t)sample->accel_z);
        /* Blend with gyro integration */
        pitch_estimate = (int16_t)(pitch_estimate * 98 / 100 +
                                   accel_pitch * 2 / 100);
    }
    if (sample->accel_z != 0) {
        int16_t accel_roll = (int16_t)((int32_t)sample->accel_x * 5730 /
                                       (int32_t)sample->accel_z);
        roll_estimate = (int16_t)(roll_estimate * 98 / 100 +
                                  accel_roll * 2 / 100);
    }

    /* Shake detection: sustained lateral acceleration > threshold */
    int32_t lateral = (sample->accel_x < 0 ? -sample->accel_x
                                           : sample->accel_x);
    if (lateral > (int32_t)GESTURE_SHAKE_ACCEL_THRESH) {
        if (!shake_active) {
            shake_active = true;
            shake_start_ms = sample->timestamp_ms;
        } else if ((sample->timestamp_ms - shake_start_ms) >
                   GESTURE_SHAKE_DUR_MS) {
            result = GESTURE_SHAKE;
            shake_active = false;
            gest_state = GEST_STATE_IDLE;
        }
    } else {
        shake_active = false;
    }

    /* Flip detection: pitch > 90° within duration window */
    if (pitch_estimate > GESTURE_FLIP_PITCH_DEG && gest_state == GEST_STATE_IDLE) {
        result = GESTURE_FLIP;
        gest_state = GEST_STATE_IDLE;
    }

    /* Tap detection: acceleration spike */
    if (gest_state == GEST_STATE_IDLE) {
        if (mag > (int32_t)GESTURE_TAP_ACCEL_THRESH) {
            gest_state = GEST_STATE_TAP_DETECTED;
            gest_timer_ms = sample->timestamp_ms;
        }
    } else if (gest_state == GEST_STATE_TAP_DETECTED) {
        /* Wait for acceleration to settle */
        if (mag < 1200) {  /* below ~1.2 g */
            gest_state = GEST_STATE_WAIT_DOUBLE;
            gest_timer_ms = sample->timestamp_ms;
        }
    } else if (gest_state == GEST_STATE_WAIT_DOUBLE) {
        uint32_t elapsed = sample->timestamp_ms - gest_timer_ms;
        if (mag > (int32_t)GESTURE_TAP_ACCEL_THRESH) {
            /* Second tap within window → double tap */
            if (elapsed < GESTURE_DOUBLE_TAP_GAP_MS) {
                result = GESTURE_DOUBLE_TAP;
            } else {
                result = GESTURE_TAP;  /* first tap expired, this is new */
            }
            gest_state = GEST_STATE_IDLE;
        } else if (elapsed > GESTURE_DOUBLE_TAP_GAP_MS) {
            /* Single tap */
            result = GESTURE_TAP;
            gest_state = GEST_STATE_IDLE;
        }
    }

    return result;
}

void imu_reset_gesture(void)
{
    gest_state = GEST_STATE_IDLE;
    gest_timer_ms = 0;
    shake_active = false;
}

void imu_get_orientation(int16_t *pitch_deg, int16_t *roll_deg)
{
    if (pitch_deg) *pitch_deg = pitch_estimate;
    if (roll_deg)  *roll_deg = roll_estimate;
}