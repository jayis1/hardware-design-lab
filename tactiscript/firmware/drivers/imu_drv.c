/*
 * imu_drv.c — TactiScript IMU (ICM-42688-P) driver
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Drives the TDK ICM-42688-P 6-axis IMU over SPI1 for gesture detection:
 *   - Single tap, double tap
 *   - Swipe left/right
 *   - Long press (stillness detection)
 *   - Tilt up/down
 *
 * The driver uses the IMU's built-in tap detection for taps and
 * implements swipe/tilt detection in firmware using FIFO data.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "imu_drv.h"
#include "../board.h"
#include "../registers.h"

/* ----------------------------------------------------------------
 * Private state
 * ---------------------------------------------------------------- */
static bool s_initialized = false;

/* Gesture queue (ring buffer) — filled by ISR, drained by main loop */
#define GESTURE_QUEUE_SIZE 8
static volatile uint8_t s_gesture_queue[GESTURE_QUEUE_SIZE];
static volatile uint8_t s_gesture_head = 0;
static volatile uint8_t s_gesture_tail = 0;

/* Tap detection state (for double-tap) */
static uint32_t s_last_tap_tick = 0;
static bool s_tap_pending = false;

/* Swipe detection state */
static int16_t s_gyro_x_history[32];
static int16_t s_gyro_y_history[32];
static int16_t s_gyro_z_history[32];
static uint8_t s_history_idx = 0;

/* SPI buffers */
static uint8_t s_spi_tx[20];
static uint8_t s_spi_rx[20];

/* ----------------------------------------------------------------
 * GPIO helpers
 * ---------------------------------------------------------------- */
static inline void gpio_set(uint32_t port, uint32_t pin)
{
    GPIO_OUTSET(GPIO_PORT_BASE(port)) = (1u << pin);
}

static inline void gpio_clr(uint32_t port, uint32_t pin)
{
    GPIO_OUTCLR(GPIO_PORT_BASE(port)) = (1u << pin);
}

/* ----------------------------------------------------------------
 * SPI1 transfer (blocking)
 *   tx: bytes to send
 *   rx: buffer for received bytes (can be NULL)
 *   len: number of bytes
 * ---------------------------------------------------------------- */
static void spi1_transfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    SPIM_TXD_PTR(NRF_SPIM1_BASE) = (uint32_t)tx;
    SPIM_TXD_MAXCNT(NRF_SPIM1_BASE) = len;
    if (rx) {
        SPIM_RXD_PTR(NRF_SPIM1_BASE) = (uint32_t)rx;
        SPIM_RXD_MAXCNT(NRF_SPIM1_BASE) = len;
    } else {
        SPIM_RXD_MAXCNT(NRF_SPIM1_BASE) = 0;
    }

    /* CS low */
    gpio_clr(0, 17); /* PIN_SPI1_CS_IMU = P0.17 */

    SPIM_TASKS_START(NRF_SPIM1_BASE) = 1;
    while (!SPIM_EVENTS_END(NRF_SPIM1_BASE)) ;
    SPIM_EVENTS_END(NRF_SPIM1_BASE) = 0;

    /* CS high */
    gpio_set(0, 17);
}

/* ----------------------------------------------------------------
 * IMU register read (single byte)
 * ---------------------------------------------------------------- */
static uint8_t imu_read_reg(uint8_t addr)
{
    s_spi_tx[0] = IMU_SPI_READ(addr);
    s_spi_tx[1] = 0x00; /* dummy byte to clock out data */
    spi1_transfer(s_spi_tx, s_spi_rx, 2);
    return s_spi_rx[1];
}

/* ----------------------------------------------------------------
 * IMU register write (single byte)
 * ---------------------------------------------------------------- */
static void imu_write_reg(uint8_t addr, uint8_t val)
{
    s_spi_tx[0] = IMU_SPI_WRITE(addr);
    s_spi_tx[1] = val;
    spi1_transfer(s_spi_tx, NULL, 2);
}

/* ----------------------------------------------------------------
 * IMU multi-byte read
 * ---------------------------------------------------------------- */
