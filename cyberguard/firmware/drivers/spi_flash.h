/*
 * spi_flash.h - MX25R1635F QSPI Flash Driver Header
 * CyberGuard Hardware Security Token
 */

#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include <stdint.h>
#include <stddef.h>

/* MX25R1635F Commands (SPI mode) */
#define MX25_CMD_READ           0x03
#define MX25_CMD_FAST_READ      0x0B
#define MX25_CMD_DUAL_READ      0x3B
#define MX25_CMD_QUAD_READ      0x6B
#define MX25_CMD_WRITE_ENABLE   0x06
#define MX25_CMD_WRITE_DISABLE  0x04
#define MX25_CMD_PAGE_PROGRAM   0x02
#define MX25_CMD_QUAD_PROGRAM   0x32
#define MX25_CMD_SECTOR_ERASE   0x20
#define MX25_CMD_BLOCK_ERASE    0xD8
#define MX25_CMD_CHIP_ERASE     0xC7
#define MX25_CMD_READ_ID        0x9F
#define MX25_CMD_READ_STATUS    0x05
#define MX25_CMD_WRITE_STATUS   0x01
#define MX25_CMD_ENTER_QPI      0x38
#define MX25_CMD_EXIT_QPI       0xFF
#define MX25_CMD_RSTEN          0x66
#define MX25_CMD_RST            0x99
#define MX25_CMD_ENTER_DEEP_PD  0xB9
#define MX25_CMD_EXIT_DEEP_PD   0xAB

/* Status register bits */
#define MX25_SR_BUSY            (1U << 0)
#define MX25_SR_WEL             (1U << 1)
#define MX25_SR_BP0             (1U << 2)
#define MX25_SR_BP1             (1U << 3)
#define MX25_SR_BP2             (1U << 4)
#define MX25_SR_QE              (1U << 1)  /* S9: Quad Enable */
#define MX25_SR_PROTECTED       (1U << 5)

/* Flash geometry */
#define MX25_PAGE_SIZE          256
#define MX25_SECTOR_SIZE        4096
#define MX25_BLOCK_SIZE         65536
#define MX25_FLASH_SIZE         (16 * 1024 * 1024)  /* 16 MB */
#define MX25_PAGE_COUNT         (MX25_FLASH_SIZE / MX25_PAGE_SIZE)
#define MX25_SECTOR_COUNT       (MX25_FLASH_SIZE / MX25_SECTOR_SIZE)

/* Expected JEDEC ID */
#define MX25_JEDEC_MANUFACTURER 0xC2
#define MX25_JEDEC_MEMORY_TYPE  0x28
#define MX25_JEDEC_CAPACITY     0x15   /* 16 Mbit = 2 MB, but 16MB for R series */

/* Partition offsets (see Phase 4 boot strategy) */
#define FLASH_PARTITION_TABLE   0x000000
#define FLASH_SPL_OFFSET        0x001000
#define FLASH_PARTITION_A        0x009000
#define FLASH_PARTITION_B        0x049000
#define FLASH_FP_TEMPLATES      0x089000
#define FLASH_BLE_BONDING       0x099000
#define FLASH_CTAP2_CREDENTIALS 0x0A9000
#define FLASH_OTA_STAGING       0x1A9000

/**
 * Initialize SPI2 and MX25R1635F flash
 * @return 0 on success, negative on error
 */
int spi_flash_init(void);

/**
 * Read JEDEC ID
 * @param id_out Buffer for 3-byte JEDEC ID
 * @return 0 on success, negative on error
 */
int spi_flash_read_id(uint8_t id_out[3]);

/**
 * Read status register
 * @return Status register value, negative on error
 */
int spi_flash_read_status(void);

/**
 * Read data from flash (standard SPI mode)
 * @param addr 24-bit address
 * @param buf Output buffer
 * @param len Number of bytes to read
 * @return 0 on success, negative on error
 */
int spi_flash_read(uint32_t addr, uint8_t *buf, size_t len);

/**
 * Write a page (up to 256 bytes) to flash
 * @param addr 24-bit address (must be page-aligned)
 * @param data Data to write
 * @param len Number of bytes (1-256)
 * @return 0 on success, negative on error
 */
int spi_flash_page_program(uint32_t addr, const uint8_t *data, size_t len);

/**
 * Erase a 4KB sector
 * @param addr Address within the sector to erase
 * @return 0 on success, negative on error
 */
int spi_flash_sector_erase(uint32_t addr);

/**
 * Erase a 64KB block
 * @param addr Address within the block to erase
 * @return 0 on success, negative on error
 */
int spi_flash_block_erase(uint32_t addr);

/**
 * Erase entire chip
 * @return 0 on success, negative on error
 */
int spi_flash_chip_erase(void);

/**
 * Wait for flash operation to complete (busy-wait)
 * @param timeout_ms Maximum time to wait
 * @return 0 on success (operation completed), negative on timeout
 */
int spi_flash_wait_busy(uint32_t timeout_ms);

/**
 * Write enable command
 * @return 0 on success, negative on error
 */
int spi_flash_write_enable(void);

/**
 * Enter deep power-down mode
 * @return 0 on success, negative on error
 */
int spi_flash_deep_powerdown(void);

/**
 * Exit deep power-down mode
 * @return 0 on success, negative on error
 */
int spi_flash_wakeup(void);

#endif /* SPI_FLASH_H */