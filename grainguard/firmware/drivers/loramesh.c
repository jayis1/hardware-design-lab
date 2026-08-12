/*
 * loramesh.c — LoRa mesh networking for GrainGuard
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Uses the SX1262 sub-GHz radio integrated in the STM32WL55.
 * Implements a simple flooding mesh: every probe relays received
 * packets (with hop-count limiting and a "recently-seen" cache for
 * duplicate suppression).  AES-128 CMAC authenticates all traffic.
 */

#include "loramesh.h"
#include "../board.h"
#include "../registers.h"

static uint8_t my_serial = 0;

/* ---- Recently-seen cache for duplicate suppression ---- */
static struct {
    uint8_t serial;
    uint16_t timestamp_min;
    uint8_t  hop_count;
} recent_cache[MESH_RECENT_CACHE_SIZE];
static uint8_t cache_idx = 0;

static int is_duplicate(uint8_t serial, uint16_t ts) {
    for (int i = 0; i < MESH_RECENT_CACHE_SIZE; i++) {
        if (recent_cache[i].serial == serial &&
            recent_cache[i].timestamp_min == ts) return 1;
    }
    return 0;
}

static void cache_add(uint8_t serial, uint16_t ts, uint8_t hop) {
    recent_cache[cache_idx].serial = serial;
    recent_cache[cache_idx].timestamp_min = ts;
    recent_cache[cache_idx].hop_count = hop;
    cache_idx = (cache_idx + 1) % MESH_RECENT_CACHE_SIZE;
}

/* ---- SX1262 radio SPI access (via SubGHz peripheral) ----
 * On STM32WL55 the radio is accessed via a 3-wire SPI bus internal
 * to the SoC.  The exact sequence involves Radio commands.
 */

static void radio_wait_busy(void) {
    /* The radio busy line is on PB3 (LORA_BUSY).  Wait for low. */
    while ((GPIOB->IDR >> PB3__LORA_BUSY) & 1) { }
}

static void radio_write_command(uint8_t cmd, const uint8_t *data, uint8_t len) {
    /* Assert NSS low */
    GPIOB->BSRR = (1 << (PB4__LORA_NCS + 16));
    radio_wait_busy();

    /* Send command byte via SPI1 */
    SPI1->CR1 |= (1 << 6);  /* SPE enable */
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    SPI1->DR = cmd;
    while (SPI1->SR & SPI_SR_BSY) { }

    for (uint8_t i = 0; i < len; i++) {
        while (!(SPI1->SR & SPI_SR_TXE)) { }
        SPI1->DR = data[i];
        while (SPI1->SR & SPI_SR_BSY) { }
    }

    /* Deassert NSS */
    GPIOB->BSRR = (1 << PB4__LORA_NCS);
}

static void radio_read_command(uint8_t cmd, uint8_t *out, uint8_t len) {
    GPIOB->BSRR = (1 << (PB4__LORA_NCS + 16));
    radio_wait_busy();
    SPI1->CR1 |= (1 << 6);
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    SPI1->DR = cmd;
    while (SPI1->SR & SPI_SR_BSY) { }
    /* Dummy byte */
    SPI1->DR = 0x00;
    while (SPI1->SR & SPI_SR_BSY) { }
    for (uint8_t i = 0; i < len; i++) {
        while (!(SPI1->SR & SPI_SR_TXE)) { }
        SPI1->DR = 0x00;  /* dummy to clock out data */
        while (SPI1->SR & SPI_SR_BSY) { }
        while (!(SPI1->SR & SPI_SR_RXNE)) { }
        out[i] = (uint8_t)SPI1->DR;
    }
    GPIOB->BSRR = (1 << PB4__LORA_NCS);
}

