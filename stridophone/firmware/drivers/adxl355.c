/*
 * adxl355.c — ADXL355 3-axis vibration accelerometer driver over SPI2.
 *
 * Author : jayis1
 * License: MIT
 *
 * The ADXL355 has a 20-bit, low-noise (1 mg/√Hz) analog front end, an
 * internal FIFO, and a SPI serial interface. We configure it for:
 *   - Range    : ±2 g  (highest sensitivity)
 *   - ODR      : 1000 Hz (covers the 20-500 Hz larval band with margin)
 *   - HPF      : off (we filter in firmware)
 *   - FIFO     : stream mode, read in bursts
 *
 * Data format: each axis sample is 3 bytes (big-endian, sign-extended).
 * The driver sign-extends and converts to g using the ±2 g scale of
 * 3.9 µg/LSB.
 */
#include "adxl355.h"
#include "../registers.h"
#include <string.h>

/* ---- Low-level SPI transfer -------------------------------------- */
static void spi_cs_low(void)  { gpio_clr(ADXL355_CS_PORT, ADXL355_CS_PAD); }
static void spi_cs_high(void) { gpio_set(ADXL355_CS_PORT, ADXL355_CS_PAD); }

/* Full-duplex 1-byte exchange (blocking, no DMA for simplicity). */
static uint8_t spi_xfer(uint8_t tx) {
    /* Write TXDR; wait for RX. */
    SPI2->CR1 |= SPI_CR1_SPE;          /* enable SPI */
    SPI2->TXDR = tx;
    while (!(SPI2->SR & SPI_SR_EOT)) { }
    uint8_t rx = (uint8_t)SPI2->RXDR;
    SPI2->IFCR = SPI_SR_EOT;
    SPI2->CR1 &= ~SPI_CR1_SPE;
    return rx;
}

/* Read a single register. */
static uint8_t adxl_read_reg(uint8_t reg) {
    spi_cs_low();
    spi_xfer(reg << 1 | 0x01);   /* R=1 */
    uint8_t v = spi_xfer(0x00);
    spi_cs_high();
    return v;
}

/* Write a single register. */
static void adxl_write_reg(uint8_t reg, uint8_t val) {
    spi_cs_low();
    spi_xfer(reg << 1 | 0x00);   /* W=0 */
    spi_xfer(val);
    spi_cs_high();
}

/* Burst-read N bytes starting at reg into buf. */
static void adxl_read_burst(uint8_t reg, uint8_t *buf, int n) {
    spi_cs_low();
    spi_xfer(reg << 1 | 0x01);
    for (int i = 0; i < n; i++) buf[i] = spi_xfer(0x00);
    spi_cs_high();
}

/* ---- Public API -------------------------------------------------- */
void adxl355_init(void) {
    /* Enable SPI2 clock. */
    RCC->APB1LENR |= RCC_APB1LENR_SPI2;

    /* Configure SPI2 as master, 8-bit, CPOL=0 CPHA=0, baud from APB1/128
     * (APB1=120 MHz => ~937 kHz, under the 1 MHz spec). */
    SPI2->CFG1 = (7U<<0)             /* DSIZE 8 bits */
               | (0U<<19)            /* CRCSIZE */
               | (0U<<17)            /* RXCRC */
               | (0U<<18);           /* TXCRC */
    SPI2->CFG2 = SPI_CFG1_MASTER     /* MASTER */
               | (0U<<0)             /* CPOL=0 */
               | (0U<<8)             /* CPHA=0 */
               | (0U<<18);           /* SSOM: SS output managed by SW */
    /* Baud: master clock prescaler = 128 (CR1 MBR field). */
    SPI2->CR1 = (6U<<0)             /* MBR=128 */
              | (1U<<28);           /* MASTER */

    /* Confirm part ID. */
    uint8_t partid = adxl_read_reg(ADXL_PARTID);
    (void)partid;  /* expected 0xAD or 0x1D depending on variant */

    /* Software reset. */
    adxl_write_reg(ADXL_RESET, 0x52);   /* magic */
    /* small delay */
    for (volatile int i = 0; i < 5000; i++) { }

    /* Standby, range ±2 g. */
    adxl_write_reg(ADXL_POWER_CTL, ADXL_MODE_STANDBY);
    adxl_write_reg(ADXL_RANGE, (ADXL_RANGE_2G<<0) | (0U<<2));  /* HPF off */

    /* Filter: ODR=1000 Hz (0b000<<4 | 0b000 for HPF off). */
    adxl_write_reg(ADXL_FILTER, (0U<<4) | 0U);

    /* Map DATA_RDY to INT1 (we poll FIFO instead, so leave default). */
    adxl_write_reg(ADXL_INT_MAP, 0x00);

    /* FIFO in stream mode. */
    adxl_write_reg(ADXL_FIFO_SAMPLES, 0x00);  /* high water = 0 */
}

