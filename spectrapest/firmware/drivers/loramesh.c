/*
 * drivers/loramesh.c — LoRa mesh networking for SpectraPest
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Implements a lightweight mesh networking protocol ("SpectraMesh") on top
 * of the Semtech SX1262 LoRa transceiver. The protocol supports:
 *
 *   - Broadcast messaging (pest alerts, beacons)
 *   - Unicast with AODV-style route discovery
 *   - Store-and-forward relaying with TTL decrement
 *   - Duplicate detection via (source, msg_id) cache
 *   - Automatic route table maintenance
 *
 * The SX1262 is driven via SPI1 with NSS chip-select. The radio's DIO1
 * interrupt signals TX-done or RX-done events. The Busy pin is polled
 * before each SPI transaction (the SX1262 goes busy during configuration
 * and state transitions).
 *
 * Mesh protocol message format (8-byte header + payload):
 *
 *   Byte 0: msg_type (1=pest, 2=beacon, 3=route_req, 4=ack, 5=config, 6=ota)
 *   Byte 1: source_addr
 *   Byte 2: dest_addr (0xFF = broadcast)
 *   Byte 3: ttl (max 8)
 *   Byte 4-5: msg_id (unique per source)
 *   Byte 6-7: payload_len (0 to 56)
 *   Byte 8+: payload
 *
 * Pest alert payload (12 bytes):
 *   Byte 0: species_id
 *   Byte 1: severity (0-3)
 *   Byte 2: confidence (0-100)
 *   Byte 3: node_id
 *   Byte 4-7: lat_e7 (int32, latitude × 10^7)
 *   Byte 8-11: lon_e7 (int32, longitude × 10^7)
 *   (timestamp is implicit — relayed by gateway)
 *
 * Route discovery uses a simple flooding RREQ: the originator broadcasts
 * a ROUTE message with the destination address. Any node that has a route
 * to the destination (or IS the destination) replies with a unicast ROUTE
 * message back along the reverse path. The route table stores next-hop
 * for each known destination.
 */

#include "loramesh.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

/* ----------------------------------------------------------------- *
 *  SX1262 SPI register commands
 * ----------------------------------------------------------------- */
#define SX1262_CMD_SET_STANDBY    0x80
#define SX1262_CMD_SET_PACKET_TYPE 0x8A
#define SX1262_CMD_SET_RF_FREQ    0x86
#define SX1262_CMD_SET_TX_PARAMS  0x8E
#define SX1262_CMD_SET_MOD_PARAMS 0x8B
#define SX1262_CMD_SET_PACKET_PARAMS 0x8C
#define SX1262_CMD_SET_CAD        0xC0
#define SX1262_CMD_SET_TX         0x83
#define SX1262_CMD_SET_RX         0x82
#define SX1262_CMD_WRITE_BUFFER   0x0D
#define SX1262_CMD_READ_BUFFER    0x1D
#define SX1262_CMD_GET_RX_BUFFER_STATUS 0x13
#define SX1262_CMD_GET_PACKET_STATUS 0x14
#define SX1262_CMD_CLEAR_IRQ     0x02
#define SX1262_CMD_GET_IRQ_STATUS 0x12
#define SX1262_CMD_SET_DIO_IRQ_PARAMS 0x08
#define SX1262_CMD_SET_DIO2_AS_RF_SW 0x9D
#define SX1262_CMD_SET_DIO3_AS_TCXO 0x97
#define SX1262_CMD_RESET_STATS    0x00

#define SX1262_PACKET_TYPE_LORA   0x01
#define SX1262_STDBY_RC           0x00
#define SX1262_STDBY_XOSC          0x01

#define SX1262_IRQ_TX_DONE        0x0001
#define SX1262_IRQ_RX_DONE        0x0002
#define SX1262_IRQ_CRC_ERR        0x0004
#define SX1262_IRQ_TIMEOUT        0x0020
#define SX1262_IRQ_HEADER_ERR    0x0010

