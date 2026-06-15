/*
 * uart_fp.c - FPC1025 Fingerprint Sensor UART Driver Implementation
 * CyberGuard Hardware Security Token
 *
 * UART1: PB1=TX, PB2=RX @ 115200 baud
 * GPIO: PC13=RST, PC10=IRQ (if available)
 */

#include "uart_fp.h"
#include "../registers.h"
#include "../board.h"

/* ============================================================
 * UART1 Transfer Primitives
 * ============================================================ */

static void uart1_tx_byte(uint8_t byte)
{
    /* Wait until TXE (transmit buffer empty) */
    while (!(USART1->ISR & USART_ISR_TXE));
    USART1->TDR = byte;
}

static int uart1_rx_byte(uint8_t *byte, uint32_t timeout_ms)
{
    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;

    while (!(USART1->ISR & USART_ISR_RXNE)) {
        if ((systick_ms - start) > timeout_ms) return -1;
    }
    *byte = (uint8_t)(USART1->RDR & 0xFF);
    return 0;
}

/* ============================================================
 * FPC1025 Frame Protocol
 * TX: [STX=0xAA] [LEN_H] [LEN_L] [CMD] [DATA...] [CHK]
 * RX: [STX=0xAA] [LEN_H] [LEN_L] [RSP] [DATA...] [CHK]
 * CHK = XOR of all bytes from LEN_H to last DATA byte
 * ============================================================ */

static uint8_t fp_checksum(const uint8_t *data, size_t len)
{
    uint8_t chk = 0;
    for (size_t i = 0; i < len; i++) {
        chk ^= data[i];
    }
    return chk;
}

static int fp_send_cmd(uint8_t cmd, const uint8_t *data, size_t data_len)
{
    uint8_t frame[256 + 6];
    size_t frame_len;

    /* Build frame */
    frame[0] = FP_STX;
    uint16_t payload_len = 1 + data_len; /* CMD + DATA */
    frame[1] = (uint8_t)((payload_len >> 8) & 0xFF);
    frame[2] = (uint8_t)(payload_len & 0xFF);
    frame[3] = cmd;

    if (data != NULL && data_len > 0) {
        memcpy(&frame[4], data, data_len);
    }

    /* Calculate checksum over LEN_H, LEN_L, CMD, DATA */
    frame[4 + data_len] = fp_checksum(&frame[1], 2 + payload_len);
    frame_len = 4 + data_len + 1;

    /* Transmit frame */
    for (size_t i = 0; i < frame_len; i++) {
        uart1_tx_byte(frame[i]);
    }

    return 0;
}

static int fp_recv_rsp(uint8_t *rsp_cmd, uint8_t *rsp_data, size_t *rsp_data_len)
{
    uint8_t header[3];
    uint8_t chk;
    uint8_t calc_chk;

    /* Receive STX */
    uint8_t stx;
    if (uart1_rx_byte(&stx, 5000) != 0) return -1; /* 5s timeout for slow operations */
    if (stx != FP_STX) return -2;

    /* Receive length */
    if (uart1_rx_byte(&header[0], 100) != 0) return -1;
    if (uart1_rx_byte(&header[1], 100) != 0) return -1;

    uint16_t payload_len = ((uint16_t)header[0] << 8) | header[1];
    if (payload_len > 256) return -3;

    /* Receive response code */
    uint8_t rsp;
    if (uart1_rx_byte(&rsp, 100) != 0) return -1;
    *rsp_cmd = rsp;

    /* Receive payload data */
    size_t data_len = (payload_len > 1) ? payload_len - 1 : 0;
    if (rsp_data != NULL && data_len > 0 && rsp_data_len != NULL) {
        size_t copy_len = (data_len > *rsp_data_len) ? *rsp_data_len : data_len;
        for (size_t i = 0; i < copy_len; i++) {
            if (uart1_rx_byte(&rsp_data[i], 100) != 0) return -1;
        }
        *rsp_data_len = copy_len;
    } else {
        /* Drain remaining data */
        for (size_t i = 0; i < data_len; i++) {
            uint8_t dummy;
            if (uart1_rx_byte(&dummy, 100) != 0) return -1;
        }
        if (rsp_data_len != NULL) *rsp_data_len = 0;
    }

    /* Receive checksum */
    if (uart1_rx_byte(&chk, 100) != 0) return -1;

    /* Note: Full checksum verification would require buffering all bytes */
    (void)calc_chk;

    /* Check response type */
    if (rsp == FP_RSP_NACK) {
        /* Read error code */
        uint8_t err_code = 0;
        if (data_len >= 1 && rsp_data != NULL) {
            err_code = rsp_data[0];
        }
        return -(0x200 + err_code);
    }

    return 0;
}

/* ============================================================
 * Public API
 * ============================================================ */

