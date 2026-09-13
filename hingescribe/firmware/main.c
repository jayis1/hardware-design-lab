/*
 * HingeScribe firmware entry point, scheduler and self-tests
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "hingescribe.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static hs_calibration_t calibration;
static hs_analyzer_t analyzer;
static hs_history_t history;
static hs_sample_t latest_sample;
static hs_event_t latest_event;
static uint32_t next_notify_ms;
static uint32_t sensor_faults;

static void defaults(void)
{
    memset(&calibration, 0, sizeof(calibration));
    calibration.closed_zero_deg = 0.0f;
    calibration.load_zero_counts = 120000.0f;
    calibration.load_counts_per_n = 8420.0f;
    calibration.sag_mm_per_deg = 0.72f;
}

static void boot(void)
{
    platform_init();
    sensor_init();
    if (!calibration_load(&calibration)) defaults();
    analyzer_init(&analyzer, &calibration);
    history_init(&history);
    memset(&latest_sample, 0, sizeof(latest_sample));
    memset(&latest_event, 0, sizeof(latest_event));
    next_notify_ms = 0u;
    sensor_faults = 0u;
}

static void publish_status(void)
{
    uint8_t packet[HS_PACKET_MAX];
    size_t length = protocol_encode_status(&latest_sample, &latest_event, packet, sizeof(packet));
    if (length > 0u) platform_ble_notify(packet, length);
}

static void process_commands(void)
{
    uint8_t packet[HS_PACKET_MAX];
    int length = platform_ble_receive(packet, sizeof(packet));
    if (length <= 0) return;
    hs_result_t result = protocol_handle_command(packet, (size_t)length, &calibration, &history);
    platform_set_led(result != HS_OK);
}

static void scheduler_step(void)
{
    hs_result_t result = sensor_read(&latest_sample, &calibration);
    if (result != HS_OK) {
        ++sensor_faults;
        latest_event.flags |= HS_FLAG_SENSOR_FAULT;
    } else {
        sensor_faults = 0u;
    }
    if (analyzer_update(&analyzer, &latest_sample, &latest_event)) {
        ++calibration.event_count;
        history_append(&history, &latest_event);
        if ((calibration.event_count % 64u) == 0u) (void)calibration_save(&calibration);
        publish_status();
    }
    if ((int32_t)(latest_sample.timestamp_ms - next_notify_ms) >= 0) {
        publish_status();
        next_notify_ms = latest_sample.timestamp_ms + 1000u;
    }
    process_commands();
    platform_set_led(sensor_faults >= 3u || latest_event.health_score < 60u);
    platform_sim_advance();
    platform_sleep();
}

static bool nearly(float a, float b, float tolerance)
{
    return fabsf(a - b) <= tolerance;
}

static int test_crc(void)
{
    static const uint8_t vector[] = "123456789";
    if (hs_crc16(vector, 9u) != 0x29B1u) {
        fprintf(stderr, "crc16 vector failed\n");
        return 1;
    }
    if (hs_crc32(vector, 9u) != 0xCBF43926u) {
        fprintf(stderr, "crc32 vector failed\n");
        return 1;
    }
    return 0;
}

static int test_history(void)
{
    hs_history_t test;
    history_init(&test);
    for (unsigned i = 0; i < HS_HISTORY_RECORDS + 5u; ++i) {
        hs_event_t event;
        memset(&event, 0, sizeof(event));
        event.sequence = i;
        event.health_score = (uint16_t)(100u - i % 100u);
        history_append(&test, &event);
    }
    if (test.count != HS_HISTORY_RECORDS) return 1;
    hs_event_t newest;
    if (!history_get(&test, 0u, &newest)) return 1;
    if (newest.sequence != HS_HISTORY_RECORDS + 4u) return 1;
    if (history_get(&test, HS_HISTORY_RECORDS, &newest)) return 1;
    return 0;
}

static int test_sensor(void)
{
    hs_sample_t sample;
    defaults();
    sensor_init();
    if (sensor_read(&sample, &calibration) != HS_OK) return 1;
    if (!sample.magnet_valid || !sample.load_valid) return 1;
    if (sample.battery_mv < BATTERY_MIN_MV) return 1;
    if (!isfinite(sample.temperature_c)) return 1;
    return 0;
}

static int test_analyzer(void)
{
    hs_analyzer_t test;
    defaults();
    analyzer_init(&test, &calibration);
    hs_event_t completed;
    hs_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    sample.magnet_valid = true;
    sample.load_valid = true;
    sample.battery_mv = 3900u;
    sample.timestamp_ms = 0u;
    if (analyzer_update(&test, &sample, &completed)) return 1;
    sample.timestamp_ms = 200u;
    sample.angle_deg = 15.0f;
    sample.angular_velocity_dps = 75.0f;
    sample.force_n = 20.0f;
    if (analyzer_update(&test, &sample, &completed)) return 1;
    sample.timestamp_ms = 600u;
    sample.angle_deg = 80.0f;
    sample.angular_velocity_dps = 30.0f;
    sample.force_n = 25.0f;
    if (analyzer_update(&test, &sample, &completed)) return 1;
    sample.timestamp_ms = 1500u;
    sample.angle_deg = 1.0f;
    sample.angular_velocity_dps = -80.0f;
    sample.force_n = 18.0f;
    (void)analyzer_update(&test, &sample, &completed);
    sample.timestamp_ms = 1800u;
    sample.angular_velocity_dps = 0.0f;
    if (!analyzer_update(&test, &sample, &completed)) return 1;
    if (completed.duration_ms != 1600u) return 1;
    if (completed.health_score > 100u) return 1;
    return 0;
}

static int test_protocol(void)
{
    hs_sample_t sample;
    hs_event_t event;
    uint8_t packet[HS_PACKET_MAX];
    memset(&sample, 0, sizeof(sample));
    memset(&event, 0, sizeof(event));
    sample.angle_deg = 42.25f;
    sample.force_n = 31.5f;
    sample.magnet_valid = true;
    sample.load_valid = true;
    event.health_score = 87u;
    size_t length = protocol_encode_status(&sample, &event, packet, sizeof(packet));
    if (length != 34u) return 1;
    if (packet[0] != 0x53u || packet[1] != 0x48u) return 1;
    if (packet[length - 1u] != 0x0Au) return 1;
    return 0;
}

static int test_calibration(void)
{
    hs_calibration_t saved;
    hs_calibration_t loaded;
    defaults();
    saved = calibration;
    if (!calibration_save(&saved)) return 1;
    if (!calibration_load(&loaded)) return 1;
    if (!nearly(loaded.load_counts_per_n, 8420.0f, 0.1f)) return 1;
    loaded.crc32 ^= 1u;
    return 0;
}

static int self_test(void)
{
    int failures = 0;
    boot();
    failures += test_crc();
    failures += test_history();
    failures += test_sensor();
    failures += test_analyzer();
    failures += test_protocol();
    failures += test_calibration();
    if (failures == 0) printf("HingeScribe self-test: 6 suites passed\n");
    else fprintf(stderr, "HingeScribe self-test: %d suite(s) failed\n", failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--self-test") == 0) return self_test();
    boot();
#ifdef HS_HOST_SIM
    for (unsigned i = 0; i < 1200u; ++i) scheduler_step();
    printf("simulation complete: %u records, last score %u\n", history.count, latest_event.health_score);
    return EXIT_SUCCESS;
#else
    for (;;) scheduler_step();
#endif
}
