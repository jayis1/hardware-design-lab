/*
 * i2c_se.c - A71CH Secure Element I2C Driver Implementation
 * CyberGuard Hardware Security Token
 *
 * I2C protocol: I2C1 (PB0=SDL, PC5=SDA) @ 400kHz
 * Device address: 0x48 (7-bit)
 *
 * A71CH frame format:
 *   [LEN_H][LEN_L][CMD][DATA...][CRC8]
 *   Response: [LEN_H][LEN_L][RSP_STATUS][DATA...][CRC8]
 */

#include "i2c_se.h"
#include "../registers.h"
#include "../board.h"

/* ============================================================
 * I2C1 Transfer Primitives
 * ============================================================ */

/* I2C1 busy-wait with timeout */
#define I2C_TIMEOUT_MS    100

static uint32_t i2c1_get_tick(void)
{
    extern volatile uint32_t systick_ms;
    return systick_ms;
}

static int i2c1_wait_flag(uint32_t flag, uint8_t expected)
{
    uint32_t start = i2c1_get_tick();
    while (((I2C1->ISR & flag) ? 1 : 0) != expected) {
        if ((i2c1_get_tick() - start) > I2C_TIMEOUT_MS) {
            return -1; /* Timeout */
        }
    }
    return 0;
}

/**
 * I2C1 write to A71CH
 * @param data Command data to send
 * @param len Length of data
 * @return 0 on success, negative on error
 */
static int i2c1_write(const uint8_t *data, size_t len)
{
    uint32_t start;

    /* Wait until bus is free */
    start = i2c1_get_tick();
    while (I2C1->ISR & I2C_ISR_BUSY) {
        if ((i2c1_get_tick() - start) > I2C_TIMEOUT_MS) return -1;
    }

    /* Configure transfer: write, A71CH address, auto-end */
    I2C1->CR2 = (A71CH_I2C_ADDR_8BIT)        /* Slave address (8-bit) */
               | ((len & 0xFF) << 16)         /* Number of bytes */
               | (1U << 13);                   /* Auto-end mode */

    /* Send each byte */
    for (size_t i = 0; i < len; i++) {
        /* Wait for TXIS flag */
        if (i2c1_wait_flag(I2C_ISR_TXE, 1) != 0) return -1;
        I2C1->TXDR = data[i];
    }

    /* Wait for STOP flag */
    start = i2c1_get_tick();
    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if ((i2c1_get_tick() - start) > I2C_TIMEOUT_MS) return -1;
    }

    /* Clear STOP flag */
    I2C1->ICR = I2C_ISR_STOPF;

    /* Check for NACK */
    if (I2C1->ISR & I2C_ISR_NACKF) {
        I2C1->ICR = I2C_ISR_NACKF;
        return -2; /* NACK received */
    }

    return 0;
}

/**
 * I2C1 read from A71CH
 * @param buf Buffer for received data
 * @param len Number of bytes to read
 * @return 0 on success, negative on error
 */
static int i2c1_read(uint8_t *buf, size_t len)
{
    uint32_t start;

    /* Wait until bus is free */
    start = i2c1_get_tick();
    while (I2C1->ISR & I2C_ISR_BUSY) {
        if ((i2c1_get_tick() - start) > I2C_TIMEOUT_MS) return -1;
    }

    /* Configure transfer: read, A71CH address, auto-end */
    I2C1->CR2 = (A71CH_I2C_ADDR_8BIT | 1U)  /* Read bit set */
               | ((len & 0xFF) << 16)         /* Number of bytes */
               | (1U << 13);                   /* Auto-end mode */

    /* Read each byte */
    for (size_t i = 0; i < len; i++) {
        /* Wait for RXNE flag */
        start = i2c1_get_tick();
        while (!(I2C1->ISR & I2C_ISR_RXNE)) {
            if ((i2c1_get_tick() - start) > I2C_TIMEOUT_MS) return -1;
        }
        buf[i] = (uint8_t)(I2C1->RXDR & 0xFF);
    }

    /* Wait for STOP flag */
    start = i2c1_get_tick();
    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if ((i2c1_get_tick() - start) > I2C_TIMEOUT_MS) return -1;
    }

    /* Clear STOP flag */
    I2C1->ICR = I2C_ISR_STOPF;

    return 0;
}

/* ============================================================
 * CRC-8 Calculation (A71CH frame integrity)
 * Polynomial: 0x07 (CRC-8/CCITT variant)
 * ============================================================ */
static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* ============================================================
 * A71CH Command Frame Builder
 * Format: [0x00 (preamble)] [LEN_H] [LEN_L] [CMD] [DATA...] [CRC]
 * ============================================================ */
