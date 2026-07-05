/*
 * fpga.c — iCE40-UP5K SPI front-end driver
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Loads the FPGA bitstream over SPI, then streams zero-suppressed hit
 * words from the iCE40's SPRAM-resident FIFO. The FPGA runs the
 * coincidence logic and TDC interface; this driver is just the bridge.
 */
#include "fpga.h"
#include "registers.h"
#include "board.h"
#include <string.h>

extern volatile uint32_t g_hit_watermark_irq_count;

/* ---- Private helpers ---- */
static void fpga_cs_assert(void)
{
    /* SPI hardware-managed NSS; here we use software CS via CRESET/CDONE
     * only for config. Normal transfers use SPI-NSS. We do nothing. */
}

static muon_status_t spi_wait_eot(spi_regs_t *spi, uint32_t timeout_ms)
{
    uint32_t t0 = muon_millis();
    while (!(spi->SR & SPI_SR_EOT)) {
        if ((muon_millis() - t0) > timeout_ms)
            return MUON_ERR_TIMEOUT;
    }
    return MUON_OK;
}

static muon_status_t spi_xfer8(spi_regs_t *spi, uint8_t tx, uint8_t *rx)
{
    /* Set 8-bit frame size */
    spi->CFG1 = (spi->CFG1 & ~0xF) | SPI_CFG1_DSIZE_8;
    /* Clear end-of-transfer flag */
    spi->IFCR = SPI_SR_EOT | SPI_SR_OVR | SPI_SR_MODF;
    /* Master transfer start */
    spi->CR1 |= SPI_CR1_CSTART;
    /* Wait TXP */
    uint32_t t0 = muon_millis();
    while (!(spi->SR & SPI_SR_TXP)) {
        if ((muon_millis() - t0) > 5) return MUON_ERR_TIMEOUT;
    }
    REG8((uint32_t)&spi->TXDR) = tx;
    /* Wait RXP */
    t0 = muon_millis();
    while (!(spi->SR & SPI_SR_RXP)) {
        if ((muon_millis() - t0) > 10) return MUON_ERR_TIMEOUT;
    }
    if (rx) *rx = REG8((uint32_t)&spi->RXDR);
    /* Wait end of transfer */
    MUON_RETURN_ON_ERR(spi_wait_eot(spi, 10));
    return MUON_OK;
}

static muon_status_t spi_xfer_burst(spi_regs_t *spi, const uint8_t *tx,
                                    uint8_t *rx, uint16_t n)
{
    spi->CFG1 = (spi->CFG1 & ~0xF) | SPI_CFG1_DSIZE_8;
    spi->CR2 = n;  /* number of frames in master transfer */
    spi->IFCR = SPI_SR_EOT | SPI_SR_OVR | SPI_SR_MODF;
    spi->CR1 |= SPI_CR1_CSTART;
    for (uint16_t i = 0; i < n; ++i) {
        uint32_t t0 = muon_millis();
        while (!(spi->SR & SPI_SR_TXP))
            if ((muon_millis() - t0) > 5) return MUON_ERR_TIMEOUT;
        REG8((uint32_t)&spi->TXDR) = tx ? tx[i] : 0;
        t0 = muon_millis();
        while (!(spi->SR & SPI_SR_RXP))
            if ((muon_millis() - t0) > 10) return MUON_ERR_TIMEOUT;
        if (rx) rx[i] = REG8((uint32_t)&spi->RXDR);
    }
    MUON_RETURN_ON_ERR(spi_wait_eot(spi, 20));
    return MUON_OK;
}

/* ---- Bitstream load (SPI configuration mode) ----
 * The iCE40 family loads bitstreams via a simple SPI protocol:
 * 1. Hold CRESET_B low for >1 us, then release
 * 2. Send 8 dummy bytes
 * 3. Send the bitstream (LSB-first per byte)
 * 4. Send 49 dummy bytes for startup clock
 * 5. Wait CDONE high
 */
