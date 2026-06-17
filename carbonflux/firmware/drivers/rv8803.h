/**
 * @file    rv8803.h
 * @brief   RV-8803 RTC driver interface.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef RV8803_H
#define RV8803_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int      rv8803_init(void);
int      rv8803_set_time(uint16_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t min, uint8_t s);
uint32_t rv8803_read_unix(void);
int      rv8803_set_alarm_s(uint32_t seconds_from_now);

#ifdef __cplusplus
}
#endif

#endif /* RV8803_H */