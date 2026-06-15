// spi_flash.c — QSPI Flash Driver for Aether-Link
// Manages the MT25QU512 512Mb QSPI flash for FPGA configuration storage
// and firmware updates. Provides read, write, erase, and status functions.
// Uses the Xilinx AXI Quad SPI controller in quad I/O mode.

#include "../registers.h"
#include "../board.h"
#include <string.h>

// SPI controller base
static spi_regs_t * const spi = (spi_regs_t *)SPI_MASTER_BASE;

// MT25QU512 commands
#define MT25Q_CMD_WREN          0x06  // Write Enable
#define MT25Q_CMD_WRDI          0x04  // Write Disable
#define MT25Q_CMD_RDID          0x9F  // Read ID
#define MT25Q_CMD_RDSR          0x05  // Read Status Register
#define MT25Q_CMD_WRSR          0x01  // Write Status Register
#define MT25Q_CMD_READ          0x03  // Read (standard SPI, up to 50 MHz)
#define MT25Q_CMD_FAST_READ     0x0B  // Fast Read (up to 133 MHz)
#define MT25Q_CMD_4READ         0x13  // 4-byte address Read
#define MT25Q_CMD_4FAST_READ    0x0C  // 4-byte address Fast Read
#define MT25Q_CMD_4QREAD        0x6C  // 4-byte Quad I/O Read
#define MT25Q_CMD_4QFAST_READ   0xEC  // 4-byte Quad I/O Fast Read (133 MHz)
#define MT25Q_CMD_PP            0x02  // Page Program (up to 256 bytes)
#define MT25Q_CMD_4PP           0x12  // 4-byte address Page Program
#define MT25Q_CMD_4QPP          0x34  // 4-byte Quad Page Program
#define MT25Q_CMD_SE            0xD8  // Sector Erase (64KB)
#define MT25Q_CMD_4SE           0xDC  // 4-byte Sector Erase
#define MT25Q_CMD_BE            0xC7  // Bulk Erase
#define MT25Q_CMD_RSTEN         0x66  // Reset Enable
#define MT25Q_CMD_RST           0x99  // Reset Device
#define MT25Q_CMD_ENTER_4BYTE   0xB7  // Enter 4-byte addressing mode
#define MT25Q_CMD_EXIT_4BYTE    0xE9  // Exit 4-byte addressing mode

// Status register bits
#define MT25Q_SR_WIP            0x01  // Write In Progress
#define MT25Q_SR_WEL            0x02  // Write Enable Latch
#define MT25Q_SR_BP0            0x04  // Block Protect 0
#define MT25Q_SR_BP1            0x08  // Block Protect 1
#define MT25Q_SR_BP2            0x10  // Block Protect 2
#define MT25Q_SR_TB             0x20  // Top/Bottom protect
#define MT25Q_SR_SEC            0x40  // Sector/Block protect
#define MT25Q_SR_SRP0           0x80  // Status Register Protect 0

// Flash geometry
#define MT25Q_SECTOR_SIZE       65536   // 64KB
#define MT25Q_PAGE_SIZE         256     // 256 bytes
#define MT25Q_TOTAL_SIZE        67108864 // 64MB (512Mb)
#define MT25Q_GOLDEN_OFFSET     0x02000000 // 32MB offset for golden image

// ============================================================================
// Low-Level SPI Operations
// ============================================================================

static void spi_wait_tx_empty(void) {
    while (spi->SPI_TX_FIFO_OCY != 0);  // Wait for TX FIFO to empty
}

static void spi_wait_rx_data(void) {
    while (spi->SPI_RX_FIFO_OCY == 0);  // Wait for RX data
}

static void spi_write_byte(uint8_t data) {
    spi_wait_tx_empty();
    spi->SPI_DTR = data;
}

static uint8_t spi_read_byte(void) {
    spi_write_byte(0xFF);  // Dummy byte to clock in data
    spi_wait_rx_data();
    return (uint8_t)(spi->SPI_DRR & 0xFF);
}

static void spi_select(void) {
    spi->SPISSR = 0x00;  // Assert SS (active low)
}

static void spi_deselect(void) {
    spi->SPISSR = 0x01;  // Deassert SS
}

// ============================================================================
// Flash Command Helpers
// ============================================================================