muon_status_t fpga_load_bitstream(const uint8_t *bit, uint32_t len)
{
    if (!bit || len == 0) return MUON_ERR_INVALID_ARG;

    /* Pulse CRESET_B low then high */
    GPIO_BSRR(FPGA_CRESET_PORT) = FPGA_CRESET_PIN << 16; /* reset */
    muon_delay_ms(2);
    GPIO_BSRR(FPGA_CRESET_PORT) = FPGA_CRESET_PIN;      /* release */
    muon_delay_ms(1);

    spi_regs_t *spi = (spi_regs_t *)SPI1_BASE;
    /* Disable SPI to reconfigure for FPGA load (CPOL=1/CPHA=1, MSB) */
    spi->CR1 &= ~SPI_CR1_SPE;
    spi->CFG2 = SPI_CFG2_CPOL | SPI_CFG2_CPHA | SPI_CFG2_MASTER | SPI_CFG2_SSOE;
    spi->CFG1 = SPI_CFG1_MASTER | SPI_CFG1_DSIZE_8 | 0x7; /* baud div /8 */
    spi->CR1 |= SPI_CR1_SPE;

    /* 8 dummy bytes */
    uint8_t dummy[8] = {0};
    MUON_RETURN_ON_ERR(spi_xfer_burst(spi, dummy, NULL, 8));

    /* Bitstream body */
    uint16_t blk = 64;
    for (uint32_t off = 0; off < len; off += blk) {
        uint16_t n = (uint16_t)((len - off < blk) ? (len - off) : blk);
        MUON_RETURN_ON_ERR(spi_xfer_burst(spi, bit + off, NULL, n));
    }

    /* 49 trailing dummy bytes for startup clocks */
    uint8_t tail[49];
    memset(tail, 0, sizeof(tail));
    MUON_RETURN_ON_ERR(spi_xfer_burst(spi, tail, NULL, 49));

    /* Wait CDONE */
    uint32_t t0 = muon_millis();
    while (!(GPIO_IDR(FPGA_CDONE_PORT) & BIT(FPGA_CDONE_PIN))) {
        if ((muon_millis() - t0) > 200) return MUON_ERR_FPGA_LOAD;
    }

    /* Reconfigure SPI for normal hit-stream mode (50 MHz, CPOL=1/CPHA=1) */
    spi->CR1 &= ~SPI_CR1_SPE;
    spi->CFG2 = SPI_CFG2_CPOL | SPI_CFG2_CPHA | SPI_CFG2_MASTER | SPI_CFG2_SSOE;
    spi->CFG1 = SPI_CFG1_MASTER | SPI_CFG1_DSIZE_8;  /* full speed */
    spi->CR1 |= SPI_CR1_SPE;

    return MUON_OK;
}

muon_status_t fpga_init(void)
{
    /* GPIO setup for CRESET, CDONE, IRQ, HOLD */
    GPIO_MODER(FPGA_CRESET_PORT) |= (1U << (FPGA_CRESET_PIN * 2));  /* output */
    GPIO_OTYPER(FPGA_CRESET_PORT) &= ~(1U << FPGA_CRESET_PIN);
    GPIO_OSPEEDR(FPGA_CRESET_PORT) |= (3U << (FPGA_CRESET_PIN * 2));

    /* CDONE input */
    GPIO_MODER(FPGA_CDONE_PORT) &= ~(3U << (FPGA_CDONE_PIN * 2));
    GPIO_PUPDR(FPGA_CDONE_PORT) |= (1U << (FPGA_CDONE_PIN * 2));   /* pull-up */

    /* HOLD output (active low) — default high */
    GPIO_MODER(FPGA_HOLD_PORT) |= (1U << (FPGA_HOLD_PIN * 2));
    GPIO_BSRR(FPGA_HOLD_PORT) = FPGA_HOLD_PIN;  /* deassert hold */

    /* IRQ input with pull-down */
    GPIO_MODER(FPGA_IRQ_PORT) &= ~(3U << (FPGA_IRQ_PIN * 2));
    GPIO_PUPDR(FPGA_IRQ_PORT) |= (2U << (FPGA_IRQ_PIN * 2));  /* pull-down */

    /* SPI pins AF5 */
    uint32_t porta = GPIOA_BASE;
    GPIO_MODER(porta) &= ~(3U << (FPGA_SCK_PIN * 2));
    GPIO_MODER(porta) &= ~(3U << (FPGA_MISO_PIN * 2));
    GPIO_MODER(porta) &= ~(3U << (FPGA_MOSI_PIN * 2));
    GPIO_MODER(porta) |= (2U << (FPGA_SCK_PIN * 2))
                      | (2U << (FPGA_MISO_PIN * 2))
                      | (2U << (FPGA_MOSI_PIN * 2));
    GPIO_AFRL(porta) &= ~(0xFU << (FPGA_SCK_PIN * 4))
                    &  ~(0xFU << (FPGA_MISO_PIN * 4))
                    &  ~(0xFU << (FPGA_MOSI_PIN * 4));
    GPIO_AFRL(porta) |= (5U << (FPGA_SCK_PIN * 4))
                     | (5U << (FPGA_MISO_PIN * 4))
                     | (5U << (FPGA_MOSI_PIN * 4));
    GPIO_OSPEEDR(porta) |= (3U << (FPGA_SCK_PIN * 2))
                        | (3U << (FPGA_MISO_PIN * 2))
                        | (3U << (FPGA_MOSI_PIN * 2));

    /* Enable SPI1 clock */
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;

    spi_regs_t *spi = (spi_regs_t *)SPI1_BASE;
    spi->CR1 = 0;
    spi->CFG2 = SPI_CFG2_CPOL | SPI_CFG2_CPHA | SPI_CFG2_MASTER | SPI_CFG2_SSOE;
    spi->CFG1 = SPI_CFG1_MASTER | SPI_CFG1_DSIZE_8;
    spi->CR1 |= SPI_CR1_SPE;

    return MUON_OK;
}

