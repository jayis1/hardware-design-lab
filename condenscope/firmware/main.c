/* CondenScope application firmware
 * Author: jayis1
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "board.h"
#include "registers.h"
#include "drivers/sensors.h"
#include "drivers/protocol.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef enum {
    STATE_BOOT = 0,
    STATE_IDLE = 1,
    STATE_SCANNING = 2,
    STATE_CALIBRATING = 3,
    STATE_FAULT = 4,
    STATE_LOW_BATTERY = 5
} device_state_t;

typedef struct {
    uint16_t emissivity_q15;
    int16_t warning_margin_centic;
    int16_t danger_margin_centic;
    uint8_t alarm_enabled;
    uint8_t stream_thermal;
} device_config_t;

typedef struct {
    uint32_t started_ms;
    uint32_t samples;
    uint32_t danger_points;
    int16_t minimum_margin_centic;
    uint16_t coldest_pixel;
    bool active;
} scan_session_t;

static device_state_t state;
static device_config_t config;
static scan_session_t session;
static protocol_context_t protocol;
static environment_t environment;
static thermal_frame_t thermal;
static map_point_t points[MAP_MAX_CELLS];
static uint8_t tx_buffer[USB_FRAME_MTU];
static uint8_t rx_buffer[256];
static uint32_t next_environment_ms;
static uint32_t next_thermal_ms;
static uint32_t next_status_ms;
static uint32_t fault_code;

static bool time_due(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

static void apply_default_config(void)
{
    config.emissivity_q15 = EMISSIVITY_DEFAULT_Q15;
    config.warning_margin_centic = DEW_WARNING_CENTIC;
    config.danger_margin_centic = DEW_DANGER_CENTIC;
    config.alarm_enabled = 1U;
    config.stream_thermal = 1U;
}

static void indicate_state(void)
{
    switch (state) {
    case STATE_BOOT:
        board_led_set(0U, 0U, 24U);
        break;
    case STATE_IDLE:
        board_led_set(0U, 18U, 4U);
        board_buzzer_set(0U, 0U);
        break;
    case STATE_SCANNING:
        board_led_set(0U, 6U, 28U);
        break;
    case STATE_CALIBRATING:
        board_led_set(20U, 12U, 0U);
        break;
    case STATE_LOW_BATTERY:
        board_led_set(24U, 8U, 0U);
        break;
    case STATE_FAULT:
    default:
        board_led_set(30U, 0U, 0U);
        break;
    }
}

static void transmit(const uint8_t *data, uint16_t length)
{
    if (length == 0U) {
        return;
    }
    uint16_t sent = board_usb_write(data, length);
    if (sent == 0U && length <= BLE_FRAME_MTU) {
        (void)board_ble_write(data, length);
    }
}

static void send_ack(uint8_t command, uint8_t result)
{
    uint8_t payload[2] = {command, result};
    uint16_t length = protocol_encode(&protocol, CS_MSG_ACK, payload,
                                      sizeof(payload), tx_buffer,
                                      sizeof(tx_buffer));
    transmit(tx_buffer, length);
}

static void send_error(uint8_t command, uint16_t error)
{
    uint8_t payload[4];
    payload[0] = command;
    payload[1] = (uint8_t)state;
    payload[2] = (uint8_t)(error & 0xFFU);
    payload[3] = (uint8_t)(error >> 8U);
    uint16_t length = protocol_encode(&protocol, CS_MSG_ERROR, payload,
                                      sizeof(payload), tx_buffer,
                                      sizeof(tx_buffer));
    transmit(tx_buffer, length);
}

static void session_start(void)
{
    memset(&session, 0, sizeof(session));
    session.started_ms = board_millis();
    session.minimum_margin_centic = 32767;
    session.active = true;
    state = STATE_SCANNING;
    indicate_state();
}

static void session_send_summary(void)
{
    uint8_t payload[18];
    uint32_t duration = board_millis() - session.started_ms;
    payload[0] = (uint8_t)(duration & 0xFFU);
    payload[1] = (uint8_t)((duration >> 8U) & 0xFFU);
    payload[2] = (uint8_t)((duration >> 16U) & 0xFFU);
    payload[3] = (uint8_t)(duration >> 24U);
    payload[4] = (uint8_t)(session.samples & 0xFFU);
    payload[5] = (uint8_t)((session.samples >> 8U) & 0xFFU);
    payload[6] = (uint8_t)((session.samples >> 16U) & 0xFFU);
    payload[7] = (uint8_t)(session.samples >> 24U);
    payload[8] = (uint8_t)(session.danger_points & 0xFFU);
    payload[9] = (uint8_t)((session.danger_points >> 8U) & 0xFFU);
    payload[10] = (uint8_t)((session.danger_points >> 16U) & 0xFFU);
    payload[11] = (uint8_t)(session.danger_points >> 24U);
    payload[12] = (uint8_t)(session.minimum_margin_centic & 0xFF);
    payload[13] = (uint8_t)((uint16_t)session.minimum_margin_centic >> 8U);
    payload[14] = (uint8_t)(session.coldest_pixel & 0xFFU);
    payload[15] = (uint8_t)(session.coldest_pixel >> 8U);
    payload[16] = environment.battery_percent;
    payload[17] = 0U;
    uint16_t length = protocol_encode(&protocol, CS_MSG_SESSION_SUMMARY,
                                      payload, sizeof(payload), tx_buffer,
                                      sizeof(tx_buffer));
    transmit(tx_buffer, length);
}

static void session_stop(void)
{
    if (session.active) {
        session.active = false;
        session_send_summary();
    }
    state = environment.battery_percent < 10U ? STATE_LOW_BATTERY : STATE_IDLE;
    indicate_state();
}

static uint16_t payload_u16(const uint8_t *payload)
{
    return (uint16_t)((uint16_t)payload[0] | ((uint16_t)payload[1] << 8U));
}

static void handle_command(const protocol_message_t *message)
{
    switch (message->type) {
    case CS_CMD_START_SESSION:
        if (state == STATE_FAULT || environment.battery_percent < 5U) {
            send_error(message->type, 1U);
        } else {
            session_start();
            send_ack(message->type, 0U);
        }
        break;
    case CS_CMD_STOP_SESSION:
        session_stop();
        send_ack(message->type, 0U);
        break;
    case CS_CMD_SET_EMISSIVITY:
        if (message->length != 2U) {
            send_error(message->type, 2U);
        } else {
            uint16_t value = payload_u16(message->payload);
            if (value < 16384U || value > 32767U) {
                send_error(message->type, 3U);
            } else {
                config.emissivity_q15 = value;
                send_ack(message->type, 0U);
            }
        }
        break;
    case CS_CMD_SET_THRESHOLDS:
        if (message->length != 4U) {
            send_error(message->type, 2U);
        } else {
            int16_t warning = (int16_t)payload_u16(message->payload);
            int16_t danger = (int16_t)payload_u16(&message->payload[2]);
            if (danger > warning || warning > 1000 || danger < -500) {
                send_error(message->type, 3U);
            } else {
                config.warning_margin_centic = warning;
                config.danger_margin_centic = danger;
                send_ack(message->type, 0U);
            }
        }
        break;
    case CS_CMD_CALIBRATE:
        if (message->length != 2U) {
            send_error(message->type, 2U);
        } else {
            device_state_t previous = state;
            state = STATE_CALIBRATING;
            indicate_state();
            bool ok = sensors_calibrate_ambient(payload_u16(message->payload));
            state = previous;
            indicate_state();
            if (ok) { send_ack(message->type, 0U); }
            else { send_error(message->type, 4U); }
        }
        break;
    case CS_CMD_ERASE:
        /* Session RAM has no retained user-identifying data. FRAM erase is
         * delegated to the storage task in production builds. */
        memset(&session, 0, sizeof(session));
        send_ack(message->type, 0U);
        break;
    default:
        send_error(message->type, 0xFFU);
        break;
    }
}