static void flash_write_enable(void) {
    spi_select();
    spi_write_byte(MT25Q_CMD_WREN);
    spi_deselect();
}

static uint8_t flash_read_status(void) {
    uint8_t sr;
    spi_select();
    spi_write_byte(MT25Q_CMD_RDSR);
    sr = spi_read_byte();
    spi_deselect();
    return sr;
}

static void flash_wait_ready(void) {
    while (flash_read_status() & MT25Q_SR_WIP) {
        // Spin-wait for write completion
        // Typical page program: ~0.5ms, sector erase: ~300ms, bulk erase: ~100s
        for (volatile int i = 0; i < 1000; i++);
    }
}

static void flash_enter_4byte_mode(void) {
    spi_select();
    spi_write_byte(MT25Q_CMD_ENTER_4BYTE);
    spi_deselect();
}

// ============================================================================
// Initialization
// ============================================================================

void spi_flash_init(void) {
    // Reset SPI controller
    spi->SRR = 0x0000000A;  // Software reset
    for (volatile int i = 0; i < 1000; i++);

    // Configure for quad SPI master mode
    // Clock: 50 MHz (from 200 MHz system clock / 4)
    // CPOL=0, CPHA=0 (SPI mode 0)
    spi->SPICR = SPICR_MASTER | SPICR_SPE | SPICR_QUAD |
                 SPICR_TXFIFO_RST | SPICR_RXFIFO_RST;
    for (volatile int i = 0; i < 100; i++);

    // Enable SPI
    spi->SPICR = SPICR_MASTER | SPICR_SPE | SPICR_QUAD;

    // Deselect flash initially
    spi->SPISSR = 0x01;

    // Reset flash device
    spi_select();
    spi_write_byte(MT25Q_CMD_RSTEN);
    spi_deselect();
    spi_select();
    spi_write_byte(MT25Q_CMD_RST);
    spi_deselect();

    // Wait for reset to complete
    for (volatile int i = 0; i < 100000; i++);  // ~500µs

    // Enter 4-byte addressing mode (required for >16MB addressing)
    flash_enter_4byte_mode();
}

// ============================================================================
// Flash Identification
// ============================================================================

void spi_flash_read_id(uint8_t *manufacturer_id, uint16_t *device_id) {
    spi_select();
    spi_write_byte(MT25Q_CMD_RDID);
    *manufacturer_id = spi_read_byte();  // Should be 0x20 (Micron)
    uint8_t id_hi = spi_read_byte();     // Device ID high byte
    uint8_t id_lo = spi_read_byte();     // Device ID low byte
    spi_deselect();

    *device_id = ((uint16_t)id_hi << 8) | id_lo;
    // MT25QU512: manufacturer=0x20, device=0xBB20
}

// ============================================================================
// Flash Read Operations
// ============================================================================

int spi_flash_read(uint32_t addr, uint8_t *buffer, uint32_t len) {
    if (addr + len > MT25Q_TOTAL_SIZE) return -1;
    if (buffer == NULL || len == 0) return -2;

    flash_wait_ready();

    spi_select();
    // Use 4-byte Quad I/O Fast Read for maximum throughput
    spi_write_byte(MT25Q_CMD_4QFAST_READ);
    // Send 4-byte address (MSB first)
    spi_write_byte((addr >> 24) & 0xFF);
    spi_write_byte((addr >> 16) & 0xFF);
    spi_write_byte((addr >> 8) & 0xFF);
    spi_write_byte(addr & 0xFF);
    // Dummy cycles (8 for 133 MHz, we use 10 for safety at 50 MHz)
    for (int i = 0; i < 10; i++) spi_write_byte(0x00);

    // Read data in quad mode (hardware handles quad I/O)
    for (uint32_t i = 0; i < len; i++) {
        buffer[i] = spi_read_byte();
    }
    spi_deselect();

    return (int)len;
}

// ============================================================================
// Flash Write Operations
// ============================================================================

int spi_flash_page_program(uint32_t addr, const uint8_t *data, uint32_t len) {
    if (addr + len > MT25Q_TOTAL_SIZE) return -1;
    if (len > MT25Q_PAGE_SIZE) return -3;  // Cannot cross page boundary
    if (data == NULL || len == 0) return -2;

    flash_wait_ready();
    flash_write_enable();

    spi_select();
    spi_write_byte(MT25Q_CMD_4PP);  // 4-byte Page Program
    spi_write_byte((addr >> 24) & 0xFF);
    spi_write_byte((addr >> 16) & 0xFF);
    spi_write_byte((addr >> 8) & 0xFF);
    spi_write_byte(addr & 0xFF);

    for (uint32_t i = 0; i < len; i++) {
        spi_write_byte(data[i]);
    }
    spi_deselect();

    flash_wait_ready();
    return (int)len;
}

