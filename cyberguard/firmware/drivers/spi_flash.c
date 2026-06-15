/*
 * spi_flash.c - MX25R1635F QSPI Flash Driver Implementation
 * CyberGuard Hardware Security Token
 *
 * SPI2 bus: PB12=NSS, PB13=SCK, PB14=MISO, PB15=MOSI
 * Initially configured in standard SPI mode (single I/O)
 * Can be switched to QSPI mode for faster access
 */

#include "spi_flash.h"
#include "../registers.h"
#include "../board.h"

/* ============================================================
 * SPI2 Transfer Primitives
 * ============================================================ */

static void spi2_cs_assert(void)
{
    GPIOB->ODR &= ~(1U << PIN_FLASH_NSS);
}

static void spi2_cs_release(void)
{
    GPIOB->ODR |= (1U << PIN_FLASH_NSS);
}

/**
 * SPI2 byte transfer (blocking)
 * @param tx_byte Byte to transmit
 * @return Received byte
 */
static uint8_t spi2_transfer(uint8_t tx_byte)
{
    /* Wait until TXE (transmit buffer empty) */
    while (!(SPI2->SR & SPI_SR_TXE));
    SPI2->DR = tx_byte;

    /* Wait until RXNE (receive buffer not empty) */
    while (!(SPI2->SR & SPI_SR_RXNE));
    return (uint8_t)(SPI2->DR & 0xFF);
}

/**
 * SPI2 multi-byte transfer (blocking, full-duplex)
 * @param tx_buf Transmit buffer (NULL for read-only)
 * @param rx_buf Receive buffer (NULL for write-only)
 * @param len Number of bytes to transfer
 * @return 0 on success
 */
static int spi2_xfer(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t tx = (tx_buf != NULL) ? tx_buf[i] : 0xFF;
        uint8_t rx = spi2_transfer(tx);
        if (rx_buf != NULL) {
            rx_buf[i] = rx;
        }
    }
    /* Wait until BSY (busy) flag cleared */
    while (SPI2->SR & SPI_SR_BSY);
    return 0;
}

/* ============================================================
 * Public API Implementation
 * ============================================================ */

int spi_flash_init(void)
{
    uint8_t id[3];

    /* Release CS */
    spi2_cs_release();

    /* Small delay for flash power-up */
    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;
    while ((systick_ms - start) < 5);

    /* Reset flash device */
    spi2_cs_assert();
    spi2_xfer((uint8_t[]){MX25_CMD_RSTEN}, NULL, 1);
    spi2_cs_release();

    start = systick_ms;
    while ((systick_ms - start) < 1);

    spi2_cs_assert();
    spi2_xfer((uint8_t[]){MX25_CMD_RST}, NULL, 1);
    spi2_cs_release();

    /* Wait for reset to complete */
    start = systick_ms;
    while ((systick_ms - start) < 50);

    /* Verify JEDEC ID */
    if (spi_flash_read_id(id) != 0) {
        return -1;
    }

    /* Expected: C2 28 15 (Macronix MX25R1635F) */
    if (id[0] != MX25_JEDEC_MANUFACTURER || id[1] != MX25_JEDEC_MEMORY_TYPE) {
        return -2; /* Wrong device */
    }

    /* Clear write protect */
    spi_flash_write_enable();
    uint8_t status_cmd[] = {MX25_CMD_WRITE_STATUS, 0x00, 0x00}; /* Clear BP bits, clear QE for now */
    spi2_cs_assert();
    spi2_xfer(status_cmd, NULL, 3);
    spi2_cs_release();

    spi_flash_wait_busy(100);

    return 0;
}

int spi_flash_read_id(uint8_t id_out[3])
{
    uint8_t cmd[] = {MX25_CMD_READ_ID, 0x00, 0x00, 0x00};

    spi2_cs_assert();
    spi2_xfer(cmd, NULL, 4);        /* Command + 3 dummy bytes */
    spi2_xfer(NULL, id_out, 3);     /* Read 3 ID bytes */
    spi2_cs_release();

    return 0;
}

int spi_flash_read_status(void)
{
    uint8_t cmd = MX25_CMD_READ_STATUS;
    uint8_t status;

    spi2_cs_assert();
    spi2_xfer(&cmd, NULL, 1);
    spi2_xfer(NULL, &status, 1);
    spi2_cs_release();

    return (int)status;
}

