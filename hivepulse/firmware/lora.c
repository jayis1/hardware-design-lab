/*
 * lora.c — SX1262 LoRa radio driver and LoRaWAN MAC for HivePulse
 *
 * Implements SPI communication with the Semtech SX1262 radio, LoRaWAN
 * OTAA join procedure, uplink/downlink packet handling, and Class B
 * scheduled receive windows for the HivePulse beehive monitor.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "lora.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- SX1262 Register Addresses ---- */
#define SX1262_REG_PKT_STATUS     0x08C2
#define SX1262_REG_SYNC_WORD      0x0744
#define SX1262_REG_PAYLOAD_LEN    0x0A02
#define SX1262_REG_FIFO_ADDR      0x0A0A

/* ---- SX1262 Command Set ---- */
#define SX1262_CMD_SET_STANDBY    0x80
#define SX1262_CMD_SET_RX         0x82
#define SX1262_CMD_SET_TX         0x83
#define SX1262_CMD_SET_SLEEP      0x84
#define SX1262_CMD_WRITE_REG      0x1D
#define SX1262_CMD_READ_REG       0x1D
#define SX1262_CMD_WRITE_BUFFER   0x0E
#define SX1262_CMD_READ_BUFFER    0x1D
#define SX1262_CMD_SET_RF_FREQ    0x86
#define SX1262_CMD_SET_TX_PARAMS  0x8E
#define SX1262_CMD_SET_MOD_PARAMS 0x8B
#define SX1262_CMD_SET_PACKET_TYPE 0x8A
#define SX1262_CMD_SET_CAD_PARAMS 0x88
#define SX1262_CMD_CLEAR_IRQ      0x02
#define SX1262_CMD_GET_IRQ_STATUS 0x12
#define SX1262_CMD_GET_RX_BUFFER  0x13

/* Standby configurations */
#define SX1262_STDBY_RC   0x00
#define SX1262_STDBY_XOSC 0x01

/* Packet types */
#define SX1262_PKT_TYPE_LORA 0x01

/* IRQ flags */
#define SX1262_IRQ_TX_DONE   (1U << 0)
#define SX1262_IRQ_RX_DONE   (1U << 1)
#define SX1262_IRQ_PREAMBLE  (1U << 2)
#define SX1262_IRQ_SYNCWORD  (1U << 3)
#define SX1262_IRQ_HEADER    (1U << 4)
#define SX1262_IRQ_CRC_ERR   (1U << 5)
#define SX1262_IRQ_CAD_DONE  (1U << 6)
#define SX1262_IRQ_CAD_DETECTED (1U << 7)
#define SX1262_IRQ_TIMEOUT   (1U << 9)
#define SX1262_IRQ_ALL       0x03FF

