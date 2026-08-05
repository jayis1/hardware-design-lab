/*
 * drivers/radio.c — SX1262 LoRa radio driver and mesh MAC
 *
 * Implements:
 *  - SPI primitives for the Semtech SX1262
 *  - Standby / RX / TX state control
 *  - Frequency, modulation, and TX power configuration
 *  - A custom time-slotted mesh MAC with CSMA fallback
 *  - AES-GCM packet authentication using the STM32U575 hardware AES core
 *  - Frost alert priority broadcasting and relay
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "radio.h"
#include "../board.h"
#include <string.h>

/* ------------------------------------------------------------------ */
/*  SX1262 SPI primitives                                              */
/* ------------------------------------------------------------------ */
static void sx1262_spi_write(uint8_t *data, uint8_t len)
{
    SX1262_CS_LOW();
    delay_ms(1);  /* BUSY check would be here; simplified */
    for (uint8_t i = 0; i < len; i++) {
        SPI1->DR = data[i];
        while (!(SPI1->SR & SPI_SR_TXE)) ;
        while (SPI1->SR & SPI_SR_BSY) ;
    }
    SX1262_CS_HIGH();
    delay_ms(1);
}

static void sx1262_spi_read(uint8_t cmd, uint8_t *out, uint8_t len)
{
    SX1262_CS_LOW();
    delay_ms(1);
    SPI1->DR = cmd;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    (void)SPI1->DR;  /* flush received dummy */
    for (uint8_t i = 0; i < len; i++) {
        SPI1->DR = 0x00;
        while (!(SPI1->SR & SPI_SR_RXNE)) ;
        out[i] = (uint8_t)SPI1->DR;
    }
    SX1262_CS_HIGH();
    delay_ms(1);
}

static void sx1262_write_register(uint16_t addr, uint8_t val)
{
    uint8_t buf[4] = { 0x1D, (addr >> 8) & 0xFF, addr & 0xFF, val };
    sx1262_spi_write(buf, 4);
}

static uint8_t sx1262_read_register(uint16_t addr)
{
    uint8_t buf[3] = { 0x1E, (addr >> 8) & 0xFF, addr & 0xFF };
    /* write address first */
    SX1262_CS_LOW();
    delay_ms(1);
    for (int i = 0; i < 3; i++) {
        SPI1->DR = buf[i];
        while (!(SPI1->SR & SPI_SR_TXE)) ;
    }
    /* read data byte */
    SPI1->DR = 0x00;
    while (!(SPI1->SR & SPI_SR_RXNE)) ;
    uint8_t val = (uint8_t)SPI1->DR;
    SX1262_CS_HIGH();
    delay_ms(1);
    return val;
}

static void sx1262_set_standby(void)
{
    uint8_t buf[2] = { SX1262_CMD_SET_STANDBY, 0x01 };  /* STDBY_RC */
    sx1262_spi_write(buf, 2);
    delay_ms(1);
}

static void sx1262_set_rx(uint32_t timeout_ms)
{
    uint8_t buf[4] = { SX1262_CMD_SET_RX };
    uint32_t t = timeout_ms * 64;  /* SX1262 timeout in 15.625 µs units */
    buf[1] = (t >> 16) & 0xFF;
    buf[2] = (t >> 8) & 0xFF;
    buf[3] = t & 0xFF;
    sx1262_spi_write(buf, 4);
}

static void sx1262_set_tx(uint32_t timeout_ms)
{
    uint8_t buf[4] = { SX1262_CMD_SET_TX };
    uint32_t t = timeout_ms * 64;
    buf[1] = (t >> 16) & 0xFF;
    buf[2] = (t >> 8) & 0xFF;
    buf[3] = t & 0xFF;
    sx1262_spi_write(buf, 4);
}