int spi_flash_write(uint32_t addr, const uint8_t *data, uint32_t len) {
    uint32_t remaining = len;
    uint32_t offset = 0;

    while (remaining > 0) {
        // Calculate bytes remaining in current page
        uint32_t page_offset = addr & (MT25Q_PAGE_SIZE - 1);
        uint32_t chunk = MT25Q_PAGE_SIZE - page_offset;
        if (chunk > remaining) chunk = remaining;

        int ret = spi_flash_page_program(addr, data + offset, chunk);
        if (ret < 0) return ret;

        addr += chunk;
        offset += chunk;
        remaining -= chunk;
    }

    return (int)len;
}

// ============================================================================
// Flash Erase Operations
// ============================================================================

int spi_flash_sector_erase(uint32_t addr) {
    if (addr >= MT25Q_TOTAL_SIZE) return -1;
    // Align to sector boundary
    addr &= ~(MT25Q_SECTOR_SIZE - 1);

    flash_wait_ready();
    flash_write_enable();

    spi_select();
    spi_write_byte(MT25Q_CMD_4SE);  // 4-byte Sector Erase
    spi_write_byte((addr >> 24) & 0xFF);
    spi_write_byte((addr >> 16) & 0xFF);
    spi_write_byte((addr >> 8) & 0xFF);
    spi_write_byte(addr & 0xFF);
    spi_deselect();

    flash_wait_ready();  // Sector erase takes ~300ms
    return 0;
}

int spi_flash_bulk_erase(void) {
    flash_wait_ready();
    flash_write_enable();

    spi_select();
    spi_write_byte(MT25Q_CMD_BE);
    spi_deselect();

    flash_wait_ready();  // Bulk erase takes ~100 seconds!
    return 0;
}

// ============================================================================
// Firmware Update Support
// ============================================================================

// Write a new FPGA bitstream to flash at the primary image location (offset 0)
int spi_flash_update_bitstream(const uint8_t *bitstream, uint32_t len) {
    if (len > MT25Q_GOLDEN_OFFSET) return -1;  // Don't overwrite golden image

    // Erase sectors from 0 to len
    for (uint32_t addr = 0; addr < len; addr += MT25Q_SECTOR_SIZE) {
        int ret = spi_flash_sector_erase(addr);
        if (ret < 0) return ret;
    }

    // Write new bitstream
    return spi_flash_write(0, bitstream, len);
}

// Verify flash contents against a buffer
int spi_flash_verify(uint32_t addr, const uint8_t *expected, uint32_t len) {
    uint8_t readback[256];
    uint32_t remaining = len;
    uint32_t offset = 0;

    while (remaining > 0) {
        uint32_t chunk = (remaining > 256) ? 256 : remaining;
        int ret = spi_flash_read(addr + offset, readback, chunk);
        if (ret < 0) return ret;

        if (memcmp(readback, expected + offset, chunk) != 0) {
            return -10;  // Verification failed
        }

        offset += chunk;
        remaining -= chunk;
    }

    return 0;  // Verification passed
}

// ============================================================================
// Flash Protection
// ============================================================================

void spi_flash_lock_golden_image(void) {
    // Protect the golden image region (32MB-64MB) using block protection bits
    // Set BP2=1, BP1=1, BP0=1 → protect top 1/2 (32MB)
    flash_write_enable();
    spi_select();
    spi_write_byte(MT25Q_CMD_WRSR);
    spi_write_byte(MT25Q_SR_BP2 | MT25Q_SR_BP1 | MT25Q_SR_BP0 | MT25Q_SR_SRP0);
    spi_deselect();
    flash_wait_ready();
}

void spi_flash_unlock_all(void) {
    flash_write_enable();
    spi_select();
    spi_write_byte(MT25Q_CMD_WRSR);
    spi_write_byte(0x00);  // Clear all block protect bits
    spi_deselect();
    flash_wait_ready();
}