static int se_send_cmd(uint8_t cmd, const uint8_t *data, size_t data_len,
                       uint8_t *rsp, size_t *rsp_len)
{
    uint8_t tx_buf[A71CH_MAX_TX_LEN + 4];
    uint8_t rx_buf[A71CH_MAX_RX_LEN + 4];
    uint16_t total_len;

    /* Build command frame */
    total_len = 1 + data_len;  /* CMD + DATA */

    tx_buf[0] = 0x00;                    /* Preamble */
    tx_buf[1] = (total_len >> 8) & 0xFF; /* Length high */
    tx_buf[2] = total_len & 0xFF;        /* Length low */
    tx_buf[3] = cmd;                     /* Command */

    /* Copy data payload */
    if (data != NULL && data_len > 0) {
        memcpy(&tx_buf[4], data, data_len);
    }

    /* Append CRC */
    size_t frame_len = 4 + data_len;
    tx_buf[frame_len] = crc8(&tx_buf[1], frame_len - 1);
    frame_len++;

    /* Send command */
    int ret = i2c1_write(tx_buf, frame_len);
    if (ret != 0) return ret;

    /* Small delay for A71CH processing */
    extern volatile uint32_t systick_ms;
    uint32_t wait_start = systick_ms;
    while ((systick_ms - wait_start) < 10); /* 10ms processing time */

    /* Read response header (3 bytes: LEN_H, LEN_L, STATUS) */
    ret = i2c1_read(rx_buf, 3);
    if (ret != 0) return ret;

    /* Parse response length */
    uint16_t rsp_data_len = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
    uint8_t status = rx_buf[2];

    /* Read remaining response data + CRC */
    if (rsp_data_len > 0) {
        ret = i2c1_read(&rx_buf[3], rsp_data_len - 1 + 1); /* data + CRC */
        if (ret != 0) return ret;
    }

    /* Verify response CRC */
    size_t total_rx = 3 + rsp_data_len;
    uint8_t expected_crc = crc8(&rx_buf[0], total_rx - 1);
    if (expected_crc != rx_buf[total_rx - 1]) {
        return -3; /* CRC error */
    }

    /* Check status */
    if (status != A71CH_RSP_OK) {
        return -(0x100 + status); /* Error code */
    }

    /* Copy response data (excluding status byte and CRC) */
    if (rsp != NULL && rsp_len != NULL) {
        size_t copy_len = rsp_data_len > 1 ? rsp_data_len - 1 : 0;
        if (copy_len > *rsp_len) copy_len = *rsp_len;
        memcpy(rsp, &rx_buf[3], copy_len);
        *rsp_len = copy_len;
    }

    return 0;
}

/* ============================================================
 * Public API Implementation
 * ============================================================ */

int se_init(void)
{
    /* De-assert reset */
    SE_RESET_RELEASE();
    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;
    while ((systick_ms - start) < 5); /* 5ms reset release time */

    /* Send INIT command */
    return se_send_cmd(A71CH_CMD_INIT, NULL, 0, NULL, NULL);
}

int se_gen_keypair(uint8_t slot, uint8_t curve, uint8_t *pubkey_out, size_t pubkey_len)
{
    uint8_t params[2] = {slot, curve};
    size_t rsp_len = pubkey_len;
    int ret = se_send_cmd(A71CH_CMD_GEN_KEYPAIR, params, 2, pubkey_out, &rsp_len);
    if (ret != 0) return ret;

    /* Expected: 64 bytes for P256 (x + y coordinates, uncompressed) */
    if (rsp_len < 64) return -4; /* Unexpected response length */

    return 0;
}

int se_sign(uint8_t slot, const uint8_t *data, size_t data_len,
            uint8_t *sig_out, size_t *sig_len)
{
    /* Build signing command: [SLOT] [HASH_LEN] [HASH...] */
    uint8_t cmd_buf[2 + 32]; /* slot + hash_len + sha256 */
    cmd_buf[0] = slot;
    cmd_buf[1] = (uint8_t)data_len;
    memcpy(&cmd_buf[2], data, data_len > 32 ? 32 : data_len);

    return se_send_cmd(A71CH_CMD_SIGN, cmd_buf, 2 + (data_len > 32 ? 32 : data_len),
                       sig_out, sig_len);
}

int se_verify(uint8_t slot, const uint8_t *data, size_t data_len,
              const uint8_t *sig, size_t sig_len)
{
    /* Build verify command: [SLOT] [DATA_LEN] [SIG_LEN] [DATA...] [SIG...] */
    uint8_t cmd_buf[2 + 32 + 72]; /* Max: slot + 2 len bytes + 32 data + 72 sig */
    cmd_buf[0] = slot;
    cmd_buf[1] = (uint8_t)data_len;

    size_t offset = 2;
    memcpy(&cmd_buf[offset], data, data_len > 32 ? 32 : data_len);
    offset += (data_len > 32 ? 32 : data_len);

    cmd_buf[offset++] = (uint8_t)sig_len;
    memcpy(&cmd_buf[offset], sig, sig_len);
    offset += sig_len;

    return se_send_cmd(A71CH_CMD_VERIFY, cmd_buf, offset, NULL, NULL);
}

int se_random(uint8_t *buf, size_t len)
{
    uint8_t len_byte = (uint8_t)(len > 64 ? 64 : len);
    size_t rsp_len = len;
    int ret = se_send_cmd(A71CH_CMD_RANDOM, &len_byte, 1, buf, &rsp_len);
    if (ret != 0) return ret;
    return 0;
}

int se_store_cert(uint8_t slot, const uint8_t *cert, size_t cert_len)
{
    uint8_t cmd_buf[2 + A71CH_MAX_CERT_LEN];
    cmd_buf[0] = slot;
    cmd_buf[1] = (uint8_t)((cert_len >> 8) & 0xFF);
    cmd_buf[2] = (uint8_t)(cert_len & 0xFF);
    memcpy(&cmd_buf[3], cert, cert_len);

    return se_send_cmd(A71CH_CMD_STORE_CERT, cmd_buf, 3 + cert_len, NULL, NULL);
}

int se_read_cert(uint8_t slot, uint8_t *cert_out, size_t *cert_len)
{
    size_t rsp_len = *cert_len;
    int ret = se_send_cmd(A71CH_CMD_READ_CERT, &slot, 1, cert_out, &rsp_len);
    if (ret != 0) return ret;
    *cert_len = rsp_len;
    return 0;
}

int se_get_info(uint8_t *info_out, size_t info_len)
{
    size_t rsp_len = info_len;
    return se_send_cmd(A71CH_CMD_GET_INFO, NULL, 0, info_out, &rsp_len);
}