/* ---- AES-128 CMAC ---- */
/* AES single-block encrypt using the hardware accelerator. */
static void aes_encrypt_block(const uint8_t key[16], const uint8_t in[16],
                               uint8_t out[16]) {
    /* Load key */
    AES->CR = 0;  /* disable */
    AES->KEYR0 = (key[0]  | (key[1]  << 8) | (key[2]  << 16) | (key[3]  << 24));
    AES->KEYR1 = (key[4]  | (key[5]  << 8) | (key[6]  << 16) | (key[7]  << 24));
    AES->KEYR2 = (key[8]  | (key[9]  << 8) | (key[10] << 16) | (key[11] << 24));
    AES->KEYR3 = (key[12] | (key[13] << 8) | (key[14] << 16) | (key[15] << 24));

    AES->CR = AES_CR_EN | (1 << 17) | (1 << 18) | (2 << 20);  /* ECB, encryption */

    /* Write 4 words */
    AES->DINR = (in[0]  | (in[1]  << 8) | (in[2]  << 16) | (in[3]  << 24));
    AES->DINR = (in[4]  | (in[5]  << 8) | (in[6]  << 16) | (in[7]  << 24));
    AES->DINR = (in[8]  | (in[9]  << 8) | (in[10] << 16) | (in[11] << 24));
    AES->DINR = (in[12] | (in[13] << 8) | (in[14] << 16) | (in[15] << 24));

    while (AES->SR & AES_SR_BUSY) { }
    while (!(AES->SR & AES_SR_CCF)) { }

    out[0]  = AES->DOUTR & 0xFF;  out[1]  = (AES->DOUTR >> 8) & 0xFF;
    out[2]  = (AES->DOUTR >> 16) & 0xFF; out[3]  = (AES->DOUTR >> 24) & 0xFF;
    out[4]  = AES->DOUTR & 0xFF;  out[5]  = (AES->DOUTR >> 8) & 0xFF;
    out[6]  = (AES->DOUTR >> 16) & 0xFF; out[7]  = (AES->DOUTR >> 24) & 0xFF;
    out[8]  = AES->DOUTR & 0xFF;  out[9]  = (AES->DOUTR >> 8) & 0xFF;
    out[10] = (AES->DOUTR >> 16) & 0xFF; out[11] = (AES->DOUTR >> 24) & 0xFF;
    out[12] = AES->DOUTR & 0xFF;  out[13] = (AES->DOUTR >> 8) & 0xFF;
    out[14] = (AES->DOUTR >> 16) & 0xFF; out[15] = (AES->DOUTR >> 24) & 0xFF;

    AES->CR = 0;
}

/* CMAC per NIST SP 800-38B (simplified for short messages) */
void aes_cmac_compute(const uint8_t *key, const uint8_t *msg, uint8_t len,
                       uint8_t mac[16]) {
    uint8_t K1[16], K2[16];
    uint8_t L[16] = {0};

    /* Subkey generation */
    aes_encrypt_block(key, L, K1);
    /* Left-shift K1, conditional XOR */
    int msb = (K1[0] >> 7) & 1;
    for (int i = 0; i < 15; i++) {
        K1[i] = (uint8_t)((K1[i] << 1) | (K1[i+1] >> 7));
    }
    K1[15] = (uint8_t)(K1[15] << 1);
    if (msb) K1[15] ^= 0x87;

    msb = (K1[0] >> 7) & 1;
    for (int i = 0; i < 15; i++) {
        K2[i] = (uint8_t)((K1[i] << 1) | (K1[i+1] >> 7));
    }
    K2[15] = (uint8_t)(K1[15] << 1);
    if (msb) K2[15] ^= 0x87;

    /* Process blocks */
    uint8_t num_blocks = (len + 15) / 16;
    if (num_blocks == 0) num_blocks = 1;

    uint8_t M_last[16] = {0};
    if (len > 0 && (len % 16) == 0) {
        /* Complete last block */
        for (int i = 0; i < 16; i++) M_last[i] = msg[(num_blocks - 1) * 16 + i];
        for (int i = 0; i < 16; i++) M_last[i] ^= K1[i];
    } else {
        /* Incomplete last block: pad with 0x80, 0x00, ... */
        uint8_t rem = len % 16;
        int start = (num_blocks - 1) * 16;
        for (int i = 0; i < rem; i++) M_last[i] = msg[start + i];
        M_last[rem] = 0x80;
        for (int i = rem + 1; i < 16; i++) M_last[i] = 0;
        for (int i = 0; i < 16; i++) M_last[i] ^= K2[i];
    }

    uint8_t X[16] = {0};
    for (uint8_t b = 0; b < num_blocks - 1; b++) {
        uint8_t Y[16];
        for (int i = 0; i < 16; i++) Y[i] = X[i] ^ msg[b * 16 + i];
        aes_encrypt_block(key, Y, X);
    }
    uint8_t Y[16];
    for (int i = 0; i < 16; i++) Y[i] = X[i] ^ M_last[i];
    aes_encrypt_block(key, Y, mac);
}

