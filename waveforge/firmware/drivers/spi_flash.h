/* ============================================
 * spi_flash.h — WaveForge SPI Flash Driver
 * W25Q256JVEIQ, 32 MB QSPI Flash
 * ============================================ */

#ifndef WAVEFORGE_SPI_FLASH_H
#define WAVEFORGE_SPI_FLASH_H

#include <stdint.h>

/* Initialize SPI1 peripheral for flash access */
void SPI_Flash_Init(void);

/* Read JEDEC ID (returns manufacturer + memory type + capacity) */
uint32_t SPI_Flash_ReadID(void);

/* Read data from flash
 * addr: 24-bit address
 * buf:  destination buffer
 * len:  number of bytes to read
 */
void SPI_Flash_Read(uint32_t addr, uint8_t *buf, uint32_t len);

/* Write a page (up to 256 bytes) to flash
 * addr: 24-bit address (must be page-aligned for best results)
 * buf:  source data
 * len:  number of bytes to write (1–256)
 * Returns: 0 on success, -1 on error
 */
int SPI_Flash_WritePage(uint32_t addr, const uint8_t *buf, uint16_t len);

/* Erase a 4KB sector */
void SPI_Flash_EraseSector(uint32_t addr);

/* Erase a 64KB block */
void SPI_Flash_EraseBlock64(uint32_t addr);

/* Erase entire chip */
void SPI_Flash_EraseChip(void);

/* Wait until flash is not busy (poll status register) */
void SPI_Flash_WaitBusy(void);

/* Enable write operations */
void SPI_Flash_WriteEnable(void);

/* Read status register */
uint8_t SPI_Flash_ReadStatus(void);

/* Check if write protect is active */
int SPI_Flash_IsWriteEnabled(void);

#endif /* WAVEFORGE_SPI_FLASH_H */