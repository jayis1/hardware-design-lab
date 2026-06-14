// ============================================================================
// drivers/spi_flash.c — W25Q128 SPI Flash Driver Implementation
// ============================================================================

#include "spi_flash.h"
#include "registers.h"
#include "board.h"

// ---- SPI Flash CS ----
static void flash_cs_low(void)  { FLASH_CS_LOW(); }
static void flash_cs_high(void) { FLASH_CS_HIGH(); }

// ---- SPI Transfer ----
static uint8_t flash_spi_xfer(uint8_t tx) {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = tx;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)SPI1->DR;
}

void spi_flash_init(void) {
    uint32_t id;
    if (spi_flash_read_jedec_id(&id) == 0) {
        if (id != W25Q128_JEDEC_ID) {
            // Flash not detected or wrong part
            // Set fault flag in production
        }
    }
}

int spi_flash_read_jedec_id(uint32_t *id) {
    flash_cs_low();
    flash_spi_xfer(W25Q_CMD_JEDEC_ID);
    uint8_t mfr  = flash_spi_xfer(0xFF);
    uint8_t type = flash_spi_xfer(0xFF);
    uint8_t cap  = flash_spi_xfer(0xFF);
    flash_cs_high();
    
    *id = ((uint32_t)mfr << 16) | ((uint32_t)type << 8) | cap;
    return 0;
}

int spi_flash_wait_busy(void) {
    flash_cs_low();
    flash_spi_xfer(W25Q_CMD_READ_STATUS1);
    uint8_t status;
    int timeout = 100000;
    do {
        status = flash_spi_xfer(0xFF);
        if (--timeout == 0) {
            flash_cs_high();
            return -1;  // Timeout
        }
    } while (status & W25Q_STATUS1_BUSY);
    flash_cs_high();
    return 0;
}

int spi_flash_read(uint32_t addr, uint8_t *buf, uint16_t len) {
    if (spi_flash_wait_busy() < 0) return -1;
    
    flash_cs_low();
    flash_spi_xfer(W25Q_CMD_READ_DATA);
    flash_spi_xfer((addr >> 16) & 0xFF);
    flash_spi_xfer((addr >> 8) & 0xFF);
    flash_spi_xfer(addr & 0xFF);
    
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = flash_spi_xfer(0xFF);
    }
    flash_cs_high();
    return 0;
}

int spi_flash_write_page(uint32_t addr, const uint8_t *buf, uint16_t len) {
    // Page program: max 256 bytes per page, must not cross page boundary
    if (len > 256) return -1;
    if (spi_flash_wait_busy() < 0) return -1;
    
    // Write enable
    flash_cs_low();
    flash_spi_xfer(W25Q_CMD_WRITE_ENABLE);
    flash_cs_high();
    
    // Page program
    flash_cs_low();
    flash_spi_xfer(W25Q_CMD_PAGE_PROGRAM);
    flash_spi_xfer((addr >> 16) & 0xFF);
    flash_spi_xfer((addr >> 8) & 0xFF);
    flash_spi_xfer(addr & 0xFF);
    
    for (uint16_t i = 0; i < len; i++) {
        flash_spi_xfer(buf[i]);
    }
    flash_cs_high();
    
    return spi_flash_wait_busy();
}

int spi_flash_erase_sector(uint32_t sector) {
    uint32_t addr = sector * 4096;
    if (spi_flash_wait_busy() < 0) return -1;
    
    // Write enable
    flash_cs_low();
    flash_spi_xfer(W25Q_CMD_WRITE_ENABLE);
    flash_cs_high();
    
    // Sector erase
    flash_cs_low();
    flash_spi_xfer(W25Q_CMD_SECTOR_ERASE);
    flash_spi_xfer((addr >> 16) & 0xFF);
    flash_spi_xfer((addr >> 8) & 0xFF);
    flash_spi_xfer(addr & 0xFF);
    flash_cs_high();
    
    return spi_flash_wait_busy();
}

int spi_flash_read_config(flash_config_t *cfg) {
    if (spi_flash_read(0, (uint8_t *)cfg, sizeof(flash_config_t)) < 0)
        return -1;
    
    // Validate magic and CRC
    if (cfg->magic != FLASH_CONFIG_MAGIC)
        return -2;  // Invalid config
    
    // Simple CRC32 check (production: use proper CRC32)
    uint32_t calc_crc = 0;
    const uint8_t *p = (const uint8_t *)cfg;
    for (uint16_t i = 0; i < sizeof(flash_config_t) - 4; i++) {
        calc_crc ^= p[i];
        for (int j = 0; j < 8; j++) {
            if (calc_crc & 1)
                calc_crc = (calc_crc >> 1) ^ 0xEDB88320;
            else
                calc_crc >>= 1;
        }
    }
    
    if (calc_crc != cfg->crc32)
        return -3;  // CRC mismatch
    
    return 0;
}

int spi_flash_write_config(const flash_config_t *cfg) {
    // Erase sector 0, then write config
    if (spi_flash_erase_sector(0) < 0) return -1;
    
    // Compute CRC32
    flash_config_t tmp = *cfg;
    tmp.magic = FLASH_CONFIG_MAGIC;
    tmp.crc32 = 0;
    
    uint32_t calc_crc = 0;
    const uint8_t *p = (const uint8_t *)&tmp;
    for (uint16_t i = 0; i < sizeof(flash_config_t) - 4; i++) {
        calc_crc ^= p[i];
        for (int j = 0; j < 8; j++) {
            if (calc_crc & 1)
                calc_crc = (calc_crc >> 1) ^ 0xEDB88320;
            else
                calc_crc >>= 1;
        }
    }
    tmp.crc32 = calc_crc;
    
    return spi_flash_write_page(0, (const uint8_t *)&tmp, sizeof(flash_config_t));
}