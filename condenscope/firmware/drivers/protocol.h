/* CondenScope transport protocol
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef CONDENSCOPE_PROTOCOL_H
#define CONDENSCOPE_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "sensors.h"

typedef struct {
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint16_t sequence;
    uint16_t payload_length;
} protocol_header_t;

typedef struct {
    uint8_t buffer[USB_FRAME_MTU];
    uint16_t used;
    uint16_t expected;
    uint16_t tx_sequence;
    uint16_t rx_sequence;
    uint32_t crc_errors;
    uint32_t frame_errors;
} protocol_context_t;

typedef struct {
    uint8_t type;
    const uint8_t *payload;
    uint16_t length;
    uint16_t sequence;
} protocol_message_t;

typedef enum {
    PROTOCOL_NONE = 0,
    PROTOCOL_MESSAGE = 1,
    PROTOCOL_BAD_FRAME = -1,
    PROTOCOL_OVERFLOW = -2
} protocol_result_t;

void protocol_init(protocol_context_t *context);
protocol_result_t protocol_consume(protocol_context_t *context,
                                   const uint8_t *bytes, uint16_t length,
                                   protocol_message_t *message);
uint16_t protocol_encode(protocol_context_t *context, uint8_t type,
                         const void *payload, uint16_t payload_length,
                         uint8_t *output, uint16_t capacity);
uint16_t protocol_crc16(const uint8_t *data, uint16_t length);
uint16_t protocol_send_status(protocol_context_t *context,
                              const environment_t *environment,
                              uint8_t state, uint8_t *output,
                              uint16_t capacity);
uint16_t protocol_send_thermal(protocol_context_t *context,
                               const thermal_frame_t *frame,
                               uint8_t *output, uint16_t capacity);
uint16_t protocol_send_point(protocol_context_t *context,
                             const map_point_t *point,
                             uint8_t *output, uint16_t capacity);

#endif
