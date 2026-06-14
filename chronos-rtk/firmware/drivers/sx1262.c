/*
 * sx1262.c — Semtech SX1262 LoRa transceiver driver implementation
 * SPI command interface for Chronos-RTK board
 */

#include "sx1262.h"
#include "board.h"
#include "registers.h"

/* ========================================================================== */
/* SPI2 access helpers (SX1262 on SPI2)                                        */
/* ========================================================================== */

static SPI_TypeDef * const SPI2 = (SPI_TypeDef *)SPI2_BASE;

/* Wait until SX1262 BUSY line is low (max 1 ms timeout) */
static void sx1262_wait_busy(void)
{
    volatile uint32_t timeout = 10000;
    while ((GPIOC->IDR & (1U << PIN_LORA_BUSY)) && --timeout)
        ;
}

/* Assert NSS (active low) */
static void sx1262_nss_low(void)
{
    GPIOA->ODR &= ~(1U << PIN_SPI2_NSS);
}

/* De-assert NSS */
static void sx1262_nss_high(void)
{
    GPIOA->ODR |= (1U << PIN_SPI2_NSS);
}

/* Transfer one byte over SPI2 */
static uint8_t spi2_transfer(uint8_t tx_byte)
{
    /* Wait for TXE */
    while (!(SPI2->SR & SPI_SR_TXE))
        ;
    /* Send byte (8-bit write to DR) */
    *(volatile uint8_t *)&SPI2->DR = tx_byte;
    /* Wait for RXNE */
    while (!(SPI2->SR & SPI_SR_RXNE))
        ;
    return *(volatile uint8_t *)&SPI2->DR;
}

/* Write command + data to SX1262 */
static int sx1262_write_cmd(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    sx1262_wait_busy();
    sx1262_nss_low();

    spi2_transfer(cmd);
    for (uint8_t i = 0; i < len; i++) {
        spi2_transfer(data[i]);
    }

    sx1262_nss_high();
    sx1262_wait_busy();
    return 0;
}

/* Read command + data from SX1262 */
static int sx1262_read_cmd(uint8_t cmd, uint8_t *data, uint8_t len)
{
    sx1262_wait_busy();
    sx1262_nss_low();

    spi2_transfer(cmd);
    /* Send dummy bytes and read response */
    for (uint8_t i = 0; i < len; i++) {
        data[i] = spi2_transfer(0x00);
    }

    sx1262_nss_high();
    sx1262_wait_busy();
    return 0;
}

/* Write a single register */
static int sx1262_write_reg(uint16_t addr, uint8_t val)
{
    uint8_t buf[3];
    buf[0] = (addr >> 8) & 0xFF;
    buf[1] = addr & 0xFF;
    buf[2] = val;
    return sx1262_write_cmd(SX1262_CMD_WRITE_REG, buf, 3);
}

/* Read a single register */
static uint8_t sx1262_read_reg(uint16_t addr)
{
    uint8_t buf[2];
    uint8_t val;
    buf[0] = (addr >> 8) & 0xFF;
    buf[1] = addr & 0xFF;

    sx1262_wait_busy();
    sx1262_nss_low();

    spi2_transfer(SX1262_CMD_READ_REG);
    spi2_transfer(buf[0]);
    spi2_transfer(buf[1]);
    /* Dummy byte for SX1262 read latency */
    spi2_transfer(0x00);
    val = spi2_transfer(0x00);

    sx1262_nss_high();
    sx1262_wait_busy();

    return val;
}

/* ========================================================================== */
/* RTCM frame handling                                                          */
/* ========================================================================== */

static rtcm_rx_callback_t g_rtcm_callback = NULL;
static volatile uint8_t g_rx_buf[256];
static volatile uint8_t g_rx_len = 0;
static volatile uint16_t g_rx_irq_flags = 0;

/* CRC-24Q computation (for RTCM v3.3 frames) */
static uint32_t crc24q_compute(const uint8_t *data, uint16_t len)
{
    uint32_t crc = 0x000000;

    for (uint16_t i = 0; i < len; i++) {
        crc ^= ((uint32_t)data[i] << 16);
        for (int j = 0; j < 8; j++) {
            crc <<= 1;
            if (crc & 0x1000000) {
                crc ^= 0x1864CFB;
            }
        }
    }
    return crc & 0xFFFFFF;
}

/* ========================================================================== */
/* Public API implementation                                                    */
/* ========================================================================== */

