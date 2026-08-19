/*
 * fpga.c — iCE40UP5K FPGA configuration + contrast pipeline control
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The FPGA is configured from the W25Q128 SPI flash at boot. After
 * configuration, the STM32 communicates with the FPGA's contrast
 * pipeline over SPI1:
 *
 *   - SPI1 MOSI → FPGA config + control registers (write)
 *   - SPI1 MISO → FPGA contrast-map output (read, DMA)
 *   - FPGA IRQ  → frame-ready interrupt (PC4)
 *   - FPGA CDONE → configuration-done signal (PC5)
 *
 * The FPGA's internal design implements:
 *   1. DVP capture (1280×800 @ 120 fps from OV9281)
 *   2. Bilinear downscaler (1280×800 → 640×480)
 *   3. 7×7 sliding-window speckle contrast engine (pipelined)
 *   4. 8-bit flow-map output over SPI (640×480 @ 60 fps)
 *
 * The STM32 reads frames via SPI1 DMA into a double buffer.
 */

#include "fpga.h"
#include "board.h"
#include "registers.h"

/* FPGA configuration flash (W25Q128) — we don't read it directly here;
 * the FPGA self-configures from flash on power-up. We just wait for CDONE. */

/* FPGA SPI register map (the FPGA presents a set of 8-bit control regs) */
#define FPGA_REG_VERSION      0x00
#define FPGA_REG_CONTROL      0x01
#define FPGA_REG_STATUS       0x02
#define FPGA_REG_WINDOW_SIZE  0x03  /* 0=5×5, 1=7×7, 2=9×9 */
#define FPGA_REG_FRAME_RATE   0x04  /* 0=30, 1=60, 2=120 fps output */
#define FPGA_REG_EXPOSURE_SYNC 0x05 /* camera sync mode */
#define FPGA_REG_FLOW_OFFSET  0x06 /* calibration offset */
#define FPGA_REG_FLOW_SCALE   0x07 /* calibration scale */
#define FPGA_REG_FRAME_CNT    0x08 /* 32-bit frame counter (4 reads) */
#define FPGA_REG_IRQ_ENABLE   0x09

/* Control bits */
#define FPGA_CTRL_ENABLE      0x01
#define FPGA_CTRL_RESET       0x02
#define FPGA_CTRL_SINGLE_SHOT 0x04
#define FPGA_CTRL_CALIBRATE   0x08

/* Status bits */
#define FPGA_STATUS_BUSY      0x01
#define FPGA_STATUS_FRAME_RDY 0x02
#define FPGA_STATUS_OVR       0x04  /* overflow */
#define FPGA_STATUS_ERR       0x08

/* ---- Static state ------------------------------------------------------- */

static volatile int fpga_frame_ready = 0;
static volatile uint32_t fpga_frame_count = 0;

/* ---- Low-level SPI access ----------------------------------------------- */

static void fpga_cs_low(void) {
    FPGA_NSS_PORT->BSRR = (1u << (FPGA_NSS_PIN + 16));
}

static void fpga_cs_high(void) {
    FPGA_NSS_PORT->BSRR = (1u << FPGA_NSS_PIN);
}

static void spi1_wait_tx(void) {
    while (!(SPI1->SR & SPI_SR_TXP)) { }
}

static void spi1_wait_rx(void) {
    while (!(SPI1->SR & SPI_SR_RXP)) { }
}

static void spi1_wait_eot(void) {
    while (!(SPI1->SR & SPI_SR_EOT)) { }
    SPI1->IFCR = SPI_IFCR_CLEAR;
}

static uint8_t spi1_xfer(uint8_t tx) {
    spi1_wait_tx();
    *(volatile uint8_t *)&SPI1->TXDR = tx;
    spi1_wait_rx();
    return *(volatile uint8_t *)&SPI1->RXDR;
}

/* ---- Register access ---------------------------------------------------- */

static uint8_t fpga_read_reg(uint8_t addr) {
    uint8_t val;
    fpga_cs_low();
    spi1_xfer(0x00);        /* read command */
    spi1_xfer(addr);
    val = spi1_xfer(0x00); /* dummy to clock out data */
    fpga_cs_high();
    return val;
}

static void fpga_write_reg(uint8_t addr, uint8_t val) {
    fpga_cs_low();
    spi1_xfer(0x80);        /* write command */
    spi1_xfer(addr);
    spi1_xfer(val);
    fpga_cs_high();
}

/* ---- Public API --------------------------------------------------------- */

int fpga_init(void) {
    /* 1. Hold FPGA in reset */
    FPGA_CRST_PORT->BSRR = (1u << (FPGA_CRST_PIN + 16));  /* low */
    for (volatile int i = 0; i < 10000; i++) { }

    /* 2. Release reset — FPGA self-configures from W25Q128 flash */
    FPGA_CRST_PORT->BSRR = (1u << FPGA_CRST_PIN);  /* high */

    /* 3. Wait for CDONE (max 2 seconds for 16 Mbit flash) */
    uint32_t timeout = 20000000;
    while (!(FPGA_CDONE_PORT->IDR & (1u << FPGA_CDONE_PIN))) {
        if (--timeout == 0) return -1;  /* configuration failed */
    }

    /* 4. Read version register to verify FPGA is alive */
    uint8_t ver = fpga_read_reg(FPGA_REG_VERSION);
    if (ver == 0xFF || ver == 0x00) return -2;  /* no response */

    /* 5. Reset the contrast pipeline */
    fpga_write_reg(FPGA_REG_CONTROL, FPGA_CTRL_RESET);
    for (volatile int i = 0; i < 1000; i++) { }
    fpga_write_reg(FPGA_REG_CONTROL, 0x00);

    /* 6. Default settings: 7×7 window, 60 fps output */
    fpga_write_reg(FPGA_REG_WINDOW_SIZE, 0x01);  /* 7×7 */
    fpga_write_reg(FPGA_REG_FRAME_RATE, 0x01);   /* 60 fps */
    fpga_write_reg(FPGA_REG_IRQ_ENABLE, 0x01);   /* enable frame-ready IRQ */

    /* 7. Enable the pipeline */
    fpga_write_reg(FPGA_REG_CONTROL, FPGA_CTRL_ENABLE);

    return 0;
}

