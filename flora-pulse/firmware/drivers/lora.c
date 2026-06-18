/*
 * lora.c — SX1276 LoRa transceiver driver
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Drives the Semtech SX1276 LoRa transceiver via SPI2 (shared with
 * ADS1220 and SD card via CS-multiplexing).  The SX1276 provides
 * long-range (up to 15 km line-of-sight) low-bandwidth communication
 * for field-deployed FloraPulse nodes that send periodic stress
 * telemetry to a gateway.
 *
 * Configuration:
 *  - Frequency: 868 MHz (EU ISM band)
 *  - Modulation: LoRa (CSS)
 *  - Spreading factor: 12 (maximum range, ~290 bits/s)
 *  - Bandwidth: 125 kHz
 *  - Coding rate: 4/5
 *  - TX power: 14 dBm (20 dBm with PA_BOOST)
 *  - Sync word: 0x34 (private network)
 *
 * The SX1276 uses a standard 4-wire SPI interface.  Register address
 * byte: MSB = 0 for write, MSB = 1 for read.
 */

#include "lora.h"
#include "../board.h"
#include "../registers.h"

/* ===================================================================== */
/*  Internal state                                                       */
/* ===================================================================== */

static int16_t last_rssi = 0;
static float last_snr = 0.0f;
static uint8_t lora_cs_pin = 7;  /* Using PB7 for LoRa CS (separate from ADS1220/SD) */

/* LoRa CS — we use PC7 as a separate CS since SPI2 is shared.
 * Actually, let's use PA7... wait, PA7 is SPI1_MOSI.
 * We need a dedicated GPIO for LoRa CS.
 * Let's use PC7 for LoRa CS (output).
 */
#define LORA_CS_PORT   GPIOC_BASE
#define LORA_CS_PIN    7

/* ===================================================================== */
/*  SPI2 low-level (same as sapflow.c — but we need our own select)     */
/* ===================================================================== */

static void spi2_wait_tx(void)
{
    while (!(SPI_REG(SPI2_BASE, SPI_SR) & SPI_SR_TXE))
        ;
}

static void spi2_wait_rx(void)
{
    while (!(SPI_REG(SPI2_BASE, SPI_SR) & SPI_SR_RXNE))
        ;
}

static uint8_t spi2_xfer(uint8_t tx)
{
    spi2_wait_tx();
    SPI_REG(SPI2_BASE, SPI_DR) = tx;
    spi2_wait_rx();
    return (uint8_t)SPI_REG(SPI2_BASE, SPI_DR);
}

static void lora_select(void)
{
    gpio_reset(LORA_CS_PORT, LORA_CS_PIN);
}

static void lora_deselect(void)
{
    gpio_set(LORA_CS_PORT, LORA_CS_PIN);
}

static uint8_t lora_rreg(uint8_t reg)
{
    lora_select();
    spi2_xfer(reg | SX1276_SPI_READ);
    uint8_t val = spi2_xfer(0x00U);
    lora_deselect();
    return val;
}

static void lora_wreg(uint8_t reg, uint8_t val)
{
    lora_select();
    spi2_xfer(reg | SX1276_SPI_WRITE);
    spi2_xfer(val);
    lora_deselect();
}

static void lora_wreg_fifo(const uint8_t *data, uint8_t len)
{
    lora_select();
    spi2_xfer(SX1276_REG_FIFO | SX1276_SPI_WRITE);
    for (uint8_t i = 0; i < len; i++)
        spi2_xfer(data[i]);
    lora_deselect();
}

static void lora_rreg_fifo(uint8_t *buf, uint8_t len)
{
    lora_select();
    spi2_xfer(SX1276_REG_FIFO | SX1276_SPI_READ);
    for (uint8_t i = 0; i < len; i++)
        buf[i] = spi2_xfer(0x00U);
    lora_deselect();
}

/* ===================================================================== */
/*  Initialization                                                       */
/* ===================================================================== */

