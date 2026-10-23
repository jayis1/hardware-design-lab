/* SPDX-License-Identifier: MIT
 * QuadraAlign framed BLE telemetry and command parser
 * Author: jayis1
 */
#include "protocol.h"
#include "board.h"
#include "registers.h"
#include <stdio.h>
#include <string.h>

#define RX_CAPACITY 256U
#define COMMAND_QUEUE 8U

typedef enum { WAIT_SOF, READ_TYPE, READ_LENGTH, READ_PAYLOAD, READ_CRC_LO, READ_CRC_HI } parser_state_t;
typedef struct { parser_state_t state; uint8_t type; uint8_t length; uint8_t index; uint8_t payload[BLE_MAX_PAYLOAD]; uint16_t received_crc; } parser_t;

static parser_t parser;
static qa_command_t queue[COMMAND_QUEUE];
static uint8_t queue_read;
static uint8_t queue_write;

static void put_u16(uint8_t *out, uint16_t value) { out[0] = (uint8_t)value; out[1] = (uint8_t)(value >> 8U); }
static void put_u32(uint8_t *out, uint32_t value) {
    out[0] = (uint8_t)value; out[1] = (uint8_t)(value >> 8U); out[2] = (uint8_t)(value >> 16U); out[3] = (uint8_t)(value >> 24U);
}
static void put_float(uint8_t *out, float value) { memcpy(out, &value, sizeof value); }
static float get_float(const uint8_t *in) { float value; memcpy(&value, in, sizeof value); return value; }

uint16_t protocol_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFFU;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8U;
        for (uint8_t bit = 0; bit < 8U; ++bit) crc = (crc & 0x8000U) ? (uint16_t)((crc << 1U) ^ 0x1021U) : (uint16_t)(crc << 1U);
    }
    return crc;
}

static bool transmit(uint8_t type, const uint8_t *payload, uint8_t length) {
    if (length > BLE_MAX_PAYLOAD) return false;
    uint8_t frame[BLE_MAX_PAYLOAD + 5U];
    frame[0] = BLE_SOF; frame[1] = type; frame[2] = length;
    if (length) memcpy(&frame[3], payload, length);
    uint16_t crc = protocol_crc16(&frame[1], (size_t)length + 2U);
    frame[3U + length] = (uint8_t)crc;
    frame[4U + length] = (uint8_t)(crc >> 8U);
    return board_uart_write(frame, (size_t)length + 5U) == 0;
}

void protocol_init(void) {
    memset(&parser, 0, sizeof parser);
    parser.state = WAIT_SOF;
    queue_read = queue_write = 0U;
}

static void enqueue(const qa_command_t *command) {
    uint8_t next = (uint8_t)((queue_write + 1U) % COMMAND_QUEUE);
    if (next == queue_read) return;
    queue[queue_write] = *command;
    queue_write = next;
}

static void decode_command(void) {
    if (parser.type != BLE_TYPE_COMMAND || parser.length < 1U) return;
    qa_command_t command = { .kind = EVENT_NONE };
    switch (parser.payload[0]) {
        case BLE_CMD_TARE: command.kind = EVENT_TARE; break;
        case BLE_CMD_CAPTURE: command.kind = EVENT_CAPTURE; break;
        case BLE_CMD_SLEEP: command.kind = EVENT_SLEEP; break;
        case BLE_CMD_SET_LIMITS:
            if (parser.length != 9U) return;
            command.kind = EVENT_SET_LIMITS;
            command.value_a = get_float(&parser.payload[1]);
            command.value_b = get_float(&parser.payload[5]);
            break;
        case BLE_CMD_CALIBRATE:
            if (parser.length != 6U) return;
            command.kind = EVENT_CALIBRATE;
            command.channel = parser.payload[1];
            command.value_a = get_float(&parser.payload[2]);
            break;
        default: return;
    }
    enqueue(&command);
}

static void consume(uint8_t byte) {
    switch (parser.state) {
        case WAIT_SOF:
            if (byte == BLE_SOF) parser.state = READ_TYPE;
            break;
        case READ_TYPE: parser.type = byte; parser.state = READ_LENGTH; break;
        case READ_LENGTH:
            parser.length = byte; parser.index = 0U;
            parser.state = byte == 0U ? READ_CRC_LO : (byte <= BLE_MAX_PAYLOAD ? READ_PAYLOAD : WAIT_SOF);
            break;
        case READ_PAYLOAD:
            parser.payload[parser.index++] = byte;
            if (parser.index >= parser.length) parser.state = READ_CRC_LO;
            break;
        case READ_CRC_LO: parser.received_crc = byte; parser.state = READ_CRC_HI; break;
        case READ_CRC_HI: {
            parser.received_crc |= (uint16_t)byte << 8U;
            uint8_t work[BLE_MAX_PAYLOAD + 2U];
            work[0] = parser.type; work[1] = parser.length;
            memcpy(&work[2], parser.payload, parser.length);
            if (protocol_crc16(work, (size_t)parser.length + 2U) == parser.received_crc) decode_command();
            parser.state = WAIT_SOF;
            break;
        }
        default: parser.state = WAIT_SOF; break;
    }
}

void protocol_poll(void) {
    uint8_t bytes[RX_CAPACITY];
    int count = board_uart_read(bytes, sizeof bytes);
    if (count <= 0) return;
    for (int i = 0; i < count; ++i) consume(bytes[i]);
}

bool protocol_next_command(qa_command_t *command) {
    if (!command || queue_read == queue_write) return false;
    *command = queue[queue_read];
    queue_read = (uint8_t)((queue_read + 1U) % COMMAND_QUEUE);
    return true;
}

bool protocol_send_measurement(const qa_measurement_t *m, uint16_t battery_mv) {
    if (!m) return false;
    uint8_t payload[80];
    uint8_t at = 0U;
    put_u32(&payload[at], m->sequence); at += 4U;
    for (uint8_t i = 0; i < QA_NUM_FEET; ++i) { put_float(&payload[at], m->foot_kg[i]); at += 4U; }
    put_float(&payload[at], m->total_kg); at += 4U;
    put_float(&payload[at], m->imbalance_percent); at += 4U;
    put_float(&payload[at], m->center_x_mm); at += 4U;
    put_float(&payload[at], m->center_y_mm); at += 4U;
    put_float(&payload[at], m->pitch_deg); at += 4U;
    put_float(&payload[at], m->roll_deg); at += 4U;
    put_float(&payload[at], m->vibration_rms_g); at += 4U;
    payload[at++] = m->recommended_foot;
    put_float(&payload[at], m->recommended_turns); at += 4U;
    put_u16(&payload[at], battery_mv); at += 2U;
    payload[at++] = (uint8_t)((m->stable ? 1U : 0U) | (m->overloaded ? 2U : 0U));
    return transmit(BLE_TYPE_TELEMETRY, payload, at);
}

bool protocol_send_event(uint8_t code, const char *message) {
    uint8_t payload[BLE_MAX_PAYLOAD];
    payload[0] = code;
    size_t length = message ? strlen(message) : 0U;
    if (length > BLE_MAX_PAYLOAD - 1U) length = BLE_MAX_PAYLOAD - 1U;
    if (length) memcpy(&payload[1], message, length);
    return transmit(BLE_TYPE_EVENT, payload, (uint8_t)(length + 1U));
}