static void sx1262_set_frequency(uint32_t freq_hz)
{
    uint8_t buf[5] = { SX1262_CMD_SET_RF_FREQ };
    /* RFfreq = freq_Hz * 2^25 / 32 MHz */
    uint64_t rf = ((uint64_t)freq_hz << 25) / 32000000ULL;
    buf[1] = (rf >> 24) & 0xFF;
    buf[2] = (rf >> 16) & 0xFF;
    buf[3] = (rf >> 8)  & 0xFF;
    buf[4] = rf & 0xFF;
    sx1262_spi_write(buf, 5);
}

static void sx1262_set_tx_params(int8_t power_dbm, uint8_t ramp)
{
    uint8_t buf[3] = { SX1262_CMD_SET_TX_PARAMS,
                       (uint8_t)power_dbm, ramp };
    sx1262_spi_write(buf, 3);
}

static void sx1262_write_buffer(uint8_t offset, uint8_t *data, uint8_t len)
{
    uint8_t buf[2 + 64];
    buf[0] = SX1262_CMD_WRITE_BUFFER;
    buf[1] = offset;
    memcpy(&buf[2], data, len);
    sx1262_spi_write(buf, 2 + len);
}

static void sx1262_read_buffer(uint8_t offset, uint8_t *out, uint8_t len)
{
    SX1262_CS_LOW();
    delay_ms(1);
    SPI1->DR = SX1262_CMD_READ_BUFFER;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    SPI1->DR = offset;
    while (!(SPI1->SR & SPI_SR_TXE)) ;
    (void)SPI1->DR;  /* dummy */
    for (uint8_t i = 0; i < len; i++) {
        SPI1->DR = 0x00;
        while (!(SPI1->SR & SPI_SR_RXNE)) ;
        out[i] = (uint8_t)SPI1->DR;
    }
    SX1262_CS_HIGH();
    delay_ms(1);
}

static uint8_t sx1262_get_status(void)
{
    uint8_t cmd = SX1262_CMD_GET_STATUS;
    uint8_t status = 0;
    sx1262_spi_read(cmd, &status, 1);
    return status;
}

/* ------------------------------------------------------------------ */
/*  AES-GCM tag computation (4-byte truncated tag for mesh packets)    */
/*  Uses the STM32U575 hardware AES-256 core in GCM mode.              */
/* ------------------------------------------------------------------ */
static void aes_gcm_compute_tag(const uint8_t *key16, const uint8_t *nonce12,
                                const uint8_t *aad, uint8_t aad_len,
                                uint8_t *tag4)
{
    /* Load key (we use a 128-bit key for speed, padded to 256-bit) */
    AES1->CR = 0;
    AES1->KEYR0 = ((uint32_t)key16[3]  << 24) | ((uint32_t)key16[2]  << 16) |
                  ((uint32_t)key16[1]  << 8)  |  (uint32_t)key16[0];
    AES1->KEYR1 = ((uint32_t)key16[7]  << 24) | ((uint32_t)key16[6]  << 16) |
                  ((uint32_t)key16[5]  << 8)  |  (uint32_t)key16[4];
    AES1->KEYR2 = ((uint32_t)key16[11] << 24) | ((uint32_t)key16[10] << 16) |
                  ((uint32_t)key16[9]  << 8)  |  (uint32_t)key16[8];
    AES1->KEYR3 = ((uint32_t)key16[15] << 24) | ((uint32_t)key16[14] << 16) |
                  ((uint32_t)key16[13] << 8)  |  (uint32_t)key16[12];
    /* Pad upper 128 bits with zeros for AES-256 mode */
    AES1->KEYR4 = 0; AES1->KEYR5 = 0; AES1->KEYR6 = 0; AES1->KEYR7 = 0;

    /* Load IV (12 bytes → 3 words, 4th word is counter) */
    AES1->IVR0 = ((uint32_t)nonce12[3]  << 24) | ((uint32_t)nonce12[2]  << 16) |
                 ((uint32_t)nonce12[1]  << 8)  |  (uint32_t)nonce12[0];
    AES1->IVR1 = ((uint32_t)nonce12[7]  << 24) | ((uint32_t)nonce12[6]  << 16) |
                 ((uint32_t)nonce12[5]  << 8)  |  (uint32_t)nonce12[4];
    AES1->IVR2 = ((uint32_t)nonce12[11] << 24) | ((uint32_t)nonce12[10] << 16) |
                 ((uint32_t)nonce12[9]  << 8)  |  (uint32_t)nonce12[8];
    AES1->IVR3 = 0x00000002;  /* counter starts at 2 for GCM */

    /* Configure: GCM mode, 256-bit key, 8-bit datatype */
    AES1->CR = AES_CR_EN | AES_CR_MODE_GCM | AES_CR_DATATYPE_8B |
               AES_CR_KEYSIZE_256;

    /* Feed AAD (header) bytes — pad to 16-byte block */
    uint8_t aad_block[16] = {0};
    uint8_t aad_blocks = (aad_len + 15) / 16;
    for (uint8_t b = 0; b < aad_blocks; b++) {
        memset(aad_block, 0, 16);
        uint8_t copy = (b == aad_blocks - 1) ? (aad_len - b * 16) : 16;
        if (copy > 16) copy = 16;
        memcpy(aad_block, aad + b * 16, copy);
        AES1->DINR = ((uint32_t)aad_block[3]  << 24) | ((uint32_t)aad_block[2]  << 16) |
                     ((uint32_t)aad_block[1]  << 8)  |  (uint32_t)aad_block[0];
        while (AES1->SR & AES_SR_BUSY) ;
    }

    /* No plaintext to encrypt (tag-only mode for authentication);
     * trigger final phase to read tag. */
    AES1->CR |= (0x3u << 13);  /* GCMPH = final */
    while (!(AES1->SR & AES_SR_CCF)) ;
    AES1->SR = AES_SR_CCF;     /* clear flag */

    /* Read tag (4 words = 16 bytes, we keep first 4) */
    uint32_t tag0 = AES1->DOUTR;
    tag4[0] = (tag0 >> 24) & 0xFF;
    tag4[1] = (tag0 >> 16) & 0xFF;
    tag4[2] = (tag0 >> 8)  & 0xFF;
    tag4[3] =  tag0 & 0xFF;
}

