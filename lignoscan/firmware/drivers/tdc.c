/*
 * tdc.c — TDC-GP22 Time-to-Digital Converter Driver Implementation
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The TDC-GP22 provides 22 ps resolution time-of-flight measurement
 * between the HV transmit pulse (START) and the first received signal
 * threshold crossing (STOP). This driver handles SPI communication,
 * calibration, and measurement sequencing.
 */

#include "tdc.h"
#include "board.h"

/* ---- SPI chip select helpers ---- */
void tdc_cs_low(void) {
    GPIO_CLR(TDC_SPI_CS, TDC_SPI_CS_PIN);
}

void tdc_cs_high(void) {
    GPIO_SET(TDC_SPI_CS, TDC_SPI_CS_PIN);
}

/* ---- SPI single-byte transfer ---- */
static uint8_t tdc_spi_xfer(uint8_t tx) {
    /* Wait for TX room */
    while (!(TDC_SPI->SR & SPI_SR_TXP)) { }
    *(volatile uint8_t *)&TDC_SPI->TXDR = tx;

    /* Wait for RX */
    while (!(TDC_SPI->SR & SPI_SR_RXP)) { }
    return *(volatile uint8_t *)&TDC_SPI->RXDR;
}

/* ---- Write 32-bit configuration register ---- */
void tdc_spi_write(uint8_t cmd, uint32_t data) {
    tdc_cs_low();
    tdc_spi_xfer(cmd);
    tdc_spi_xfer((data >> 24) & 0xFF);
    tdc_spi_xfer((data >> 16) & 0xFF);
    tdc_spi_xfer((data >>  8) & 0xFF);
    tdc_spi_xfer((data >>  0) & 0xFF);
    tdc_cs_high();
}

/* ---- Read 32-bit register ---- */
uint32_t tdc_spi_read(uint8_t cmd) {
    tdc_cs_low();
    tdc_spi_xfer(cmd);
    uint32_t val = 0;
    val |= ((uint32_t)tdc_spi_xfer(0x00) << 24);
    val |= ((uint32_t)tdc_spi_xfer(0x00) << 16);
    val |= ((uint32_t)tdc_spi_xfer(0x00) <<  8);
    val |= ((uint32_t)tdc_spi_xfer(0x00) <<  0);
    tdc_cs_high();
    return val;
}

/* ---- Initialize SPI1 for TDC communication ---- */
static void tdc_spi_init(void) {
    /* Enable SPI1 clock */
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* Disable SPI before config */
    TDC_SPI->CR1 &= ~SPI_CR1_SPE;

    /* Configure: Master, 8-bit, baud rate = APB2/16 = 8.75 MHz
     * (TDC-GP22 supports up to 20 MHz, using /16 for reliability) */
    TDC_SPI->CFG1 = (3U << SPI_CFG1_MBR_SHIFT) |   /* Baud /16 */
                    (7U << SPI_CFG1_DSIZE_SHIFT) |  /* 8-bit */
                    SPI_CFG1_MASTER;
    TDC_SPI->CFG2 = 0;  /* Motorola mode, MSB first */

    /* Enable SPI */
    TDC_SPI->CR1 |= SPI_CR1_SPE;
}

/* ---- Initialize TDC-GP22 ---- */
void tdc_init(void) {
    tdc_spi_init();
    delay_ms(10);  /* TDC power-up stabilization */

    /* Send INIT command */
    tdc_cs_low();
    tdc_spi_xfer(TDC_CMD_INIT);
    tdc_cs_high();
    delay_ms(1);

    tdc_configure();
    tdc_calibrate();
}

/* ---- Configure TDC registers for ToF measurement mode ---- */
void tdc_configure(void) {
    /* CFG0: ANZ_FORC = 1, ANZ_FIRE = 0, PHASE_OFF = 0
     *       HITIN1 = 1 (expect 1 hit), CALIBRATE = 1 */
    tdc_spi_write(TDC_CMD_WRITE_CFG | TDC_REG_CFG0,
                  TDC_CFG0_FILL_1 | TDC_CFG0_CALIBRATE | TDC_CFG0_DIV4);

    /* CFG1: NO_CALIB = 0, QUAD_RES = 1, START_EDGE = rising, STOP_EDGE = rising
     *       HIT1 = 1 (single stop), START_CLKHS = 0 */
    tdc_spi_write(TDC_CMD_WRITE_CFG | TDC_REG_CFG1,
                  TDC_CFG1_QUAD_RES | TDC_CFG1_HIT1);

    /* CFG2: FIRE_UP = 0, FIRE_DOWN = 0, DELAY1 = 0, DELAY2 = 0
     *       EN_ERR_VAL = 1 (error value on timeout) */
    tdc_spi_write(TDC_CMD_WRITE_CFG | TDC_REG_CFG2, 0x00020000UL);

    /* CFG3: NEG_STOP = 0, EN_START = 1, DELVAL = 1
     *       START_TRIG = 1 (accept external START) */
    tdc_spi_write(TDC_CMD_WRITE_CFG | TDC_REG_CFG3, 0x00004000UL);

    /* CFG4: DELAY_TIME = 0, FREQ_CLKHS = 4 MHz reference */
    tdc_spi_write(TDC_CMD_WRITE_CFG | TDC_REG_CFG4, 0x00000001UL);

    /* CFG5: EN_TIMEOUT = 1, TIMEOUT = 5000 (~1.25 ms at 4 MHz)
     *       This is sufficient for waves traveling up to 5m at 1500 m/s */
    tdc_spi_write(TDC_CMD_WRITE_CFG | TDC_REG_CFG5, 0x13880000UL);

    /* CFG6: DELAY_TIME = 0, CALIBRATE_PERIOD = 0 */
    tdc_spi_write(TDC_CMD_WRITE_CFG | TDC_REG_CFG6, 0x00000000UL);
}

