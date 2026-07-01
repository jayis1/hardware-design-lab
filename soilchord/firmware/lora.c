/*
 * lora.c — Soilchord SX1262 LoRa radio driver
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Bit-banged SPI to the SX1262 on SPI2 pins (see board.h). Provides a
 * minimal TX/RX interface sufficient for the Soilchord uplink/downlink.
 * A production build links the Semtech SX126x HAL; this reference keeps
 * the bus-level operations explicit so the design intent is visible.
 */
#include "soilchord.h"
#include "proto.h"
#include "registers.h"
#include "board.h"

/* ---- GPIO helpers (same as piezo.c, kept local for independence) ---- */
static volatile uint32_t *port_base(char p)
{
    if (p == 'A') return (volatile uint32_t *)GPIOA_BASE_REG;
    if (p == 'B') return (volatile uint32_t *)GPIOB_BASE_REG;
    return (volatile uint32_t *)GPIOC_BASE_REG;
}
static void gpio_write(char port, int pin, int val)
{
    volatile uint32_t *b = port_base(port);
    if (val) GPIO_BSRR(b) = (1U << pin);
    else     GPIO_BSRR(b) = (1U << (pin + 16));
}
static int gpio_read(char port, int pin)
{
    volatile uint32_t *b = port_base(port);
    return (GPIO_IDR(b) >> pin) & 1;
}

/* ---- SX1262 register and command set (subset) ---- */
#define SX_CMD_WRITEREG         0x0D
#define SX_CMD_READREG          0x1D
#define SX_CMD_WRITEBUFFER      0x0D
#define SX_CMD_READBUFFER        0x1E
#define SX_CMD_SETSTANDBY        0x04
#define SX_CMD_SETTX             0x03
#define SX_CMD_SETRX             0x05
#define SX_CMD_SETCAD            0x07
#define SX_CMD_RESETSTATS       0x01
#define SX_CMD_GETRXBUFFERSTATUS 0x14
#define SX_CMD_GETPACKETSTATUS   0x15

#define SX_REG_PKTPARAMS1       0x907
#define SX_REG_PKTPARAMS2       0x908
#define SX_REG_PAYLOADLEN       0x909
#define SX_REG_IRQFLAGS         0x0866
#define SX_REG_IRQMASK          0x0867

/* IRQ flags we care about */
#define IRQ_TXDONE              (1U << 0)
#define IRQ_RXDONE              (1U << 2)
#define IRQ_CRCERR              (1U << 5)
#define IRQ_TIMEOUT             (1U << 9)

static int8_t s_last_rssi = -127;

int8_t lora_last_rssi_dbm(void) { return s_last_rssi; }

/* ---- Bit-banged SPI ---- */
static void spi_cs(int v) { gpio_write('B', 12, v); }
static void spi_sck(int v){ gpio_write('B', 13, v); }
static void spi_mosi(int v){gpio_write('B', 15, v); }
static int  spi_miso(void){ return gpio_read('B', 14); }

static void spi_wait_busy(void)
{
    uint32_t to = 100000;
    while (gpio_read('A', 15) && to-- > 0) { }   /* BUSY high → wait */
}

static uint8_t spi_xfer(uint8_t tx)
{
    uint8_t rx = 0;
    for (int i = 7; i >= 0; i--) {
        spi_mosi((tx >> i) & 1);
        spi_sck(1);
        if (spi_miso()) rx |= (1U << i);
        spi_sck(0);
    }
    return rx;
}

static void sx_write(uint8_t cmd, const uint8_t *data, uint8_t n)
{
    spi_wait_busy();
    spi_cs(0);
    spi_xfer(cmd);
    for (uint8_t i = 0; i < n; i++) spi_xfer(data[i]);
    spi_cs(1);
}

static void sx_read(uint8_t cmd, uint8_t *out, uint8_t n)
{
    spi_wait_busy();
    spi_cs(0);
    spi_xfer(cmd);
    /* The SX1262 inserts a dummy byte after the command on reads. */
    spi_xfer(0xFF);
    for (uint8_t i = 0; i < n; i++) out[i] = spi_xfer(0xFF);
    spi_cs(1);
}

static void sx_write_reg(uint16_t addr, uint8_t val)
{
    uint8_t buf[3] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF), val };
    sx_write(SX_CMD_WRITEREG, buf, 3);
}

static uint8_t sx_read_reg(uint16_t addr)
{
    uint8_t buf[2] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF) };
    uint8_t out;
    spi_wait_busy();
    spi_cs(0);
    spi_xfer(SX_CMD_READREG);
    spi_xfer(buf[0]); spi_xfer(buf[1]);
    spi_xfer(0xFF);                 /* dummy */
    out = spi_xfer(0xFF);
    spi_cs(1);
    return out;
}

static void sx_reset(void)
{
    gpio_write('B', 8, 0);
    for (volatile int i = 0; i < 100000; i++) __asm__("nop");
    gpio_write('B', 8, 1);
    for (volatile int i = 0; i < 100000; i++) __asm__("nop");
}

