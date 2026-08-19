/*
 * camera.c — OV9281 global-shutter camera driver for SpeckleFlow
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The OV9281 is configured via SCCB (I²C-compatible) on I2C1 at 0x60.
 * We run it in 8-bit DVP monochrome mode at 1280×800, 120 fps, with
 * global shutter. The FPGA captures the DVP stream and performs the
 * speckle contrast pipeline.
 *
 * The STM32 only needs to:
 *   1. Power up the camera (PWDN pin low, XSHUTDOWN high)
 *   2. Program registers via SCCB for the desired mode
 *   3. Trigger / gate exposure via the EXPOSURE register
 */

#include "camera.h"
#include "board.h"
#include "registers.h"

/* OV9281 SCCB slave address (7-bit, shifted left for I2C) */
#define OV9281_ADDR  0x60

/* OV9281 register map (selected registers) */
#define OV9281_REG_CHIP_ID        0x300A  /* should read 0x9281 */
#define OV9281_REG_CHIP_ID_2      0x300B  /* should read 0x4281 */
#define OV9281_REG_MODE_SELECT    0x0100  /* 0=standby, 1=active */
#define OV9281_REG_EXPOSURE_H     0x3500  /* [19:16] */
#define OV9281_REG_EXPOSURE_M     0x3501  /* [15:8] */
#define OV9281_REG_EXPOSURE_L     0x3502  /* [7:0] */
#define OV9281_REG_ANALOG_GAIN_H  0x3508  /* [9:8] */
#define OV9281_REG_ANALOG_GAIN_L  0x3509  /* [7:0] */
#define OV9281_REG_TIMING_HTS_H   0x3800  /* HTS [11:8] */
#define OV9281_REG_TIMING_HTS_L   0x3801  /* HTS [7:0] */
#define OV9281_REG_TIMING_VTS_H   0x3802  /* VTS [11:8] */
#define OV9281_REG_TIMING_VTS_L   0x3803  /* VTS [7:0] */
#define OV9281_REG_TIMING_VSTART_H 0x3804
#define OV9281_REG_TIMING_VSTART_L 0x3805
#define OV9281_REG_TIMING_VEND_H   0x3806
#define OV9281_REG_TIMING_VEND_L   0x3807
#define OV9281_REG_TIMING_HSTART_H 0x3808
#define OV9281_REG_TIMING_HSTART_L 0x3809
#define OV9281_REG_TIMING_HEND_H   0x380A
#define OV9281_REG_TIMING_HEND_L   0x380B
#define OV9281_REG_FORMAT          0x3820 /* format / mirror / flip */
#define OV9281_REG_DVP_HTS         0x3800
#define OV9281_REG_PIXEL_CLOCK     0x4837 /* PCLK divider */
#define OV9281_REG_PLL_CTRL0       0x0303
#define OV9281_REG_PLL_CTRL1       0x0304
#define OV9281_REG_PLL_CTRL2       0x0305
#define OV9281_REG_PLL_CTRL3       0x0306
#define OV9281_REG_PLL_CTRL4       0x0307
#define OV9281_REG_BANDING_FILTER  0x5000
#define OV9281_REG_AEC_AGC         0x3503 /* bit[1]=0 manual, bit[1]=1 auto */
#define OV9281_REG_AEC_MAX         0x3504
#define OV9281_REG_FRAME_CTRL      0x4201
#define OV9281_REG_SYNC_CTRL       0x4740

/* Power-up sequence: PWDN low → XSHUTDOWN high → wait 5 ms → SCCB */
#define CAM_PWDN_PORT   GPIOC
#define CAM_PWDN_PIN    6
#define CAM_XSHUT_PORT  GPIOC
#define CAM_XSHUT_PIN   7

/* ---- I2C (SCCB) primitives ---------------------------------------------- */

static void i2c_wait_tx(void) {
    while (!(I2C1->ISR & I2C_ISR_TXE)) { }
}

static void i2c_wait_rx(void) {
    while (!(I2C1->ISR & I2C_ISR_RXNE)) { }
}

static void i2c_wait_tc(void) {
    while (!(I2C1->ISR & I2C_ISR_TC)) { }
}

static void i2c_wait_stop(void) {
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;
}

static int i2c_check_nack(void) {
    if (I2C1->ISR & I2C_ISR_NACKF) {
        I2C1->ICR = I2C_ISR_NACKF;
        return -1;
    }
    return 0;
}

