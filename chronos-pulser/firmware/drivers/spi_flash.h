// spi_flash.h — SPI Flash Driver Header (W25Q128JVSIQ)
#ifndef SPI_FLASH_H
#define SPI_FLASH_H
#include <stdint.h>

int  spi_flash_init(void);
int  spi_flash_read(uint32_t addr, uint8_t *data, uint32_t len);
int  spi_flash_write(uint32_t addr, const uint8_t *data, uint32_t len);
int  spi_flash_erase_sector(uint32_t addr);
int  spi_flash_erase_block(uint32_t addr);
int  spi_flash_erase_chip(void);
int  spi_flash_get_id(uint8_t *manufacturer, uint16_t *device_id);
int  spi_flash_get_status(uint8_t *status);
int  spi_flash_wait_busy(uint32_t timeout_ms);
#endif