static void service_transport(void)
{
    uint16_t count = board_transport_read(rx_buffer, sizeof(rx_buffer));
    if (count == 0U) {
        return;
    }
    protocol_message_t message;
    protocol_result_t result = protocol_consume(&protocol, rx_buffer, count,
                                                 &message);
    if (result == PROTOCOL_MESSAGE) {
        handle_command(&message);
        /* The decoder points into its receive buffer. Consume exactly one
         * command per read; reset prevents stale bytes from being replayed. */
        protocol.used = 0U;
        protocol.expected = 0U;
    } else if (result < 0) {
        send_error(0U, (uint16_t)(-result));
    }
}

static void update_environment(uint32_t now)
{
    if (!time_due(now, next_environment_ms)) {
        return;
    }
    next_environment_ms = now + 1000U;
    sensor_result_t result = sensors_read_environment(&environment);
    if (result != SENSOR_OK && result != SENSOR_E_TIMEOUT) {
        fault_code = (uint32_t)(-result);
        state = STATE_FAULT;
        indicate_state();
        return;
    }
    if (environment.battery_percent < 5U && state == STATE_SCANNING) {
        session_stop();
    } else if (environment.battery_percent < 10U && state == STATE_IDLE) {
        state = STATE_LOW_BATTERY;
        indicate_state();
    } else if (environment.battery_percent >= 12U && state == STATE_LOW_BATTERY) {
        state = STATE_IDLE;
        indicate_state();
    }
}