void lora_init(void)
{
    /* Enable GPIOC and SPI2 clocks (SPI2 may already be enabled by sapflow) */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    RCC_APB1ENR1 |= RCC_APB1ENR1_SPI2EN;

    /* PC7 (LoRa CS), PC5 (RST) as outputs */
    volatile uint32_t *pc_moder = (volatile uint32_t *)(GPIOC_BASE + GPIO_MODER);
    *pc_moder |= (0x1U << (LORA_CS_PIN * 2));
    *pc_moder |= (0x1U << (LORA_RST_PIN * 2));

    /* PC3 (DIO0), PC4 (DIO1) as inputs */
    *pc_moder &= ~(0x3U << (LORA_DIO0_PIN * 2));
    *pc_moder &= ~(0x3U << (LORA_DIO1_PIN * 2));

    gpio_set(LORA_CS_PORT, LORA_CS_PIN);
    gpio_set(LORA_RST_PORT, LORA_RST_PIN);

    /* Hardware reset */
    gpio_reset(LORA_RST_PORT, LORA_RST_PIN);
    for (volatile int i = 0; i < 10000; i++)
        ;
    gpio_set(LORA_RST_PORT, LORA_RST_PIN);
    for (volatile int i = 0; i < 10000; i++)
        ;

    /* Verify chip version */
    uint8_t version = lora_rreg(SX1276_REG_VERSION);
    (void)version;  /* Should be 0x12 for SX1276 */

    /* Sleep mode to configure */
    lora_wreg(SX1276_REG_OP_MODE, SX1276_MODE_LONG_RANGE | SX1276_MODE_SLEEP);
    for (volatile int i = 0; i < 1000; i++)
        ;

    /* Set frequency: 868 MHz
     * Frf = freq × 2^19 / 32 MHz
     * 868e6 × 524288 / 32e6 = 868e6 / 32 × 524288 = 14221312
     * RegFrf = 0xD8 0x46 0x00? Let's compute:
     * 868000000 * 524288 / 32000000 = 14221312.0
     * = 0xD90000? 14221312 = 0xD90000
     * MSB = 0xD9, MID = 0x00, LSB = 0x00
     * Actually: 14221312 = 0xD90000
     * MSB = 0xD9, MID = 0x00, LSB = 0x00
     */
    lora_wreg(SX1276_REG_FRF_MSB, 0xD9U);
    lora_wreg(SX1276_REG_FRF_MID, 0x00U);
    lora_wreg(SX1276_REG_FRF_LSB, 0x00U);

    /* PA config: PA_BOOST pin, 14 dBm
     * Bit 7 = PA_BOOST, bits [3:0] = power (14 → 0x0E)
     * MaxPower bits [6:4] = 0x4 (default)
     */
    lora_wreg(SX1276_REG_PA_CONFIG, 0x80U | 0x4EU);  /* PA_BOOST + 14 dBm */

    /* OCP (over-current protection): 100 mA
     * RegOcp = 0x0B, bits [3:0] = trim, bit 5 = OcpOn
     * 100 mA → trim = 0x0B (0x20 | 0x0B = 0x2B)
     */
    lora_wreg(0x0BU, 0x2BU);

    /* LNA: max gain, HF mode
     * RegLna = 0x0C, bits [7:5] = gain (001 = max), bits [2:0] = boost
     */
    lora_wreg(0x0CU, 0x23U);

    /* Modem config 1: BW 125 kHz, CR 4/5, explicit header
     * BW[7:4] = 0111 (125 kHz), CR[3:1] = 001 (4/5), HeaderMode[0] = 0
     */
    lora_wreg(SX1276_REG_MODEM_CONFIG1, 0x72U);

    /* Modem config 2: SF12, CRC on
     * SpreadingFactor[7:4] = 0xC (SF12), TxContMode[3] = 0, RxPayloadCrcOn[2] = 1
     * SymbTimeoutMSB[1:0] = 00
     */
    lora_wreg(SX1276_REG_MODEM_CONFIG2, 0xC4U);

    /* Modem config 3: LowDataRateOptimize on, AGC auto on
     * MobileNode[7] = 0, LowDataRateOptimize[3] = 1, AGCAutoOn[2] = 1
     */
    lora_wreg(SX1276_REG_MODEM_CONFIG3, 0x0CU);

    /* Symbol timeout: 1023 (max) */
    lora_wreg(SX1276_REG_SYMB_TIMEOUT, 0xFFU);

    /* Preamble length: 8 */
    lora_wreg(SX1276_REG_PREAMBLE_MSB, 0x00U);
    lora_wreg(SX1276_REG_PREAMBLE_LSB, 0x08U);

    /* Sync word: 0x34 (private network) */
    lora_wreg(0x39U, SX1276_REG_SYNC_WORD);

    /* FIFO pointers */
    lora_wreg(SX1276_REG_FIFO_TX_BASE, 0x80U);
    lora_wreg(SX1276_REG_FIFO_RX_BASE, 0x00U);

    /* DIO mapping: DIO0 = TxDone/RxDone, DIO1 = RxTimeout */
    lora_wreg(SX1276_REG_DIO_MAPPING1, 0x00U);

    /* PA DAC: normal mode (not +20 dBm) */
    lora_wreg(SX1276_REG_PADAC, 0x84U);

    /* Switch to standby */
    lora_wreg(SX1276_REG_OP_MODE, SX1276_MODE_LONG_RANGE | SX1276_MODE_STDBY);
    for (volatile int i = 0; i < 1000; i++)
        ;
}

