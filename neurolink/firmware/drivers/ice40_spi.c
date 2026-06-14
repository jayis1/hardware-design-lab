/*
 * NeuroLink — iCE40UP5K FPGA SPI Driver Implementation
 * Handles bitstream loading from QSPI flash and runtime SPI communication
 */

#include "ice40_spi.h"
#include "board.h"
#include "registers.h"

/* ============================================================
 * SPI2 Chip Select & Transfer Helpers
 * ============================================================ */

static inline void ice40_cs_assert(void)
{
    GPIO_BSRR(GPIOB) |= (1 << (FPGA_CS_PIN + 16));  /* PB12 low */
}

static inline void ice40_cs_release(void)
{
    GPIO_BSRR(GPIOB) |= (1 << FPGA_CS_PIN);  /* PB12 high */
}

static inline uint8_t ice40_spi_xfer(uint8_t data)
{
    while (!(SPI2_SR & (1 << 1)));   /* Wait TXE */
    *((volatile uint8_t *)&SPI2_DR) = data;
    while (!(SPI2_SR & (1 << 0)));   /* Wait RXNE */
    return *((volatile uint8_t *)&SPI2_DR);
}

/* ============================================================
 * Load FPGA Bitstream from QSPI Flash
 * Returns 0 on success, -1 on timeout
 * ============================================================ */
int ice40_load_bitstream(void)
{
    /* Step 1: Assert reset */
    GPIO_BSRR(GPIOB) |= (1 << (FPGA_RESET_PIN + 16));  /* PB0 low (reset active) */

    /* Step 2: Wait 10 ms */
    for (volatile int i = 0; i < 1000000; i++);

    /* Step 3: Configure QSPI for memory-mapped mode to read bitstream */
    /* (In production, W25Q128 stores the bitstream at offset 0x400000) */
    /* The QSPI controller reads the bitstream and sends it via SPI2 */

    /* Step 4: Switch SPI2 to master mode for bitstream loading */
    SPI2_CR1 &= ~(1 << 6);  /* Disable SPI */
    SPI2_CR1 = (0 << 0) |   /* CPHA = 0 */
               (0 << 1) |   /* CPOL = 0 */
               (1 << 2) |   /* MSTR = 1 */
               (7 << 3) |   /* BR = /256 (slow for bitstream) */
               (1 << 8) |   /* SSM = 1 */
               (1 << 9);    /* SSI = 1 */
    SPI2_CR1 |= (1 << 6);   /* Enable SPI */

    /* Step 5: De-assert reset (start configuration) */
    GPIO_BSRR(GPIOB) |= (1 << FPGA_RESET_PIN);  /* PB0 high (reset inactive) */

    /* Step 6: Send bitstream via SPI2 */
    /* In production: read from QSPI memory-mapped address */
    /* For now, send dummy configuration clocks */
    ice40_cs_assert();

    /* Send 49 dummy clocks (required by iCE40 configuration protocol) */
    for (int i = 0; i < 49; i++) {
        ice40_spi_xfer(0x00);
    }

    ice40_cs_release();

    /* Step 7: Wait for CDONE to go high (configuration complete) */
    uint32_t timeout = 500000;  /* ~5 ms at 480 MHz */
    while (!(GPIO_IDR(GPIOB) & (1 << FPGA_CDONE_PIN)) && timeout--) {
        for (volatile int i = 0; i < 10; i++);
    }

    if (timeout == 0) {
        return -1;  /* Configuration failed */
    }

    /* Step 8: Send additional 49 dummy clocks to complete */
    for (int i = 0; i < 49; i++) {
        ice40_spi_xfer(0x00);
    }

    /* Step 9: Reconfigure SPI2 for high-speed runtime communication */
    SPI2_CR1 &= ~(1 << 6);  /* Disable SPI */
    SPI2_CR1 = (0 << 0) |   /* CPHA = 0 */
               (0 << 1) |   /* CPOL = 0 */
               (1 << 2) |   /* MSTR = 1 */
               (3 << 3) |   /* BR = /32 (fast runtime) */
               (1 << 8) |   /* SSM = 1 */
               (1 << 9);    /* SSI = 1 */
    SPI2_CR2 = (1 << 0) |   /* RXDMAEN */
               (1 << 1) |   /* TXDMAEN */
               (1 << 12);   /* FRXTH */
    SPI2_CR1 |= (1 << 6);   /* Enable SPI */

    /* Step 10: Verify FPGA by reading ID */
    uint8_t id = ice40_read_id();
    if (id == 0xFF || id == 0x00) {
        return -2;  /* FPGA not responding */
    }

    return 0;
}

/* ============================================================
 * FPGA Reset Control
 * ============================================================ */