int spi_flash_wait_busy(uint32_t timeout_ms)
{
    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;

    while (1) {
        int status = spi_flash_read_status();
        if (status < 0) return status;
        if (!(status & MX25_SR_BUSY)) return 0;
        if ((systick_ms - start) > timeout_ms) return -1; /* Timeout */
    }
}

int spi_flash_write_enable(void)
{
    uint8_t cmd = MX25_CMD_WRITE_ENABLE;

    spi2_cs_assert();
    spi2_xfer(&cmd, NULL, 1);
    spi2_cs_release();

    return 0;
}

int spi_flash_read(uint32_t addr, uint8_t *buf, size_t len)
{
    /* Bounds check */
    if ((addr + len) > MX25_FLASH_SIZE) return -1;

    uint8_t cmd[4] = {
        MX25_CMD_READ,
        (uint8_t)((addr >> 16) & 0xFF),
        (uint8_t)((addr >> 8) & 0xFF),
        (uint8_t)(addr & 0xFF)
    };

    spi2_cs_assert();
    spi2_xfer(cmd, NULL, 4);      /* Command + 24-bit address */
    spi2_xfer(NULL, buf, len);    /* Read data */
    spi2_cs_release();

    return 0;
}

int spi_flash_page_program(uint32_t addr, const uint8_t *data, size_t len)
{
    /* Page alignment check */
    if (len > MX25_PAGE_SIZE) return -1;
    if ((addr & 0xFF) + len > MX25_PAGE_SIZE) return -2; /* Crosses page boundary */

    /* Write enable first */
    spi_flash_write_enable();

    uint8_t cmd[4] = {
        MX25_CMD_PAGE_PROGRAM,
        (uint8_t)((addr >> 16) & 0xFF),
        (uint8_t)((addr >> 8) & 0xFF),
        (uint8_t)(addr & 0xFF)
    };

    spi2_cs_assert();
    spi2_xfer(cmd, NULL, 4);       /* Command + 24-bit address */
    spi2_xfer(data, NULL, len);    /* Write data */
    spi2_cs_release();

    /* Wait for write completion */
    return spi_flash_wait_busy(100);  /* 100ms timeout for page program */
}

int spi_flash_sector_erase(uint32_t addr)
{
    /* Align to sector boundary */
    addr &= ~(MX25_SECTOR_SIZE - 1);

    /* Write enable */
    spi_flash_write_enable();

    uint8_t cmd[4] = {
        MX25_CMD_SECTOR_ERASE,
        (uint8_t)((addr >> 16) & 0xFF),
        (uint8_t)((addr >> 8) & 0xFF),
        (uint8_t)(addr & 0xFF)
    };

    spi2_cs_assert();
    spi2_xfer(cmd, NULL, 4);
    spi2_cs_release();

    /* Wait for erase completion (typical 200ms, max 2s) */
    return spi_flash_wait_busy(2000);
}

int spi_flash_block_erase(uint32_t addr)
{
    addr &= ~(MX25_BLOCK_SIZE - 1);

    spi_flash_write_enable();

    uint8_t cmd[4] = {
        MX25_CMD_BLOCK_ERASE,
        (uint8_t)((addr >> 16) & 0xFF),
        (uint8_t)((addr >> 8) & 0xFF),
        (uint8_t)(addr & 0xFF)
    };

    spi2_cs_assert();
    spi2_xfer(cmd, NULL, 4);
    spi2_cs_release();

    return spi_flash_wait_busy(5000);
}

int spi_flash_chip_erase(void)
{
    spi_flash_write_enable();

    uint8_t cmd = MX25_CMD_CHIP_ERASE;

    spi2_cs_assert();
    spi2_xfer(&cmd, NULL, 1);
    spi2_cs_release();

    /* Chip erase can take up to 60 seconds */
    return spi_flash_wait_busy(60000);
}

int spi_flash_deep_powerdown(void)
{
    uint8_t cmd = MX25_CMD_ENTER_DEEP_PD;

    spi2_cs_assert();
    spi2_xfer(&cmd, NULL, 1);
    spi2_cs_release();

    return 0;
}

int spi_flash_wakeup(void)
{
    uint8_t cmd = MX25_CMD_EXIT_DEEP_PD;

    spi2_cs_assert();
    spi2_xfer(&cmd, NULL, 1);
    spi2_cs_release();

    extern volatile uint32_t systick_ms;
    uint32_t start = systick_ms;
    while ((systick_ms - start) < 10); /* tRES = 10µs min */

    return 0;
}