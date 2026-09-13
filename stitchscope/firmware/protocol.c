/* StitchScope bounded BLE framing protocol
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "stitchscope.h"
#include <string.h>

#define PREAMBLE_0 0x53u
#define PREAMBLE_1 0x53u
#define PROTOCOL_VERSION 0x01u
#define HEADER_SIZE 8u
#define CRC_SIZE 2u

uint16_t protocol_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0u; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0u; bit < 8u; ++bit) {
            crc = (crc & 0x8000u) != 0u ? (uint16_t)((crc << 1) ^ 0x1021u) :
                                         (uint16_t)(crc << 1);
        }
    }
    return crc;
}

void protocol_init(protocol_parser_t *parser) {
    if (parser != NULL) {
        memset(parser, 0, sizeof(*parser));
    }
}

size_t protocol_encode(uint8_t type, uint16_t sequence, const uint8_t *payload,
                       uint16_t length, uint8_t *output, size_t capacity) {
    size_t total = HEADER_SIZE + length + CRC_SIZE;
    if (output == NULL || length > BLE_MAX_PAYLOAD - HEADER_SIZE - CRC_SIZE ||
        capacity < total || (length > 0u && payload == NULL)) {
        return 0u;
    }
    output[0] = PREAMBLE_0;
    output[1] = PREAMBLE_1;
    output[2] = PROTOCOL_VERSION;
    output[3] = type;
    output[4] = (uint8_t)(sequence & 0xFFu);
    output[5] = (uint8_t)(sequence >> 8);
    output[6] = (uint8_t)(length & 0xFFu);
    output[7] = (uint8_t)(length >> 8);
    if (length > 0u) {
        memcpy(&output[HEADER_SIZE], payload, length);
    }
    uint16_t crc = protocol_crc16(&output[2], 6u + length);
    output[HEADER_SIZE + length] = (uint8_t)(crc & 0xFFu);
    output[HEADER_SIZE + length + 1u] = (uint8_t)(crc >> 8);
    return total;
}

static void parser_restart(protocol_parser_t *parser, uint8_t byte) {
    parser->length = 0u;
    parser->expected = 0u;
    parser->state = byte == PREAMBLE_0 ? 1u : 0u;
    if (parser->state == 1u) {
        parser->data[parser->length++] = byte;
    }
}

bool protocol_push(protocol_parser_t *parser, uint8_t byte, uint8_t *type,
                   uint16_t *sequence, const uint8_t **payload, uint16_t *length) {
    if (parser == NULL) {
        return false;
    }
    if (parser->state == 0u) {
        parser_restart(parser, byte);
        return false;
    }
    if (parser->state == 1u) {
        if (byte != PREAMBLE_1) {
            parser_restart(parser, byte);
            return false;
        }
        parser->data[parser->length++] = byte;
        parser->state = 2u;
        return false;
    }
    if (parser->length >= sizeof(parser->data)) {
        parser_restart(parser, byte);
        return false;
    }
    parser->data[parser->length++] = byte;
    if (parser->length == HEADER_SIZE) {
        if (parser->data[2] != PROTOCOL_VERSION) {
            protocol_init(parser);
            return false;
        }
        uint16_t body = (uint16_t)(parser->data[6] | ((uint16_t)parser->data[7] << 8));
        parser->expected = (uint16_t)(HEADER_SIZE + body + CRC_SIZE);
        if (parser->expected > sizeof(parser->data)) {
            protocol_init(parser);
            return false;
        }
    }
    if (parser->expected == 0u || parser->length < parser->expected) {
        return false;
    }
    uint16_t body_length = (uint16_t)(parser->data[6] | ((uint16_t)parser->data[7] << 8));
    uint16_t supplied_crc = (uint16_t)(parser->data[HEADER_SIZE + body_length] |
                           ((uint16_t)parser->data[HEADER_SIZE + body_length + 1u] << 8));
    uint16_t computed_crc = protocol_crc16(&parser->data[2], 6u + body_length);
    if (supplied_crc != computed_crc) {
        protocol_init(parser);
        return false;
    }
    if (type != NULL) *type = parser->data[3];
    if (sequence != NULL) *sequence = (uint16_t)(parser->data[4] |
                                                ((uint16_t)parser->data[5] << 8));
    if (payload != NULL) *payload = &parser->data[HEADER_SIZE];
    if (length != NULL) *length = body_length;
    /* Payload remains valid until the next byte is pushed. */
    parser->state = 0u;
    parser->length = 0u;
    parser->expected = 0u;
    return true;
}