/* ---- Public API ---- */

void mesh_set_serial(uint8_t serial) { my_serial = serial; }
uint8_t mesh_get_serial(void) { return my_serial; }

int mesh_init(uint32_t freq_hz) {
    /* Configure GPIO for radio control lines */
    GPIOB->MODER = (GPIOB->MODER & ~(0x3 << (PB4__LORA_NCS * 2)))
                  | (GPIO_MODE_OUTPUT << (PB4__LORA_NCS * 2));
    GPIOB->MODER = (GPIOB->MODER & ~(0x3 << (PB5__LORA_RESET * 2)))
                  | (GPIO_MODE_OUTPUT << (PB5__LORA_RESET * 2));
    GPIOB->MODER = (GPIOB->MODER & ~(0x3 << (PB3__LORA_BUSY * 2)))
                  | (GPIO_MODE_INPUT << (PB3__LORA_BUSY * 2));
    GPIOB->MODER = (GPIOB->MODER & ~(0x3 << (PB6__LORA_DIO1 * 2)))
                  | (GPIO_MODE_INPUT << (PB6__LORA_DIO1 * 2));

    /* Configure SPI1 pins (PA5-PA7) as AF */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (PA5__SPI_SCK * 2)))
                  | (GPIO_MODE_AF << (PA5__SPI_SCK * 2));
    /* ... similar for MISO, MOSI; AF number depends on STM32WL55 ... */

    /* Hardware reset the radio */
    GPIOB->BSRR = (1 << (PB5__LORA_RESET + 16));  /* low */
    delay_ms(10);
    GPIOB->BSRR = (1 << PB5__LORA_RESET);         /* high */
    delay_ms(10);

    /* Set standby */
    uint8_t standby_cfg = 0x00;  /* RC mode */
    radio_write_command(RADIO_CMD_SET_STANDBY, &standby_cfg, 1);
    delay_ms(1);

    /* Set packet type to LoRa */
    uint8_t ptype = RADIO_PACKET_TYPE_LORA;
    radio_write_command(RADIO_CMD_SET_PACKET_TYPE, &ptype, 1);

    /* Set RF frequency (4-byte frequency = freq_hz × 2^25 / 32 MHz) */
    uint32_t rf_freq_word = (uint32_t)((uint64_t)freq_hz << 25) / 32000000ULL;
    uint8_t freq_buf[4] = {
        (uint8_t)(rf_freq_word >> 24), (uint8_t)(rf_freq_word >> 16),
        (uint8_t)(rf_freq_word >> 8),  (uint8_t)(rf_freq_word)
    };
    radio_write_command(RADIO_CMD_SET_RF_FREQ, freq_buf, 4);

    /* Set modulation parameters: SF, BW, CR */
    uint8_t mod_params[4] = {
        LORA_SF,
        (uint8_t)(LORA_BANDWIDTH_KHZ / 250 * 4 + 4),  /* BW code */
        LORA_CODING_RATE,
        0x00  /* low-data-rate optimize off */
    };
    radio_write_command(RADIO_CMD_SET_MOD_PARAMS, mod_params, 4);

    /* Set TX power */
    uint8_t tx_params[2] = { LORA_TX_POWER_DBM, 0x04 /* ramp 200 us */ };
    radio_write_command(RADIO_CMD_SET_TX_PARAMS, tx_params, 2);

    return 0;
}