/* ===================================================================== */
/*  TX / RX                                                              */
/* ===================================================================== */

uint8_t lora_send(const uint8_t *data, uint8_t len)
{
    if (len > 64) return 1;  /* Max payload for SF12/BW125 */

    /* Standby mode */
    lora_wreg(SX1276_REG_OP_MODE, SX1276_MODE_LONG_RANGE | SX1276_MODE_STDBY);
    for (volatile int i = 0; i < 1000; i++)
        ;

    /* Set FIFO pointer to TX base */
    lora_wreg(SX1276_REG_FIFO_ADDR_PTR, 0x80U);

    /* Write payload length */
    lora_wreg(SX1276_REG_PAYLOAD_LEN, len);

    /* Write data to FIFO */
    lora_wreg_fifo(data, len);

    /* Clear IRQ flags */
    lora_wreg(SX1276_REG_IRQ_FLAGS, 0xFFU);

    /* Switch to TX mode */
    lora_wreg(SX1276_REG_OP_MODE, SX1276_MODE_LONG_RANGE | SX1276_MODE_TX);

    /* Wait for TxDone (DIO0 rising edge or IRQ flag) */
    uint32_t timeout = 5000000;
    while (!(lora_rreg(SX1276_REG_IRQ_FLAGS) & SX1276_IRQ_TX_DONE) && timeout--)
        ;

    /* Clear flag, return to standby */
    lora_wreg(SX1276_REG_IRQ_FLAGS, 0xFFU);
    lora_wreg(SX1276_REG_OP_MODE, SX1276_MODE_LONG_RANGE | SX1276_MODE_STDBY);

    return (timeout == 0) ? 2 : 0;
}

void lora_rx_start(void)
{
    /* Standby first */
    lora_wreg(SX1276_REG_OP_MODE, SX1276_MODE_LONG_RANGE | SX1276_MODE_STDBY);
    for (volatile int i = 0; i < 1000; i++)
        ;

    /* Clear IRQ flags */
    lora_wreg(SX1276_REG_IRQ_FLAGS, 0xFFU);

    /* Set FIFO pointer to RX base */
    lora_wreg(SX1276_REG_FIFO_ADDR_PTR, 0x00U);

    /* RX continuous mode */
    lora_wreg(SX1276_REG_OP_MODE, SX1276_MODE_LONG_RANGE | SX1276_MODE_RX_CONTINUOUS);
}

uint8_t lora_rx_read(uint8_t *buf, uint8_t maxlen)
{
    /* Check if RxDone */
    uint8_t flags = lora_rreg(SX1276_REG_IRQ_FLAGS);
    if (!(flags & SX1276_IRQ_RX_DONE))
        return 0;

    /* Read SNR and RSSI */
    uint8_t snr_val = lora_rreg(SX1276_REG_PKT_SNR);
    last_snr = (float)((int8_t)snr_val) / 4.0f;

    uint8_t rssi_val = lora_rreg(SX1276_REG_PKT_RSSI);
    last_rssi = -157 + (int16_t)rssi_val;  /* HF: RSSI = -157 + value */

    /* Read packet */
    uint8_t curr_addr = lora_rreg(SX1276_REG_FIFO_RX_CURR);
    uint8_t nb_bytes = lora_rreg(SX1276_REG_RX_NB_BYTES);
    if (nb_bytes > maxlen) nb_bytes = maxlen;

    lora_wreg(SX1276_REG_FIFO_ADDR_PTR, curr_addr);
    lora_rreg_fifo(buf, nb_bytes);

    /* Clear IRQ flags */
    lora_wreg(SX1276_REG_IRQ_FLAGS, 0xFFU);

    return nb_bytes;
}

int16_t lora_get_rssi(void) { return last_rssi; }
float lora_get_snr(void)    { return last_snr; }

void lora_sleep(void)
{
    lora_wreg(SX1276_REG_OP_MODE, SX1276_MODE_LONG_RANGE | SX1276_MODE_SLEEP);
}

void lora_wakeup(void)
{
    lora_wreg(SX1276_REG_OP_MODE, SX1276_MODE_LONG_RANGE | SX1276_MODE_STDBY);
    for (volatile int i = 0; i < 1000; i++)
        ;
}

uint8_t lora_irq_pending(void)
{
    return gpio_read(LORA_DIO0_PORT, LORA_DIO0_PIN);
}

void lora_clear_irq(void)
{
    lora_wreg(SX1276_REG_IRQ_FLAGS, 0xFFU);
}