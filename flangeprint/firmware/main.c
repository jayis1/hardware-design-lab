/* FlangePrint scheduler, job state machine and verification
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */
#include "board.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static fp_calibration_t calibration;
static fp_job_t job;
static fp_geometry_frame_t geometry;
static fp_fit_t fit;
static fp_acoustic_frame_t acoustic;
static uint32_t next_capture_ms;
static uint32_t sensor_failures;

static void calibration_defaults(void)
{
    memset(&calibration, 0, sizeof(calibration));
    calibration.calibration_id = 2026092401u;
    for (size_t i = 0; i < FP_SECTORS; ++i) {
        calibration.scale[i] = 1.0f;
        for (size_t band = 0; band < FP_ACOUSTIC_BANDS; ++band)
            calibration.acoustic_baseline[i][band] = 1.0f + (float)band;
    }
    calibration.crc32 = fp_crc32((const uint8_t *)&calibration,
                                 offsetof(fp_calibration_t, crc32));
}

static void boot(void)
{
    fp_platform_init();
    fp_sensors_init();
    calibration_defaults();
    fp_job_init(&job);
    memset(&geometry, 0, sizeof(geometry));
    memset(&fit, 0, sizeof(fit));
    memset(&acoustic, 0, sizeof(acoustic));
    next_capture_ms = 0u;
    sensor_failures = 0u;
}

static void publish_status(void)
{
    uint8_t packet[FP_PACKET_MAX];
    size_t length = fp_encode_status(&job, &geometry, &fit, &acoustic,
                                     packet, sizeof(packet));
    if (length > 0u) fp_ble_notify(packet, length);
}

static void process_commands(void)
{
    uint8_t packet[FP_PACKET_MAX];
    int received = fp_ble_receive(packet, sizeof(packet));
    if (received <= 0) return;
    if (fp_handle_command(packet, (size_t)received, &job) != FP_OK) {
        ++job.fault_count;
        fp_set_status(true, false, false);
    }
}

static void finish_record(void)
{
    fp_job_record_t record;
    memset(&record, 0, sizeof(record));
    record.sequence = job.sequence;
    record.started_ms = job.started_ms;
    record.completed_ms = fp_millis();
    record.calibration_id = calibration.calibration_id;
    record.bolt_count = job.bolt_count;
    record.battery_mv = geometry.battery_mv;
    record.service_mode = job.service_mode;
    record.final_fit = fit;
    record.acoustic = acoustic;
    record.crc32 = fp_crc32((const uint8_t *)&record,
                            offsetof(fp_job_record_t, crc32));
    if (fp_record_save(&record) != FP_OK) {
        job.state = FP_JOB_FAULT;
        ++job.fault_count;
    } else job.state = FP_JOB_COMPLETE;
}

static void scheduler_step(void)
{
    uint32_t now = fp_millis();
    process_commands();
    if ((int32_t)(now - next_capture_ms) >= 0) {
        fp_result_t result = fp_capture_geometry(&geometry, &calibration);
        if (result == FP_OK && fp_fit_geometry(&geometry, &fit) == FP_OK) {
            sensor_failures = 0u;
            (void)fp_job_step(&job, &geometry, &fit);
        } else {
            ++sensor_failures;
            if (sensor_failures >= 3u) job.state = FP_JOB_FAULT;
        }
        next_capture_ms = now + FP_GEOMETRY_PERIOD_MS;
        publish_status();
    }
    if (job.state == FP_JOB_VERIFY) {
        bool active = job.service_mode != FP_SERVICE_FORBID_EXCITATION;
        fp_result_t result = fp_capture_acoustic(&acoustic, &calibration,
                                                 job.service_mode, active);
        if (result == FP_OK) finish_record();
        else if (result != FP_ERR_INTERLOCK) job.state = FP_JOB_FAULT;
    }
    bool warning = fit.tilt_magnitude_mm > FP_TILT_WARN_MM ||
                   fit.rms_residual_mm > FP_RESIDUAL_WARN_MM ||
                   acoustic.risk_score >= FP_LEAK_WARN_SCORE;
    fp_set_status(job.state == FP_JOB_FAULT,
                  job.state == FP_JOB_COMPLETE, warning);
    fp_platform_advance();
    fp_platform_sleep();
}

static bool close_enough(float actual, float expected, float tolerance)
{
    return fabsf(actual - expected) <= tolerance;
}

static int test_crc(void)
{
    static const uint8_t text[] = "123456789";
    if (fp_crc16(text, 9u) != 0x29B1u) return 1;
    if (fp_crc32(text, 9u) != 0xCBF43926u) return 1;
    return 0;
}

static int test_fit_level(void)
{
    fp_geometry_frame_t frame;
    fp_fit_t result;
    memset(&frame, 0, sizeof(frame));
    frame.valid_mask = 0xFFu;
    for (size_t i = 0; i < FP_SECTORS; ++i) frame.gap_mm[i] = 1.25f;
    if (fp_fit_geometry(&frame, &result) != FP_OK) return 1;
    if (!close_enough(result.mean_gap_mm, 1.25f, 0.001f)) return 1;
    if (result.tilt_magnitude_mm > 0.001f) return 1;
    if (result.rms_residual_mm > 0.001f) return 1;
    return 0;
}

