/*
 * lora_drv.c — SX1262 sub-GHz LoRa transceiver driver.
 *
 * Implements just enough of the SX1262 SPI command set to do reliable
 * LoRa uplinks and short downlink windows for the Lichenwatch use case:
 * a 15–60 minute cadence with tiny (19-byte) payloads over EU868.
 *
 * The SX1262 is controlled over SPI2 with a BUSY pin, DIO1 IRQ, and hardware
 * RESET. This driver is blocking (no IRQ-driven state machine) because the
 * firmware is duty-cycled and each radio event is a discrete, short action.
 *
 * Author: jayis1
 * License: MIT
 */

#include "lora_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ------------------------------------------------------------------------ */
/* SX1262 SPI commands                                                       */
/* ------------------------------------------------------------------------ */
#define SX1262_CMD_SET_STANDBY       0x80
#define SX1262_CMD_SET_PACKET_TYPE    0x8A
#define SX1262_CMD_SET_RF_FREQ        0x86
#define SX1262_CMD_SET_TX_PARAMS     0x8E
#define SX1262_CMD_SET_MOD_PARAMS    0x8B
#define SX1262_CMD_SET_PACKET_PARAMS 0x8C
#define SX1262_CMD_SET_TX             0x83
#define SX1262_CMD_SET_RX             0x82
#define SX1262_CMD_WRITE_BUFFER      0x0D
#define SX1262_CMD_READ_BUFFER       0x1D
#define SX1262_CMD_GET_STATUS         0xC0
#define SX1262_CMD_CLEAR_IRQ          0x02
#define SX1262_CMD_GET_IRQ_STATUS     0x12

#define SX1262_PACKET_TYPE_LORA       0x01
#define SX1262_STDBY_RC                0x00
#define SX1262_STDBY_XOSC              0x01

#define SX1262_IRQ_TX_DONE            0x0001
#define SX1262_IRQ_RX_DONE            0x0002
#define SX1262_IRQ_TIMEOUT            0x0040
#define SX1262_IRQ_CRC_ERR             0x0020

#define SX1262_RAM_BASE               0x0000

static int s_initialized = 0;

/* ------------------------------------------------------------------------ */
/* SPI2 primitives                                                            */
/* ------------------------------------------------------------------------ */
static void spi2_cs_low(void)
{
    volatile uint32_t *pb = (volatile uint32_t *)GPIOB;
    pb[6] = (1U << 2);  /* BRR — NSS low */
}

static void spi2_cs_high(void)
{
    volatile uint32_t *pb = (volatile uint32_t *)GPIOB;
    pb[5] = (1U << 2);  /* BSRR — NSS high */
}