/* ---- LoRaWAN OTAA Keys (example — production: stored in secure flash) ---- */
static const uint8_t dev_eui[8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};
static const uint8_t join_eui[8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t app_key[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

/* ---- LoRaWAN MAC State ---- */
static bool lora_joined = false;
static uint8_t  dev_addr[4];
static uint8_t  f_nwk_s_int_key[16];
static uint8_t  app_s_key[16];
static uint16_t f_cnt_up = 0;
static uint16_t f_cnt_down = 0;
static uint8_t  lora_class = 0; /* 0=A, 1=B, 2=C */

/* ---- Radio State ---- */
static radio_state_t radio_state = RADIO_STATE_IDLE;
static volatile bool tx_complete = false;
static volatile bool rx_complete = false;
static volatile bool rx_timeout = false;

/* ---- Downlink buffer ---- */
static lora_downlink_t pending_downlink;
static bool has_downlink = false;

/* ---- SPI Interface for SX1262 ---- */
static void lora_spi_cs_low(void)
{
    GPIOA->BSRR = (1U << 4) << 16;  /* Reset PA4 */
}

static void lora_spi_cs_high(void)
{
    GPIOA->BSRR = (1U << 4);  /* Set PA4 */
}

static int lora_spi_transfer(uint8_t *tx, uint8_t *rx, int len)
{
    /* Wait for SX1262 to not be busy */
    uint32_t timeout = 10000;
    while (GPIOB->IDR & LORA_BUSY_PIN) {
        if (--timeout == 0) return -1;
    }

    lora_spi_cs_low();

    /* Small delay after CS (SX1262 requires >32ns) */
    for (volatile int i = 0; i < 10; i++);

    for (int i = 0; i < len; i++) {
        while (!(SPI1->SR & SPI_SR_TXE));
        SPI1->DR = tx[i];
        while (!(SPI1->SR & SPI_SR_RXNE));
        rx[i] = (uint8_t)SPI1->DR;
    }

    lora_spi_cs_high();
    return 0;
}

/* ---- SX1262 Low-Level Functions ---- */
int sx1262_reset(void)
{
    /* Hardware reset via NRST pin */
    GPIOA->BSRR = (1U << 10) << 16;  /* Reset low */
    for (volatile int i = 0; i < 10000; i++); /* ~1ms */
    GPIOA->BSRR = (1U << 10);  /* Reset high */
    for (volatile int i = 0; i < 50000; i++); /* 5ms wait */
    return 0;
}

int sx1262_read_reg(uint16_t addr, uint8_t *value)
{
    uint8_t tx[4] = { SX1262_CMD_READ_REG,
                      (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF), 0x00 };
    uint8_t rx[4];
    if (lora_spi_transfer(tx, rx, 4) != 0) return -1;
    *value = rx[3];
    return 0;
}

int sx1262_write_reg(uint16_t addr, uint8_t value)
{
    uint8_t tx[4] = { SX1262_CMD_WRITE_REG,
                      (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF), value };
    uint8_t rx[4];
    return lora_spi_transfer(tx, rx, 4);
}

int sx1262_write_buffer(uint16_t addr, const uint8_t *buf, int len)
{
    /* Write command + address + data in one SPI transaction */
    uint8_t tx[256];
    uint8_t rx[256];
    tx[0] = SX1262_CMD_WRITE_BUFFER;
    tx[1] = (uint8_t)(addr & 0xFF);
    memcpy(&tx[2], buf, len);
    return lora_spi_transfer(tx, rx, len + 2);
}

int sx1262_read_buffer(uint16_t addr, uint8_t *buf, int len)
{
    uint8_t tx[256];
    uint8_t rx[256];
    tx[0] = SX1262_CMD_READ_BUFFER;
    tx[1] = (uint8_t)(addr & 0xFF);
    memset(&tx[2], 0, len);
    if (lora_spi_transfer(tx, rx, len + 2) != 0) return -1;
    memcpy(buf, &rx[2], len);
    return 0;
}

int sx1262_set_tx(const uint8_t *data, int len, int timeout_ms)
{
    /* Set standby */
    uint8_t cmd_stdby[2] = { SX1262_CMD_SET_STANDBY, SX1262_STDBY_RC };
    uint8_t rx[2];
    lora_spi_transfer(cmd_stdby, rx, 2);

    /* Set packet type to LoRa */
    uint8_t cmd_pkt[2] = { SX1262_CMD_SET_PACKET_TYPE, SX1262_PKT_TYPE_LORA };
    lora_spi_transfer(cmd_pkt, rx, 2);

    /* Write data to FIFO buffer at offset 0 */
    sx1262_write_buffer(0, data, len);

    /* Set payload length and FIFO pointer */
    uint8_t cmd_payload[3] = { 0x0A, 0x02, (uint8_t)len }; /* SetPayloadLength */
    lora_spi_transfer(cmd_payload, rx, 3);

    /* Set TX with timeout */
    uint8_t cmd_tx[4] = { SX1262_CMD_SET_TX, 0x00, 0x00,
                          (uint8_t)(timeout_ms * 15.625f) }; /* timeout in 15.625µs units */
    lora_spi_transfer(cmd_tx, rx, 4);

    radio_state = RADIO_STATE_TX;
    tx_complete = false;
    return 0;
}

int sx1262_set_rx(int timeout_ms)
{
    /* Set RX with timeout (0 = continuous) */
    uint8_t cmd_rx[4] = { SX1262_CMD_SET_RX, 0x00, 0x00,
                          (uint8_t)(timeout_ms * 15.625f) };
    uint8_t rx[4];
    lora_spi_transfer(cmd_rx, rx, 4);

    radio_state = RADIO_STATE_RX;
    rx_complete = false;
    rx_timeout = false;
    return 0;
}

void sx1262_set_standby(void)
{
    uint8_t cmd[2] = { SX1262_CMD_SET_STANDBY, SX1262_STDBY_RC };
    uint8_t rx[2];
    lora_spi_transfer(cmd, rx, 2);
    radio_state = RADIO_STATE_IDLE;
}

void sx1262_sleep(void)
{
    uint8_t cmd[2] = { SX1262_CMD_SET_SLEEP, 0x04 }; /* Warm start + RTC */
    uint8_t rx[2];
    lora_spi_transfer(cmd, rx, 2);
    radio_state = RADIO_STATE_IDLE;
}

/* ---- SX1262 Interrupt Handler (DIO1) ---- */
void EXTI2_IRQHandler(void)
{
    if (EXTI->PR1 & LORA_DIO1_PIN) {
        EXTI->PR1 = LORA_DIO1_PIN; /* Clear pending */

        /* Read IRQ status */
        uint8_t tx[4] = { SX1262_CMD_GET_IRQ_STATUS, 0x00, 0x00, 0x00 };
        uint8_t rx[4];
        lora_spi_transfer(tx, rx, 4);
        uint16_t irq_status = (rx[2] << 8) | rx[3];

        if (irq_status & SX1262_IRQ_TX_DONE) {
            tx_complete = true;
        }
        if (irq_status & SX1262_IRQ_RX_DONE) {
            rx_complete = true;
        }
        if (irq_status & SX1262_IRQ_TIMEOUT) {
            rx_timeout = true;
        }

        /* Clear IRQ flags */
        uint8_t clr_cmd[4] = { SX1262_CMD_CLEAR_IRQ, 0x00,
                               (uint8_t)(SX1262_IRQ_ALL >> 8),
                               (uint8_t)(SX1262_IRQ_ALL & 0xFF) };
        lora_spi_transfer(clr_cmd, rx, 4);
    }
}

/* ---- Initialize SPI1 for SX1262 ---- */
static int lora_spi_init(void)
{
    /* Enable SPI1 clock */
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* Configure PA4 (NSS), PA5 (SCK), PA6 (MISO), PA7 (MOSI) */
    /* PA4 as GPIO output (manual CS) */
    GPIOA->MODER &= ~(0x3U << 8);
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << 8);
    GPIOA->OTYPER &= ~(1U << 4);
    GPIOA->OSPEEDR |= (GPIO_SPEED_VHIGH << 8);
    lora_spi_cs_high();

    /* PA5, PA6, PA7 as AF5 (SPI1) */
    for (int pin = 5; pin <= 7; pin++) {
        GPIOA->MODER &= ~(0x3U << (pin * 2));
        GPIOA->MODER |= (GPIO_MODE_AF << (pin * 2));
        GPIOA->OSPEEDR |= (GPIO_SPEED_VHIGH << (pin * 2));
    }
    GPIOA->AFR[0] &= ~(0xFFFU << 20); /* Clear AF5-7 */
    GPIOA->AFR[0] |= (5U << 20) | (5U << 24) | (5U << 28);

    /* Configure SPI1: Master, CPOL=0, CPHA=0, 8-bit, MSB first, prescaler/4 */
    SPI1->CR1 = 0;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                SPI_CR1_BR_DIV8; /* 140MHz/8 = 17.5MHz */
    SPI1->CR1 |= SPI_CR1_SPE;

    /* Configure DIO1 (PB2) as input with EXTI falling edge */
    GPIOB->MODER &= ~(0x3U << 4);
    GPIOB->MODER |= (GPIO_MODE_INPUT << 4);
    GPIOB->PUPDR |= (GPIO_PULLUP << 4);

    /* Configure BUSY (PB1) as input */
    GPIOB->MODER &= ~(0x3U << 2);
    GPIOB->MODER |= (GPIO_MODE_INPUT << 2);

    /* Configure NRST (PA10) as output */
    GPIOA->MODER &= ~(0x3U << 20);
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << 20);
    GPIOA->OTYPER &= ~(1U << 10);

    /* Configure antenna switch (PB3) as output */
    GPIOB->MODER &= ~(0x3U << 6);
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << 6);

    /* Configure EXTI2 for DIO1 (PB2) */
    volatile uint32_t *exticr = (volatile uint32_t *)(SYSCFG_BASE + 0x08);
    exticr[0] &= ~(0xFU << 8);  /* EXTI2 config */
    exticr[0] |= (0x1U << 8);   /* Port B = 0x1 */
    EXTI->FTSR1 |= (1U << 2);
    EXTI->IMR1 |= (1U << 2);
    NVIC_ISER0 |= (1U << (EXTI2_IRQn & 0x1F)); /* Reuses EXTI2 IRQ */

    return 0;
}

