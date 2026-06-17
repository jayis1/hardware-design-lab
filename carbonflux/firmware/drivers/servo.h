/**
 * @file    servo.h
 * @brief   SG90 micro servo driver interface.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef SERVO_H
#define SERVO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void     servo_init(void);
void     servo_set_position(uint16_t pulse_us);
uint16_t servo_get_position(void);
bool     servo_is_stalled(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVO_H */