static int test_fit_tilt(void)
{
    static const float cos8[FP_SECTORS] = {
        1.0f, 0.70710678f, 0.0f, -0.70710678f,
        -1.0f, -0.70710678f, 0.0f, 0.70710678f
    };
    static const float sin8[FP_SECTORS] = {
        0.0f, 0.70710678f, 1.0f, 0.70710678f,
        0.0f, -0.70710678f, -1.0f, -0.70710678f
    };
    fp_geometry_frame_t frame;
    fp_fit_t result;
    memset(&frame, 0, sizeof(frame));
    frame.valid_mask = 0xFFu;
    for (size_t i = 0; i < FP_SECTORS; ++i)
        frame.gap_mm[i] = 2.0f + 0.6f * cos8[i] - 0.2f * sin8[i];
    if (fp_fit_geometry(&frame, &result) != FP_OK) return 1;
    if (!close_enough(result.mean_gap_mm, 2.0f, 0.002f)) return 1;
    if (!close_enough(result.tilt_x_mm, 0.6f, 0.002f)) return 1;
    if (!close_enough(result.tilt_y_mm, -0.2f, 0.002f)) return 1;
    if (!result.recommendation_valid) return 1;
    return 0;
}

static int test_invalid_channels(void)
{
    fp_geometry_frame_t frame;
    fp_fit_t result;
    memset(&frame, 0, sizeof(frame));
    frame.valid_mask = 0x1Fu;
    if (fp_fit_geometry(&frame, &result) != FP_ERR_RANGE) return 1;
    frame.valid_mask = 0x3Fu;
    for (size_t i = 0; i < FP_SECTORS; ++i) frame.gap_mm[i] = 1.0f;
    if (fp_fit_geometry(&frame, &result) != FP_OK) return 1;
    return result.valid_count == 6u ? 0 : 1;
}

static int test_job(void)
{
    fp_job_t local;
    fp_geometry_frame_t frame;
    fp_fit_t result;
    fp_job_init(&local);
    if (fp_job_start(&local, FP_SERVICE_OFFLINE, 8u) != FP_OK) return 1;
    memset(&frame, 0, sizeof(frame));
    memset(&result, 0, sizeof(result));
    frame.valid_mask = 0xFFu;
    result.valid_count = 8u;
    result.valid_mask = 0xFFu;
    result.mean_gap_mm = 2.0f;
    if (fp_job_step(&local, &frame, &result) != FP_OK) return 1;
    if (local.state != FP_JOB_GEOMETRY) return 1;
    for (unsigned i = 0; i < 15u; ++i)
        if (fp_job_step(&local, &frame, &result) != FP_OK) return 1;
    if (local.state != FP_JOB_GUIDANCE) return 1;
    return 0;
}

static int test_storage(void)
{
    fp_job_record_t input;
    fp_job_record_t output;
    memset(&input, 0, sizeof(input));
    input.sequence = 77u;
    input.bolt_count = 12u;
    input.crc32 = fp_crc32((const uint8_t *)&input,
                           offsetof(fp_job_record_t, crc32));
    if (fp_record_save(&input) != FP_OK) return 1;
    if (!fp_record_get(0u, &output)) return 1;
    if (output.sequence != 77u || output.bolt_count != 12u) return 1;
    return 0;
}

static int test_protocol(void)
{
    uint8_t packet[FP_PACKET_MAX];
    fp_job_t local;
    fp_geometry_frame_t frame;
    fp_fit_t result;
    fp_acoustic_frame_t sound;
    memset(&frame, 0, sizeof(frame));
    memset(&result, 0, sizeof(result));
    memset(&sound, 0, sizeof(sound));
    fp_job_init(&local);
    size_t length = fp_encode_status(&local, &frame, &result, &sound,
                                     packet, sizeof(packet));
    if (length < 40u || packet[0] != 'F' || packet[1] != 'P') return 1;
    uint16_t stored = (uint16_t)packet[length - 2u] |
                      (uint16_t)((uint16_t)packet[length - 1u] << 8u);
    if (stored != fp_crc16(packet, length - 2u)) return 1;
    return 0;
}

static int self_test(void)
{
    boot();
    int failed = 0;
    failed += test_crc();
    failed += test_fit_level();
    failed += test_fit_tilt();
    failed += test_invalid_channels();
    failed += test_job();
    failed += test_storage();
    failed += test_protocol();
    if (failed == 0) printf("FlangePrint self-test: 7 suites passed\n");
    else fprintf(stderr, "FlangePrint self-test: %d suite(s) failed\n", failed);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int demo(void)
{
    boot();
    if (fp_job_start(&job, FP_SERVICE_OFFLINE, 8u) != FP_OK)
        return EXIT_FAILURE;
    for (unsigned i = 0; i < 80u && job.state != FP_JOB_FAULT; ++i)
        scheduler_step();
    printf("demo: state=%u valid=%u mean=%.3f tilt=%.3f direction=%.1f suggested=%u records=%zu\n",
           (unsigned)job.state, fit.valid_count, fit.mean_gap_mm,
           fit.tilt_magnitude_mm, fit.tilt_direction_deg,
           fit.suggested_sector, fp_record_count());
    return job.state == FP_JOB_FAULT ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--self-test") == 0) return self_test();
    if (argc > 1 && strcmp(argv[1], "--demo") == 0) return demo();
    boot();
#ifdef FP_HOST_SIM
    for (unsigned i = 0; i < 20u; ++i) scheduler_step();
    puts("FlangePrint host simulation ready; use --self-test or --demo");
    return EXIT_SUCCESS;
#else
    for (;;) scheduler_step();
#endif
}