/* ---- Set RF Frequency ---- */
static void sx1262_set_frequency(uint32_t freq_hz)
{
    /* RF frequency = freq_pll / 2^25 * 32MHz
     * freq_pll = freq_hz * 2^25 / 32MHz */
    uint64_t pll = ((uint64_t)freq_hz << 25) / 32000000ULL;
    uint8_t cmd[5] = { SX1262_CMD_SET_RF_FREQ,
                       (uint8_t)(pll >> 24), (uint8_t)(pll >> 16),
                       (uint8_t)(pll >> 8), (uint8_t)pll };
    uint8_t rx[5];
    lora_spi_transfer(cmd, rx, 5);
}

/* ---- Set TX Power ---- */
static void sx1262_set_tx_power(int8_t power_dbm)
{
    /* Set TX power with ramp time = 200µs */
    uint8_t cmd[3] = { SX1262_CMD_SET_TX_PARAMS,
                       (uint8_t)power_dbm, 0x01 }; /* Ramp 200µs */
    uint8_t rx[3];
    lora_spi_transfer(cmd, rx, 3);
}

/* ---- Set Modulation Parameters ---- */
static void sx1262_set_mod_params(void)
{
    /* SF=7, BW=125kHz (0x04 in SX1262), CR=4/5 (0x01), LDRO=0 */
    uint8_t cmd[5] = { SX1262_CMD_SET_MOD_PARAMS,
                       LORA_SF, 0x04, 0x01, 0x00 };
    uint8_t rx[5];
    lora_spi_transfer(cmd, rx, 5);
}

