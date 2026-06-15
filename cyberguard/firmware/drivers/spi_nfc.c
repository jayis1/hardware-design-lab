/*
 * spi_nfc.c - PN7150 NFC Controller Driver Implementation
 * CyberGuard Hardware Security Token
 *
 * SPI1 bus: PA4=NSS, PA5=SCK, PA6=MISO, PA7=MOSI
 * GPIO: PC2=IRQ, PC3=RST
 */

#include "spi_nfc.h"
#include "../registers.h"
#include "../board.h"

/* ============================================================
 * SPI1 Transfer Primitives
 * ============================================================ */

static void spi1_cs_assert(void)
{
    GPIOA->ODR &= ~(1U << PIN_NFC_NSS);
}

static void spi1_cs_release(void)
{
    GPIOA->ODR |= (1U << PIN_NFC_NSS);
}

static uint8_t spi1_transfer(uint8_t tx_byte)
{
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = tx_byte;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)(SPI1->DR & 0xFF);
}

static int spi1_xfer(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t tx = (tx_buf != NULL) ? tx_buf[i] : 0xFF;
        uint8_t rx = spi1_transfer(tx);
        if (rx_buf != NULL) {
            rx_buf[i] = rx;
        }
    }
    while (SPI1->SR & SPI_SR_BSY);
    return 0;
}

/* ============================================================
 * PN7150 SPI Protocol
 * 
 * Frame format:
 *   TX: [PREAMBLE] [STARTCODE] [LEN] [DATA...] [CRC]
 *   RX: [PREAMBLE] [STARTCODE] [LEN] [DATA...] [CRC]
 * ============================================================ */

static uint8_t pn7150_crc(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}

/**
 * Send SPI frame to PN7150
 * Format: [0x00 PREAMBLE] [0xFF START] [LEN] [DATA] [CRC]
 */
static int pn7150_send_frame(const uint8_t *data, size_t len)
{
    if (len > PN7150_MAX_FRAME_LEN) return -1;

    uint8_t header[3] = {
        PN7150_SPI_PREAMBLE,
        PN7150_SPI_STARTCODE,
        (uint8_t)len
    };

    spi1_cs_assert();

    /* Wait for PN7150 ready (MISO high) */
    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;
    while (!(GPIOA->IDR & (1U << PIN_NFC_MISO))) {
        if ((systick_ms - start) > 10) {
            spi1_cs_release();
            return -2;
        }
    }

    /* Send header */
    spi1_xfer(header, NULL, 3);

    /* Send data */
    spi1_xfer(data, NULL, len);

    /* Send CRC */
    uint8_t crc = pn7150_crc(data, len);
    spi1_xfer(&crc, NULL, 1);

    spi1_cs_release();

    return 0;
}

/**
 * Receive SPI frame from PN7150
 */
static int pn7150_recv_frame(uint8_t *buf, size_t *len)
{
    uint8_t header[3];
    uint8_t crc;

    spi1_cs_assert();

    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;

    /* Read preamble */
    spi1_xfer((uint8_t[]){0xFF}, NULL, 1);
    /* Read start code */
    uint8_t start_code;
    spi1_xfer(NULL, &start_code, 1);
    if (start_code != PN7150_SPI_STARTCODE) {
        spi1_cs_release();
        return -1;
    }

    /* Read length */
    spi1_xfer(NULL, &header[0], 1);
    size_t frame_len = header[0];

    /* Read data */
    spi1_xfer(NULL, buf, frame_len);

    /* Read CRC */
    spi1_xfer(NULL, &crc, 1);

    spi1_cs_release();

    /* Verify CRC */
    if (pn7150_crc(buf, frame_len) != crc) {
        return -2;
    }

    *len = frame_len;
    return 0;
}

/* ============================================================
 * NCI Protocol Layer
 * ============================================================ */

/**
 * Send NCI command and receive response
 * NCI frame: [MT(4bits)+GID(4bits)] [OID] [PAYLOAD_LEN] [PAYLOAD...]
 */
