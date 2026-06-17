/**
 * @file    sx1262.h
 * @brief   SX1262 LoRa modem driver interface.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef SX1262_H
#define SX1262_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

int  sx1262_init(void);
int  sx1262_set_frequency(uint32_t freq_hz);
int  sx1262_lora_send(const uint8_t *data, uint8_t len, uint8_t fport, bool confirm);
void sx1262_sleep(void);
void sx1262_wake(void);

#ifdef __cplusplus
}
#endif

#endif /* SX1262_H */