/* ----------------------------------------------------------------- *
 *  Internal state
 * ----------------------------------------------------------------- */
static uint8_t  g_self_addr = 0;
static uint16_t g_next_msg_id = 1;
static uint8_t  g_lora_initialized = 0;
static uint8_t  g_rx_buffer[256];
static uint8_t  g_tx_buffer[256];

/* Route table: dest -> next_hop */
typedef struct {
    uint8_t  dest;
    uint8_t  next_hop;
    uint8_t  hop_count;
    uint32_t last_seen;
} route_entry_t;

#define MAX_ROUTES 16
static route_entry_t g_route_table[MAX_ROUTES];

/* Duplicate detection cache: (source, msg_id) ring buffer */
#define DUP_CACHE_SIZE 32
typedef struct {
    uint8_t  source;
    uint16_t msg_id;
} dup_cache_entry_t;
static dup_cache_entry_t g_dup_cache[DUP_CACHE_SIZE];
static uint8_t g_dup_cache_idx = 0;

/* Pending TX state */
static volatile uint8_t g_tx_done = 0;
static volatile uint8_t g_rx_ready = 0;
static volatile uint16_t g_last_irq_flags = 0;

/* ----------------------------------------------------------------- *
 *  SX1262 SPI interface
 * ----------------------------------------------------------------- */
static void sx1262_wait_busy(void) {
    uint32_t timeout = board_get_tick_ms() + 100;
    while (gpio_read(LORA_BUSY_PORT, LORA_BUSY_PIN)) {
        if (board_get_tick_ms() > timeout) break;
    }
}

static void sx1262_spi_write(uint8_t *data, uint8_t len) {
    sx1262_wait_busy();

    /* NSS low */
    gpio_write(LORA_NSS_PORT, LORA_NSS_PIN, 0);

    /* Enable SPI1 */
    SPI1->CR1 |= SPI_CR1_SPE;

    for (uint8_t i = 0; i < len; i++) {
        SPI1->TXDR = data[i];
        while (!(SPI1->SR & SPI_SR_TXP));
    }
    /* Wait for completion */
    while (!(SPI1->SR & SPI_SR_EOT));

    /* NSS high */
    gpio_write(LORA_NSS_PORT, LORA_NSS_PIN, 1);

    /* Disable SPI1 */
    SPI1->CR1 &= ~SPI_CR1_SPE;
}

static void sx1262_spi_read(uint8_t cmd, uint8_t *out, uint8_t len) {
    sx1262_wait_busy();

    gpio_write(LORA_NSS_PORT, LORA_NSS_PIN, 0);
    SPI1->CR1 |= SPI_CR1_SPE;

    /* Send command */
    SPI1->TXDR = cmd;
    while (!(SPI1->SR & SPI_SR_TXP));
    /* Dummy byte for read */
    SPI1->TXDR = 0x00;
    while (!(SPI1->SR & SPI_SR_TXP));

    /* Read response */
    for (uint8_t i = 0; i < len; i++) {
        SPI1->TXDR = 0x00;
        while (!(SPI1->SR & SPI_SR_TXP));
        out[i] = SPI1->RXDR;
    }

    while (!(SPI1->SR & SPI_SR_EOT));
    gpio_write(LORA_NSS_PORT, LORA_NSS_PIN, 1);
    SPI1->CR1 &= ~SPI_CR1_SPE;
}

static void sx1262_write_reg(uint16_t addr, uint8_t value) {
    uint8_t buf[4];
    buf[0] = SX1262_CMD_WRITE_BUFFER;  /* Actually write register */
    buf[1] = (addr >> 8) & 0xFF;
    buf[2] = addr & 0xFF;
    buf[3] = value;
    sx1262_spi_write(buf, 4);
}

static void sx1262_set_standby(uint8_t mode) {
    uint8_t cmd = SX1262_CMD_SET_STANDBY;
    sx1262_spi_write(&cmd, 1);
    sx1262_wait_busy();
}