static void alarm_for_points(const map_point_t *map, uint16_t count)
{
    if (config.alarm_enabled == 0U) {
        board_buzzer_set(0U, 0U);
        return;
    }
    uint16_t danger = 0U;
    uint16_t warning = 0U;
    for (uint16_t i = 0; i < count; ++i) {
        if (map[i].margin_centic <= config.danger_margin_centic) {
            ++danger;
        } else if (map[i].margin_centic <= config.warning_margin_centic) {
            ++warning;
        }
    }
    if (danger != 0U) {
        board_led_set(30U, 0U, 0U);
        board_buzzer_set(1800U, 20U);
    } else if (warning != 0U) {
        board_led_set(28U, 12U, 0U);
        board_buzzer_set(900U, 5U);
    } else {
        board_led_set(0U, 8U, 28U);
        board_buzzer_set(0U, 0U);
    }
}

static void update_session_statistics(const map_point_t *map, uint16_t count)
{
    ++session.samples;
    for (uint16_t i = 0; i < count; ++i) {
        if (map[i].margin_centic < session.minimum_margin_centic) {
            session.minimum_margin_centic = map[i].margin_centic;
            session.coldest_pixel = map[i].pixel;
        }
        if (map[i].risk == 2U) {
            ++session.danger_points;
        }
    }
}

static void update_thermal(uint32_t now)
{
    if (state != STATE_SCANNING || !time_due(now, next_thermal_ms)) {
        return;
    }
    next_thermal_ms = now + (1000U / SENSOR_SAMPLE_HZ);
    sensor_result_t result = sensors_read_thermal(&thermal,
                                                  config.emissivity_q15);
    if (result == SENSOR_E_TIMEOUT) {
        return;
    }
    if (result != SENSOR_OK) {
        fault_code = 0x100U | (uint32_t)(-result);
        state = STATE_FAULT;
        indicate_state();
        return;
    }
    if (!thermal.complete) {
        return;
    }
    uint16_t point_count = 0U;
    sensors_generate_points(&thermal, &environment, points,
                            ARRAY_LEN(points), &point_count);
    update_session_statistics(points, point_count);
    alarm_for_points(points, point_count);

    if (config.stream_thermal != 0U) {
        uint16_t length = protocol_send_thermal(&protocol, &thermal,
                                                tx_buffer, sizeof(tx_buffer));
        transmit(tx_buffer, length);
    }
    for (uint16_t i = 0; i < point_count; ++i) {
        if (points[i].risk != 0U) {
            uint16_t length = protocol_send_point(&protocol, &points[i],
                                                  tx_buffer, sizeof(tx_buffer));
            transmit(tx_buffer, length);
        }
    }
}

static void update_status(uint32_t now)
{
    if (!time_due(now, next_status_ms)) {
        return;
    }
    next_status_ms = now + 1000U;
    uint16_t length = protocol_send_status(&protocol, &environment,
                                           (uint8_t)state, tx_buffer,
                                           sizeof(tx_buffer));
    transmit(tx_buffer, length);
    if (state == STATE_FAULT) {
        send_error(0U, (uint16_t)fault_code);
    }
}

static void process_trigger(void)
{
    static bool previous;
    static uint32_t changed_ms;
    bool pressed = board_trigger_pressed();
    uint32_t now = board_millis();
    if (pressed != previous && time_due(now, changed_ms + 25U)) {
        changed_ms = now;
        previous = pressed;
        if (pressed) {
            if (state == STATE_SCANNING) {
                session_stop();
            } else if (state == STATE_IDLE || state == STATE_LOW_BATTERY) {
                if (environment.battery_percent >= 5U) {
                    session_start();
                }
            }
        }
    }
}

int main(void)
{
    state = STATE_BOOT;
    apply_default_config();
    protocol_init(&protocol);
    memset(&environment, 0, sizeof(environment));
    memset(&thermal, 0, sizeof(thermal));

    board_clock_init();
    board_gpio_init();
    indicate_state();
    board_i2c_init();
    board_adc_init();
    board_usb_init();
    board_ble_uart_init();
    board_watchdog_init();

    sensor_result_t init_result = sensors_init();
    if (init_result != SENSOR_OK) {
        fault_code = (uint32_t)(-init_result);
        state = STATE_FAULT;
    } else {
        state = STATE_IDLE;
    }
    indicate_state();

    uint32_t now = board_millis();
    next_environment_ms = now;
    next_thermal_ms = now;
    next_status_ms = now + 250U;

    for (;;) {
        now = board_millis();
        service_transport();
        process_trigger();
        update_environment(now);
        update_thermal(now);
        update_status(now);
        board_watchdog_feed();
        if (state == STATE_IDLE && !time_due(now + 5U, next_environment_ms)) {
            board_enter_stop2();
        }
    }
}