int nfc_nci_cmd(uint8_t group, uint8_t opcode,
                const uint8_t *payload, size_t payload_len,
                uint8_t *rsp_buf, size_t *rsp_len)
{
    uint8_t cmd_buf[3 + PN7150_NCI_MAX_PAYLOAD];
    uint8_t rsp_frame[PN7150_MAX_FRAME_LEN];
    size_t rsp_frame_len;

    /* Build NCI command header */
    cmd_buf[0] = (0x01 << 4) | (group & 0x0F);  /* MT=1 (command) + GID */
    cmd_buf[1] = opcode & 0x3F;                  /* OID */
    cmd_buf[2] = (uint8_t)payload_len;            /* Payload length */

    /* Copy payload */
    if (payload != NULL && payload_len > 0) {
        memcpy(&cmd_buf[3], payload, payload_len);
    }

    /* Send command */
    int ret = pn7150_send_frame(cmd_buf, 3 + payload_len);
    if (ret != 0) return ret;

    /* Wait for response */
    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;
    while (!(GPIOC->IDR & (1U << PIN_NFC_IRQ))) {
        if ((systick_ms - start) > 100) return -3; /* Timeout */
    }

    /* Read response */
    ret = pn7150_recv_frame(rsp_frame, &rsp_frame_len);
    if (ret != 0) return ret;

    /* Parse NCI response */
    if (rsp_frame_len < 3) return -4; /* Too short */
    if ((rsp_frame[0] & 0xF0) != 0x60) return -5; /* Not a response */

    /* Copy response payload */
    size_t data_len = rsp_frame[2];
    if (data_len > *rsp_len) data_len = *rsp_len;
    memcpy(rsp_buf, &rsp_frame[3], data_len);
    *rsp_len = data_len;

    /* Check NCI status */
    uint8_t status = rsp_frame[0] & 0x0F;
    return (status == 0x00) ? 0 : -(0x100 + status);
}

/* ============================================================
 * Public API
 * ============================================================ */

int nfc_init(void)
{
    extern volatile uint32_t systick_ms;

    /* Assert reset */
    NFC_RESET_ASSERT();
    uint32_t start = systick_ms;
    while ((systick_ms - start) < 1);  /* 1ms reset pulse */

    /* Release reset */
    NFC_RESET_RELEASE();
    while ((systick_ms - start) < 10);  /* 10ms boot time */

    /* NCI Core Reset */
    uint8_t reset_payload[] = {0x01}; /* Keep config */
    uint8_t rsp_buf[32];
    size_t rsp_len = sizeof(rsp_buf);
    int ret = nfc_nci_cmd(0x02, 0x00, reset_payload, 1, rsp_buf, &rsp_len);
    if (ret != 0) return ret;

    /* NCI Core Init */
    uint8_t init_payload[] = {0x00}; /* Normal mode */
    rsp_len = sizeof(rsp_buf);
    ret = nfc_nci_cmd(0x02, 0x01, init_payload, 1, rsp_buf, &rsp_len);
    if (ret != 0) return ret;

    return 0;
}

int nfc_start_discovery(void)
{
    /* Configure discovery for Type A only (FIDO2 uses ISO 14443-A) */
    uint8_t discover_payload[] = {
        0x01,       /* Number of discoveries */
        0x01,       /* Mode: Poll */
        0x01,       /* Protocol: Type A */
        0x01,       /* Technology: 106kbps A */
        0x00,       /* No specific host */
    };

    uint8_t rsp_buf[32];
    size_t rsp_len = sizeof(rsp_buf);
    return nfc_nci_cmd(0x21, 0x03, discover_payload, 5, rsp_buf, &rsp_len);
}

int nfc_stop_discovery(void)
{
    uint8_t deactivate_payload[] = {0x01}; /* Idle mode */
    uint8_t rsp_buf[32];
    size_t rsp_len = sizeof(rsp_buf);
    return nfc_nci_cmd(0x21, 0x06, deactivate_payload, 1, rsp_buf, &rsp_len);
}

int nfc_send_apdu(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2,
                  const uint8_t *data, size_t data_len,
                  uint8_t *rsp, size_t *rsp_len)
{
    /* Build extended APDU frame */
    uint8_t apdu[PN7150_NCI_MAX_PAYLOAD];
    size_t apdu_len = 0;

    apdu[apdu_len++] = cla;
    apdu[apdu_len++] = ins;
    apdu[apdu_len++] = p1;
    apdu[apdu_len++] = p2;
    apdu[apdu_len++] = (uint8_t)((data_len >> 8) & 0xFF); /* Lc high */
    apdu[apdu_len++] = (uint8_t)(data_len & 0xFF);         /* Lc low */

    if (data != NULL && data_len > 0) {
        memcpy(&apdu[apdu_len], data, data_len);
        apdu_len += data_len;
    }

    apdu[apdu_len++] = 0x00; /* Le high */
    apdu[apdu_len++] = 0x00; /* Le low (max response) */

    /* Send via NCI data message */
    return nfc_nci_cmd(0x00, 0x00, apdu, apdu_len, rsp, rsp_len);
}

int nfc_field_active(void)
{
    /* Read NFC IRQ pin (active high) */
    return (GPIOC->IDR & (1U << PIN_NFC_IRQ)) ? 1 : 0;
}

int nfc_irq_handler(void)
{
    /* Clear EXTI pending bit for PC2 */
    EXTI->RPR1 = (1U << 2);
    EXTI->FPR1 = (1U << 2);

    /* Set global flag */
    extern volatile uint8_t nfc_field_active;
    nfc_field_active = 1;

    return 0;
}

int nfc_deinit(void)
{
    /* Stop discovery */
    nfc_stop_discovery();

    /* Assert reset */
    NFC_RESET_ASSERT();

    return 0;
}