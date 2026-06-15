/* ============================================
 * spi_flash.c — WaveForge SPI Flash Driver
 * W25Q256JVEIQ, 32 MB QSPI Flash
 * ============================================ */

#include "spi_flash.h"
#include "registers.h"
#include "board.h"

/* ---- SPI1 Transfer Helpers ---- */

static uint8_t SPI1_Transfer(uint8_t tx_byte) {
    /* Wait until TX buffer is empty */
    uint32_t timeout = 0;
    while (!(SPI1->SR & SPI_SR_TXE)) {
        if (timeout++ > 100000) return 0xFF;
    }

    /* Send byte */
    SPI1->DR = tx_byte;

    /* Wait until RX buffer is not empty */
    timeout = 0;
    while (!(SPI1->SR & SPI_SR_RXNE)) {
        if (timeout++ > 100000) return 0xFF;
    }

    /* Read received byte */
    return (uint8_t)(SPI1->DR & 0xFF);
}

static void SPI1_WaitBusy(void) {
    uint32_t timeout = 0;
    while (SPI1->SR & SPI_SR_BSY) {
        if (timeout++ > 100000) break;
    }
}

/* ---- Public API ---- */

void SPI_Flash_Init(void) {
    /* SPI1 already initialized in main.c */
    /* Ensure flash is not held or write-protected */
    FLASH_WP_DISABLE();
    FLASH_HOLD_HIGH();
    FLASH_CS_HIGH();

    /* Release from deep power-down */
    FLASH_CS_LOW();
    SPI1_Transfer(W25Q256_CMD_RELEASE_PWRDN);
    FLASH_CS_HIGH();

    /* Wait for flash to be ready */
    SPI_Flash_WaitBusy();
}

uint32_t SPI_Flash_ReadID(void) {
    uint32_t id = 0;

    FLASH_CS_LOW();
    SPI1_Transfer(W25Q256_CMD_READ_ID);
    id = SPI1_Transfer(0x00) << 16;   /* Manufacturer ID */
    id |= SPI1_Transfer(0x00) << 8;   /* Memory type */
    id |= SPI1_Transfer(0x00);         /* Capacity */
    FLASH_CS_HIGH();

    return id;
}

void SPI_Flash_Read(uint32_t addr, uint8_t *buf, uint32_t len) {
    FLASH_CS_LOW();
    SPI1_Transfer(W25Q256_CMD_FAST_READ);
    SPI1_Transfer((addr >> 16) & 0xFF);  /* Address [23:16] */
    SPI1_Transfer((addr >> 8) & 0xFF);   /* Address [15:8] */
    SPI1_Transfer(addr & 0xFF);           /* Address [7:0] */
    SPI1_Transfer(0x00);                  /* Dummy byte for fast read */

    for (uint32_t i = 0; i < len; i++) {
        buf[i] = SPI1_Transfer(0x00);
    }

    FLASH_CS_HIGH();
}

int SPI_Flash_WritePage(uint32_t addr, const uint8_t *buf, uint16_t len) {
    if (len > 256) return -1;

    /* Enable writes */
    SPI_Flash_WriteEnable();

    FLASH_CS_LOW();
    SPI1_Transfer(W25Q256_CMD_PAGE_PROGRAM);
    SPI1_Transfer((addr >> 16) & 0xFF);
    SPI1_Transfer((addr >> 8) & 0xFF);
    SPI1_Transfer(addr & 0xFF);

    for (uint16_t i = 0; i < len; i++) {
        SPI1_Transfer(buf[i]);
    }

    FLASH_CS_HIGH();

    /* Wait for write to complete */
    SPI_Flash_WaitBusy();

    return 0;
}

void SPI_Flash_EraseSector(uint32_t addr) {
    SPI_Flash_WriteEnable();

    FLASH_CS_LOW();
    SPI1_Transfer(W25Q256_CMD_SECTOR_ERASE);
    SPI1_Transfer((addr >> 16) & 0xFF);
    SPI1_Transfer((addr >> 8) & 0xFF);
    SPI1_Transfer(addr & 0xFF);
    FLASH_CS_HIGH();

    SPI_Flash_WaitBusy();
}

void SPI_Flash_EraseBlock64(uint32_t addr) {
    SPI_Flash_WriteEnable();

    FLASH_CS_LOW();
    SPI1_Transfer(W25Q256_CMD_BLOCK_ERASE_64K);
    SPI1_Transfer((addr >> 16) & 0xFF);
    SPI1_Transfer((addr >> 8) & 0xFF);
    SPI1_Transfer(addr & 0xFF);
    FLASH_CS_HIGH();

    SPI_Flash_WaitBusy();
}

void SPI_Flash_EraseChip(void) {
    SPI_Flash_WriteEnable();

    FLASH_CS_LOW();
    SPI1_Transfer(W25Q256_CMD_CHIP_ERASE);
    FLASH_CS_HIGH();

    SPI_Flash_WaitBusy();
}

void SPI_Flash_WaitBusy(void) {
    uint32_t timeout = 0;
    while (SPI_Flash_ReadStatus() & 0x01) {  /* WIP bit */
        if (timeout++ > 10000000) break;       /* ~10 seconds max */
    }
}

uint8_t SPI_Flash_ReadStatus(void) {
    uint8_t status;
    FLASH_CS_LOW();
    SPI1_Transfer(W25Q256_CMD_READ_STATUS);
    status = SPI1_Transfer(0x00);
    FLASH_CS_HIGH();
    return status;
}

void SPI_Flash_WriteEnable(void) {
    FLASH_CS_LOW();
    SPI1_Transfer(W25Q256_CMD_WRITE_ENABLE);
    FLASH_CS_HIGH();
}

int SPI_Flash_IsWriteEnabled(void) {
    return (SPI_Flash_ReadStatus() & 0x02) ? 1 : 0;  /* WEL bit */
}