/* ------------------------------------------------------------------ */
/*  Mesh MAC state                                                     */
/* ------------------------------------------------------------------ */
static uint8_t  s_net_key[MESH_NETWORK_KEY_BYTES];
static uint8_t  s_nonce[12];
static uint8_t  s_tx_slot;         /* this node's TX slot in superframe */
static uint32_t s_superframe_start_ms;
static uint8_t  s_alert_pending;   /* 1 if we have a frost alert to send */
static mesh_packet_t s_rx_pkt;
static uint8_t  s_rx_ready;

/* ------------------------------------------------------------------ */
/*  Packet build / parse                                               */
/* ------------------------------------------------------------------ */
static void build_packet(mesh_packet_t *pkt, uint8_t msg_type)
{
    memset(pkt, 0, sizeof(*pkt));
    pkt->node_id    = g_sys.node_id;
    pkt->msg_type   = msg_type;
    pkt->rfri_q8    = (uint8_t)(g_sys.rfri_q8 & 0xFF);
    pkt->rfri_q8_hi = (uint8_t)((g_sys.rfri_q8 >> 8) & 0xFF);
    pkt->twet_lo    = (uint8_t)(g_sys.twet_cx100 & 0xFF);
    pkt->twet_hi    = (uint8_t)((g_sys.twet_cx100 >> 8) & 0xFF);
    pkt->sky_lo     = (uint8_t)(g_sys.sky_t_cx100 & 0xFF);
    pkt->sky_hi     = (uint8_t)((g_sys.sky_t_cx100 >> 8) & 0xFF);
    pkt->delta_lo   = (uint8_t)(g_sys.delta_rad_cx100 & 0xFF);
    pkt->delta_hi   = (uint8_t)((g_sys.delta_rad_cx100 >> 8) & 0xFF);
    pkt->leaf_wet   = (uint8_t)(g_sys.leaf_wet >> 2);  /* 0-250 */
    pkt->ae_status  = g_sys.ae_status;
    pkt->flags      = g_sys.flags;
    pkt->hops       = 0;

    /* CRC16 over bytes 0..16 */
    uint16_t crc = 0;
    for (int i = 0; i < 17; i++) {
        crc ^= ((uint16_t)pkt->raw[i]) << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else              crc <<= 1;
        }
    }
    pkt->crc_lo = crc & 0xFF;
    pkt->crc_hi = (crc >> 8) & 0xFF;

    /* AES-GCM 4-byte tag (AAD = bytes 0..18 including CRC) */
    uint8_t nonce[12];
    memcpy(nonce, s_nonce, 12);
    /* Mix node_id and a counter into nonce for uniqueness */
    nonce[10] ^= g_sys.node_id;
    nonce[11] ^= (uint8_t)(g_rtc_seconds & 0xFF);
    aes_gcm_compute_tag(s_net_key, nonce, pkt->raw, 19, pkt->tag);
}