static int sccb_write(uint8_t reg_hi, uint8_t reg_lo, uint8_t val) {
    uint32_t timeout = 100000;

    /* Wait for bus free */
    while (I2C1->ISR & I2C_ISR_BUSY) {
        if (--timeout == 0) return -1;
    }

    /* Program CR2: 7-bit addr << 1, 3 bytes, auto-end, write */
    I2C1->CR2 = ((uint32_t)OV9281_ADDR << 1)
              | (3u << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_AUTOEND;

    I2C1->CR2 |= I2C_CR2_START;

    /* Send register address high byte */
    i2c_wait_tx();
    if (i2c_check_nack()) return -1;
    I2C1->TXDR = reg_hi;

    /* Send register address low byte */
    i2c_wait_tx();
    if (i2c_check_nack()) return -1;
    I2C1->TXDR = reg_lo;

    /* Send data byte */
    i2c_wait_tx();
    if (i2c_check_nack()) return -1;
    I2C1->TXDR = val;

    i2c_wait_stop();
    return 0;
}

static int sccb_read(uint8_t reg_hi, uint8_t reg_lo, uint8_t *val) {
    uint32_t timeout = 100000;

    while (I2C1->ISR & I2C_ISR_BUSY) {
        if (--timeout == 0) return -1;
    }

    /* Phase 1: write register address (2 bytes, reload, no stop) */
    I2C1->CR2 = ((uint32_t)OV9281_ADDR << 1)
              | (2u << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_RELOAD;
    I2C1->CR2 |= I2C_CR2_START;

    i2c_wait_tx();
    if (i2c_check_nack()) return -1;
    I2C1->TXDR = reg_hi;
    i2c_wait_tx();
    if (i2c_check_nack()) return -1;
    I2C1->TXDR = reg_lo;

    i2c_wait_tc();

    /* Phase 2: read 1 byte, auto-end */
    I2C1->CR2 = ((uint32_t)OV9281_ADDR << 1)
              | I2C_CR2_RD_WRN
              | (1u << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;

    i2c_wait_rx();
    *val = (uint8_t)(I2C1->RXDR & 0xFF);
    i2c_wait_stop();
    return 0;
}

/* ---- Public API --------------------------------------------------------- */

int camera_init(void) {
    /* 1. Hardware power-up: PWDN=0, XSHUTDOWN=1, wait 5 ms */
    CAM_PWDN_PORT->BSRR = (1u << (CAM_PWDN_PIN + 16));   /* reset PWDN low */
    CAM_XSHUT_PORT->BSRR = (1u << CAM_XSHUT_PIN);        /* set XSHUT high */

    /* crude delay: ~5 ms at 480 MHz (loop ~480k iters) */
    for (volatile uint32_t i = 0; i < 500000; i++) { }

    /* 2. Read chip ID to verify camera is present */
    uint8_t id_hi = 0, id_lo = 0;
    if (sccb_read(0x30, 0x0A, &id_hi) != 0) return -1;
    if (sccb_read(0x30, 0x0B, &id_lo) != 0) return -1;
    if (id_hi != 0x92 || id_lo != 0x81) {
        /* Camera not detected or wrong part */
        return -2;
    }

    /* 3. Software standby */
    sccb_write(0x01, 0x00, 0x00);

    /* 4. PLL configuration for 120 fps at 1280×800
     *    Input clock = 24 MHz (onboard oscillator)
     *    PLL: VCO = 24 / 1 * 80 = 1920 MHz
     *    SYS = 1920 / 4 = 480 MHz
     *    PCLK = 480 / 4 = 120 MHz
     */
    sccb_write(0x03, 0x03, 0x01);  /* PLL pre-div = 1 */
    sccb_write(0x03, 0x04, 0x50);  /* PLL multiplier = 80 */
    sccb_write(0x03, 0x05, 0x00);  /* PLL charge pump */
    sccb_write(0x03, 0x06, 0x03);  /* PLL div = 4 for SYS */
    sccb_write(0x03, 0x07, 0x03);  /* PLL div = 4 for PCLK */
    sccb_write(0x48, 0x37, 0x18);  /* PCLK divider */

    /* 5. Timing: 1280×800 active, 120 fps
     *    HTS = 1652, VTS = 800
     */
    sccb_write(0x38, 0x00, 0x06);  /* HTS[11:8] */
    sccb_write(0x38, 0x01, 0x74);  /* HTS[7:0]  → 1652 */
    sccb_write(0x38, 0x02, 0x03);  /* VTS[11:8] */
    sccb_write(0x38, 0x03, 0x20);  /* VTS[7:0]  → 800 */

    /* 6. Active window: 1280×800 starting at (16, 8) */
    sccb_write(0x38, 0x08, 0x00);  /* HSTART[11:8] */
    sccb_write(0x38, 0x09, 0x10);  /* HSTART[7:0] → 16 */
    sccb_write(0x38, 0x0A, 0x05);  /* HEND[11:8] */
    sccb_write(0x38, 0x0B, 0x0F);  /* HEND[7:0]  → 1295 */
    sccb_write(0x38, 0x0C, 0x00);  /* VSTART[11:8] */
    sccb_write(0x38, 0x0D, 0x08);  /* VSTART[7:0] → 8 */
    sccb_write(0x38, 0x0E, 0x03);  /* VEND[11:8] */
    sccb_write(0x38, 0x0F, 0x18);  /* VEND[7:0]  → 792 */

    /* 7. Format: monochrome, global shutter, no mirror/flip */
    sccb_write(0x38, 0x20, 0x00);  /* format: RAW mono, no flip */
    sccb_write(0x38, 0x21, 0x00);

    /* 8. AEC/AGC: manual mode for LSCI (must be manual!) */
    sccb_write(0x35, 0x03, 0x00);  /* AEC/AGC manual */
    sccb_write(0x35, 0x04, 0x0F);  /* AEC max = 0x0FFF */
    sccb_write(0x35, 0x08, 0x00);  /* analog gain high = 0 */
    sccb_write(0x35, 0x09, 0x10);  /* analog gain low = 1× */

    /* 9. Set default exposure (5 ms) */
    camera_set_exposure(CAM_EXPOSURE_US);

    /* 10. DVP output format: 8-bit parallel, global shutter sync */
    sccb_write(0x47, 0x40, 0x01);  /* DVP mode enable */
    sccb_write(0x42, 0x01, 0x00);  /* frame ctrl */

    /* 11. Start streaming */
    sccb_write(0x01, 0x00, 0x01);  /* mode = active */

    return 0;
}

int camera_set_exposure(uint32_t us) {
    /* Exposure register is in units of 1/120 MHz = 8.33 ns.
     * For 5000 µs → 5000 / 0.00833 = 600000 → 0x927C0
     * Clamp to 20-bit max (0xFFFFF).
     */
    uint32_t exp_val = (us * 120u) / 1u;  /* simplified: us × 120 */
    if (exp_val > 0xFFFFFu) exp_val = 0xFFFFFu;

    uint8_t h = (uint8_t)((exp_val >> 16) & 0x0F);
    uint8_t m = (uint8_t)((exp_val >> 8) & 0xFF);
    uint8_t l = (uint8_t)(exp_val & 0xFF);

    if (sccb_write(0x35, 0x00, h) != 0) return -1;
    if (sccb_write(0x35, 0x01, m) != 0) return -1;
    if (sccb_write(0x35, 0x02, l) != 0) return -1;
    return 0;
}

int camera_set_gain(uint16_t gain) {
    /* Analog gain: 1× to 4× in 64 steps.
     * gain 0x0010 = 1×, 0x0020 = 2×, 0x0040 = 4×
     */
    if (gain > 0x7F) gain = 0x7F;
    if (sccb_write(0x35, 0x08, (uint8_t)(gain >> 8)) != 0) return -1;
    if (sccb_write(0x35, 0x09, (uint8_t)(gain & 0xFF)) != 0) return -1;
    return 0;
}

int camera_set_fps(uint8_t fps) {
    /* Adjust VTS to change frame rate. At HTS=1652, PCLK=120 MHz:
     * fps = PCLK / (HTS × VTS) → VTS = PCLK / (HTS × fps)
     * For 120 fps: VTS = 120e6 / (1652 × 120) = 605 → but our VTS is 800.
     * We tune VTS per desired fps.
     */
    uint32_t vts = 120000000u / ((uint32_t)1652 * fps);
    if (vts > 0xFFF) vts = 0xFFF;
    if (vts < 100) vts = 100;

    if (sccb_write(0x38, 0x02, (uint8_t)(vts >> 8)) != 0) return -1;
    if (sccb_write(0x38, 0x03, (uint8_t)(vts & 0xFF)) != 0) return -1;
    return 0;
}

void camera_trigger(void) {
    /* In global-shutter mode, a trigger is implicit on each VSYNC.
     * For single-shot capture, we pulse the FRAME_CTRL register.
     */
    sccb_write(0x42, 0x01, 0x01);
    for (volatile int i = 0; i < 1000; i++) { }
    sccb_write(0x42, 0x01, 0x00);
}

int camera_standby(void) {
    return sccb_write(0x01, 0x00, 0x00);
}

int camera_read_id(uint16_t *id) {
    uint8_t hi = 0, lo = 0;
    if (sccb_read(0x30, 0x0A, &hi) != 0) return -1;
    if (sccb_read(0x30, 0x0B, &lo) != 0) return -1;
    *id = ((uint16_t)hi << 8) | lo;
    return 0;
}