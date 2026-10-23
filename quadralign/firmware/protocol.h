/* SPDX-License-Identifier: MIT
 * QuadraAlign BLE serial protocol
 * Author: jayis1
 */
#ifndef QUADRALIGN_PROTOCOL_H
#define QUADRALIGN_PROTOCOL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "sensors.h"

typedef enum { EVENT_NONE, EVENT_TARE, EVENT_CAPTURE, EVENT_SET_LIMITS, EVENT_CALIBRATE, EVENT_SLEEP } command_kind_t;
typedef struct { command_kind_t kind; uint8_t channel; float value_a; float value_b; } qa_command_t;
void protocol_init(void);
void protocol_poll(void);
bool protocol_next_command(qa_command_t *command);
bool protocol_send_measurement(const qa_measurement_t *measurement, uint16_t battery_mv);
bool protocol_send_event(uint8_t code, const char *message);
uint16_t protocol_crc16(const uint8_t *data, size_t length);
#endif
