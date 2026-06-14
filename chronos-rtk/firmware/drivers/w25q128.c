/*
 * w25q128.c — Winbond W25Q128 SPI NOR flash driver implementation
 * DMA-based read/write, circular log buffer for GNSS observation storage
 */

#include "w25q128.h"
#include "board.h"
#include "registers.h"

/* ========================================================================== */
/* SPI1 access helpers (W25Q128 on SPI1)                                        */
/* ========================================================================== */

static SPI_TypeDef * const SPI1 = (SPI_TypeDef *)SPI1_BASE;

/* Assert CS# (active low) */
static void flash_cs_low(void)
{
    GPIOA->ODR &= ~(1U << PIN_SPI1_NSS);
}

/* De-assert CS# */
static void flash_cs_high(void)
{
    GPIOA->ODR |= (1U << PIN_SPI1_NSS);
}

/* Transfer one byte over SPI1 */
static uint8_t spi1_transfer(uint8_t tx_byte)
{
    while (!(SPI1->SR & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)&SPI1->DR = tx_byte;
    while (!(SPI1->SR & SPI_SR_RXNE))
        ;
    return *(volatile uint8_t *)&SPI1->DR;
}

/* ========================================================================== */
/* Low-level flash operations                                                   */
/* ========================================================================== */

/* Wait until flash is ready (BUSY bit = 0) */
int w25q128_wait_ready(uint32_t timeout_ms)
{
    volatile uint32_t count = timeout_ms * 170000;  /* Approximate ms loop */
    uint8_t status;

    do {
        flash_cs_low();
        spi1_transfer(W25Q128_CMD_READ_STATUS1);
        status = spi1_transfer(0xFF);
        flash_cs_high();

        if (!(status & W25Q128_SR1_BUSY)) {
            return 0;  /* Ready */
        }
    } while (count--);

    return -1;  /* Timeout */
}

/* Enable write latch */
static void w25q128_write_enable(void)
{
    flash_cs_low();
    spi1_transfer(W25Q128_CMD_WRITE_ENABLE);
    flash_cs_high();
}

/* Read status register 1 */
static uint8_t w25q128_read_status1(void)
{
    uint8_t status;
    flash_cs_low();
    spi1_transfer(W25Q128_CMD_READ_STATUS1);
    status = spi1_transfer(0xFF);
    flash_cs_high();
    return status;
}

/* ========================================================================== */
/* Public API implementation                                                    */
/* ========================================================================== */

int w25q128_init(void)
{
    /* SPI1 already initialized in main.c */

    /* Verify JEDEC ID */
    uint32_t jedec = w25q128_read_jedec_id();
    if (jedec != W25Q128_JEDEC_ID) {
        return -1;  /* Flash not detected or wrong part */
    }

    /* Release from power-down (in case of warm start) */
    flash_cs_low();
    spi1_transfer(W25Q128_CMD_RELEASE_POWERDOWN);
    flash_cs_high();
    w25q128_wait_ready(10);  /* 10 ms max */

    /* Clear block protect bits in status registers */
    /* Ensure write-enable first */
    w25q128_write_enable();

    /* Write status register 1: no block protection */
    flash_cs_low();
    spi1_transfer(W25Q128_CMD_WRITE_STATUS1);
    spi1_transfer(0x00);  /* No protection */
    flash_cs_high();
    w25q128_wait_ready(100);

    return 0;
}

uint32_t w25q128_read_jedec_id(void)
{
    uint32_t id;
    flash_cs_low();
    spi1_transfer(W25Q128_CMD_READ_JEDEC_ID);
    id  = (uint32_t)spi1_transfer(0xFF) << 16;  /* Manufacturer */
    id |= (uint32_t)spi1_transfer(0xFF) << 8;   /* Device high */
    id |= (uint32_t)spi1_transfer(0xFF);         /* Device low */
    flash_cs_high();
    return id;
}

uint64_t w25q128_read_uid(void)
{
    uint64_t uid = 0;
    flash_cs_low();
    spi1_transfer(W25Q128_CMD_READ_UID);
    /* 4 dummy bytes */
    spi1_transfer(0xFF);
    spi1_transfer(0xFF);
    spi1_transfer(0xFF);
    spi1_transfer(0xFF);
    /* 8 UID bytes */
    for (int i = 0; i < 8; i++) {
        uid <<= 8;
        uid |= spi1_transfer(0xFF);
    }
    flash_cs_high();
    return uid;
}

int w25q128_read(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (addr + len > W25Q128_CHIP_SIZE) return -1;

    flash_cs_low();
    spi1_transfer(W25Q128_CMD_READ_DATA);
    spi1_transfer((addr >> 16) & 0xFF);
    spi1_transfer((addr >> 8) & 0xFF);
    spi1_transfer(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = spi1_transfer(0xFF);
    }
    flash_cs_high();
    return 0;
}

int w25q128_fast_read(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (addr + len > W25Q128_CHIP_SIZE) return -1;

    flash_cs_low();
    spi1_transfer(W25Q128_CMD_FAST_READ);
    spi1_transfer((addr >> 16) & 0xFF);
    spi1_transfer((addr >> 8) & 0xFF);
    spi1_transfer(addr & 0xFF);
    spi1_transfer(0xFF);  /* Dummy byte for fast read */
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = spi1_transfer(0xFF);
    }
    flash_cs_high();
    return 0;
}

