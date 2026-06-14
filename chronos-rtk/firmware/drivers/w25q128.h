/*
 * w25q128.h — Winbond W25Q128 SPI NOR flash driver
 * For observation logging on Chronos-RTK board
 */

#ifndef W25Q128_H
#define W25Q128_H

#include <stdint.h>
#include <stddef.h>

/* ========================================================================== */
/* W25Q128 constants                                                            */
/* ========================================================================== */
#define W25Q128_PAGE_SIZE        256
#define W25Q128_SECTOR_SIZE      4096
#define W25Q128_BLOCK_SIZE       65536
#define W25Q128_CHIP_SIZE        (16UL * 1024UL * 1024UL)  /* 16 MB */
#define W25Q128_NUM_PAGES        (W25Q128_CHIP_SIZE / W25Q128_PAGE_SIZE)
#define W25Q128_NUM_SECTORS      (W25Q128_CHIP_SIZE / W25Q128_SECTOR_SIZE)
#define W25Q128_NUM_BLOCKS       (W25Q128_CHIP_SIZE / W25Q128_BLOCK_SIZE)

/* Expected JEDEC ID: Winbond W25Q128JV */
#define W25Q128_JEDEC_ID        0xEF4018

/* ========================================================================== */
/* W25Q128 SPI commands                                                         */
/* ========================================================================== */
#define W25Q128_CMD_WRITE_ENABLE      0x06
#define W25Q128_CMD_WRITE_DISABLE     0x04
#define W25Q128_CMD_READ_STATUS1      0x05
#define W25Q128_CMD_READ_STATUS2      0x35
#define W25Q128_CMD_READ_STATUS3      0x0F
#define W25Q128_CMD_WRITE_STATUS1     0x01
#define W25Q128_CMD_WRITE_STATUS2     0x31
#define W25Q128_CMD_WRITE_STATUS3     0x11
#define W25Q128_CMD_READ_JEDEC_ID     0x9F
#define W25Q128_CMD_READ_DATA         0x03
#define W25Q128_CMD_FAST_READ         0x0B
#define W25Q128_CMD_FAST_READ_DUAL    0x3B
#define W25Q128_CMD_FAST_READ_QUAD    0x6B
#define W25Q128_CMD_PAGE_PROGRAM      0x02
#define W25Q128_CMD_SECTOR_ERASE      0x20
#define W25Q128_CMD_BLOCK_ERASE_32K   0x52
#define W25Q128_CMD_BLOCK_ERASE_64K   0xD8
#define W25Q128_CMD_CHIP_ERASE        0xC7
#define W25Q128_CMD_RELEASE_POWERDOWN 0xAB
#define W25Q128_CMD_POWER_DOWN        0xB9
#define W25Q128_CMD_READ_UID          0x4B

/* Status register bits */
#define W25Q128_SR1_BUSY              (1U << 0)
#define W25Q128_SR1_WEL               (1U << 1)
#define W25Q128_SR2_QE                (1U << 1)
#define W25Q128_SR3_SRP               (1U << 0)

/* ========================================================================== */
/* Circular log buffer structure                                                */
/* ========================================================================== */
#define LOG_PAGE_SIZE    256
#define LOG_HEADER_MAGIC 0x43524C47  /* "CRLG" */

typedef struct {
    uint32_t magic;          /* LOG_HEADER_MAGIC */
    uint32_t page_index;     /* Current write page (0..8191) */
    uint32_t wrap_count;     /* Number of times buffer has wrapped */
    uint32_t entry_count;    /* Total entries written */
    uint32_t crc32;          /* CRC of header */
} log_header_t;

/* ========================================================================== */
/* Public API                                                                   */
/* ========================================================================== */

/**
 * Initialize W25Q128 flash driver
 * - Verify JEDEC ID
 * - Initialize SPI1 interface
 * - Read log header
 * @return 0 on success, negative on error
 */
int w25q128_init(void);

/**
 * Read JEDEC ID
 * @return 24-bit JEDEC ID (manufacturer + device)
 */
uint32_t w25q128_read_jedec_id(void);

/**
 * Read 64-bit unique ID
 * @return 64-bit unique ID
 */
uint64_t w25q128_read_uid(void);

/**
 * Read data from flash (slow read, up to 33 MHz)
 * @param addr  24-bit address
 * @param buf   Destination buffer
 * @param len   Number of bytes to read
 * @return 0 on success, negative on error
 */
int w25q128_read(uint32_t addr, uint8_t *buf, uint16_t len);

/**
 * Read data from flash (fast read, up to 50 MHz)
 * @param addr  24-bit address
 * @param buf   Destination buffer
 * @param len   Number of bytes to read
 * @return 0 on success, negative on error
 */
int w25q128_fast_read(uint32_t addr, uint8_t *buf, uint16_t len);

/**
 * Program a page (1-256 bytes within a single 256-byte page)
 * @param addr  24-bit address (must be page-aligned for multi-page)
 * @param buf   Source data
 * @param len   Number of bytes to write (1-256)
 * @return 0 on success, negative on error
 */
int w25q128_page_program(uint32_t addr, const uint8_t *buf, uint16_t len);

/**
 * Write data spanning multiple pages (handles page boundaries)
 * @param addr  24-bit start address
 * @param buf   Source data
 * @param len   Total bytes to write
 * @return 0 on success, negative on error
 */
int w25q128_write(uint32_t addr, const uint8_t *buf, uint32_t len);

/**
 * Erase a 4 KB sector
 * @param addr  Address within the sector to erase
 * @return 0 on success, negative on error
 */
int w25q128_sector_erase(uint32_t addr);

/**
 * Erase a 64 KB block
 * @param addr  Address within the block to erase
 * @return 0 on success, negative on error
 */
int w25q128_block_erase(uint32_t addr);

/**
 * Erase the entire chip (takes ~60 seconds)
 * @return 0 on success, negative on error
 */
int w25q128_chip_erase(void);

/**
 * Wait until flash is not busy (polls status register)
 * @param timeout_ms  Maximum wait time in ms
 * @return 0 on success, -1 on timeout
 */
int w25q128_wait_ready(uint32_t timeout_ms);

/**
 * Get current log write address
 * @return Current write address
 */
uint32_t w25q128_log_get_addr(void);

/**
 * Append data to the circular log buffer
 * @param data  Pointer to data
 * @param len   Data length
 * @return 0 on success, negative on error
 */
int w25q128_log_append(const uint8_t *data, uint16_t len);

/**
 * Read data from the log at a given offset
 * @param offset  Byte offset from start of log
 * @param buf     Destination buffer
 * @param len     Bytes to read
 * @return 0 on success, negative on error
 */
int w25q128_log_read(uint32_t offset, uint8_t *buf, uint16_t len);

#endif /* W25Q128_H */