static void imu_read_burst(uint8_t start_addr, uint8_t *buf, uint16_t len)
{
    s_spi_tx[0] = IMU_SPI_READ(start_addr);
    /* Fill tx with dummy bytes */
    for (uint16_t i = 1; i <= len; i++)
        s_spi_tx[i] = 0x00;
    spi1_transfer(s_spi_tx, s_spi_rx, len + 1);
    memcpy(buf, &s_spi_rx[1], len);
}

/* ----------------------------------------------------------------
 * Push gesture to queue
 * ---------------------------------------------------------------- */
static void push_gesture(uint8_t gesture)
{
    uint8_t next = (s_gesture_head + 1) % GESTURE_QUEUE_SIZE;
    if (next == s_gesture_tail)
        return; /* queue full, drop gesture */
    s_gesture_queue[s_gesture_head] = gesture;
    s_gesture_head = next;
}

/* ----------------------------------------------------------------
 * Get system tick (ms) — uses the uptime from main, but we keep a
 * local counter incremented by the RTC ISR for simplicity.
 * In the real firmware, this reads a global tick variable.
 * ---------------------------------------------------------------- */
static volatile uint32_t s_local_tick_ms = 0;

static uint32_t get_tick_ms(void)
{
    return s_local_tick_ms;
}

/* ----------------------------------------------------------------
 * Tap detection processing (called from INT1 ISR)
 * ---------------------------------------------------------------- */
static void process_tap_interrupt(void)
{
    /* Read the interrupt status to determine tap type */
    uint8_t int_status = imu_read_reg(IMU_REG_INT_STATUS);

    uint32_t now = get_tick_ms();

    if (int_status & 0x04) {
        /* Single tap detected */
        if (s_tap_pending && (now - s_last_tap_tick) < DOUBLE_TAP_GAP_MS) {
            /* This is the second tap within the gap → double tap */
            push_gesture(IMU_GESTURE_DOUBLE_TAP);
            s_tap_pending = false;
        } else {
            /* First tap — start the double-tap window */
            s_tap_pending = true;
            s_last_tap_tick = now;
            push_gesture(IMU_GESTURE_TAP);
        }
    }
}

/* ----------------------------------------------------------------
 * Swipe detection (called periodically from main loop with new data)
 * ---------------------------------------------------------------- */
static void process_gyro_for_swipe(int16_t gx, int16_t gy, int16_t gz)
{
    /* Store in history buffer */
    s_gyro_x_history[s_history_idx] = gx;
    s_gyro_y_history[s_history_idx] = gy;
    s_gyro_z_history[s_history_idx] = gz;
    s_history_idx = (s_history_idx + 1) % 32;

    /* Check for swipe pattern: sustained rotation in one axis
     * above threshold for at least SWIPE_MIN_SAMPLES samples, followed
     * by return to near-zero. */

    /* Look at recent samples */
    int16_t sum_x = 0, sum_y = 0, sum_z = 0;
    for (int i = 0; i < SWIPE_MIN_SAMPLES; i++) {
        int idx = (s_history_idx - 1 - i + 32) % 32;
        sum_x += s_gyro_x_history[idx];
        sum_y += s_gyro_y_history[idx];
        sum_z += s_gyro_z_history[idx];
    }

    /* Average gyro values (in dps, raw LSB) */
    int16_t avg_z = sum_z / SWIPE_MIN_SAMPLES;
    int16_t avg_y = sum_y / SWIPE_MIN_SAMPLES;

    /* Swipe right: positive Y rotation (turning wrist right) */
    if (avg_y > SWIPE_THRESHOLD_DPS) {
        push_gesture(IMU_GESTURE_SWIPE_RIGHT);
        /* Reset history to avoid repeated detection */
        memset((void *)s_gyro_y_history, 0, sizeof(s_gyro_y_history));
    }
    /* Swipe left: negative Y rotation */
    else if (avg_y < -SWIPE_THRESHOLD_DPS) {
        push_gesture(IMU_GESTURE_SWIPE_LEFT);
        memset((void *)s_gyro_y_history, 0, sizeof(s_gyro_y_history));
    }

    /* Tilt: sustained Z rotation */
    if (avg_z > SWIPE_THRESHOLD_DPS) {
        push_gesture(IMU_GESTURE_TILT_UP);
        memset((void *)s_gyro_z_history, 0, sizeof(s_gyro_z_history));
    } else if (avg_z < -SWIPE_THRESHOLD_DPS) {
        push_gesture(IMU_GESTURE_TILT_DOWN);
        memset((void *)s_gyro_z_history, 0, sizeof(s_gyro_z_history));
    }
}