int sx1262_init(void)
{
    /* Hardware reset: assert NRST low for 1 ms, then release */
    GPIOC->ODR &= ~(1U << PIN_LORA_RST);
    for (volatile int i = 0; i < 170000; i++)  /* ~1 ms at 170 MHz */
        ;
    GPIOC->ODR |= (1U << PIN_LORA_RST);
    /* Wait for BUSY to go low (calibration completes) */
    for (volatile int i = 0; i < 1700000; i++)  /* ~10 ms */
        ;

    /* Set standby mode with XOSC */
    uint8_t standby_cmd = 0x01;  /* STDBY_XOSC */
    sx1262_write_cmd(SX1262_CMD_SET_STANDBY, &standby_cmd, 1);

    /* Set packet type to LoRa */
    uint8_t pkt_type = 0x01;  /* LORA */
    sx1262_write_cmd(SX1262_CMD_SET_LORA_PACKET_TYPE, &pkt_type, 1);

    /* Configure TCXO (1.8 V, 10 ms delay) */
    uint8_t tcxo_cmd[4];
    tcxo_cmd[0] = 0x01;  /* TCXO voltage = 1.8 V */
    tcxo_cmd[1] = 0x00;  /* Timeout MSB */
    tcxo_cmd[2] = 0x27;  /* Timeout mid (10 ms) */
    tcxo_cmd[3] = 0x10;  /* Timeout LSB */
    sx1262_write_cmd(0x09, tcxo_cmd, 4);  /* SetDIO3AsTcxoMode */

    /* Set RX gain to boosted mode */
    sx1262_write_reg(SX1262_REG_RX_GAIN, 0x94);

    /* Set DIO1 to map all IRQs */
    uint16_t irq_mask = SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE
                       | SX1262_IRQ_CRC_ERR | SX1262_IRQ_TIMEOUT
                       | SX1262_IRQ_HEADER_VALID | SX1262_IRQ_HEADER_ERR;
    uint8_t irq_buf[8];
    irq_buf[0] = (irq_mask >> 8) & 0xFF;
    irq_buf[1] = irq_mask & 0xFF;
    irq_buf[2] = (irq_mask >> 8) & 0xFF;  /* DIO1 mask same */
    irq_buf[3] = irq_mask & 0xFF;
    /* DIO2 and DIO3 not used */
    irq_buf[4] = 0x00;
    irq_buf[5] = 0x00;
    irq_buf[6] = 0x00;
    irq_buf[7] = 0x00;
    sx1262_write_cmd(SX1262_CMD_SET_DIO_IRQ_PARAMS, irq_buf, 8);

    /* Clear all IRQ flags */
    uint8_t clr_cmd[2] = {0xFF, 0xFF};
    sx1262_write_cmd(SX1262_CMD_CLR_IRQ, clr_cmd, 2);

    return 0;
}

int sx1262_set_frequency(uint32_t freq_hz)
{
    /* SX1262 frequency register: freq = freq_hz * (2^25) / 32e6
     * For 868 MHz: 868000000 * 33554432 / 32000000 = 910163968 (0x363E0000 approx) */
    uint32_t freq_reg = (uint32_t)((uint64_t)freq_hz * 33554432ULL / 32000000ULL);
    uint8_t buf[4];
    buf[0] = (freq_reg >> 24) & 0xFF;
    buf[1] = (freq_reg >> 16) & 0xFF;
    buf[2] = (freq_reg >> 8) & 0xFF;
    buf[3] = freq_reg & 0xFF;
    return sx1262_write_cmd(SX1262_CMD_SET_FREQ, buf, 4);
}

int sx1262_set_modulation_params(lora_spreading_factor_t sf,
                                  lora_bandwidth_t bw,
                                  lora_coding_rate_t cr)
{
    uint8_t buf[4];
    buf[0] = (uint8_t)sf;       /* Spreading factor */
    buf[1] = (uint8_t)bw;       /* Bandwidth */
    buf[2] = (uint8_t)cr;       /* Coding rate */
    buf[3] = 0x00;               /* Low DROptimize = off (for SF7) */
    /* Note: LDRO should be 0x01 for SF11/SF12 */
    if (sf >= SF11) {
        buf[3] = 0x01;
    }
    return sx1262_write_cmd(SX1262_CMD_SET_MOD_PARAMS, buf, 4);
}