int mesh_send(const mesh_packet_t *pkt, uint8_t *aes_key) {
    /* Compute AES-128 CMAC over the 18-byte packet */
    uint8_t mac[16];
    aes_cmac_compute(aes_key, (const uint8_t *)pkt, MESH_PACKET_SIZE, mac);

    /* Build TX buffer: packet + 2-byte truncated MAC */
    uint8_t tx_buf[MESH_PACKET_SIZE + 2];
    for (int i = 0; i < MESH_PACKET_SIZE; i++) {
        tx_buf[i] = ((const uint8_t *)pkt)[i];
    }
    tx_buf[MESH_PACKET_SIZE]     = mac[0];
    tx_buf[MESH_PACKET_SIZE + 1] = mac[1];

    /* Write buffer to radio */
    uint8_t offset_cmd[1] = { 0 };
    radio_write_command(RADIO_CMD_WRITE_BUFFER, offset_cmd, 1);
    radio_write_command(0x00, tx_buf, MESH_PACKET_SIZE + 2);

    /* Set packet params (preamble, header type, payload length, CRC) */
    uint8_t pkt_params[9] = {
        0x00, 0x08,  /* preamble length 8 */
        0x01,        /* explicit header */
        (uint8_t)(MESH_PACKET_SIZE + 2),
        0x01,        /* CRC on */
        0x00, 0x00, 0x00, 0x00
    };
    radio_write_command(RADIO_CMD_SET_PACKET_PARAMS, pkt_params, 9);

    /* Clear IRQ flags */
    uint8_t clear_cmd[3] = { 0xFF, 0xFF, 0xFF };
    radio_write_command(RADIO_CMD_CLEAR_IRQ, clear_cmd, 3);

    /* Enter TX */
    uint8_t tx_time_cmd[3] = { 0, 0, 0 };  /* no timeout */
    radio_write_command(RADIO_CMD_SET_TX, tx_time_cmd, 3);

    /* Wait for TX done (poll DIO1 or busy) */
    uint32_t timeout = 0;
    while (!((GPIOB->IDR >> PB6__LORA_DIO1) & 1) && timeout < 100000) {
        delay_ms(1);
        timeout++;
    }

    radio_write_command(RADIO_CMD_CLEAR_IRQ, clear_cmd, 3);
    return (timeout < 100000) ? 0 : -1;
}

int mesh_recv(mesh_packet_t *out, uint8_t *aes_key, uint32_t timeout_ms) {
    /* Set RX with timeout */
    uint32_t rx_timeout = timeout_ms * 64;  /* 15.625 us per unit */
    uint8_t rx_cmd[3] = {
        (uint8_t)(rx_timeout >> 16),
        (uint8_t)(rx_timeout >> 8),
        (uint8_t)(rx_timeout)
    };
    radio_write_command(RADIO_CMD_SET_RX, rx_cmd, 3);

    /* Wait for DIO1 (RX done) */
    uint32_t waited = 0;
    while (!((GPIOB->IDR >> PB6__LORA_DIO1) & 1) && waited < timeout_ms) {
        delay_ms(10);
        waited += 10;
    }
    if (waited >= timeout_ms) return MESH_ERR_RX_TIMEOUT;

    /* Read received buffer */
    uint8_t rx_buf[MESH_PACKET_SIZE + 2];
    uint8_t offset[1] = { 0 };
    radio_write_command(RADIO_CMD_READ_BUFFER, offset, 1);
    radio_read_command(0x00, rx_buf, MESH_PACKET_SIZE + 2);

    /* Parse packet */
    for (int i = 0; i < MESH_PACKET_SIZE; i++) {
        ((uint8_t *)out)[i] = rx_buf[i];
    }

    /* Verify CMAC */
    uint8_t mac[16];
    aes_cmac_compute(aes_key, (const uint8_t *)out, MESH_PACKET_SIZE, mac);
    if (mac[0] != rx_buf[MESH_PACKET_SIZE] || mac[1] != rx_buf[MESH_PACKET_SIZE + 1]) {
        return MESH_ERR_CRC;
    }

    /* Check duplicate + hop limit */
    if (is_duplicate(out->serial, out->timestamp_min)) return MESH_ERR_CRC;
    if (out->hop_count >= MESH_MAX_HOPS) return MESH_ERR_CRC;

    cache_add(out->serial, out->timestamp_min, out->hop_count);

    /* Relay: increment hop count and re-transmit with random backoff */
    out->hop_count++;
    for (int t = 0; t < (my_serial % 3 + 1) * 10; t++) delay_ms(100);

    mesh_send(out, aes_key);

    return MESH_OK;
}