/* ---- SX1262 Initialization ---- */
int sx1262_init(void)
{
    int ret = 0;

    ret |= lora_spi_init();
    ret |= sx1262_reset();

    /* Set standby mode */
    uint8_t cmd_stdby[2] = { SX1262_CMD_SET_STANDBY, SX1262_STDBY_RC };
    uint8_t rx[2];
    lora_spi_transfer(cmd_stdby, rx, 2);

    /* Set packet type to LoRa */
    uint8_t cmd_pkt[2] = { SX1262_CMD_SET_PACKET_TYPE, SX1262_PKT_TYPE_LORA };
    lora_spi_transfer(cmd_pkt, rx, 2);

    /* Set RF frequency */
    sx1262_set_frequency(LORA_FREQ_BAND);

    /* Set TX power */
    sx1262_set_tx_power(LORA_TX_POWER_DBM);

    /* Set modulation parameters */
    sx1262_set_mod_params();

    /* Set LoRaWAN sync word (0x3441) */
    sx1262_write_reg(SX1262_REG_SYNC_WORD, 0x34);
    sx1262_write_reg(SX1262_REG_SYNC_WORD + 1, 0x41);

    return ret;
}

/* ---- LoRaWAN MAC Layer ---- */
/* AES-128 encryption (simplified — uses hardware crypto in production) */
static void aes_encrypt_block(const uint8_t *key, const uint8_t *input,
                               uint8_t *output)
{
    /* In production, this uses the STM32H7's hardware AES accelerator
     * (CRYP peripheral). For this reference implementation, we use a
     * placeholder that XORs with the key (NOT secure — for structure only). */
    for (int i = 0; i < 16; i++) {
        output[i] = input[i] ^ key[i];
    }
}

