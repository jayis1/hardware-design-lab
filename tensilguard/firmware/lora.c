/*
 * lora.c — SX1262 LoRa radio driver and mesh relay layer
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Provides a blocking TX path, an async RX path (DIO1 interrupt), and a
 * store-and-forward mesh relay so a whole cable fan reports through a single
 * gateway. Packets carry a mesh header (relay_id, hop_seq, snr) so the
 * gateway can reconstruct topology and the installer can see link quality.
 *
 * The SX1262 is driven over SPI1 with NSS, BUSY, DIO1 and RST. The driver
 * implements a subset of the Semtech command set sufficient for TensilGuard's
 * LoRaWAN-class uplinks (SF7..SF12, 868/915 MHz, +22 dBm, CRC on, explicit
 * header). No LoRaWAN MAC is implemented — the mesh is a simple flood-with-
 * hop-limit scheme that is adequate for the small node counts on a single
 * bridge tower (typically 20-80 nodes within a few hundred metres).
 */
#include <string.h>
#include "tensilguard.h"
#include "board.h"
#include "registers.h"
#include "proto.h"
#include "hal.h"

/* SX1262 commands (subset) */
#define SX1262_SET_SLEEP          0x84
#define SX1262_SET_STANDBY        0x80
#define SX1262_SET_TX             0x83
#define SX1262_SET_RX             0x82
#define SX1262_WRITE_BUFFER       0x0E
#define SX1262_READ_BUFFER        0x1E
#define SX1262_SET_DIO_IRQ        0x08
#define SX1262_CLR_IRQ            0x02
#define SX1262_GET_IRQ            0x12
#define SX1262_SET_RF_FREQ         0x86
#define SX1262_SET_TX_PARAM       0x8E
#define SX1262_SET_MOD_PARAMS     0x8B
#define SX1262_SET_PACKET_PARAMS  0x8C
#define SX1262_SET_BUFFER_BASE    0x8F
#define SX1262_GET_RX_BUFFER       0x13

#define IRQ_TX_DONE                0x0001
#define IRQ_RX_DONE                0x0002
#define IRQ_CRC_ERR                0x0010
#define IRQ_TIMEOUT                 0x0040

#define LORA_FREQ_HZ              868000000UL  /* EU 868 */
#define LORA_BW                    0x00         /* 125 kHz */
#define LORA_SF                    7            /* SF7 for short uplinks */
#define LORA_CR                    1            /* 4/5 */
#define LORA_PREAMBLE              12
#define LORA_TX_PWR_DBM            22
#define LORA_TX_TIMEOUT_MS         3000
#define LORA_RX_TIMEOUT_MS         0xFFFFFFFF   /* continuous until IRQ */

static uint8_t s_node_id[6];
static uint8_t s_hop_limit = 4;
static int8_t  s_last_snr = 0;
static int16_t s_last_rssi = 0;

/* ----------------------------- Prototypes --------------------------------- */
static void sx_write(uint8_t cmd, const uint8_t *data, uint8_t len);
static void sx_read(uint8_t cmd, uint8_t *out, uint8_t len);
static void sx_wait_busy(void);
static void sx_set_rf_freq(uint32_t hz);
static void sx_set_modparams(void);
static void sx_set_pktparams(uint8_t pkt_type, uint16_t preamble,
                              uint8_t payload_len, uint8_t crc_on);
static void sx_set_tx_param(int8_t dbm);
static void sx_write_buffer(uint8_t offset, const uint8_t *data, uint8_t len);
static void sx_read_buffer(uint8_t offset, uint8_t *out, uint8_t len);
static void spi1_xfer(const uint8_t *tx, uint8_t *rx, uint16_t n);

/* ----------------------------- Public API --------------------------------- */

void lora_init(const uint8_t node_id[6])
{
    memcpy(s_node_id, node_id, 6);
    gpio_set(PIN_LORA_RST, 0);
    delay_ms(10);
    gpio_set(PIN_LORA_RST, 1);
    delay_ms(50);

    /* standby mode */
    uint8_t standby[] = { 0x00 };   /* STDBY_RC */
    sx_write(SX1262_SET_STANDBY, standby, 1);
    delay_ms(5);

    sx_set_rf_freq(LORA_FREQ_HZ);
    sx_set_modparams();
    sx_set_tx_param(LORA_TX_PWR_DBM);

    /* set buffer base address 0/0 */
    uint8_t base[] = { 0x00, 0x00 };
    sx_write(SX1262_SET_BUFFER_BASE, base, 2);

    /* DIO1: TX_DONE | RX_DONE | CRC_ERR */
    uint8_t irq_cfg[] = { 0x00, 0x00, 0x00, 0x00, IRQ_TX_DONE | IRQ_RX_DONE, 0x00, 0x00, 0x00 };
    sx_write(SX1262_SET_DIO_IRQ, irq_cfg, 8);
}