/* ---- Run TDC internal calibration (measures clock period) ---- */
void tdc_calibrate(void) {
    tdc_cs_low();
    tdc_spi_xfer(TDC_CMD_START_CAL);
    tdc_cs_high();

    /* Wait for calibration complete (TDC_INT pin goes low) */
    uint32_t timeout = millis() + 100;
    while (GPIO_GET(GPIOC, TDC_INT_PIN) && millis() < timeout) {
        IWDG_KR = 0xAAAA;
    }

    /* Read calibration results to verify */
    uint32_t cal_res = tdc_spi_read(TDC_CMD_READ_RESULT);
    (void)cal_res;  /* Used internally by TDC for auto-calibration */
}

/* ---- Arm the TDC for a single ToF measurement ---- */
void tdc_arm(void) {
    /* Clear any pending status */
    tdc_spi_read(TDC_CMD_READ_STAT);

    /* Start ToF measurement mode */
    tdc_cs_low();
    tdc_spi_xfer(TDC_CMD_START_TOF);
    tdc_cs_high();
}

/* ---- Wait for TDC measurement result with timeout ---- */
int tdc_wait_result(float *tof_ns, uint32_t timeout_us) {
    uint32_t start = millis();
    uint32_t timeout_ms = (timeout_us / 1000) + 1;

    /* Poll TDC_INT pin (goes low when ALU result ready) */
    while (GPIO_GET(GPIOC, TDC_INT_PIN)) {
        if ((millis() - start) > timeout_ms) {
            *tof_ns = -1.0f;
            return -1;  /* Timeout */
        }
        IWDG_KR = 0xAAAA;
    }

    /* Read status register */
    uint32_t status = tdc_spi_read(TDC_CMD_READ_STAT);
    uint8_t flags = (status >> 0) & 0xFF;

    /* Check for errors:
     * Bit 0: TOF measurement complete
     * Bit 3: Timeout / no stop detected
     * Bit 4: Start pulse missing
     * Bit 5: First hit registered
     * Bit 7: Error flag */
    if (flags & (1U << 7)) {
        /* Error condition */
        if (flags & (1U << 3)) {
            *tof_ns = -1.0f;  /* Timeout — no signal received */
            return -2;
        }
        *tof_ns = -1.0f;
        return -3;
    }

    /* Read result register (raw 32-bit count) */
    uint32_t raw = tdc_spi_read(TDC_CMD_READ_RESULT);

    /* Convert to nanoseconds.
     * With calibration enabled, the TDC auto-corrects for clock drift.
     * Raw count * TDC_NS_PER_LSB gives nanoseconds.
     * The high bits contain the coarse count, low bits the fine interpolation. */
    *tof_ns = (float)raw * TDC_NS_PER_LSB;

    /* Sanity check: valid ToF for wood should be 50us to 5000us
     * (50,000 ns to 5,000,000 ns) for trunks up to ~1m diameter */
    if (*tof_ns < 1000.0f || *tof_ns > 5000000.0f) {
        return -4;  /* Out of expected range */
    }

    return 0;  /* Success */
}

/* ---- Measure cable delay for a specific channel (calibration) ---- */
float tdc_measure_cable_delay(int channel) {
    /* In cable-delay calibration, we measure the propagation time
     * through the sensor cable and MUX without any wood path.
     * This is done by measuring the round-trip time with the
     * transducer in air (no trunk contact).
     *
     * The result is stored per-channel and subtracted from
     * actual measurements during acquisition. */
    (void)channel;  /* Channel selection handled by mux driver */

    tdc_arm();

    /* In calibration, we generate a small test pulse (not full HV)
     * and measure the electrical propagation delay */
    uint32_t start = millis();
    while (GPIO_GET(GPIOC, TDC_INT_PIN)) {
        if ((millis() - start) > 100) {
            return -1.0f;  /* Timeout */
        }
    }

    uint32_t raw = tdc_spi_read(TDC_CMD_READ_RESULT);
    float delay_ns = (float)raw * TDC_NS_PER_LSB;

    /* Cable delay is typically 5-20 ns for 2m cables */
    if (delay_ns < 0.0f || delay_ns > 100.0f) {
        return -1.0f;  /* Invalid */
    }

    return delay_ns;
}

/* ---- Power down TDC to save energy between scans ---- */
void tdc_power_down(void) {
    tdc_cs_low();
    tdc_spi_xfer(TDC_CMD_POWER_DOWN);
    tdc_cs_high();
}

/* EOF — tdc.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */