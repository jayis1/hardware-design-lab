// spi_flash.c — W25Q128JVSIQ SPI NOR Flash Driver
// 128 Mbit (16 MB) SPI NOR flash for FPGA bitstream and firmware storage
// Supports 4 KB sector erase, 64 KB block erase, page program (256 bytes)
// Connected to dedicated SPI master at 0x20005000

#include "spi_flash.h"
#include "../registers.h"
#include "../board.h"

static volatile uint32_t *spi_base = (volatile uint32_t *)SPI_FLASH_BASE;
static bool driver_initialized = false;

// W25Q128JVSIQ command set
#define FLASH_CMD_WRITE_ENABLE      0x06
#define FLASH_CMD_WRITE_DISABLE     0x04
#define FLASH_CMD_READ_STATUS1      0x05
#define FLASH_CMD_READ_STATUS2      0x35
#define FLASH_CMD_READ_STATUS3      0x15
#define FLASH_CMD_READ_DATA         0x03
#define FLASH_CMD_READ_FAST         0x0B
#define FLASH_CMD_PAGE_PROGRAM      0x02
#define FLASH_CMD_SECTOR_ERASE      0x20
#define FLASH_CMD_BLOCK_ERASE_32K   0x52
#define FLASH_CMD_BLOCK_ERASE_64K   0xD8
#define FLASH_CMD_CHIP_ERASE        0xC7
#define FLASH_CMD_READ_ID           0x9F
#define FLASH_CMD_READ_UNIQUE_ID    0x4B
#define FLASH_CMD_POWER_DOWN        0xB9
#define FLASH_CMD_RELEASE_POWERDOWN 0xAB

// Status register bit definitions
#define FLASH_STATUS_BUSY           (1 << 0)
#define FLASH_STATUS_WEL            (1 << 1)  // Write enable latch
#define FLASH_STATUS_BP0            (1 << 2)
#define FLASH_STATUS_BP1            (1 << 3)
#define FLASH_STATUS_BP2            (1 << 4)
#define FLASH_STATUS_TB             (1 << 5)
#define FLASH_STATUS_SEC            (1 << 6)
#define FLASH_STATUS_SRP0           (1 << 7)

// Expected JEDEC ID for W25Q128JV
#define FLASH_MANUFACTURER_WINBOND  0xEF
#define FLASH_DEVICE_ID_W25Q128JV   0x4018

// SPI transaction helper
static int spi_flash_transfer(const uint8_t *tx, uint8_t *rx, uint32_t len) {
    // Assert CS
    gpio_set(GPIO_SPI_FLASH_CS, 0);

    for (uint32_t i = 0; i < len; i++) {
        // Wait for TX ready
        uint32_t timeout = 10000;
        while (timeout > 0) {
            if (REG_READ(SPI_STATUS_REG(SPI_FLASH_BASE)) & SPI_STATUS_TX_EMPTY) break;
            timeout--;
        }
        if (timeout == 0) {
            gpio_set(GPIO_SPI_FLASH_CS, 1);
            return -1;
        }

        // Send byte
        uint32_t tx_val = (tx ? tx[i] : 0xFF) | SPI_TX_START;
        REG_WRITE(SPI_TX_REG(SPI_FLASH_BASE), tx_val);

        // Wait for RX ready
        timeout = 10000;
        while (timeout > 0) {
            if (REG_READ(SPI_STATUS_REG(SPI_FLASH_BASE)) & SPI_STATUS_RX_FULL) break;
            timeout--;
        }
        if (timeout == 0) {
            gpio_set(GPIO_SPI_FLASH_CS, 1);
            return -2;
        }

        // Read byte
        if (rx) {
            rx[i] = REG_READ(SPI_RX_REG(SPI_FLASH_BASE)) & 0xFF;
        } else {
            REG_READ(SPI_RX_REG(SPI_FLASH_BASE));  // Discard
        }
    }

    gpio_set(GPIO_SPI_FLASH_CS, 1);
    return 0;
}

// Send command only (no data)
static int spi_flash_command(uint8_t cmd) {
    return spi_flash_transfer(&cmd, NULL, 1);
}

// Write enable
static int spi_flash_write_enable(void) {
    int ret = spi_flash_command(FLASH_CMD_WRITE_ENABLE);
    if (ret != 0) return ret;

    // Verify WEL bit
    uint8_t status;
    ret = spi_flash_get_status(&status);
    if (ret != 0) return ret;
    if (!(status & FLASH_STATUS_WEL)) return -3;

    return 0;
}

// Wait for flash to be not busy
static int spi_flash_wait_ready(uint32_t timeout_ms) {
    uint32_t timeout = timeout_ms * 100000;
    while (timeout > 0) {
        uint8_t status;
        int ret = spi_flash_get_status(&status);
        if (ret != 0) return ret;
        if (!(status & FLASH_STATUS_BUSY)) return 0;
        timeout--;
    }
    return -4;
}

int spi_flash_init(void) {
    if (driver_initialized) return 0;

    // Configure SPI for flash: CPOL=0, CPHA=0, 25 MHz
    uint32_t spi_ctrl = SPI_CTRL_ENABLE;
    REG_WRITE(SPI_CTRL_REG(SPI_FLASH_BASE), spi_ctrl);

    // Clock divider: 100 MHz / 4 = 25 MHz
    REG_WRITE(SPI_DIV_REG(SPI_FLASH_BASE), 4);

    // De-assert CS
    gpio_set(GPIO_SPI_FLASH_CS, 1);

    // Verify flash is present
    uint8_t manufacturer;
    uint16_t device_id;
    int ret = spi_flash_get_id(&manufacturer, &device_id);
    if (ret != 0) return -1;
    if (manufacturer != FLASH_MANUFACTURER_WINBOND) return -2;
    if (device_id != FLASH_DEVICE_ID_W25Q128JV) return -3;

    // Release from power-down if needed
    spi_flash_command(FLASH_CMD_RELEASE_POWERDOWN);

    driver_initialized = true;
    return 0;
}