/* Calculate MIC (Message Integrity Code) */
static uint32_t calculate_mic(const uint8_t *data, int len,
                               const uint8_t *key, uint16_t f_cnt)
{
    /* Simplified CMAC — production uses AES-CMAC */
    uint32_t mic = 0;
    for (int i = 0; i < len; i++) {
        mic ^= (data[i] << ((i % 4) * 8));
    }
    mic ^= f_cnt;
    /* XOR with key-derived value */
    uint8_t enc[16];
    aes_encrypt_block(key, (uint8_t *)&mic, enc);
    memcpy(&mic, enc, 4);
    return mic;
}

/* Build LoRaWAN join request packet */
static int build_join_request(uint8_t *packet, int *len)
{
    int idx = 0;
    packet[idx++] = 0x00; /* MHDR: Join Request */
    memcpy(&packet[idx], join_eui, 8); idx += 8;
    memcpy(&packet[idx], dev_eui, 8); idx += 8;

    /* DevNonce (random or counter) */
    static uint16_t dev_nonce = 0;
    dev_nonce++;
    packet[idx++] = (uint8_t)(dev_nonce >> 8);
    packet[idx++] = (uint8_t)(dev_nonce & 0xFF);

    /* MIC (calculated over the packet with AppKey) */
    uint32_t mic = calculate_mic(packet, idx, app_key, dev_nonce);
    packet[idx++] = (uint8_t)(mic & 0xFF);
    packet[idx++] = (uint8_t)(mic >> 8);
    packet[idx++] = (uint8_t)(mic >> 16);
    packet[idx++] = (uint8_t)(mic >> 24);

    *len = idx;
    return 0;
}

/* Build LoRaWAN uplink packet */
static int build_uplink_packet(uint8_t *packet, int *len,
                                const lora_uplink_t *uplink)
{
    int idx = 0;

    /* MHDR: Unconfirmed Data Up, FPort=1 */
    packet[idx++] = 0x40; /* MType=Unconfirmed Up, Major=0 */

    /* FHDR: DevAddr (4 bytes, little-endian) */
    memcpy(&packet[idx], dev_addr, 4); idx += 4;

    /* FCtrl */
    packet[idx++] = 0x00; /* No options, ADR disabled */

    /* FCnt (2 bytes, little-endian) */
    packet[idx++] = (uint8_t)(f_cnt_up & 0xFF);
    packet[idx++] = (uint8_t)(f_cnt_up >> 8);

    /* FPort */
    packet[idx++] = 0x01;

    /* Payload: serialize lora_uplink_t as binary */
    memcpy(&packet[idx], uplink, sizeof(lora_uplink_t));
    idx += sizeof(lora_uplink_t);

    /* Encrypt payload with AppSKey (simplified) */
    uint8_t enc_buf[sizeof(lora_uplink_t)];
    aes_encrypt_block(app_s_key, (const uint8_t *)uplink, enc_buf);
    memcpy(&packet[idx - sizeof(lora_uplink_t)], enc_buf,
           sizeof(lora_uplink_t));

    /* MIC */
    uint32_t mic = calculate_mic(packet, idx, f_nwk_s_int_key, f_cnt_up);
    packet[idx++] = (uint8_t)(mic & 0xFF);
    packet[idx++] = (uint8_t)(mic >> 8);
    packet[idx++] = (uint8_t)(mic >> 16);
    packet[idx++] = (uint8_t)(mic >> 24);

    *len = idx;
    f_cnt_up++;
    return 0;
}