muon_status_t fpga_set_coincidence_window(uint8_t ns)
{
    spi_regs_t *spi = (spi_regs_t *)SPI1_BASE;
    uint8_t cmd[2] = { FPGA_CMD_SET_WINDOW, ns };
    MUON_RETURN_ON_ERR(spi_xfer_burst(spi, cmd, NULL, 2));
    return MUON_OK;
}

uint32_t fpga_read_version(void)
{
    spi_regs_t *spi = (spi_regs_t *)SPI1_BASE;
    uint8_t tx[4] = { FPGA_CMD_GET_VER, 0, 0, 0 };
    uint8_t rx[4] = {0};
    if (spi_xfer_burst(spi, tx, rx, 4) != MUON_OK) return 0;
    return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
}

uint32_t fpga_hit_count(void)
{
    spi_regs_t *spi = (spi_regs_t *)SPI1_BASE;
    uint8_t tx[4] = { FPGA_CMD_GET_HITS, 0, 0, 0 };
    uint8_t rx[4] = {0};
    if (spi_xfer_burst(spi, tx, rx, 4) != MUON_OK) return 0xFFFFFFFF;
    /* Returns 24-bit count in rx[1..3] */
    return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
}

muon_status_t fpga_read_burst(muon_hit_t *out, uint16_t max, uint16_t *got)
{
    if (!out || !got) return MUON_ERR_INVALID_ARG;
    spi_regs_t *spi = (spi_regs_t *)SPI1_BASE;

    uint32_t avail = fpga_hit_count();
    uint16_t n = (uint16_t)((avail > max) ? max : avail);
    if (n == 0) { *got = 0; return MUON_OK; }

    /* Burst: opcode, length, then n*4 bytes hit data */
    uint8_t tx[2 + FPGA_BURST_MAX * 4];
    uint8_t rx[2 + FPGA_BURST_MAX * 4];
    tx[0] = FPGA_CMD_RD_BURST;
    tx[1] = (uint8_t)n;
    uint16_t total = 2 + n * 4;
    if (total > sizeof(tx)) return MUON_ERR_MEMORY;

    MUON_RETURN_ON_ERR(spi_xfer_burst(spi, tx, rx, total));
    for (uint16_t i = 0; i < n; ++i) {
        uint8_t *p = &rx[2 + i * 4];
        out[i].raw = ((uint32_t)p[0]) | ((uint32_t)p[1] << 8)
                   | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    }
    *got = n;
    return MUON_OK;
}

muon_status_t fpga_clear_fifo(void)
{
    spi_regs_t *spi = (spi_regs_t *)SPI1_BASE;
    uint8_t cmd = FPGA_CMD_FIFO_CLR;
    return spi_xfer_burst(spi, &cmd, NULL, 1);
}

void fpga_hold_set(int hold)
{
    if (hold)
        GPIO_BSRR(FPGA_HOLD_PORT) = FPGA_HOLD_PIN << 16;  /* low = hold */
    else
        GPIO_BSRR(FPGA_HOLD_PORT) = FPGA_HOLD_PIN;          /* high = run */
}

void fpga_irq_enable(int en)
{
    if (en) {
        /* EXTI line = FPGA_IRQ_PIN; configure rising edge */
        /* ( EXTI setup omitted for brevity — would write EXTI->IMR1 etc. ) */
        NVIC_ISER0 |= BIT(23);  /* EXTI9_5 IRQ */
    } else {
        NVIC_ICER0 |= BIT(23);
    }
}

void fpga_irq_handler(void)
{
    g_hit_watermark_irq_count++;
}

/* ---- Embedded bitstream (placeholder) ----
 * The full iCE40 bitstream is generated from the Verilog in
 * firmware/fpga/muonscape_top.v (not committed to keep size down).
 * For bring-up we ship a known-good minimal bitstream that just
 * forwards a heartbeat counter; replace with the real one after
 * synthesis. The array below is a tiny stand-in.
 */
static const uint8_t k_default_bitstream[64] = {
    0x7E, 0xAA, 0x99, 0x7E, 0x12, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E
};

muon_status_t fpga_load_default(void)
{
    return fpga_load_bitstream(k_default_bitstream, sizeof(k_default_bitstream));
}