static void sx1262_set_packet_type(uint8_t type) {
    uint8_t buf[2] = { SX1262_CMD_SET_PACKET_TYPE, type };
    sx1262_spi_write(buf, 2);
}

static void sx1262_set_freq(uint32_t freq_hz) {
    uint8_t buf[4];
    uint32_t rf_freq = (uint64_t)freq_hz * (1 << 25) / 32000000UL;
    buf[0] = SX1262_CMD_SET_RF_FREQ;
    buf[1] = (rf_freq >> 24) & 0xFF;
    buf[2] = (rf_freq >> 16) & 0xFF;
    buf[3] = (rf_freq >> 8) & 0xFF;
    sx1262_spi_write(buf, 4);
    /* Last byte */
    buf[0] = rf_freq & 0xFF;
    sx1262_spi_write(buf, 1);
}

static void sx1262_set_tx_params(uint8_t power_dbm, uint8_t ramp) {
    uint8_t buf[3] = { SX1262_CMD_SET_TX_PARAMS, power_dbm, ramp };
    sx1262_spi_write(buf, 3);
}

static void sx1262_set_mod_params(uint8_t sf, uint8_t bw, uint8_t cr) {
    /* LoRa modulation parameters: SF, BW, CR, LDRO */
    uint8_t buf[5] = {
        SX1262_CMD_SET_MOD_PARAMS,
        sf,       /* Spreading factor 7 */
        bw,       /* Bandwidth 125 kHz = 0x04 */
        cr,       /* Coding rate 4/5 = 0x01 */
        0x00      /* LDRO disabled */
    };
    sx1262_spi_write(buf, 5);
}

static void sx1262_set_packet_params(void) {
    /* LoRa packet parameters: preamble=8, header type=explicit, payload=64,
     * CRC=on, IQ=standard */
    uint8_t buf[9] = {
        SX1262_CMD_SET_PACKET_PARAMS,
        0x00, 0x08,      /* Preamble length = 8 (MSB, LSB) */
        0x00,            /* Explicit header */
        64,              /* Payload length */
        0x01,            /* CRC on */
        0x00,            /* IQ standard */
        0x00, 0x00
    };
    sx1262_spi_write(buf, 7);
}

static void sx1262_set_dio_irq(uint16_t irq_mask, uint16_t dio1_mask) {
    uint8_t buf[9];
    buf[0] = SX1262_CMD_SET_DIO_IRQ_PARAMS;
    buf[1] = (irq_mask >> 8) & 0xFF;
    buf[2] = irq_mask & 0xFF;
    buf[3] = (dio1_mask >> 8) & 0xFF;
    buf[4] = dio1_mask & 0xFF;
    buf[5] = 0x00; buf[6] = 0x00;  /* DIO2 mask */
    buf[7] = 0x00; buf[8] = 0x00;  /* DIO3 mask */
    sx1262_spi_write(buf, 9);
}

static void sx1262_clear_irq(uint16_t mask) {
    uint8_t buf[3] = { SX1262_CMD_CLEAR_IRQ, (mask >> 8) & 0xFF, mask & 0xFF };
    sx1262_spi_write(buf, 3);
}

static uint16_t sx1262_get_irq_status(void) {
    uint8_t out[3];
    sx1262_spi_read(SX1262_CMD_GET_IRQ_STATUS, out, 3);
    return ((uint16_t)out[1] << 8) | out[2];
}

/* ----------------------------------------------------------------- *
 *  DIO1 interrupt handler (TX-done / RX-done)
 * ----------------------------------------------------------------- */
