/*
 * drivers/sx1262.h — Semtech SX1262 LoRa radio driver (SPI)
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#ifndef MUSSEL_SX1262_H
#define MUSSEL_SX1262_H

#include <stdint.h>
#include <stdbool.h>

#define SX1262_FREQ_868MHZ  868100000u

/* SX1262 opcodes */
#define SX1262_SET_SLEEP      0x84u
#define SX1262_SET_STANDBY   0x80u
#define SX1262_SET_TX        0x83u
#define SX1262_SET_RX        0x82u
#define SX1262_WRITE_REG     0x0Du
#define SX1262_READ_REG      0x1Du
#define SX1262_WRITE_BUFFER 0x0Eu
#define SX1262_READ_BUFFER  0x1Eu
#define SX1262_SET_RF_FREQ   0x86u
#define SX1262_SET_TX_PARAMS 0x8Eu
#define SX1262_SET_MOD_PARAMS 0x8Bu
#define SX1262_SET_PACKET_TYPE 0x8Au
#define SX1262_SET_PACKET_PARAMS 0x8Cu
#define SX1262_GET_RX_BUFFER_STATUS 0x13u
#define SX1262_GET_TX_BUFFER_STATUS 0x17u
#define SX1262_GET_STATUS    0xC0u
#define SX1262_CLEAR_IRQ     0x02u
#define SX1262_GET_IRQ       0x12u
#define SX1262_SET_DIO2_AS_RF_SWITCH 0x9Du

/* IRQ flags */
#define SX1262_IRQ_TX_DONE   0x01u
#define SX1262_IRQ_RX_DONE   0x02u
#define SX1262_IRQ_TIMEOUT   0x20u
#define SX1262_IRQ_CRC_ERR   0x40u

typedef enum {
    SX1262_PKT_LORA = 0x01u,
} sx_pkt_type_t;

bool sx1262_init(void);
bool sx1262_send(const uint8_t *data, uint8_t len);
bool sx1262_receive(uint8_t *data, uint8_t *len, uint32_t timeout_ms);
void sx1262_sleep(void);
bool sx1262_set_frequency(uint32_t freq_hz);
bool sx1262_set_tx_power(int8_t dbm);
void sx1262_reset(void);

#endif