int sx1262_set_packet_params(uint16_t preamble_len,
                               lora_header_t header_mode,
                               uint8_t payload_len,
                               lora_crc_t crc)
{
    uint8_t buf[6];
    buf[0] = (preamble_len >> 8) & 0xFF;
    buf[1] = preamble_len & 0xFF;
    buf[2] = (uint8_t)header_mode;
    buf[3] = payload_len;
    buf[4] = (uint8_t)crc;
    buf[5] = 0x00;  /* Standard IQ setup */
    return sx1262_write_cmd(SX1262_CMD_SET_PKT_PARAMS, buf, 6);
}

int sx1262_set_tx_power(int8_t power_dbm)
{
    /* PA config: OCP = 140 mA for +22 dBm */
    uint8_t pa_buf[4];
    pa_buf[0] = 0x04;  /* PA duty cycle for +22 dBm */
    pa_buf[1] = (uint8_t)(power_dbm);  /* Power */
    pa_buf[2] = 0x01;  /* Ramp time: 200 us */
    pa_buf[3] = 0x00;  /* Reserved */

    /* Set PA config register first */
    sx1262_write_reg(SX1262_REG_PA_CONFIG, 0x04);  /* Default PA config */
    sx1262_write_reg(SX1262_REG_PA_DUTY_CYCLE, 0x04);
    sx1262_write_reg(SX1262_REG_OCP_CONFIG, 0x38);  /* OCP = 140 mA */

    return sx1262_write_cmd(SX1262_CMD_SET_TX_PARAMS, pa_buf, 4);
}

int sx1262_set_sync_word(uint8_t sync_word)
{
    /* LoRa sync word is 2 bytes at register 0x0740 */
    uint8_t buf[2];
    buf[0] = sync_word;
    buf[1] = 0x24;  /* Default network ID byte */
    return sx1262_write_reg(SX1262_REG_LORA_SYNC_WORD, buf[0]);
}

int sx1262_set_tx(uint32_t timeout_ms)
{
    uint8_t buf[3];
    /* Timeout in SX1262 units: timeout_ms * 1.024 / 1000 * 2^25 / 1000 */
    /* Simplified: timeout = timeout_ms * 64 (for SX1262 step size) */
    uint32_t timeout = timeout_ms * 64;
    if (timeout_ms == 0) {
        buf[0] = 0x00;  /* No timeout */
        buf[1] = 0x00;
        buf[2] = 0x00;
    } else {
        buf[0] = (timeout >> 16) & 0xFF;
        buf[1] = (timeout >> 8) & 0xFF;
        buf[2] = timeout & 0xFF;
    }
    return sx1262_write_cmd(SX1262_CMD_SET_TX, buf, 3);
}

int sx1262_set_rx(uint32_t timeout_ms)
{
    uint8_t buf[3];
    if (timeout_ms == 0) {
        /* Continuous RX: 0xFFFFFF */
        buf[0] = 0xFF;
        buf[1] = 0xFF;
        buf[2] = 0xFF;
    } else {
        uint32_t timeout = timeout_ms * 64;
        buf[0] = (timeout >> 16) & 0xFF;
        buf[1] = (timeout >> 8) & 0xFF;
        buf[2] = timeout & 0xFF;
    }
    return sx1262_write_cmd(SX1262_CMD_SET_RX, buf, 3);
}

int sx1262_set_standby(uint8_t rc_clock)
{
    return sx1262_write_cmd(SX1262_CMD_SET_STANDBY, &rc_clock, 1);
}

int sx1262_write_buffer(uint8_t offset, const uint8_t *data, uint8_t len)
{
    uint8_t buf[257];  /* Max: 1 offset + 255 data */
    buf[0] = offset;
    for (uint8_t i = 0; i < len; i++) {
        buf[1 + i] = data[i];
    }
    return sx1262_write_cmd(SX1262_CMD_WRITE_BUF, buf, 1 + len);
}

int sx1262_read_buffer(uint8_t offset, uint8_t *data, uint8_t len)
{
    sx1262_wait_busy();
    sx1262_nss_low();

    spi2_transfer(SX1262_CMD_READ_BUF);
    spi2_transfer(offset);
    /* Dummy byte for read latency */
    spi2_transfer(0x00);
    /* Read actual data */
    for (uint8_t i = 0; i < len; i++) {
        data[i] = spi2_transfer(0x00);
    }

    sx1262_nss_high();
    sx1262_wait_busy();
    return 0;
}