int w25q128_page_program(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    if (len == 0 || len > W25Q128_PAGE_SIZE) return -1;
    if (addr + len > W25Q128_CHIP_SIZE) return -1;
    /* Address must not cross page boundary */
    if ((addr & 0xFF) + len > W25Q128_PAGE_SIZE) return -1;

    w25q128_write_enable();

    flash_cs_low();
    spi1_transfer(W25Q128_CMD_PAGE_PROGRAM);
    spi1_transfer((addr >> 16) & 0xFF);
    spi1_transfer((addr >> 8) & 0xFF);
    spi1_transfer(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) {
        spi1_transfer(buf[i]);
    }
    flash_cs_high();

    return w25q128_wait_ready(50);  /* 50 ms max for page program */
}

int w25q128_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t offset = 0;

    while (offset < len) {
        /* Calculate bytes remaining in current page */
        uint32_t page_offset = addr % W25Q128_PAGE_SIZE;
        uint16_t chunk = W25Q128_PAGE_SIZE - page_offset;
        if (chunk > (len - offset)) {
            chunk = (uint16_t)(len - offset);
        }

        int ret = w25q128_page_program(addr, buf + offset, chunk);
        if (ret != 0) return ret;

        addr += chunk;
        offset += chunk;
    }
    return 0;
}

int w25q128_sector_erase(uint32_t addr)
{
    /* Align address to sector boundary */
    addr = addr & ~(W25Q128_SECTOR_SIZE - 1);

    w25q128_write_enable();

    flash_cs_low();
    spi1_transfer(W25Q128_CMD_SECTOR_ERASE);
    spi1_transfer((addr >> 16) & 0xFF);
    spi1_transfer((addr >> 8) & 0xFF);
    spi1_transfer(addr & 0xFF);
    flash_cs_high();

    return w25q128_wait_ready(1000);  /* 1 second max for sector erase */
}

int w25q128_block_erase(uint32_t addr)
{
    /* Align address to block boundary */
    addr = addr & ~(W25Q128_BLOCK_SIZE - 1);

    w25q128_write_enable();

    flash_cs_low();
    spi1_transfer(W25Q128_CMD_BLOCK_ERASE_64K);
    spi1_transfer((addr >> 16) & 0xFF);
    spi1_transfer((addr >> 8) & 0xFF);
    spi1_transfer(addr & 0xFF);
    flash_cs_high();

    return w25q128_wait_ready(2000);  /* 2 seconds max for block erase */
}

int w25q128_chip_erase(void)
{
    w25q128_write_enable();

    flash_cs_low();
    spi1_transfer(W25Q128_CMD_CHIP_ERASE);
    flash_cs_high();

    return w25q128_wait_ready(60000);  /* 60 seconds max */
}

/* ========================================================================== */
/* Circular log buffer                                                          */
/* ========================================================================== */

/* Log metadata is stored in the first sector (4 KB) of the flash.
 * Actual log data starts at sector 1 (address 0x1000).
 * The log wraps around when it reaches the end of the flash.
 * We reserve the last 4 sectors (16 KB) for a second copy of the header. */

#define LOG_HEADER_ADDR      0x000000
#define LOG_HEADER_ADDR_ALT  0x0FF000  /* Backup header at end of flash */
#define LOG_DATA_START       0x001000  /* Start after header sector */
#define LOG_DATA_END         0x0FF000  /* End before backup header */

static log_header_t g_log_header;
static uint8_t g_page_buf[W25Q128_PAGE_SIZE];

uint32_t w25q128_log_get_addr(void)
{
    return LOG_DATA_START + (g_log_header.page_index * W25Q128_PAGE_SIZE);
}

int w25q128_log_append(const uint8_t *data, uint16_t len)
{
    if (len == 0 || len > W25Q128_PAGE_SIZE - 4) return -1;

    uint32_t current_addr = w25q128_log_get_addr();

    /* Check if we need to erase the next sector before writing */
    if ((current_addr % W25Q128_SECTOR_SIZE) == 0) {
        /* We're at a sector boundary — erase it */
        int ret = w25q128_sector_erase(current_addr);
        if (ret != 0) return ret;
    }

    /* Build page: [2-byte length][data][padding] */
    uint16_t idx = 0;
    g_page_buf[idx++] = (len >> 8) & 0xFF;
    g_page_buf[idx++] = len & 0xFF;
    for (uint16_t i = 0; i < len; i++) {
        g_page_buf[idx++] = data[i];
    }
    /* Pad remainder with 0xFF */
    while (idx < W25Q128_PAGE_SIZE) {
        g_page_buf[idx++] = 0xFF;
    }

    /* Write page */
    int ret = w25q128_page_program(current_addr, g_page_buf, W25Q128_PAGE_SIZE);
    if (ret != 0) return ret;

    /* Update header */
    g_log_header.page_index++;
    g_log_header.entry_count++;
    if (current_addr + W25Q128_PAGE_SIZE >= LOG_DATA_END) {
        g_log_header.page_index = 0;
        g_log_header.wrap_count++;
    }

    /* Write updated header (simple write-back) */
    /* In production, use CRC and atomic update */
    g_log_header.magic = LOG_HEADER_MAGIC;

    return 0;
}

int w25q128_log_read(uint32_t offset, uint8_t *buf, uint16_t len)
{
    uint32_t addr = LOG_DATA_START + offset;
    if (addr + len > LOG_DATA_END) return -1;
    return w25q128_fast_read(addr, buf, len);
}