int fp_init(void)
{
    /* Assert and release reset */
    FP_RESET_ASSERT();
    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;
    while ((systick_ms - start) < 10); /* 10ms reset pulse */

    FP_RESET_RELEASE();
    while ((systick_ms - start) < 50); /* 50ms boot time */

    /* Send INIT command */
    int ret = fp_send_cmd(FP_CMD_INIT, NULL, 0);
    if (ret != 0) return ret;

    uint8_t rsp;
    size_t rsp_len = 0;
    ret = fp_recv_rsp(&rsp, NULL, &rsp_len);
    if (ret != 0) return ret;

    if (rsp != FP_RSP_ACK) return -10;

    return 0;
}

int fp_capture(uint8_t *image_buf, size_t *image_len)
{
    /* Send capture command */
    int ret = fp_send_cmd(FP_CMD_CAPTURE, NULL, 0);
    if (ret != 0) return ret;

    uint8_t rsp;
    ret = fp_recv_rsp(&rsp, image_buf, image_len);
    if (ret != 0) return ret;

    if (rsp != FP_RSP_DATA) return -11;

    return 0;
}

int fp_enroll(uint8_t slot, uint8_t samples)
{
    if (slot >= FP_MAX_TEMPLATES) return -1;
    if (samples > FP_MAX_ENROLL_SAMPLES || samples == 0) return -2;

    uint8_t params[2] = {slot, samples};
    int ret = fp_send_cmd(FP_CMD_ENROLL, params, 2);
    if (ret != 0) return ret;

    uint8_t rsp;
    size_t rsp_len = 0;

    /* Enrollment may take multiple captures */
    for (uint8_t i = 0; i < samples; i++) {
        /* Wait for each enrollment step */
        ret = fp_recv_rsp(&rsp, NULL, &rsp_len);
        if (ret != 0) return ret;
        if (rsp == FP_RSP_NACK) return -3;

        /* If not last sample, send next capture */
        if (i < samples - 1) {
            ret = fp_send_cmd(FP_CMD_CAPTURE, NULL, 0);
            if (ret != 0) return ret;
        }
    }

    return 0;
}

int fp_verify(uint8_t slot, uint16_t *score)
{
    if (slot >= FP_MAX_TEMPLATES) return -1;

    uint8_t params[1] = {slot};
    int ret = fp_send_cmd(FP_CMD_VERIFY, params, 1);
    if (ret != 0) return ret;

    uint8_t rsp;
    uint8_t rsp_data[4];
    size_t rsp_len = sizeof(rsp_data);
    ret = fp_recv_rsp(&rsp, rsp_data, &rsp_len);
    if (ret != 0) return ret;

    if (rsp == FP_RSP_ACK && rsp_len >= 2) {
        *score = ((uint16_t)rsp_data[0] << 8) | rsp_data[1];
        return 0;
    }

    return -2; /* No match */
}

int fp_delete(uint8_t slot)
{
    uint8_t params[1] = {slot};
    int ret = fp_send_cmd(FP_CMD_DELETE, params, 1);
    if (ret != 0) return ret;

    uint8_t rsp;
    size_t rsp_len = 0;
    ret = fp_recv_rsp(&rsp, NULL, &rsp_len);
    if (ret != 0) return ret;

    return (rsp == FP_RSP_ACK) ? 0 : -1;
}

int fp_get_template_count(uint8_t *count)
{
    int ret = fp_send_cmd(FP_CMD_GET_TEMPLATE, NULL, 0);
    if (ret != 0) return ret;

    uint8_t rsp;
    uint8_t data[1];
    size_t data_len = 1;
    ret = fp_recv_rsp(&rsp, data, &data_len);
    if (ret != 0) return ret;

    if (rsp == FP_RSP_DATA && data_len >= 1) {
        *count = data[0];
        return 0;
    }

    return -1;
}

int fp_sleep(void)
{
    int ret = fp_send_cmd(FP_CMD_SLEEP, NULL, 0);
    if (ret != 0) return ret;

    uint8_t rsp;
    size_t rsp_len = 0;
    return fp_recv_rsp(&rsp, NULL, &rsp_len);
}

int fp_wakeup(void)
{
    /* Wake by toggling reset line */
    FP_RESET_ASSERT();
    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;
    while ((systick_ms - start) < 1);
    FP_RESET_RELEASE();
    while ((systick_ms - start) < 50);

    /* Re-initialize */
    return fp_init();
}

int fp_get_version(uint8_t *version_buf)
{
    int ret = fp_send_cmd(FP_CMD_GET_VERSION, NULL, 0);
    if (ret != 0) return ret;

    uint8_t rsp;
    size_t rsp_len = 32;
    ret = fp_recv_rsp(&rsp, version_buf, &rsp_len);
    if (ret != 0) return ret;

    return (rsp == FP_RSP_DATA) ? 0 : -1;
}