static uint8_t spi2_xfer(uint8_t tx)
{
    while (!(SPI2->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&SPI2->DR = tx;
    while (!(SPI2->SR & SPI_SR_RXNE)) { }
    return (uint8_t)SPI2->DR;
}

static void sx1262_write(const uint8_t *cmd, int clen,
                          const uint8_t *data, int dlen)
{
    /* Wait for BUSY low. */
    volatile uint32_t *pc = (volatile uint32_t *)GPIOC;
    while (pc[2] & (1U << 4)) { }  /* PC4 BUSY */

    spi2_cs_low();
    for (int i = 0; i < clen; i++) spi2_xfer(cmd[i]);
    for (int i = 0; i < dlen; i++) spi2_xfer(data[i]);
    spi2_cs_high();
}

static void sx1262_read(const uint8_t *cmd, int clen,
                         uint8_t *out, int olen)
{
    volatile uint32_t *pc = (volatile uint32_t *)GPIOC;
    while (pc[2] & (1U << 4)) { }

    spi2_cs_low();
    for (int i = 0; i < clen; i++) spi2_xfer(cmd[i]);
    /* Read olen bytes (NOP = 0x00 to clock out status + data). */
    for (int i = 0; i < olen; i++) out[i] = spi2_xfer(0x00);
    spi2_cs_high();
}

static void sx1262_wait_busy(void)
{
    volatile uint32_t *pc = (volatile uint32_t *)GPIOC;
    while (pc[2] & (1U << 4)) { }
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                 */
/* ------------------------------------------------------------------------ */
int lora_init(uint32_t freq_hz, uint8_t sf, uint32_t bw_hz, int8_t tx_power_dbm)
{
    /* Configure SPI2 pins: PB2 (NSS), PB10 (SCK), PC2 (MISO), PC3 (MOSI) as AF? */
    /* On STM32L4, SPI2_SCK is PB10 (AF5), MISO is PC2 (AF5), MOSI is PC3 (AF5), NSS PB2 (AF5). */
    volatile uint32_t *pb = (volatile uint32_t *)GPIOB;
    volatile uint32_t *pc = (volatile uint32_t *)GPIOC;
    /* PB2 AF5 (AFRL nibble 2) */
    pb[7] = (pb[7] & ~(0xFU << (2*4))) | (0x5U << (2*4));
    pb[0] = (pb[0] & ~(0x3U << (2*2))) | (GPIO_MODE_AF << (2*2));
    /* PB10 AF5 (AFRH nibble 2) */
    pb[8] = (pb[8] & ~(0xFU << (2*4))) | (0x5U << (2*4));
    pb[0] = (pb[0] & ~(0x3U << (10*2))) | (GPIO_MODE_AF << (10*2));
    /* PC2 AF5, PC3 AF5 */
    pc[7] = (pc[7] & ~(0xFU << (2*4))) | (0x5U << (2*4));
    pc[7] = (pc[7] & ~(0xFU << (3*4))) | (0x5U << (3*4));
    pc[0] = (pc[0] & ~(0x3U << (2*2))) | (GPIO_MODE_AF << (2*2));
    pc[0] = (pc[0] & ~(0x3U << (3*2))) | (GPIO_MODE_AF << (3*2));

    /* Configure BUSY (PC4 input), DIO1 (PC5 input), NRST (PC1 output). */
    pc[0] = (pc[0] & ~(0x3U << (4*2))) | (GPIO_MODE_INPUT << (4*2));   /* PC4 */
    pc[0] = (pc[0] & ~(0x3U << (5*2))) | (GPIO_MODE_INPUT << (5*2));   /* PC5 */
    pc[0] = (pc[0] & ~(0x3U << (1*2))) | (GPIO_MODE_OUTPUT << (1*2));  /* PC1 */
    /* Reset the radio. */
    pc[6] = (1U << 1);  /* BRR reset low */
    for (volatile int d = 0; d < 10000; d++) { }
    pc[5] = (1U << 1);  /* BSRR high */
    for (volatile int d = 0; d < 10000; d++) { }

    /* Configure SPI2. */
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM
              | SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI2->CR1 |= SPI_CR1_SPE;

    /* Set standby. */
    uint8_t standby = SX1262_STDBY_RC;
    sx1262_write((uint8_t[]){SX1262_CMD_SET_STANDBY}, 1, &standby, 1);
    sx1262_wait_busy();

    /* Set packet type = LoRa. */
    uint8_t ptype = SX1262_PACKET_TYPE_LORA;
    sx1262_write((uint8_t[]){SX1262_CMD_SET_PACKET_TYPE}, 1, &ptype, 1);
    sx1262_wait_busy();

    /* Set RF frequency (Hz → 32-bit register: freq = freq_hz * 2^25 / 32 MHz). */
    uint64_t rf = ((uint64_t)freq_hz << 25) / 32000000ULL;
    uint8_t freq_buf[4] = {
        (uint8_t)(rf >> 24), (uint8_t)(rf >> 16),
        (uint8_t)(rf >> 8),  (uint8_t)(rf)
    };
    sx1262_write((uint8_t[]){SX1262_CMD_SET_RF_FREQ}, 1, freq_buf, 4);
    sx1262_wait_busy();

    /* Set TX params: power (signed) + ramp (0x04 = 200 µs). */
    uint8_t tx_params[2] = { (uint8_t)tx_power_dbm, 0x04 };
    sx1262_write((uint8_t[]){SX1262_CMD_SET_TX_PARAMS}, 1, tx_params, 2);
    sx1262_wait_busy();

    /* Set modulation params: SF, BW code, CR (4/5=1), LDRO. */
    uint8_t bw_code = (bw_hz == 125000) ? 0x04 : (bw_hz == 250000 ? 0x05 : 0x06);
    uint8_t ldro = (sf >= 11) ? 0x01 : 0x00;
    uint8_t mod_params[4] = { sf, bw_code, 0x01, ldro };
    sx1262_write((uint8_t[]){SX1262_CMD_SET_MOD_PARAMS}, 1, mod_params, 4);
    sx1262_wait_busy();

    /* Set packet params: preamble 8, explicit header, payload length (set per TX),
     * CRC on, IQ standard. */
    uint8_t pkt_params[9] = {
        0x00, 0x08,   /* preamble length = 8 */
        0x00,         /* explicit header */
        0x13,         /* payload length = 19 (overwritten on TX) */
        0x01,         /* CRC on */
        0x00,         /* IQ standard */
        0x00, 0x00
    };
    sx1262_write((uint8_t[]){SX1262_CMD_SET_PACKET_PARAMS}, 1, pkt_params, 9);
    sx1262_wait_busy();

    s_initialized = 1;
    return 0;
}

int lora_send(const uint8_t *payload, uint8_t len)
{
    if (!s_initialized || len > 64) return -1;

    /* Clear IRQ flags. */
    uint8_t irq_clear[3] = { 0xFF, 0xFF, 0xFF };
    sx1262_write((uint8_t[]){SX1262_CMD_CLEAR_IRQ}, 1, irq_clear, 3);
    sx1262_wait_busy();

    /* Write payload to SX1262 buffer at offset 0. */
    uint8_t hdr = SX1262_RAM_BASE >> 8;
    uint8_t off = SX1262_RAM_BASE & 0xFF;
    uint8_t wbuf[2 + 64] = { hdr, off };
    memcpy(&wbuf[2], payload, len);
    sx1262_write((uint8_t[]){SX1262_CMD_WRITE_BUFFER}, 1, wbuf, 2 + len);
    sx1262_wait_busy();

    /* Set TX with a timeout of ~5 s (0x00013880 = 80000 ms in 15.625 µs units). */
    uint8_t tx_cmd[3] = { 0x00, 0x13, 0x88 };  /* timeout */
    sx1262_write((uint8_t[]){SX1262_CMD_SET_TX}, 1, tx_cmd, 3);

    /* Poll IRQ status for TX_DONE. */
    for (uint32_t i = 0; i < 500000; i++) {
        uint8_t irq[3] = {0,0,0};
        sx1262_read((uint8_t[]){SX1262_CMD_GET_IRQ_STATUS}, 1, irq, 3);
        uint16_t flags = (uint16_t)((irq[1] << 8) | irq[2]);
        if (flags & SX1262_IRQ_TX_DONE) {
            sx1262_write((uint8_t[]){SX1262_CMD_CLEAR_IRQ}, 1, irq_clear, 3);
            return (int)len;
        }
        if (flags & SX1262_IRQ_TIMEOUT) {
            sx1262_write((uint8_t[]){SX1262_CMD_CLEAR_IRQ}, 1, irq_clear, 3);
            return -2;
        }
        for (volatile int d = 0; d < 100; d++) { }
    }
    return -3;
}

int lora_receive(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms)
{
    if (!s_initialized) return -1;

    /* Clear IRQ flags. */
    uint8_t irq_clear[3] = { 0xFF, 0xFF, 0xFF };
    sx1262_write((uint8_t[]){SX1262_CMD_CLEAR_IRQ}, 1, irq_clear, 3);
    sx1262_wait_busy();

    /* Set RX with timeout in 15.625 µs units. */
    uint32_t rx_units = timeout_ms * 64;  /* ms → 15.625 µs */
    uint8_t rx_cmd[3] = {
        (uint8_t)(rx_units >> 16),
        (uint8_t)(rx_units >> 8),
        (uint8_t)(rx_units)
    };
    sx1262_write((uint8_t[]){SX1262_CMD_SET_RX}, 1, rx_cmd, 3);

    /* Poll for RX_DONE or timeout. */
    for (uint32_t i = 0; i < timeout_ms * 100; i++) {
        uint8_t irq[3] = {0,0,0};
        sx1262_read((uint8_t[]){SX1262_CMD_GET_IRQ_STATUS}, 1, irq, 3);
        uint16_t flags = (uint16_t)((irq[1] << 8) | irq[2]);
        if (flags & SX1262_IRQ_RX_DONE) {
            /* Read the payload from buffer. */
            uint8_t rd_cmd[2] = { 0x00, 0x00 };  /* offset 0 */
            uint8_t tmp[64];
            sx1262_read((uint8_t[]){SX1262_CMD_READ_BUFFER}, 1, tmp, max_len + 1);
            /* First byte is status, rest is payload. */
            int n = max_len < 63 ? max_len : 63;
            memcpy(buf, &tmp[1], n);
            sx1262_write((uint8_t[]){SX1262_CMD_CLEAR_IRQ}, 1, irq_clear, 3);
            return n;
        }
        if (flags & SX1262_IRQ_TIMEOUT) {
            sx1262_write((uint8_t[]){SX1262_CMD_CLEAR_IRQ}, 1, irq_clear, 3);
            return 0;  /* no downlink */
        }
        for (volatile int d = 0; d < 100; d++) { }
    }
    return 0;
}