int spi_flash_read(uint32_t addr, uint8_t *data, uint32_t len) {
    if (!driver_initialized) return -1;
    if (addr + len > SPI_FLASH_SIZE_BYTES) return -2;

    uint8_t cmd[4] = {
        FLASH_CMD_READ_DATA,
        (uint8_t)((addr >> 16) & 0xFF),
        (uint8_t)((addr >> 8) & 0xFF),
        (uint8_t)(addr & 0xFF)
    };

    // Send command + address
    gpio_set(GPIO_SPI_FLASH_CS, 0);
    int ret = spi_flash_transfer(cmd, NULL, 4);
    if (ret != 0) {
        gpio_set(GPIO_SPI_FLASH_CS, 1);
        return ret;
    }

    // Read data
    ret = spi_flash_transfer(NULL, data, len);
    gpio_set(GPIO_SPI_FLASH_CS, 1);

    return ret;
}

int spi_flash_write(uint32_t addr, const uint8_t *data, uint32_t len) {
    if (!driver_initialized) return -1;
    if (addr + len > SPI_FLASH_SIZE_BYTES) return -2;

    uint32_t offset = 0;
    while (offset < len) {
        // Calculate bytes in this page
        uint32_t page_offset = addr & (SPI_FLASH_PAGE_SIZE - 1);
        uint32_t bytes_this_page = SPI_FLASH_PAGE_SIZE - page_offset;
        uint32_t remaining = len - offset;
        uint32_t chunk = (bytes_this_page < remaining) ? bytes_this_page : remaining;

        // Write enable
        int ret = spi_flash_write_enable();
        if (ret != 0) return ret;

        // Send page program command
        uint8_t cmd[4] = {
            FLASH_CMD_PAGE_PROGRAM,
            (uint8_t)(((addr + offset) >> 16) & 0xFF),
            (uint8_t)(((addr + offset) >> 8) & 0xFF),
            (uint8_t)((addr + offset) & 0xFF)
        };

        gpio_set(GPIO_SPI_FLASH_CS, 0);
        ret = spi_flash_transfer(cmd, NULL, 4);
        if (ret != 0) {
            gpio_set(GPIO_SPI_FLASH_CS, 1);
            return ret;
        }

        // Write data chunk
        ret = spi_flash_transfer(data + offset, NULL, chunk);
        gpio_set(GPIO_SPI_FLASH_CS, 1);
        if (ret != 0) return ret;

        // Wait for write to complete
        ret = spi_flash_wait_ready(10);
        if (ret != 0) return ret;

        offset += chunk;
    }

    return 0;
}

int spi_flash_erase_sector(uint32_t addr) {
    if (!driver_initialized) return -1;
    if (addr % SPI_FLASH_SECTOR_SIZE != 0) return -2;

    int ret = spi_flash_write_enable();
    if (ret != 0) return ret;

    uint8_t cmd[4] = {
        FLASH_CMD_SECTOR_ERASE,
        (uint8_t)((addr >> 16) & 0xFF),
        (uint8_t)((addr >> 8) & 0xFF),
        (uint8_t)(addr & 0xFF)
    };

    ret = spi_flash_transfer(cmd, NULL, 4);
    if (ret != 0) return ret;

    return spi_flash_wait_ready(400);  // Sector erase: ~400 ms max
}

int spi_flash_erase_block(uint32_t addr) {
    if (!driver_initialized) return -1;
    if (addr % SPI_FLASH_BLOCK_SIZE != 0) return -2;

    int ret = spi_flash_write_enable();
    if (ret != 0) return ret;

    uint8_t cmd[4] = {
        FLASH_CMD_BLOCK_ERASE_64K,
        (uint8_t)((addr >> 16) & 0xFF),
        (uint8_t)((addr >> 8) & 0xFF),
        (uint8_t)(addr & 0xFF)
    };

    ret = spi_flash_transfer(cmd, NULL, 4);
    if (ret != 0) return ret;

    return spi_flash_wait_ready(2000);  // Block erase: ~2 seconds max
}

int spi_flash_erase_chip(void) {
    if (!driver_initialized) return -1;

    int ret = spi_flash_write_enable();
    if (ret != 0) return ret;

    ret = spi_flash_command(FLASH_CMD_CHIP_ERASE);
    if (ret != 0) return ret;

    return spi_flash_wait_ready(80000);  // Chip erase: ~80 seconds max
}

int spi_flash_get_id(uint8_t *manufacturer, uint16_t *device_id) {
    if (!driver_initialized) return -1;

    uint8_t cmd = FLASH_CMD_READ_ID;
    uint8_t rx[3];

    int ret = spi_flash_transfer(&cmd, NULL, 1);
    if (ret != 0) return ret;

    ret = spi_flash_transfer(NULL, rx, 3);
    if (ret != 0) return ret;

    *manufacturer = rx[0];
    *device_id = ((uint16_t)rx[1] << 8) | rx[2];

    return 0;
}

int spi_flash_get_status(uint8_t *status) {
    uint8_t cmd = FLASH_CMD_READ_STATUS1;
    return spi_flash_transfer(&cmd, status, 2);
}

int spi_flash_wait_busy(uint32_t timeout_ms) {
    return spi_flash_wait_ready(timeout_ms);
}