int fpga_set_window(uint8_t size) {
    /* size: 0=5×5, 1=7×7, 2=9×9 */
    if (size > 2) return -1;
    fpga_write_reg(FPGA_REG_WINDOW_SIZE, size);
    return 0;
}

int fpga_set_frame_rate(uint8_t mode) {
    /* mode: 0=30, 1=60, 2=120 fps */
    if (mode > 2) return -1;
    fpga_write_reg(FPGA_REG_FRAME_RATE, mode);
    return 0;
}

void fpga_set_calibration(uint16_t offset, uint16_t scale) {
    fpga_write_reg(FPGA_REG_FLOW_OFFSET, (uint8_t)(offset & 0xFF));
    fpga_write_reg(FPGA_REG_FLOW_SCALE, (uint8_t)(scale & 0xFF));
}

void fpga_enable(int on) {
    if (on) {
        fpga_write_reg(FPGA_REG_CONTROL, FPGA_CTRL_ENABLE);
    } else {
        fpga_write_reg(FPGA_REG_CONTROL, 0x00);
    }
}

void fpga_trigger_single(void) {
    fpga_write_reg(FPGA_REG_CONTROL, FPGA_CTRL_ENABLE | FPGA_CTRL_SINGLE_SHOT);
}

void fpga_start_calibration(void) {
    fpga_write_reg(FPGA_REG_CONTROL, FPGA_CTRL_ENABLE | FPGA_CTRL_CALIBRATE);
}

uint8_t fpga_get_status(void) {
    return fpga_read_reg(FPGA_REG_STATUS);
}

uint32_t fpga_get_frame_count(void) {
    uint8_t b0, b1, b2, b3;
    fpga_cs_low();
    spi1_xfer(0x00);
    spi1_xfer(FPGA_REG_FRAME_CNT);
    b0 = spi1_xfer(0x00);
    b1 = spi1_xfer(0x00);
    b2 = spi1_xfer(0x00);
    b3 = spi1_xfer(0x00);
    fpga_cs_high();
    return ((uint32_t)b3 << 24) | ((uint32_t)b2 << 16) |
           ((uint32_t)b1 << 8) | b0;
}

/* ---- DMA frame reception ----------------------------------------------- */

/* Double buffer for incoming flow-map frames from FPGA.
 * Each frame is 640×480 = 307,200 bytes. */
static uint8_t frame_buf_a[FRAME_BYTES] __attribute__((aligned(32)));
static uint8_t frame_buf_b[FRAME_BYTES] __attribute__((aligned(32)));
static volatile uint8_t *active_rx_buf = frame_buf_a;
static volatile int rx_buf_index = 0;   /* 0 = A, 1 = B */
static volatile int frame_rx_done = 0;

void fpga_start_frame_dma(uint8_t *buf) {
    /* Configure DMA2 Stream 0 for SPI1 RX (peripheral-to-memory) */
    DMA2->Stream[0].CR = 0;
    DMA2->Stream[0].PAR = (uint32_t)&SPI1->RXDR;
    DMA2->Stream[0].M0AR = (uint32_t)buf;
    DMA2->Stream[0].NDTR = FRAME_BYTES;
    DMA2->Stream[0].FCR = 0x05;  /* FIFO threshold 1/4 full */
    DMA2->Stream[0].CR = DMA_CR_DIR_P2M | DMA_CR_MINC | DMA_CR_PSIZE_8 |
                          DMA_CR_MSIZE_8 | DMA_CR_PL_VHIGH |
                          DMA_CR_TCIE | DMA_CR_EN;

    /* Enable SPI RX DMA */
    SPI1->CFG1 |= SPI_CFG1_RXDMAEN;

    /* Start SPI transfer: send dummy bytes to clock out FPGA data */
    fpga_cs_low();
    SPI1->CR1 |= SPI_CR1_CSTART;
}

void fpga_isr_frame_ready(void) {
    /* Called from the FPGA IRQ (PC4 external interrupt).
     * Start DMA reception of the new frame. */
    fpga_frame_ready = 1;
    fpga_frame_count++;

    /* Swap buffers */
    uint8_t *buf = (rx_buf_index == 0) ? frame_buf_b : frame_buf_a;
    rx_buf_index ^= 1;
    active_rx_buf = (rx_buf_index == 0) ? frame_buf_a : frame_buf_b;

    fpga_start_frame_dma(buf);
}

void fpga_isr_dma_complete(void) {
    /* DMA2 Stream0 transfer complete — frame fully received */
    SPI1->CFG1 &= ~SPI_CFG1_RXDMAEN;
    fpga_cs_high();
    frame_rx_done = 1;
}

uint8_t *fpga_get_frame(void) {
    /* Returns the most recently completed frame buffer, or NULL. */
    if (!frame_rx_done) return NULL;
    frame_rx_done = 0;
    return (rx_buf_index == 0) ? frame_buf_b : frame_buf_a;
}

int fpga_is_frame_ready(void) {
    return fpga_frame_ready;
}

uint32_t fpga_get_version(void) {
    return fpga_read_reg(FPGA_REG_VERSION);
}