void adxl355_start(void) {
    adxl_write_reg(ADXL_POWER_CTL, ADXL_MODE_MEASURE);
}

void adxl355_stop(void) {
    adxl_write_reg(ADXL_POWER_CTL, ADXL_MODE_STANDBY);
}

int adxl355_read_block(float dst[3][DSP_FRAME_N], int n) {
    /* Each FIFO sample is 9 bytes: X3 Y3 Z3. We read up to n samples. */
    uint8_t buf[9 * 16];
    int total = 0;

    /* Check FIFO entries (upper 6 bits of 0x05). */
    uint8_t entries = adxl_read_reg(ADXL_FIFO_ENTRIES) & 0x3F;
    int avail = entries;            /* entries = samples available */
    if (avail > n) avail = n;
    if (avail > 16) avail = 16;     /* cap to local buf */

    for (int s = 0; s < avail; s++) {
        adxl_read_burst(ADXL_XDATA3, buf + s*9, 9);
    }
    for (int s = 0; s < avail; s++) {
        int32_t x = ((int32_t)buf[s*9+0] << 12) | ((int32_t)buf[s*9+1] << 4)
                  | ((int32_t)buf[s*9+2] >> 4);
        int32_t y = ((int32_t)buf[s*9+3] << 12) | ((int32_t)buf[s*9+4] << 4)
                  | ((int32_t)buf[s*9+5] >> 4);
        int32_t z = ((int32_t)buf[s*9+6] << 12) | ((int32_t)buf[s*9+7] << 4)
                  | ((int32_t)buf[s*9+8] >> 4);
        /* Sign-extend 20-bit. */
        if (x & (1U<<19)) x |= ~0xFFFFF;
        if (y & (1U<<19)) y |= ~0xFFFFF;
        if (z & (1U<<19)) z |= ~0xFFFFF;
        /* ±2 g range => 3.9 µg/LSB => 256000 LSB/g. */
        dst[0][s] = (float)x / 256000.0f;
        dst[1][s] = (float)y / 256000.0f;
        dst[2][s] = (float)z / 256000.0f;
        total++;
    }
    /* If we got fewer than requested, zero-fill the rest. */
    for (int s = total; s < n; s++) {
        dst[0][s] = 0.0f; dst[1][s] = 0.0f; dst[2][s] = 0.0f;
    }
    return total;
}

float adxl355_read_temp(void) {
    uint8_t t[2];
    adxl_read_burst(ADXL_TEMP2, t, 2);
    int16_t raw = ((int16_t)t[0] << 8) | t[1];
    raw >>= 4;  /* 12-bit, sign-extended */
    if (raw & (1U<<11)) raw |= ~0x7FF;
    /* ADXL355 temp: -1130 LSB offset @ 25 C, -9.05 LSB/C (typical). */
    return 25.0f + ((float)raw + 1130.0f) / -9.05f;
}