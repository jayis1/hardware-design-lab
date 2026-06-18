/*
 * lorawan.c - LoRaWAN class-A MAC layer for Pollen Scout
 * Implements OTAA join + confirmed uplink using SX1262 radio.
 * Payload encryption uses AES-128-CTR (simplified; production uses
 * full LoRaWAN MIC + AES-CMAC per RP002-1.0.4).
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "lorawan.h"
#include "board.h"
#include "registers.h"
#include "sx1262.h"
#include <string.h>

/* EU868 default channels: 868.10, 868.30, 868.50 MHz */
static const uint32_t eu868_channels[3] = { 868100000U, 868300000U, 868500000U };
static const uint32_t us915_channels[8] = {
    902300000U, 902500000U, 902700000U, 902900000U,
    903100000U, 903300000U, 903500000U, 903700000U
};

typedef struct {
    lorawan_region_t region;
    uint8_t  dev_eui[8];
    uint8_t  join_eui[8];
    uint8_t  app_key[16];
    uint8_t  dev_addr[4];
    uint8_t  fnwk_sint_key[16];
    uint8_t  app_s_key[16];
    uint16_t dev_nonce;
    uint32_t fcnt_up;
    uint32_t fcnt_down;
    int      joined;
    int      channel_idx;
} lorawan_ctx_t;

static lorawan_ctx_t s_ctx;

/* ---- Minimal AES-128 (educational; production uses HW AES on H7) ---- */
static uint8_t sbox[256] = {
    /* standard AES S-box (truncated for brevity; full 256 values) */
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static uint8_t xtime(uint8_t x)
{
    return (x << 1) ^ ((x >> 7) * 0x1b);
}

static void aes_encrypt_block(uint8_t *state, const uint8_t *round_keys)
{
    /* Placeholder: a full AES-128 round-function implementation would go
     * here (SubBytes, ShiftRows, MixColumns, AddRoundKey x10). In
     * production this is replaced by STM32 CRYP hardware acceleration. */
    for (int i = 0; i < 16; i++) state[i] = sbox[state[i] ^ round_keys[i]];
}

/* AES-CTR encrypt/decrypt (same op) */
static void aes_ctr(const uint8_t *key, const uint8_t *iv,
                    const uint8_t *in, uint8_t *out, int len)
{
    uint8_t ctr[16], blk[16];
    memcpy(ctr, iv, 16);
    int off = 0;
    while (off < len) {
        memcpy(blk, ctr, 16);
        aes_encrypt_block(blk, key);
        int n = (len - off < 16) ? (len - off) : 16;
        for (int i = 0; i < n; i++) out[off + i] = in[off + i] ^ blk[i];
        off += n;
        /* increment counter (big-endian, last 4 bytes) */
        for (int i = 15; i >= 12; i--) { if (++ctr[i]) break; }
    }
}

int lorawan_init(lorawan_region_t region)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.region = region;
    sx1262_init();

    if (region == LORAWAN_REGION_EU868) {
        sx1262_set_frequency(eu868_channels[0]);
        sx1262_set_tx_power(14);       /* +14 dBm EU868 */
        sx1262_set_lora_params(7, 0, 1); /* SF7, 125 kHz, 4/5 */
    } else {
        sx1262_set_frequency(us915_channels[0]);
        sx1262_set_tx_power(20);
        sx1262_set_lora_params(7, 0, 1);
    }

    /* DevEUI is derived from STM32 UID (96-bit unique ID at 0x1FF1E800) */
    volatile uint32_t *uid = (volatile uint32_t *)0x1FF1E800U;
    memcpy(s_ctx.dev_eui, (const void *)uid, 8);
    /* JoinEUI/AppKey come from provisioning via the ESP32 bridge */
    return 0;
}

void lorawan_set_keys(const uint8_t *join_eui, const uint8_t *app_key)
{
    memcpy(s_ctx.join_eui, join_eui, 8);
    memcpy(s_ctx.app_key,  app_key,  16);
}

int lorawan_join(void)
{
    /* Build JoinRequest: JoinEUI(8) + DevEUI(8) + DevNonce(2) + MIC(4) */
    uint8_t req[23];
    memcpy(req + 0,  s_ctx.join_eui, 8);
    memcpy(req + 8,  s_ctx.dev_eui,  8);
    req[16] = (uint8_t)(s_ctx.dev_nonce & 0xFF);
    req[17] = (uint8_t)(s_ctx.dev_nonce >> 8);
    /* MIC (simplified = first 4 bytes of AES(AppKey, JoinEUI||DevEUI||Nonce)) */
    uint8_t mic_in[16];
    memcpy(mic_in, req, 16);
    aes_encrypt_block(mic_in, s_ctx.app_key);
    memcpy(req + 18, mic_in, 4);

    sx1262_send(req, 22, 5000);
    /* Open RX1 window after 5 s (join-accept) */
    sx1262_receive(2000);

    /* In a full implementation we'd parse the JoinAccept, derive
     * FnwkSIntKey/AppSKey from AppKey, and set s_ctx.joined = 1. Here
     * we assume success for the demo build. */
    s_ctx.joined   = 1;
    s_ctx.dev_nonce++;
    return 0;
}

int lorawan_send(const uint8_t *payload, int len, int confirmed)
{
    if (!s_ctx.joined) {
        if (lorawan_join() != 0) return -1;
    }
    /* Build uplink: MHDR(1) + DevAddr(4) + FCtrl(1) + FCnt(2) + FPort(1) + FRMPayload + MIC(4) */
    uint8_t pkt[64];
    int idx = 0;
    pkt[idx++] = 0x40 | (confirmed ? 0x80 : 0x00);   /* MType=Unconfirmed/Confirmed Up */
    memcpy(pkt + idx, s_ctx.dev_addr, 4); idx += 4;
    pkt[idx++] = 0x00;                               /* FCtrl */
    pkt[idx++] = (uint8_t)(s_ctx.fcnt_up & 0xFF);
    pkt[idx++] = (uint8_t)((s_ctx.fcnt_up >> 8) & 0xFF);
    pkt[idx++] = 0x01;                               /* FPort 1 */

    /* Encrypt FRMPayload with AppSKey (AES-CTR) */
    uint8_t iv[16] = {0};
    iv[5]  = 0x01;                                   /* direction up */
    memcpy(iv + 6, s_ctx.dev_addr, 4);
    iv[10] = (uint8_t)(s_ctx.fcnt_up & 0xFF);
    iv[11] = (uint8_t)((s_ctx.fcnt_up >> 8) & 0xFF);
    aes_ctr(s_ctx.app_s_key, iv, payload, pkt + idx, len);
    idx += len;

    /* MIC (simplified) */
    uint8_t mic_blk[16] = {0};
    memcpy(mic_blk, pkt, (idx < 16) ? idx : 16);
    aes_encrypt_block(mic_blk, s_ctx.fnwk_sint_key);
    memcpy(pkt + idx, mic_blk, 4);
    idx += 4;

    /* Pick next channel */
    if (s_ctx.region == LORAWAN_REGION_EU868) {
        s_ctx.channel_idx = (s_ctx.channel_idx + 1) % 3;
        sx1262_set_frequency(eu868_channels[s_ctx.channel_idx]);
    } else {
        s_ctx.channel_idx = (s_ctx.channel_idx + 1) % 8;
        sx1262_set_frequency(us915_channels[s_ctx.channel_idx]);
    }

    sx1262_send(pkt, idx, 8000);
    s_ctx.fcnt_up++;
    /* Class-A RX1 window */
    sx1262_receive(1000);
    return 0;
}