void EXTI0_IRQHandler(void) {
    /* Clear pending interrupt */
    *(volatile uint32_t *)0x50000210 = (1 << 0);  /* EXTI PR1 */

    g_last_irq_flags = sx1262_get_irq_status();

    if (g_last_irq_flags & SX1262_IRQ_TX_DONE) {
        g_tx_done = 1;
        sx1262_clear_irq(SX1262_IRQ_TX_DONE);
    }
    if (g_last_irq_flags & SX1262_IRQ_RX_DONE) {
        g_rx_ready = 1;
        sx1262_clear_irq(SX1262_IRQ_RX_DONE);
    }
    if (g_last_irq_flags & (SX1262_IRQ_CRC_ERR | SX1262_IRQ_TIMEOUT)) {
        sx1262_clear_irq(SX1262_IRQ_CRC_ERR | SX1262_IRQ_TIMEOUT);
    }
}

/* ----------------------------------------------------------------- *
 *  Initialize LoRa radio and mesh stack
 * ----------------------------------------------------------------- */
int lora_init(uint8_t node_addr, uint32_t freq_hz) {
    g_self_addr = node_addr;

    /* Configure GPIO pins */
    gpio_set_mode(LORA_NSS_PORT, LORA_NSS_PIN, GPIO_MODE_OUTPUT);
    gpio_write(LORA_NSS_PORT, LORA_NSS_PIN, 1);

    gpio_set_mode(LORA_RST_PORT, LORA_RST_PIN, GPIO_MODE_OUTPUT);
    gpio_write(LORA_RST_PORT, LORA_RST_PIN, 0);
    board_delay_ms(10);
    gpio_write(LORA_RST_PORT, LORA_RST_PIN, 1);
    board_delay_ms(50);

    gpio_set_mode(LORA_BUSY_PORT, LORA_BUSY_PIN, GPIO_MODE_INPUT);
    gpio_set_mode(LORA_DIO1_PORT, LORA_DIO1_PIN, GPIO_MODE_INPUT);

    /* Configure SPI1: SCK=PA5, MISO=PA6, MOSI=PA7 */
    gpio_set_mode(GPIOA, 5, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 5, AF_SPI1_SCK);
    gpio_set_ospeed(GPIOA, 5, GPIO_SPEED_VHIGH);
    gpio_set_mode(GPIOA, 6, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 6, AF_SPI1_MISO);
    gpio_set_mode(GPIOA, 7, GPIO_MODE_AF);
    gpio_set_af(GPIOA, 7, AF_SPI1_MOSI);
    gpio_set_ospeed(GPIOA, 7, GPIO_SPEED_VHIGH);

    /* Enable SPI1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* Configure SPI1: master, 8-bit, CPOL=0, CPHA=0, MSB first */
    SPI1->CFG1 = SPI_CFG1_MASTER | SPI_CFG1_CSIZE_8BIT;
    SPI1->CFG2 = SPI_CFG2_COMM_FULL | SPI_CFG2_CPHA;
    SPI1->CR1 = 0;

    /* Hardware reset of SX1262 */
    gpio_write(LORA_RST_PORT, LORA_RST_PIN, 0);
    board_delay_ms(10);
    gpio_write(LORA_RST_PORT, LORA_RST_PIN, 1);
    board_delay_ms(50);

    /* Configure SX1262 */
    sx1262_set_standby(SX1262_STDBY_RC);
    sx1262_set_packet_type(SX1262_PACKET_TYPE_LORA);
    sx1262_set_freq(freq_hz);
    sx1262_set_tx_params(LORA_TX_POWER_DBM, 0x04);  /* 20 dBm, 200us ramp */
    sx1262_set_mod_params(LORA_SF, 0x04, 0x01);    /* SF7, 125kHz, CR 4/5 */
    sx1262_set_packet_params();
    sx1262_set_dio_irq(SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE | SX1262_IRQ_CRC_ERR,
                       SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE);

    /* Enable EXTI0 for DIO1 (PB0) */
    *(volatile uint32_t *)0x50000000 |= (1 << 0);  /* EXTI IMR1 bit 0 */
    /* Falling edge trigger */
    *(volatile uint32_t *)0x50000008 &= ~(1 << 0);  /* FTSR1 */
    *(volatile uint32_t *)0x5000000C |= (1 << 0);   /* RTSR1 (rising) */
    NVIC_ISER0 |= (1 << IRQ_EXTI0);

    /* Clear route table */
    memset(g_route_table, 0, sizeof(g_route_table));
    memset(g_dup_cache, 0, sizeof(g_dup_cache));

    g_lora_initialized = 1;
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Send a LoRa packet
 * ----------------------------------------------------------------- */
int lora_send(mesh_message_t *msg) {
    if (!msg || !g_lora_initialized) return -1;

    /* Serialize message into TX buffer */
    g_tx_buffer[0] = msg->msg_type;
    g_tx_buffer[1] = msg->source_addr;
    g_tx_buffer[2] = msg->dest_addr;
    g_tx_buffer[3] = msg->ttl;
    g_tx_buffer[4] = (msg->msg_id >> 8) & 0xFF;
    g_tx_buffer[5] = msg->msg_id & 0xFF;
    g_tx_buffer[6] = (msg->payload_len >> 8) & 0xFF;
    g_tx_buffer[7] = msg->payload_len & 0xFF;
    if (msg->payload_len > 0) {
        memcpy(&g_tx_buffer[8], msg->payload, msg->payload_len);
    }
    uint8_t total_len = 8 + msg->payload_len;

    /* Write to SX1262 buffer */
    uint8_t cmd[2] = { SX1262_CMD_WRITE_BUFFER, 0x00 };
    sx1262_spi_write(cmd, 1);
    sx1262_spi_write(g_tx_buffer, total_len);

    /* Set to TX mode with timeout (3 seconds) */
    uint8_t tx_cmd[3] = { SX1262_CMD_SET_TX, 0x00, 0x00 };
    sx1262_spi_write(tx_cmd, 3);

    g_tx_done = 0;
    uint32_t start = board_get_tick_ms();
    while (!g_tx_done) {
        if (board_get_tick_ms() - start > 3000) {
            sx1262_set_standby(SX1262_STDBY_RC);
            return -1;
        }
    }

    sx1262_set_standby(SX1262_STDBY_RC);
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Receive a LoRa packet (blocking with timeout)
 * ----------------------------------------------------------------- */
int lora_receive(mesh_message_t *msg, uint32_t timeout_ms) {
    if (!msg || !g_lora_initialized) return -1;

    /* Set to RX mode with timeout */
    uint32_t timeout_ticks = timeout_ms * 64;  /* SX1262 timeout is in 15.625us units */
    uint8_t cmd[4] = {
        SX1262_CMD_SET_RX,
        (timeout_ticks >> 16) & 0xFF,
        (timeout_ticks >> 8) & 0xFF,
        timeout_ticks & 0xFF
    };
    sx1262_spi_write(cmd, 4);

    g_rx_ready = 0;
    uint32_t start = board_get_tick_ms();
    while (!g_rx_ready) {
        if (board_get_tick_ms() - start > timeout_ms) {
            sx1262_set_standby(SX1262_STDBY_RC);
            return -1;
        }
    }

    /* Read RX buffer status */
    uint8_t status[3];
    sx1262_spi_read(SX1262_CMD_GET_RX_BUFFER_STATUS, status, 3);
    uint8_t payload_len = status[1];
    uint8_t rx_start_ptr = status[2];

    if (payload_len > sizeof(g_rx_buffer)) return -1;

    /* Read packet from SX1262 buffer */
    uint8_t read_cmd[2] = { SX1262_CMD_READ_BUFFER, rx_start_ptr };
    sx1262_spi_write(read_cmd, 2);
    sx1262_spi_read(0x00, g_rx_buffer, payload_len);

    /* Deserialize message */
    if (payload_len < 8) return -1;

    msg->msg_type    = g_rx_buffer[0];
    msg->source_addr = g_rx_buffer[1];
    msg->dest_addr    = g_rx_buffer[2];
    msg->ttl         = g_rx_buffer[3];
    msg->msg_id      = ((uint16_t)g_rx_buffer[4] << 8) | g_rx_buffer[5];
    msg->payload_len  = ((uint16_t)g_rx_buffer[6] << 8) | g_rx_buffer[7];

    if (msg->payload_len > MESH_MAX_MSG_LEN - 8) return -1;

    memcpy(msg->payload, &g_rx_buffer[8], msg->payload_len);

    sx1262_set_standby(SX1262_STDBY_RC);
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Send a pest alert
 * ----------------------------------------------------------------- */
int lora_send_pest_alert(const pest_alert_t *alert, uint8_t dest) {
    mesh_message_t msg;
    msg.msg_type = MESH_MSG_PEST;
    msg.source_addr = g_self_addr;
    msg.dest_addr = dest;
    msg.ttl = MESH_TTL_MAX;
    msg.msg_id = g_next_msg_id++;
    msg.payload_len = 12;  /* 12 bytes for pest alert */

    msg.payload[0] = alert->species_id;
    msg.payload[1] = alert->severity;
    msg.payload[2] = alert->confidence_pct;
    msg.payload[3] = alert->node_id;
    memcpy(&msg.payload[4], &alert->lat_e7, 4);
    memcpy(&msg.payload[8], &alert->lon_e7, 4);

    return lora_send(&msg);
}

void lora_sleep(void) {
    sx1262_set_standby(SX1262_STDBY_RC);
    /* Reduce power: set to XOSC standby */
    uint8_t cmd = SX1262_CMD_SET_STANDBY;
    sx1262_spi_write(&cmd, 1);
}

void lora_wakeup(void) {
    sx1262_set_standby(SX1262_STDBY_RC);
}

int lora_get_rssi(void) {
    uint8_t status[3];
    sx1262_spi_read(SX1262_CMD_GET_PACKET_STATUS, status, 3);
    return -(int)status[1];  /* RSSI is negative, stored as -dBm */
}

/* ----------------------------------------------------------------- *
 *  Mesh routing functions
 * ----------------------------------------------------------------- */
int mesh_init(uint8_t self_addr) {
    return lora_init(self_addr, LORA_FREQ_HZ);
}

int mesh_broadcast(mesh_message_t *msg) {
    msg->dest_addr = 0xFF;  /* Broadcast */
    msg->source_addr = g_self_addr;
    msg->msg_id = g_next_msg_id++;
    msg->ttl = MESH_TTL_MAX;
    return lora_send(msg);
}

int mesh_send(uint8_t dest, mesh_message_t *msg) {
    /* Look up route */
    uint8_t next_hop = 0xFF;
    for (int i = 0; i < MAX_ROUTES; i++) {
        if (g_route_table[i].dest == dest) {
            next_hop = g_route_table[i].next_hop;
            break;
        }
    }

    if (next_hop == 0xFF) {
        /* No route: initiate discovery */
        mesh_discover_route(dest);
        /* Try again after a brief wait */
        board_delay_ms(2000);
        for (int i = 0; i < MAX_ROUTES; i++) {
            if (g_route_table[i].dest == dest) {
                next_hop = g_route_table[i].next_hop;
                break;
            }
        }
        if (next_hop == 0xFF) return -1;
    }

    msg->dest_addr = next_hop;
    msg->source_addr = g_self_addr;
    msg->msg_id = g_next_msg_id++;
    msg->ttl = MESH_TTL_MAX;
    return lora_send(msg);
}

int mesh_process_rx(mesh_message_t *msg) {
    if (!msg) return -1;

    /* Check if this is for us or broadcast */
    uint8_t is_for_me = (msg->dest_addr == g_self_addr || msg->dest_addr == 0xFF);

    /* Duplicate detection */
    if (mesh_is_duplicate(msg->msg_id, msg->source_addr)) {
        return 1;  /* Duplicate, skip */
    }
    mesh_record_msg(msg->msg_id, msg->source_addr);

    /* Update route table (learn from source) */
    for (int i = 0; i < MAX_ROUTES; i++) {
        if (g_route_table[i].dest == msg->source_addr) {
            g_route_table[i].next_hop = msg->source_addr;
            g_route_table[i].hop_count = 1;
            g_route_table[i].last_seen = board_get_tick_ms();
            break;
        }
        if (g_route_table[i].dest == 0) {
            g_route_table[i].dest = msg->source_addr;
            g_route_table[i].next_hop = msg->source_addr;
            g_route_table[i].hop_count = 1;
            g_route_table[i].last_seen = board_get_tick_ms();
            break;
        }
    }

    /* Relay if not for me and TTL > 1 */
    if (!is_for_me && msg->ttl > 1) {
        msg->ttl--;
        lora_send(msg);
        return 2;  /* Relayed */
    }

    /* Process based on message type */
    switch (msg->msg_type) {
        case MESH_MSG_PEST:
            /* Store in FRAM event log (handled by main) */
            return 0;

        case MESH_MSG_BEACON:
            /* Update route table with beacon source */
            return 0;

        case MESH_MSG_ROUTE:
            /* Route reply: update our table */
            if (msg->payload_len >= 2) {
                uint8_t target = msg->payload[0];
                uint8_t hop_count = msg->payload[1];
                for (int i = 0; i < MAX_ROUTES; i++) {
                    if (g_route_table[i].dest == target) {
                        if (hop_count < g_route_table[i].hop_count || g_route_table[i].hop_count == 0) {
                            g_route_table[i].next_hop = msg->source_addr;
                            g_route_table[i].hop_count = hop_count + 1;
                            g_route_table[i].last_seen = board_get_tick_ms();
                        }
                        break;
                    }
                    if (g_route_table[i].dest == 0) {
                        g_route_table[i].dest = target;
                        g_route_table[i].next_hop = msg->source_addr;
                        g_route_table[i].hop_count = hop_count + 1;
                        g_route_table[i].last_seen = board_get_tick_ms();
                        break;
                    }
                }
            }
            return 0;

        default:
            return 0;
    }
}

int mesh_discover_route(uint8_t dest) {
    mesh_message_t rreq;
    rreq.msg_type = MESH_MSG_ROUTE;
    rreq.source_addr = g_self_addr;
    rreq.dest_addr = 0xFF;  /* Broadcast */
    rreq.ttl = MESH_TTL_MAX;
    rreq.msg_id = g_next_msg_id++;
    rreq.payload_len = 2;
    rreq.payload[0] = dest;    /* Target we're looking for */
    rreq.payload[1] = 0;       /* Hop count from us */
    return lora_send(&rreq);
}

void mesh_tick(void) {
    /* Periodic route table maintenance: expire old routes */
    uint32_t now = board_get_tick_ms();
    for (int i = 0; i < MAX_ROUTES; i++) {
        if (g_route_table[i].dest != 0) {
            if (now - g_route_table[i].last_seen > 300000) {  /* 5 minutes */
                g_route_table[i].dest = 0;
                g_route_table[i].next_hop = 0;
                g_route_table[i].hop_count = 0;
            }
        }
    }
}

int mesh_is_duplicate(uint16_t msg_id, uint8_t source) {
    for (int i = 0; i < DUP_CACHE_SIZE; i++) {
        if (g_dup_cache[i].source == source && g_dup_cache[i].msg_id == msg_id) {
            return 1;
        }
    }
    return 0;
}

void mesh_record_msg(uint16_t msg_id, uint8_t source) {
    g_dup_cache[g_dup_cache_idx].source = source;
    g_dup_cache[g_dup_cache_idx].msg_id = msg_id;
    g_dup_cache_idx = (g_dup_cache_idx + 1) % DUP_CACHE_SIZE;
}