/*
 * lora_send — blocking transmit with optional mesh relay header.
 * If relay=true, prepend a relay_header_t so downstream nodes can retransmit.
 * Returns 0 on success, negative on timeout/error.
 */
int lora_send(const uint8_t *payload, uint8_t len, bool relay, int8_t snr)
{
    if (len > TG_UPLINK_MAX_BYTES) return -2;

    static uint8_t txbuf[TG_UPLINK_MAX_BYTES + sizeof(relay_header_t)];
    uint8_t total = len;
    memcpy(txbuf, payload, len);

    if (relay) {
        relay_header_t rh;
        memcpy(rh.relay_id, s_node_id, 6);
        rh.hop_seq = 0x01;
        rh.snr_db = snr;
        memmove(txbuf + sizeof(relay_header_t), txbuf, len);
        memcpy(txbuf, &rh, sizeof(relay_header_t));
        total = len + sizeof(relay_header_t);
    }

    /* write payload to SX1262 buffer at offset 0 */
    uint8_t hdr[2] = { 0x00, 0x00 };
    sx_write(SX1262_WRITE_BUFFER, hdr, 1 + 0);  /* offset=0 */
    /* the above is conceptual; real driver writes addr+data in one xfer */
    sx_write_buffer(0x00, txbuf, total);

    sx_set_pktparams(0x01 /* explicit */, LORA_PREAMBLE, total, 0x01 /* CRC on */);

    uint8_t timeout[3] = { (LORA_TX_TIMEOUT_MS >> 16) & 0xFF,
                           (LORA_TX_TIMEOUT_MS >> 8) & 0xFF,
                           LORA_TX_TIMEOUT_MS & 0xFF };
    sx_write(SX1262_SET_TX, timeout, 3);

    /* wait for TX_DONE IRQ */
    uint32_t deadline = 5000;
    while (deadline--) {
        uint8_t irq[3];
        sx_read(SX1262_GET_IRQ, irq, 3);
        uint16_t irqstat = (uint16_t)((irq[0] << 8) | irq[1]);
        if (irqstat & IRQ_TX_DONE) {
            uint8_t clr[] = { 0xFF, 0xFF };
            sx_write(SX1262_CLR_IRQ, clr, 2);
            return 0;
        }
        if (irqstat & IRQ_TIMEOUT) return -3;
        delay_ms(1);
    }
    return -1;
}

/*
 * lora_listen — enter continuous RX. Packets received are relayed if they
 * are not destined for us and hop_seq has free bits.
 */
void lora_listen(void)
{
    uint8_t timeout[3] = { 0xFF, 0xFF, 0xFF };
    sx_write(SX1262_SET_RX, timeout, 3);
}

/*
 * lora_isr — DIO1 interrupt handler. Called on RX_DONE / TX_DONE / CRC_ERR.
 * For RX: read the packet, check mesh header, relay if not for us, else
 * deliver to the measurement uplink queue.
 */
void lora_isr(void)
{
    uint8_t irq[3];
    sx_read(SX1262_GET_IRQ, irq, 3);
    uint16_t irqstat = (uint16_t)((irq[0] << 8) | irq[1]);
    if (irqstat & IRQ_CRC_ERR) {
        uint8_t clr[] = { 0xFF, 0xFF };
        sx_write(SX1262_CLR_IRQ, clr, 2);
        return;
    }
    if (!(irqstat & IRQ_RX_DONE)) return;

    /* read RX buffer status */
    uint8_t rxstat[2];
    sx_read(SX1262_GET_RX_BUFFER, rxstat, 2);
    uint8_t payload_len = rxstat[0];
    uint8_t offset = rxstat[1];

    static uint8_t rxbuf[TG_UPLINK_MAX_BYTES + sizeof(relay_header_t)];
    sx_read_buffer(offset, rxbuf, payload_len);

    /* parse mesh header */
    relay_header_t rh;
    if (payload_len < sizeof(relay_header_t)) return;
    memcpy(&rh, rxbuf, sizeof(relay_header_t));
    uint8_t *inner = rxbuf + sizeof(relay_header_t);
    uint8_t inner_len = payload_len - sizeof(relay_header_t);

    /* is it for us? compare dest in ul_header_t.node_id */
    ul_header_t *ul = (ul_header_t *)inner;
    if (memcmp(ul->node_id, s_node_id, 6) == 0 || inner_len < sizeof(ul_header_t)) {
        /* destined to us — or a broadcast; deliver to main scheduler */
        extern void lora_on_receive(const uint8_t *pkt, uint8_t len, int8_t snr);
        lora_on_receive(inner, inner_len, rh.snr_db);
    } else if (rh.hop_seq != 0xFF && s_hop_limit > 0) {
        /* relay: add our id to the hop bitmap and retransmit */
        rh.hop_seq = (rh.hop_seq << 1) | 0x01;
        rh.snr_db = s_last_snr;
        memcpy(rxbuf, &rh, sizeof(relay_header_t));
        /* re-enter TX briefly */
        sx_write_buffer(0x00, rxbuf, payload_len);
        sx_set_pktparams(0x01, LORA_PREAMBLE, payload_len, 0x01);
        uint8_t timeout[3] = { (LORA_TX_TIMEOUT_MS >> 16) & 0xFF,
                               (LORA_TX_TIMEOUT_MS >> 8) & 0xFF,
                               LORA_TX_TIMEOUT_MS & 0xFF };
        sx_write(SX1262_SET_TX, timeout, 3);
        /* (wait briefly for TX_DONE — omitted for brevity) */
    }

    /* clear IRQ and return to RX */
    uint8_t clr[] = { 0xFF, 0xFF };
    sx_write(SX1262_CLR_IRQ, clr, 2);
    lora_listen();
}