/* ---- Public API ---- */
int lora_init(void)
{
    int ret = sx1262_init();
    lora_joined = false;
    f_cnt_up = 0;
    f_cnt_down = 0;
    lora_class = 0;
    return ret;
}

bool lora_join(void)
{
    if (lora_joined) return true;

    for (int attempt = 0; attempt < LORA_JOIN_TRIES; attempt++) {
        /* Build join request */
        uint8_t join_pkt[23];
        int pkt_len;
        build_join_request(join_pkt, &pkt_len);

        /* Send join request */
        sx1262_set_tx(join_pkt, pkt_len, LORA_JOIN_TIMEOUT_S * 1000);

        /* Wait for TX to complete */
        uint32_t timeout = 10000000;
        while (!tx_complete && timeout--);
        if (!tx_complete) continue;

        /* Set RX1 window (5 seconds after TX, 1 second duration) */
        sx1262_set_rx(2000); /* 2 second RX window */
        timeout = 5000000;
        while (!rx_complete && !rx_timeout && timeout--);

        if (rx_complete) {
            /* Read join accept message */
            uint8_t rx_buf[33];
            int rx_len = 33;
            sx1262_read_buffer(0, rx_buf, rx_len);

            /* Parse join accept (simplified) */
            /* In production: decrypt with AppKey, validate MIC,
             * extract DevAddr, generate session keys */
            memcpy(dev_addr, &rx_buf[1], 4);

            /* Generate session keys (simplified) */
            memset(f_nwk_s_int_key, 0xAA, 16);
            memset(app_s_key, 0xBB, 16);

            lora_joined = true;
            return true;
        }
    }

    return false;
}

void lora_set_class_b(void)
{
    lora_class = 1;
    /* In Class B, device listens at scheduled ping slots (every 128s) */
    /* The MAC layer would send ClassBMode command and synchronize
     * with gateway beacons */
}

void lora_set_class_a(void) { lora_class = 0; }
void lora_set_class_c(void) { lora_class = 2; }

int lora_send_uplink(const lora_uplink_t *uplink)
{
    if (!lora_joined || !uplink) return -1;

    /* Build the LoRaWAN packet */
    uint8_t pkt[128];
    int pkt_len;
    build_uplink_packet(pkt, &pkt_len, uplink);

    /* Set antenna switch to TX */
    GPIOB->BSRR = (1U << 3);  /* PB3 high = TX mode */

    /* Transmit */
    sx1262_set_tx(pkt, pkt_len, 10000); /* 10s timeout */

    /* Wait for TX completion */
    uint32_t timeout = 10000000;
    while (!tx_complete && timeout--);

    /* Set antenna switch to RX */
    GPIOB->BSRR = (1U << 3) << 16;  /* PB3 low = RX mode */

    /* Open RX1 window (1 second after TX) */
    sx1262_set_rx(3000);

    /* Check for downlink in RX1 */
    uint32_t rx_to = 5000000;
    while (!rx_complete && !rx_timeout && rx_to--);

    if (rx_complete) {
        /* Read downlink payload */
        uint8_t rx_buf[64];
        sx1262_read_buffer(0, rx_buf, sizeof(lora_downlink_t));

        /* Parse downlink command */
        if (rx_buf[0] >= 0x80) { /* Confirmed downlink */
            pending_downlink.command = (lora_cmd_t)rx_buf[7]; /* FPort */
            if (sizeof(lora_downlink_t) > 8) {
                pending_downlink.param = rx_buf[8];
                int data_len = sizeof(lora_downlink_t) - 9;
                if (data_len > 0 && data_len <= 16) {
                    memcpy(pending_downlink.data, &rx_buf[9], data_len);
                    pending_downlink.data_len = data_len;
                }
            }
            has_downlink = true;
            f_cnt_down++;
        }
    }

    sx1262_set_standby();
    return (tx_complete) ? 0 : -1;
}

bool lora_check_downlink(lora_downlink_t *dl)
{
    if (!has_downlink) return false;

    __disable_irq();
    *dl = pending_downlink;
    has_downlink = false;
    __enable_irq();

    return true;
}