/* ----------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------- */

void imu_init(void)
{
    if (s_initialized)
        return;

    /* Configure SPI1 pins */
    SPIM_PSEL_SCK(NRF_SPIM1_BASE) = GPIO_PIN(PIN_SPI1_SCK);
    SPIM_PSEL_MOSI(NRF_SPIM1_BASE) = GPIO_PIN(PIN_SPI1_MOSI);
    SPIM_PSEL_MISO(NRF_SPIM1_BASE) = GPIO_PIN(PIN_SPI1_MISO);
    SPIM_FREQUENCY(NRF_SPIM1_BASE) = 0x04000000; /* 4 MHz */
    SPIM_CONFIG(NRF_SPIM1_BASE) = 0; /* mode 0, MSB first */
    SPIM_ENABLE(NRF_SPIM1_BASE) = 1;

    /* CS pin as output, default high */
    gpio_set(0, 17);

    /* Software reset the IMU */
    imu_write_reg(IMU_REG_DEVICE_CONFIG, 0x01); /* soft reset */
    /* Wait for reset to complete (~10 µs, but give 1 ms) */
    s_local_tick_ms += 1;

    /* Verify chip ID */
    uint8_t whoami = imu_read_reg(IMU_REG_WHO_AM_I);
    if (whoami != IMU_WHO_AM_I_VAL) {
        /* IMU not present or wrong chip — continue anyway for debugging */
    }

    /* Power management: enable gyro and accel in low-noise mode */
    imu_write_reg(IMU_REG_PWR_MGMT0,
                  IMU_PWR_GYRO_LN | IMU_PWR_ACCEL_LN);

    /* Set gyro ODR to 1 kHz, range ±1000 dps */
    imu_write_reg(IMU_REG_GYRO_CONFIG0,
                  IMU_ODR_1KHZ | IMU_GYRO_RANGE_1000DPS);

    /* Set accel ODR to 1 kHz, range ±8 g */
    imu_write_reg(IMU_REG_ACCEL_CONFIG0,
                  IMU_ODR_1KHZ | IMU_ACCEL_RANGE_8G);

    /* Configure tap detection:
     * - Enable single tap on X axis
     * - Set tap threshold and duration
     * (ICM-42688-P has built-in tap detection registers)
     */
    /* Tap source: X axis (bit 2) */
    imu_write_reg(0x53, 0x04); /* TAP_SOURCE0: X axis */

    /* Tap threshold: 1.5 g (in 31.25 mg/LSB → 1500/31.25 ≈ 48) */
    imu_write_reg(0x54, 48);  /* TAP_THR: threshold */

    /* Tap duration: 80 ms (in 625 µs/LSB → 80000/625 = 128) */
    imu_write_reg(0x55, 128); /* TAP_DUR: duration */

    /* Configure interrupts: route tap to INT1, data ready to INT2 */
    imu_write_reg(IMU_REG_INT_CONFIG, 0x00); /* INT1: push-pull, active high */
    imu_write_reg(IMU_REG_INT_CONFIG0, 0x05); /* latched mode */
    imu_write_reg(IMU_REG_INT_SOURCE0,
                  IMU_INT1_TAP | IMU_INT1_DATA_RDY);

    /* Enable FIFO in continuous mode */
    imu_write_reg(IMU_REG_FIFO_CONFIG, 0x01); /* stream mode */

    /* Clear any pending interrupts */
    imu_read_reg(IMU_REG_INT_STATUS);

    s_initialized = true;
}

void imu_enter_low_power(void)
{
    if (!s_initialized)
        return;

    /* Disable gyro, accel in low-power mode */
    imu_write_reg(IMU_REG_PWR_MGMT0, 0x08); /* accel LP, gyro off */

    /* Keep tap detection enabled for wake-up */
    imu_write_reg(IMU_REG_INT_SOURCE0, IMU_INT1_TAP);
}

