// ============================================================================
// drivers/spi_flash.h — W25Q128 SPI Flash Driver
// ============================================================================

#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include <stdint.h>

// W25Q128 commands
#define W25Q_CMD_WRITE_ENABLE      0x06
#define W25Q_CMD_WRITE_DISABLE     0x04
#define W25Q_CMD_READ_STATUS1      0x05
#define W25Q_CMD_READ_STATUS2      0x35
#define W25Q_CMD_READ_STATUS3      0x15
#define W25Q_CMD_WRITE_STATUS1     0x01
#define W25Q_CMD_PAGE_PROGRAM      0x02
#define W25Q_CMD_READ_DATA         0x03
#define W25Q_CMD_FAST_READ         0x0B
#define W25Q_CMD_SECTOR_ERASE      0x20
#define W25Q_CMD_BLOCK_ERASE_32K   0x52
#define W25Q_CMD_BLOCK_ERASE_64K   0xD8
#define W25Q_CMD_CHIP_ERASE        0xC7
#define W25Q_CMD_POWER_DOWN       0xB9
#define W25Q_CMD_RELEASE_POWERDOWN 0xAB
#define W25Q_CMD_JEDEC_ID         0x9F
#define W25Q_CMD_READ_UID         0x4B

// Status register bits
#define W25Q_STATUS1_BUSY          (1 << 0)
#define W25Q_STATUS1_WEL           (1 << 1)
#define W25Q_STATUS2_QE            (1 << 1)
#define W25Q_STATUS3_WPS           (1 << 2)

// Flash layout for RideCore
#define FLASH_CONFIG_SECTOR        0     // Sector 0 (4 KB): config params
#define FLASH_LOG_SECTOR_START     1     // Sectors 1-3: log buffer
#define FLASH_FW_BACKUP_SECTOR     8     // Sector 8+: firmware backup

// Config structure stored at sector 0
typedef struct {
    uint32_t magic;           // 0xRIDECORE
    uint16_t version;         // Config version
    uint16_t pole_pairs;
    uint32_t phase_r_mohm;
    uint32_t phase_l_uh;
    uint32_t max_current_ma;
    uint32_t cont_current_ma;
    uint32_t pwm_freq_hz;
    uint32_t deadtime_ns;
    float    kp_d, ki_d;
    float    kp_q, ki_q;
    float    kp_speed, ki_speed;
    uint8_t  can_node_id;
    uint8_t  hall_polarity;
    uint8_t  sensorless_mode;
    uint8_t  reserved[24];
    uint32_t crc32;
} flash_config_t;

#define FLASH_CONFIG_MAGIC  0x52494445  // "RIDE"

void spi_flash_init(void);
int spi_flash_read_jedec_id(uint32_t *id);
int spi_flash_read(uint32_t addr, uint8_t *buf, uint16_t len);
int spi_flash_write_page(uint32_t addr, const uint8_t *buf, uint16_t len);
int spi_flash_erase_sector(uint32_t sector);
int spi_flash_wait_busy(void);
int spi_flash_read_config(flash_config_t *cfg);
int spi_flash_write_config(const flash_config_t *cfg);

#endif // SPI_FLASH_H