void ice40_reset_assert(void)
{
    GPIO_BSRR(GPIOB) |= (1 << (FPGA_RESET_PIN + 16));
}

void ice40_reset_release(void)
{
    GPIO_BSRR(GPIOB) |= (1 << FPGA_RESET_PIN);
}

/* ============================================================
 * Check Configuration Done
 * ============================================================ */
uint8_t ice40_is_config_done(void)
{
    return (GPIO_IDR(GPIOB) & (1 << FPGA_CDONE_PIN)) ? 1 : 0;
}

/* ============================================================
 * Read FPGA ID Register
 * ============================================================ */
uint8_t ice40_read_id(void)
{
    uint8_t id;
    ice40_cs_assert();
    ice40_spi_xfer(ICE40_CMD_READ_ID);
    ice40_spi_xfer(0x00);  /* Dummy byte */
    id = ice40_spi_xfer(0x00);
    ice40_cs_release();
    return id;
}

/* ============================================================
 * Read FPGA Status Register
 * ============================================================ */
uint8_t ice40_read_status(void)
{
    uint8_t status;
    ice40_cs_assert();
    ice40_spi_xfer(ICE40_CMD_READ_STATUS);
    ice40_spi_xfer(0x00);
    status = ice40_spi_xfer(0x00);
    ice40_cs_release();
    return status;
}

/* ============================================================
 * Send Raw Sample Data to FPGA DSP Pipeline
 * Frame format: [CMD][LEN_H][LEN_L][DATA...][CRC16_H][CRC16_L]
 * ============================================================ */
void ice40_send_samples(const uint8_t *data, uint16_t len)
{
    uint16_t crc;

    ice40_cs_assert();

    /* Command byte */
    ice40_spi_xfer(ICE40_CMD_WRITE_DATA);

    /* Length (big-endian) */
    ice40_spi_xfer((len >> 8) & 0xFF);
    ice40_spi_xfer(len & 0xFF);

    /* Data payload */
    for (uint16_t i = 0; i < len; i++) {
        ice40_spi_xfer(data[i]);
    }

    /* CRC-16 over data */
    crc = ice40_crc16(data, len);
    ice40_spi_xfer((crc >> 8) & 0xFF);
    ice40_spi_xfer(crc & 0xFF);

    ice40_cs_release();
}

/* ============================================================
 * Read Processed Data from FPGA
 * ============================================================ */
void ice40_read_processed(uint8_t *buf, uint16_t len)
{
    ice40_cs_assert();

    /* Command byte */
    ice40_spi_xfer(ICE40_CMD_READ_DATA);

    /* Length (big-endian) */
    ice40_spi_xfer((len >> 8) & 0xFF);
    ice40_spi_xfer(len & 0xFF);

    /* Read data */
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = ice40_spi_xfer(0x00);
    }

    /* Read CRC (discarded in this simplified version) */
    ice40_spi_xfer(0x00);
    ice40_spi_xfer(0x00);

    ice40_cs_release();
}

/* ============================================================
 * Set DSP Filter Configuration
 * ============================================================ */
void ice40_set_filter(const ice40_filter_t *filt)
{
    ice40_cs_assert();

    ice40_spi_xfer(ICE40_CMD_SET_FILTER);
    ice40_spi_xfer(filt->filter_type);
    ice40_spi_xfer(filt->order);
    ice40_spi_xfer((filt->cutoff_low >> 8) & 0xFF);
    ice40_spi_xfer(filt->cutoff_low & 0xFF);
    ice40_spi_xfer((filt->cutoff_high >> 8) & 0xFF);
    ice40_spi_xfer(filt->cutoff_high & 0xFF);
    ice40_spi_xfer((filt->sample_rate >> 8) & 0xFF);
    ice40_spi_xfer(filt->sample_rate & 0xFF);

    ice40_cs_release();
}

/* ============================================================
 * Set Feature Extraction Configuration
 * ============================================================ */
void ice40_set_feature_config(const ice40_feature_config_t *cfg)
{
    ice40_cs_assert();

    ice40_spi_xfer(ICE40_CMD_SET_GAIN);
    ice40_spi_xfer(cfg->feature_mask);
    ice40_spi_xfer((cfg->window_size >> 8) & 0xFF);
    ice40_spi_xfer(cfg->window_size & 0xFF);

    ice40_cs_release();
}

/* ============================================================
 * Reset DSP Pipeline
 * ============================================================ */
void ice40_reset_pipeline(void)
{
    ice40_cs_assert();
    ice40_spi_xfer(ICE40_CMD_RESET_PIPE);
    ice40_cs_release();
}

/* ============================================================
 * CRC-16/CCITT Calculation
 * Polynomial: 0x1021, Init: 0xFFFF
 * ============================================================ */
uint16_t ice40_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i] << 8);
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}