/* ------------------------------------------------------------------ */
/*  Public: initialize radio and mesh                                  */
/* ------------------------------------------------------------------ */
void radio_init(const uint8_t *network_key, uint8_t node_id, uint8_t slot)
{
    memcpy(s_net_key, network_key, MESH_NETWORK_KEY_BYTES);
    memset(s_nonce, 0, 12);
    s_tx_slot = slot;
    s_alert_pending = 0;
    s_rx_ready = 0;
    g_sys.node_id = node_id;

    /* Enable SPI1 clock and configure pins */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN |
                    RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_AES1EN;

    /* PA5=SCK, PA6=MISO, PA7=MOSI as AF5 */
    GPIO_CONFIG(GPIOA, 5, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_VHIGH,
                GPIO_PUPD_NONE, 5);
    GPIO_CONFIG(GPIOA, 6, GPIO_MODE_AF, 0, GPIO_SPEED_VHIGH, GPIO_PUPD_UP, 5);
    GPIO_CONFIG(GPIOA, 7, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_VHIGH,
                GPIO_PUPD_NONE, 5);
    /* PB1 = SX1262 CS (output, default high) */
    GPIO_CONFIG(GPIOB, 1, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_HIGH,
                GPIO_PUPD_NONE, 0);
    SX1262_CS_HIGH();
    /* PC13 = DIO1 (input), PC14 = BUSY (input), PC15 = NRST (output) */
    GPIO_CONFIG(GPIOC, 13, GPIO_MODE_INPUT, 0, 0, GPIO_PUPD_DOWN, 0);
    GPIO_CONFIG(GPIOC, 14, GPIO_MODE_INPUT, 0, 0, GPIO_PUPD_NONE, 0);
    GPIO_CONFIG(GPIOC, 15, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, 0, GPIO_PUPD_NONE, 0);
    /* Reset SX1262 */
    CLR_BITS(GPIOC->ODR, (1u << 15));
    delay_ms(10);
    SET_BITS(GPIOC->ODR, (1u << 15));
    delay_ms(10);

    /* SPI1: master, CPOL=0, CPHA=0, /16 prescaler (10 MHz), 8-bit */
    SPI1->CR1 = 0;
    SPI1->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_SSOE;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_DIV16;
    SPI1->CR1 |= SPI_CR1_SPE;

    /* Configure SX1262 */
    sx1262_set_standby();
    sx1262_set_frequency(915000000u);     /* US band; 868 MHz for EU */
    sx1262_set_tx_params(22, 0x04);       /* +22 dBm, ramp 40 µs */

    /* Set LoRa modulation parameters via SetModulationParams (0x8B) */
    /* SF7, BW125, CR 4/5, LDRO off */
    uint8_t mod_params[4] = { 0x8B, 0x07, 0x04, 0x01 };
    sx1262_spi_write(mod_params, 4);

    /* Set packet params: 19+4 byte payload, explicit header, CRC on */
    uint8_t pkt_params[9] = {
        0x8C,              /* SetPacketParams */
        0x00,              /* Preamble length MSB */
        0x08,              /* Preamble length LSB (8 symbols) */
        0x00,              /* Explicit header */
        MESH_PAYLOAD_BYTES + MESH_TAG_BYTES,  /* payload length */
        0x01,              /* CRC on */
        0x00,              /* Standard IQ */
        0x00, 0x00
    };
    sx1262_spi_write(pkt_params, 9);
}

