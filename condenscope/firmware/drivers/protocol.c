/* CondenScope framed USB/BLE protocol
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "protocol.h"
#include "../registers.h"
#include <stddef.h>
#include <string.h>

#define HEADER_SIZE 8U
#define CRC_SIZE 2U
#define FRAME_OVERHEAD (HEADER_SIZE + CRC_SIZE)

static void put_u16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)(value >> 8U);
}

static void put_u32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)((value >> 8U) & 0xFFU);
    out[2] = (uint8_t)((value >> 16U) & 0xFFU);
    out[3] = (uint8_t)(value >> 24U);
}

static uint16_t get_u16(const uint8_t *in)
{
    return (uint16_t)((uint16_t)in[0] | ((uint16_t)in[1] << 8U));
}

uint16_t protocol_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFU;
    for (uint16_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            crc = (crc & 1U) != 0U ? (uint16_t)((crc >> 1U) ^ 0xA001U)
                                   : (uint16_t)(crc >> 1U);
        }
    }
    return crc;
}

void protocol_init(protocol_context_t *context)
{
    if (context != NULL) {
        memset(context, 0, sizeof(*context));
    }
}

static bool seek_magic(protocol_context_t *context)
{
    uint16_t offset = 0U;
    while (offset + 1U < context->used) {
        if (context->buffer[offset] == (uint8_t)(CS_FRAME_MAGIC & 0xFFU) &&
            context->buffer[offset + 1U] == (uint8_t)(CS_FRAME_MAGIC >> 8U)) {
            break;
        }
        ++offset;
    }
    if (offset != 0U) {
        memmove(context->buffer, &context->buffer[offset], context->used - offset);
        context->used = (uint16_t)(context->used - offset);
    }
    return context->used >= 2U;
}

protocol_result_t protocol_consume(protocol_context_t *context,
                                   const uint8_t *bytes, uint16_t length,
                                   protocol_message_t *message)
{
    if (context == NULL || bytes == NULL || message == NULL) {
        return PROTOCOL_BAD_FRAME;
    }
    if ((uint32_t)context->used + length > sizeof(context->buffer)) {
        context->used = 0U;
        context->expected = 0U;
        ++context->frame_errors;
        return PROTOCOL_OVERFLOW;
    }
    memcpy(&context->buffer[context->used], bytes, length);
    context->used = (uint16_t)(context->used + length);
    if (!seek_magic(context) || context->used < HEADER_SIZE) {
        return PROTOCOL_NONE;
    }
    if (context->buffer[2] != CS_PROTOCOL_VERSION) {
        memmove(context->buffer, &context->buffer[2], context->used - 2U);
        context->used = (uint16_t)(context->used - 2U);
        ++context->frame_errors;
        return PROTOCOL_BAD_FRAME;
    }
    uint16_t payload_length = get_u16(&context->buffer[6]);
    if ((uint32_t)payload_length + FRAME_OVERHEAD > sizeof(context->buffer)) {
        context->used = 0U;
        ++context->frame_errors;
        return PROTOCOL_OVERFLOW;
    }
    context->expected = (uint16_t)(payload_length + FRAME_OVERHEAD);
    if (context->used < context->expected) {
        return PROTOCOL_NONE;
    }
    uint16_t received_crc = get_u16(&context->buffer[HEADER_SIZE + payload_length]);
    uint16_t calculated_crc = protocol_crc16(context->buffer,
                                              (uint16_t)(HEADER_SIZE + payload_length));
    if (received_crc != calculated_crc) {
        memmove(context->buffer, &context->buffer[2], context->used - 2U);
        context->used = (uint16_t)(context->used - 2U);
        context->expected = 0U;
        ++context->crc_errors;
        return PROTOCOL_BAD_FRAME;
    }
    message->type = context->buffer[3];
    message->sequence = get_u16(&context->buffer[4]);
    message->length = payload_length;
    message->payload = &context->buffer[HEADER_SIZE];
    context->rx_sequence = message->sequence;
    return PROTOCOL_MESSAGE;
}

uint16_t protocol_encode(protocol_context_t *context, uint8_t type,
                         const void *payload, uint16_t payload_length,
                         uint8_t *output, uint16_t capacity)
{
    if (context == NULL || output == NULL ||
        (payload_length != 0U && payload == NULL) ||
        (uint32_t)payload_length + FRAME_OVERHEAD > capacity) {
        return 0U;
    }
    put_u16(output, CS_FRAME_MAGIC);
    output[2] = CS_PROTOCOL_VERSION;
    output[3] = type;
    put_u16(&output[4], context->tx_sequence++);
    put_u16(&output[6], payload_length);
    if (payload_length != 0U) {
        memcpy(&output[HEADER_SIZE], payload, payload_length);
    }
    uint16_t crc = protocol_crc16(output, (uint16_t)(HEADER_SIZE + payload_length));
    put_u16(&output[HEADER_SIZE + payload_length], crc);
    return (uint16_t)(payload_length + FRAME_OVERHEAD);
}

uint16_t protocol_send_status(protocol_context_t *context,
                              const environment_t *environment,
                              uint8_t state, uint8_t *output,
                              uint16_t capacity)
{
    if (environment == NULL) {
        return 0U;
    }
    uint8_t payload[16];
    put_u16(&payload[0], (uint16_t)environment->air_temperature_centic);
    put_u16(&payload[2], environment->relative_humidity_centi_percent);
    put_u16(&payload[4], (uint16_t)environment->dew_point_centic);
    put_u16(&payload[6], environment->distance_mm);
    put_u16(&payload[8], environment->battery_mv);
    payload[10] = environment->battery_percent;
    payload[11] = state;
    put_u32(&payload[12], environment->timestamp_ms);
    return protocol_encode(context, CS_MSG_STATUS, payload, sizeof(payload),
                           output, capacity);
}

uint16_t protocol_send_thermal(protocol_context_t *context,
                               const thermal_frame_t *frame,
                               uint8_t *output, uint16_t capacity)
{
    if (frame == NULL) {
        return 0U;
    }
    /* Delta encode centi-degree samples around ambient to fit one USB packet
     * in ordinary indoor conditions. Values outside int8 are escaped. */
    uint8_t payload[USB_FRAME_MTU - FRAME_OVERHEAD];
    uint16_t cursor = 0U;
    put_u32(&payload[cursor], frame->sequence); cursor += 4U;
    put_u16(&payload[cursor], (uint16_t)frame->ambient_centic); cursor += 2U;
    put_u16(&payload[cursor], (uint16_t)frame->min_centic); cursor += 2U;
    put_u16(&payload[cursor], (uint16_t)frame->max_centic); cursor += 2U;
    payload[cursor++] = frame->subpage;
    payload[cursor++] = frame->complete ? 1U : 0U;
    for (uint16_t i = 0; i < THERMAL_PIXELS; ++i) {
        int16_t delta = (int16_t)((frame->pixels_centic[i] -
                                  frame->ambient_centic) / 10);
        if (delta >= -127 && delta <= 127) {
            if (cursor >= sizeof(payload)) { return 0U; }
            payload[cursor++] = (uint8_t)(int8_t)delta;
        } else {
            if ((uint32_t)cursor + 3U > sizeof(payload)) { return 0U; }
            payload[cursor++] = 0x80U;
            put_u16(&payload[cursor], (uint16_t)frame->pixels_centic[i]);
            cursor += 2U;
        }
    }
    return protocol_encode(context, CS_MSG_THERMAL_FRAME, payload, cursor,
                           output, capacity);
}

uint16_t protocol_send_point(protocol_context_t *context,
                             const map_point_t *point,
                             uint8_t *output, uint16_t capacity)
{
    if (point == NULL) {
        return 0U;
    }
    uint8_t payload[15];
    put_u16(&payload[0], point->pixel);
    put_u16(&payload[2], (uint16_t)point->surface_centic);
    put_u16(&payload[4], (uint16_t)point->margin_centic);
    payload[6] = point->risk;
    put_u16(&payload[7], point->distance_mm);
    put_u16(&payload[9], (uint16_t)point->bearing_cdeg);
    put_u16(&payload[11], (uint16_t)point->elevation_cdeg);
    put_u16(&payload[13], 0U);
    return protocol_encode(context, CS_MSG_MAP_POINT, payload, sizeof(payload),
                           output, capacity);
}
