/*
 * flash_drv.c — W25Q80 SPI flash ring buffer driver.
 *
 * The W25Q80 is an 8 MB (64 Mbit) SPI NOR flash. We use it as a circular
 * ring buffer of 64-byte records (STORAGE_RECORD_BYTES). A write pointer
 * and record count are kept in the first sector (header) so power loss
 * doesn't lose the buffer position.
 *
 * Author: jayis1
 * License: MIT
 */

#include "flash_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* W25Q80 commands */
#define W25Q_CMD_READ_DATA      0x03
#define W25Q_CMD_PAGE_PROGRAM   0x02
#define W25Q_CMD_SECTOR_ERASE   0x20
#define W25Q_CMD_READ_STATUS1   0x05
#define W25Q_CMD_WRITE_ENABLE   0x06
#define W25Q_CMD_POWER_DOWN      0xB9
#define W25Q_CMD_WAKE            0xAB

#define W25Q_STATUS_BUSY         0x01
#define W25Q_PAGE_SIZE           256
#define W25Q_SECTOR_SIZE         4096

/* Header lives in sector 0, records start at sector 1. */
#define HEADER_ADDR      0x00000000UL
#define HEADER_MAGIC      0x48454144UL  /* 'HEAD' */
#define RECORDS_BASE      0x00001000UL   /* sector 1 onward */
#define RECORD_SIZE       STORAGE_RECORD_BYTES
#define MAX_RECORDS       ((FLASH_SIZE_BYTES - RECORDS_BASE) / RECORD_SIZE)

typedef struct {
    uint32_t magic;
    uint32_t write_ptr;   /* next record index */
    uint32_t count;       /* valid records (caps at MAX_RECORDS) */
} flash_header_t;

static int s_initialized = 0;
static flash_header_t s_header;

/* ------------------------------------------------------------------------ */
/* SPI1 primitives                                                            */
/* ------------------------------------------------------------------------ */
static void spi1_cs_low(void)
{
    volatile uint32_t *pa = (volatile uint32_t *)GPIOA;
    pa[6] = (1U << 4);  /* BRR — NSS low */
}

static void spi1_cs_high(void)
{
    volatile uint32_t *pa = (volatile uint32_t *)GPIOA;
    pa[5] = (1U << 4);  /* BSRR — NSS high */
}

static uint8_t spi1_xfer(uint8_t tx)
{
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&SPI1->DR = tx;
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return (uint8_t)SPI1->DR;
}

static void spi1_write(const uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++) spi1_xfer(buf[i]);
}

static void spi1_read(uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++) buf[i] = spi1_xfer(0xFF);
}

/* ------------------------------------------------------------------------ */
/* W25Q80 helpers                                                            */
/* ------------------------------------------------------------------------ */
static void w25q_wait_busy(void)
{
    uint8_t status;
    do {
        spi1_cs_low();
        spi1_xfer(W25Q_CMD_READ_STATUS1);
        status = spi1_xfer(0xFF);
        spi1_cs_high();
    } while (status & W25Q_STATUS_BUSY);
}

static void w25q_write_enable(void)
{
    spi1_cs_low();
    spi1_xfer(W25Q_CMD_WRITE_ENABLE);
    spi1_cs_high();
}

static void w25q_erase_sector(uint32_t addr)
{
    w25q_write_enable();
    spi1_cs_low();
    spi1_xfer(W25Q_CMD_SECTOR_ERASE);
    spi1_xfer((uint8_t)(addr >> 16));
    spi1_xfer((uint8_t)(addr >> 8));
    spi1_xfer((uint8_t)(addr));
    spi1_cs_high();
    w25q_wait_busy();
}

static void w25q_read(uint32_t addr, uint8_t *buf, int len)
{
    spi1_cs_low();
    spi1_xfer(W25Q_CMD_READ_DATA);
    spi1_xfer((uint8_t)(addr >> 16));
    spi1_xfer((uint8_t)(addr >> 8));
    spi1_xfer((uint8_t)(addr));
    spi1_read(buf, len);
    spi1_cs_high();
}