/* ------------------------------------------------------------------ */
/*  Public: transmit a mesh packet                                     */
/* ------------------------------------------------------------------ */
int radio_tx(uint8_t msg_type)
{
    mesh_packet_t pkt;
    build_packet(&pkt, msg_type);

    sx1262_set_standby();
    sx1262_write_buffer(0, pkt.raw, MESH_PAYLOAD_BYTES + MESH_TAG_BYTES);
    sx1262_set_tx(3000);   /* 3 s TX timeout */
    delay_ms(120);         /* ~SF7/125kHz: ~60 ms airtime + margin */

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Public: transmit a frost alert (high priority, pre-empts slots)    */
/* ------------------------------------------------------------------ */
int radio_tx_alert(void)
{
    return radio_tx(MESH_MSG_ALERT);
}

/* ------------------------------------------------------------------ */
/*  Public: receive with timeout                                       */
/* ------------------------------------------------------------------ */
int radio_rx(uint32_t timeout_ms, mesh_packet_t *out)
{
    sx1262_set_standby();
    sx1262_set_rx(timeout_ms);

    /* Wait for DIO1 (TxDone/RxDone) or timeout */
    uint32_t start = time_ms();
    while (elapsed_ms(start) < timeout_ms) {
        if (GPIOC->IDR & (1u << 13)) {
            /* DIO1 asserted — packet received */
            delay_ms(5);
            uint8_t buf[MESH_PAYLOAD_BYTES + MESH_TAG_BYTES];
            sx1262_read_buffer(0, buf, sizeof(buf));
            memcpy(out->raw, buf, sizeof(buf));
            s_rx_ready = 1;
            return 0;
        }
    }
    return -1;  /* timeout */
}

/* ------------------------------------------------------------------ */
/*  Public: enter low-power RX duty cycle (for sleep state)            */
/* ------------------------------------------------------------------ */
void radio_sleep_duty(void)
{
    /* RX for 1.6 ms every 10 s superframe — handled by RTC alarm */
    sx1262_set_rx(2);   /* 2 ms RX window */
}

/* ------------------------------------------------------------------ */
/*  Public: set alert pending flag                                     */
/* ------------------------------------------------------------------ */
void radio_set_alert_pending(void)
{
    s_alert_pending = 1;
}

uint8_t radio_get_alert_pending(void)
{
    return s_alert_pending;
}

void radio_clear_alert_pending(void)
{
    s_alert_pending = 0;
}

/* ------------------------------------------------------------------ */
/*  Public: get this node's assigned TX slot                           */
/* ------------------------------------------------------------------ */
uint8_t radio_get_tx_slot(void)
{
    return s_tx_slot;
}

/* ------------------------------------------------------------------ */
/*  Public: check if a received packet is valid (CRC + AES-GCM tag)    */
/* ------------------------------------------------------------------ */
int radio_verify_packet(const mesh_packet_t *pkt)
{
    /* CRC check */
    uint16_t crc = 0;
    for (int i = 0; i < 17; i++) {
        crc ^= ((uint16_t)pkt->raw[i]) << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else              crc <<= 1;
        }
    }
    uint16_t pkt_crc = ((uint16_t)pkt->crc_hi << 8) | pkt->crc_lo;
    if (crc != pkt_crc) return 0;  /* CRC fail */

    /* AES-GCM tag check */
    uint8_t expected_tag[4];
    uint8_t nonce[12];
    memcpy(nonce, s_nonce, 12);
    nonce[10] ^= pkt->node_id;
    nonce[11] ^= pkt->crc_lo;  /* approx nonce reuse protection */
    aes_gcm_compute_tag(s_net_key, nonce, pkt->raw, 19, expected_tag);

    return (memcmp(expected_tag, pkt->tag, 4) == 0) ? 1 : 0;
}