int sx1262_clear_irq(uint16_t irq_mask)
{
    uint8_t buf[2];
    buf[0] = (irq_mask >> 8) & 0xFF;
    buf[1] = irq_mask & 0xFF;
    return sx1262_write_cmd(SX1262_CMD_CLR_IRQ, buf, 2);
}

uint8_t sx1262_get_status(void)
{
    uint8_t status;
    sx1262_read_cmd(SX1262_CMD_GET_STATUS, &status, 1);
    return status;
}

int16_t sx1262_get_rssi_inst(void)
{
    uint8_t buf[2];
    sx1262_read_cmd(SX1262_CMD_GET_RSSI_INST, buf, 2);
    int16_t rssi = ((int16_t)buf[0] << 8) | buf[1];
    return rssi;  /* In dBm, negative */
}

void sx1262_handle_irq(void)
{
    /* Read IRQ status from SX1262 */
    /* In a real implementation, we'd read the IRQ registers */
    /* For now, use the IRQ flags captured by EXTI */
    uint8_t irq_buf[2];
    sx1262_read_cmd(0x12, irq_buf, 2);  /* GetIrqStatus */
    uint16_t irq_flags = ((uint16_t)irq_buf[0] << 8) | irq_buf[1];

    if (irq_flags & SX1262_IRQ_RX_DONE) {
        /* Read received payload */
        uint8_t len_buf[1];
        sx1262_read_cmd(0x13, len_buf, 1);  /* GetRxBufferLength */
        g_rx_len = len_buf[0];
        if (g_rx_len > 0 && g_rx_len <= 255) {
            sx1262_read_buffer(0, (uint8_t *)g_rx_buf, g_rx_len);

            /* Process as potential RTCM frame */
            if (g_rtcm_callback && g_rx_buf[0] == 0xD3) {
                rtcm_frame_t frame;
                frame.preamble = g_rx_buf[0];
                frame.length = ((uint16_t)g_rx_buf[1] << 8) | g_rx_buf[2];
                if (frame.length <= RTCM_FRAME_MAX_SIZE) {
                    for (uint8_t i = 0; i < frame.length && i < g_rx_len - 3; i++) {
                        frame.data[i] = g_rx_buf[3 + i];
                    }
                    g_rtcm_callback(&frame);
                }
            }
        }

        /* Restart RX */
        sx1262_set_rx(0);  /* Continuous RX */
    }

    if (irq_flags & SX1262_IRQ_TX_DONE) {
        /* TX complete, return to RX */
        sx1262_set_rx(0);
    }

    if (irq_flags & SX1262_IRQ_CRC_ERR) {
        /* CRC error, restart RX */
        sx1262_set_rx(0);
    }

    if (irq_flags & SX1262_IRQ_TIMEOUT) {
        /* RX timeout, restart RX */
        sx1262_set_rx(0);
    }

    /* Clear all processed IRQ flags */
    sx1262_clear_irq(irq_flags);
}

int sx1262_send_rtcm(const rtcm_frame_t *frame)
{
    /* Build LoRa payload: [preamble_byte][length_hi][length_lo][data...][crc_24] */
    uint8_t tx_buf[260];
    uint16_t idx = 0;

    tx_buf[idx++] = frame->preamble;
    tx_buf[idx++] = (frame->length >> 8) & 0xFF;
    tx_buf[idx++] = frame->length & 0xFF;

    for (uint16_t i = 0; i < frame->length && i < RTCM_FRAME_MAX_SIZE; i++) {
        tx_buf[idx++] = frame->data[i];
    }

    /* Append CRC-24Q */
    uint32_t crc = crc24q_compute(tx_buf, idx);
    tx_buf[idx++] = (crc >> 16) & 0xFF;
    tx_buf[idx++] = (crc >> 8) & 0xFF;
    tx_buf[idx++] = crc & 0xFF;

    /* Write to SX1262 TX buffer */
    int ret = sx1262_write_buffer(0, tx_buf, idx);
    if (ret != 0) return ret;

    /* Update packet params with current payload length */
    sx1262_set_packet_params(8, HEADER_EXPLICIT, idx, CRC_ON);

    /* Start TX with 5-second timeout */
    sx1262_set_tx(5000);

    return 0;
}

void sx1262_set_rtcm_callback(rtcm_rx_callback_t callback)
{
    g_rtcm_callback = callback;
}