/* ---- Public API ---- */
sc_err_t lora_init(void)
{
    /* NSS idle high, SCK low. */
    spi_cs(1); spi_sck(0);

    sx_reset();

    /* Set STANDBY (STDBY_RC). */
    uint8_t standby = 0x00;
    sx_write(SX_CMD_SETSTANDBY, &standby, 1);

    /* Configure packet params: LoRa, header-enabled, 51-byte payload, etc.
     * A production build calls SetPacketType(LoRa), SetModParams(SF9/125kHz),
     * SetPacketParams. Here we just set the payload length register. */
    sx_write_reg(SX_REG_PAYLOADLEN, 51);

    /* Clear IRQ flags. */
    uint8_t irqclr[3] = { 0xFF, 0xFF, 0xFF };
    sx_write(0x02, irqclr, 3);     /* ClearIrqStatus */

    return SC_OK;
}

static sc_err_t sx_set_tx(uint32_t timeout_ms)
{
    uint8_t buf[3] = {
        (uint8_t)(timeout_ms >> 16),
        (uint8_t)(timeout_ms >> 8),
        (uint8_t)(timeout_ms & 0xFF)
    };
    sx_write(SX_CMD_SETTX, buf, 3);
    return SC_OK;
}

static sc_err_t sx_set_rx(uint32_t timeout_ms)
{
    uint8_t buf[3] = {
        (uint8_t)(timeout_ms >> 16),
        (uint8_t)(timeout_ms >> 8),
        (uint8_t)(timeout_ms & 0xFF)
    };
    sx_write(SX_CMD_SETRX, buf, 3);
    return SC_OK;
}

static uint16_t sx_get_irq(void)
{
    uint8_t lo = sx_read_reg(SX_REG_IRQFLAGS);
    uint8_t hi = sx_read_reg(SX_REG_IRQFLAGS + 1);
    return (uint16_t)((hi << 8) | lo);
}

sc_err_t lora_send(const uint8_t *payload, uint8_t len, uint8_t retries)
{
    if (len > 51) return SC_ERR_RANGE;

    for (uint8_t attempt = 0; attempt < retries; attempt++) {
        /* Random jitter to avoid collisions on retry. */
        for (volatile int j = 0; j < (attempt * 5000); j++) __asm__("nop");

        /* Write payload to radio buffer at offset 0. */
        spi_wait_busy();
        spi_cs(0);
        spi_xfer(SX_CMD_WRITEBUFFER);
        spi_xfer(0);                 /* offset 0 */
        for (uint8_t i = 0; i < len; i++) spi_xfer(payload[i]);
        spi_cs(1);

        /* Set payload length. */
        sx_write_reg(SX_REG_PAYLOADLEN, len);

        /* Clear IRQ and start TX. */
        uint8_t clr[3] = { 0xFF, 0xFF, 0xFF };
        sx_write(0x02, clr, 3);
        sx_set_tx(5000);              /* 5 s timeout */

        /* Wait for TxDone or timeout. */
        uint32_t to = 2000000;
        uint16_t irq;
        do {
            irq = sx_get_irq();
        } while ((irq & (IRQ_TXDONE | IRQ_TIMEOUT)) == 0 && to-- > 0);

        if (irq & IRQ_TXDONE) {
            /* Clear flags and return to standby. */
            sx_write(0x02, clr, 3);
            uint8_t sb = 0x00;
            sx_write(SX_CMD_SETSTANDBY, &sb, 1);
            return SC_OK;
        }
        /* Timeout: clear and retry. */
        sx_write(0x02, clr, 3);
    }
    return SC_ERR_TIMEOUT;
}

sc_err_t lora_recv(uint8_t *buf, uint8_t *len, uint32_t timeout_ms)
{
    uint8_t clr[3] = { 0xFF, 0xFF, 0xFF };
    sx_write(0x02, clr, 3);
    sx_set_rx(timeout_ms);

    uint32_t to = timeout_ms * 1000;     /* coarse */
    uint16_t irq;
    do {
        irq = sx_get_irq();
    } while ((irq & (IRQ_RXDONE | IRQ_TIMEOUT | IRQ_CRCERR)) == 0 && to-- > 0);

    if (irq & IRQ_CRCERR) { sx_write(0x02, clr, 3); return SC_ERR_NACK; }
    if (!(irq & IRQ_RXDONE)) { sx_write(0x02, clr, 3); return SC_ERR_TIMEOUT; }

    /* Read RX buffer status: [status, payloadLen, startOffset]. */
    uint8_t status[3] = {0, 0, 0};
    sx_read(SX_CMD_GETRXBUFFERSTATUS, status, 3);
    uint8_t plen = status[1];
    uint8_t off  = status[2];
    if (*len < plen) { sx_write(0x02, clr, 3); return SC_ERR_RANGE; }

    /* Read packet RSSI (for the host's record). */
    uint8_t ps[2] = {0, 0};
    sx_read(SX_CMD_GETPACKETSTATUS, ps, 2);
    s_last_rssi = -(int8_t)ps[0];        /* PSRssiPkt is negative dBm in 2's-comp-ish */

    spi_wait_busy();
    spi_cs(0);
    spi_xfer(SX_CMD_READBUFFER);
    spi_xfer(off);
    spi_xfer(0xFF);                     /* dummy */
    for (uint8_t i = 0; i < plen; i++) buf[i] = spi_xfer(0xFF);
    spi_cs(1);

    *len = plen;
    sx_write(0x02, clr, 3);
    uint8_t sb = 0x00;
    sx_write(SX_CMD_SETSTANDBY, &sb, 1);
    return SC_OK;
}