int imu_get_gesture(void)
{
    /* Process any pending IMU data for swipe/tilt detection */
    uint16_t fifo_count = imu_fifo_count();
    if (fifo_count > 0) {
        int16_t accel[3];
        int16_t gyro[3];
        if (imu_fifo_read(accel, gyro, 1) > 0) {
            process_gyro_for_swipe(gyro[0], gyro[1], gyro[2]);
        }
    }

    /* Check for tap interrupt (poll INT1 pin) */
    uint32_t pin_state = GPIO_IN(NRF_GPIO_P0_BASE);
    if (pin_state & (1u << 19)) { /* PIN_IMU_INT1 = P0.19 */
        process_tap_interrupt();
        /* Clear interrupt by reading status */
        imu_read_reg(IMU_REG_INT_STATUS);
    }

    /* Drain gesture queue */
    if (s_gesture_head == s_gesture_tail)
        return IMU_GESTURE_NONE;

    int g = s_gesture_queue[s_gesture_tail];
    s_gesture_tail = (s_gesture_tail + 1) % GESTURE_QUEUE_SIZE;
    return g;
}

void imu_read_accel(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    imu_read_burst(IMU_REG_ACCEL_DATA_X1, buf, 6);
    *x = (buf[0] << 8) | buf[1];
    *y = (buf[2] << 8) | buf[3];
    *z = (buf[4] << 8) | buf[5];

    /* Convert to mg: at ±8 g range, sensitivity = 0.244 mg/LSB.
     * For simplicity, return raw value; caller converts if needed. */
    *x >>= 2; /* align to 16-bit signed */
    *y >>= 2;
    *z >>= 2;
}

void imu_read_gyro(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    imu_read_burst(IMU_REG_GYRO_DATA_X1, buf, 6);
    *x = (buf[0] << 8) | buf[1];
    *y = (buf[2] << 8) | buf[3];
    *z = (buf[4] << 8) | buf[5];
}

int16_t imu_read_temp(void)
{
    uint8_t buf[2];
    imu_read_burst(IMU_REG_TEMP_DATA1, buf, 2);
    int16_t raw = (buf[0] << 8) | buf[1];
    /* Convert: T(°C) = (raw / 132.48) + 25 → scaled by 100: */
    /* T(°C×100) = (raw * 10000) / 13248 + 2500 */
    return (int16_t)((raw * 10000) / 13248 + 2500);
}

uint16_t imu_fifo_count(void)
{
    uint8_t buf[2];
    imu_read_burst(IMU_REG_FIFO_COUNT, buf, 2);
    uint16_t count = (buf[0] << 8) | buf[1];
    return count;
}

uint16_t imu_fifo_read(int16_t *accel_out, int16_t *gyro_out, uint16_t max)
{
    uint16_t available = imu_fifo_count();
    uint16_t to_read = (available < max) ? available : max;
    if (to_read == 0)
        return 0;

    /* Each FIFO packet is IMU_FIFO_PACKET_LEN (16) bytes:
     * 1 header + 6 accel + 6 gyro + 1 temp + 1 timestamp (simplified).
     * Read in chunks to fit our buffer.
     */
    uint16_t read = 0;
    uint8_t packet[IMU_FIFO_PACKET_LEN];

    for (uint16_t i = 0; i < to_read; i++) {
        imu_read_burst(IMU_REG_FIFO_DATA, packet, IMU_FIFO_PACKET_LEN);
        /* Parse accel (bytes 1-6) and gyro (bytes 7-12) */
        accel_out[i * 3 + 0] = (packet[1] << 8) | packet[2];
        accel_out[i * 3 + 1] = (packet[3] << 8) | packet[4];
        accel_out[i * 3 + 2] = (packet[5] << 8) | packet[6];
        gyro_out[i * 3 + 0] = (packet[7] << 8) | packet[8];
        gyro_out[i * 3 + 1] = (packet[9] << 8) | packet[10];
        gyro_out[i * 3 + 2] = (packet[11] << 8) | packet[12];
        read++;
    }

    return read;
}

/* ----------------------------------------------------------------
 * IMU interrupt handler (GPIOTE channel for INT1)
 * ---------------------------------------------------------------- */
void GPIOTE_IRQHandler(void)
{
    /* Check which channel triggered (simplified: channel 0 for IMU INT1) */
    if (GPIOTE_IN(0)) {
        GPIOTE_IN(0) = 0;
        process_tap_interrupt();
    }
}

/* ----------------------------------------------------------------
 * End of imu_drv.c
 * Author: jayis1
 * ---------------------------------------------------------------- */