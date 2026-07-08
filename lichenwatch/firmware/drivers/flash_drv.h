/*
 * flash_drv.h — W25Q80 SPI flash ring buffer for raw measurement records.
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef FLASH_DRV_H
#define FLASH_DRV_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;        /* STORAGE_MAGIC = 'LWN1'  */
    uint16_t seq;
    int16_t  fv_fm;        /* × 1000 */
    int16_t  lndvi;        /* × 1000 */
    uint16_t chl_idx;     /* × 100  */
    uint16_t wetness;     /* × 1000 */
    int16_t  co2;
    int8_t   temp;
    uint8_t  rh;
    uint8_t  uv;          /* UV index × 10 */
    uint16_t aclicks;
    uint16_t bat_mv;
    uint8_t  flags;
    uint8_t  class_state;
    uint8_t  reserved[32];
} flash_record_t;
#pragma pack(pop)

int  flash_init(void);
int  flash_append(const flash_record_t *rec);
int  flash_read_record(uint32_t index, flash_record_t *rec);
uint32_t flash_record_count(void);

#endif /* FLASH_DRV_H */