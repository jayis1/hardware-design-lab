/**
 * @file    w25q128.h
 * @brief   W25Q128 SPI flash and ring buffer log interface.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef W25Q128_H
#define W25Q128_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "flux_engine.h"

int      w25q128_init(void);
int      flash_log_init(void);
int      flash_log_write(const flux_record_t *record);
int      flash_log_read(uint32_t index, flux_record_t *record);
void     flash_log_dump(uint32_t max_entries);
void     flash_read_config(uint8_t *buf, uint32_t len);
uint32_t flash_log_get_head(void);

#ifdef __cplusplus
}
#endif

#endif /* W25Q128_H */