static void w25q_program_page(uint32_t addr, const uint8_t *buf, int len)
{
    /* Page program can write up to 256 bytes within a single page boundary. */
    w25q_write_enable();
    spi1_cs_low();
    spi1_xfer(W25Q_CMD_PAGE_PROGRAM);
    spi1_xfer((uint8_t)(addr >> 16));
    spi1_xfer((uint8_t)(addr >> 8));
    spi1_xfer((uint8_t)(addr));
    spi1_write(buf, len);
    spi1_cs_high();
    w25q_wait_busy();
}

/* ------------------------------------------------------------------------ */
/* Header management                                                          */
/* ------------------------------------------------------------------------ */
static void header_load(void)
{
    uint8_t buf[sizeof(flash_header_t)];
    w25q_read(HEADER_ADDR, buf, sizeof(flash_header_t));
    memcpy(&s_header, buf, sizeof(s_header));
    if (s_header.magic != HEADER_MAGIC) {
        /* Fresh flash — initialize. */
        s_header.magic = HEADER_MAGIC;
        s_header.write_ptr = 0;
        s_header.count = 0;
        w25q_erase_sector(HEADER_ADDR);
        w25q_program_page(HEADER_ADDR, (uint8_t*)&s_header, sizeof(s_header));
    }
}

static void header_save(void)
{
    w25q_erase_sector(HEADER_ADDR);
    w25q_program_page(HEADER_ADDR, (uint8_t*)&s_header, sizeof(s_header));
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                 */
/* ------------------------------------------------------------------------ */
int flash_init(void)
{
    /* Configure SPI1 pins: PA4 (NSS), PA5 (SCK), PA6 (MISO), PA7 (MOSI) as AF5. */
    volatile uint32_t *pa = (volatile uint32_t *)GPIOA;
    for (int pin = 4; pin <= 7; pin++) {
        if (pin < 8) {
            pa[7] = (pa[7] & ~(0xFU << (pin*4))) | (0x5U << (pin*4));  /* AFRL */
        }
        pa[0] = (pa[0] & ~(0x3U << (pin*2))) | (GPIO_MODE_AF << (pin*2));
    }

    /* Configure SPI1: master, CPOL=0, CPHA=0, 8-bit, /8 divider. */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM | SPI_CR1_BR_DIV8
               | SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI1->CR1 |= SPI_CR1_SPE;

    /* Wake the flash from power-down if it was there. */
    spi1_cs_low();
    spi1_xfer(W25Q_CMD_WAKE);
    spi1_cs_high();
    for (volatile int d = 0; d < 1000; d++) { }  /* wake-up time ~3 µs */

    header_load();
    s_initialized = 1;
    return 0;
}

int flash_append(const flash_record_t *rec)
{
    if (!s_initialized || rec == NULL) return -1;

    /* Compute the flash address of the next record. */
    uint32_t rec_idx = s_header.write_ptr;
    uint32_t addr = RECORDS_BASE + rec_idx * RECORD_SIZE;

    /* If we're at a sector boundary (every 64 records for 64-byte records in
     * a 4 KB sector), erase that sector first. */
    if ((rec_idx % (W25Q_SECTOR_SIZE / RECORD_SIZE)) == 0) {
        w25q_erase_sector(addr);
    }

    /* Write the record (64 bytes fits within a 256-byte page). */
    uint8_t buf[RECORD_SIZE];
    memset(buf, 0xFF, sizeof(buf));
    memcpy(buf, rec, sizeof(flash_record_t) < RECORD_SIZE ? sizeof(flash_record_t) : RECORD_SIZE);
    w25q_program_page(addr, buf, RECORD_SIZE);

    /* Advance the write pointer (ring). */
    s_header.write_ptr = (s_header.write_ptr + 1) % MAX_RECORDS;
    if (s_header.count < MAX_RECORDS) s_header.count++;
    header_save();

    return 0;
}

int flash_read_record(uint32_t index, flash_record_t *rec)
{
    if (!s_initialized || rec == NULL || index >= s_header.count) return -1;
    uint32_t addr = RECORDS_BASE + index * RECORD_SIZE;
    uint8_t buf[RECORD_SIZE];
    w25q_read(addr, buf, RECORD_SIZE);
    memcpy(rec, buf, sizeof(flash_record_t));
    return (rec->magic == STORAGE_MAGIC) ? 0 : -2;
}

uint32_t flash_record_count(void)
{
    return s_header.count;
}