void lora_set_hop_limit(uint8_t h) { s_hop_limit = h; }
int8_t lora_last_snr(void) { return s_last_snr; }
int16_t lora_last_rssi(void) { return s_last_rssi; }

/* ----------------------------- Internals ---------------------------------- */

static void sx_write(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    sx_wait_busy();
    gpio_set(PIN_LORA_NSS, 0);
    uint8_t tx[16];
    tx[0] = cmd;
    if (len && data) memcpy(tx + 1, data, len > 15 ? 15 : len);
    spi1_xfer(tx, NULL, 1 + (len > 15 ? 15 : len));
    gpio_set(PIN_LORA_NSS, 1);
}

static void sx_read(uint8_t cmd, uint8_t *out, uint8_t len)
{
    sx_wait_busy();
    gpio_set(PIN_LORA_NSS, 0);
    uint8_t tx[16] = { cmd, 0x00 };
    uint8_t rx[16];
    spi1_xfer(tx, rx, 2 + len);
    gpio_set(PIN_LORA_NSS, 1);
    if (out && len) memcpy(out, rx + 2, len);
}

static void sx_write_buffer(uint8_t offset, const uint8_t *data, uint8_t len)
{
    sx_wait_busy();
    gpio_set(PIN_LORA_NSS, 0);
    uint8_t hdr[2] = { SX1262_WRITE_BUFFER, offset };
    spi1_xfer(hdr, NULL, 1);
    spi1_xfer(&hdr[1], NULL, 1);     /* offset byte */
    spi1_xfer(data, NULL, len);
    gpio_set(PIN_LORA_NSS, 1);
}

static void sx_read_buffer(uint8_t offset, uint8_t *out, uint8_t len)
{
    sx_wait_busy();
    gpio_set(PIN_LORA_NSS, 0);
    uint8_t hdr[3] = { SX1262_READ_BUFFER, offset, 0x00 };
    uint8_t rxbuf[64];
    spi1_xfer(hdr, NULL, 3);
    spi1_xfer(NULL, rxbuf, len);
    gpio_set(PIN_LORA_NSS, 1);
    memcpy(out, rxbuf, len);
}

static void sx_wait_busy(void)
{
    uint32_t timeout = 10000;
    while ((GPIO_IDR(gpio_base(PIN_LORA_BUSY.port)) & BIT(PIN_LORA_BUSY.pin)) && timeout--)
        ;
}

static void sx_set_rf_freq(uint32_t hz)
{
    uint64_t f = ((uint64_t)hz << 25) / 32000000ULL;
    uint8_t buf[4] = { (f >> 24) & 0xFF, (f >> 16) & 0xFF, (f >> 8) & 0xFF, f & 0xFF };
    sx_write(SX1262_SET_RF_FREQ, buf, 4);
}

static void sx_set_modparams(void)
{
    uint8_t buf[6] = { LORA_SF, LORA_BW, LORA_CR, 0x00, 0x00, 0x00 };
    sx_write(SX1262_SET_MOD_PARAMS, buf, 6);
}

static void sx_set_pktparams(uint8_t pkt_type, uint16_t preamble,
                              uint8_t payload_len, uint8_t crc_on)
{
    (void)pkt_type;
    uint8_t buf[9] = { (preamble >> 8) & 0xFF, preamble & 0xFF,
                       payload_len, crc_on, 0x00, 0x00, 0x00, 0x00, 0x00 };
    sx_write(SX1262_SET_PACKET_PARAMS, buf, 9);
}

static void sx_set_tx_param(int8_t dbm)
{
    uint8_t buf[2] = { (uint8_t)dbm, 0x04 /* ramp 200 µs */ };
    sx_write(SX1262_SET_TX_PARAM, buf, 2);
}

static void spi1_xfer(const uint8_t *tx, uint8_t *rx, uint16_t n)
{
    (void)tx; (void)rx; (void)n;
    /* real firmware: configure SPI1 in full-duplex master, 8-bit, DMA the
     * tx and rx buffers; block on DMA-complete. */
}

/* ---- CRC16-CCITT ---- */
uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else              crc <<= 1;
        }
    }
    return crc;
}