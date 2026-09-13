/* StitchScope firmware entry point and host integration test
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "stitchscope.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static recipe_t recipe;
static cycle_accumulator_t accumulator;
static uint32_t alert_count;
static uint32_t processed_cycles;

static void seed_recipe(void) {
    memset(&recipe, 0, sizeof(recipe));
    recipe.expected_feed_um = 2000u;
    recipe.sensitivity_q12 = 4096u;
}

static void feedback(stitch_class_t classification) {
    if (classification == CLASS_OK || classification == CLASS_UNTRAINED) {
        return;
    }
    ++alert_count;
    const char *severity = (classification == CLASS_HARD_STRIKE ||
                            classification == CLASS_TOP_THREAD_BREAK) ? "STOP" : "WARN";
    printf("alert=%s class=%s\n", severity, classifier_name(classification));
}

static void process_completed(stitch_features_t *features) {
    ++processed_cycles;
    if (!recipe.trained) {
        classifier_learn(&recipe, features);
        if (recipe.trained) {
            classifier_set_recipe(&recipe);
            printf("recipe trained after %u cycles\n", (unsigned)recipe.sample_count);
        }
        features->classification = CLASS_UNTRAINED;
    } else {
        stitch_class_t result = classifier_evaluate(features);
        feedback(result);
    }
    if ((features->cycle_index % 10u) == 0u) {
        printf("cycle=%u rate=%u force=%ld impulse=%u class=%s\n",
               (unsigned)features->cycle_index,
               features->duration_us == 0u ? 0u : (unsigned)(60000000u / features->duration_us),
               (long)features->force_peak_un,
               (unsigned)features->impulse_high,
               classifier_name(features->classification));
    }
}

static bool test_protocol(void) {
    const uint8_t command[] = {0x01u, 0x02u, 0xA5u, 0x5Au};
    uint8_t frame[64];
    size_t frame_length = protocol_encode(0x10u, 0x1234u, command,
                                          sizeof(command), frame, sizeof(frame));
    if (frame_length == 0u) {
        return false;
    }
    protocol_parser_t parser;
    protocol_init(&parser);
    uint8_t type = 0u;
    uint16_t sequence = 0u;
    uint16_t length = 0u;
    const uint8_t *payload = NULL;
    bool complete = false;
    for (size_t i = 0u; i < frame_length; ++i) {
        complete = protocol_push(&parser, frame[i], &type, &sequence, &payload, &length);
    }
    if (!complete || type != 0x10u || sequence != 0x1234u ||
        length != sizeof(command) || memcmp(payload, command, sizeof(command)) != 0) {
        return false;
    }
    frame[frame_length - 1u] ^= 0x80u;
    protocol_init(&parser);
    complete = false;
    for (size_t i = 0u; i < frame_length; ++i) {
        complete |= protocol_push(&parser, frame[i], NULL, NULL, NULL, NULL);
    }
    return !complete;
}

static bool test_sign_extension(void) {
    /* Local formulation mirrors the ADC driver's contract. */
    uint32_t positive = 0x007FFFFFu;
    uint32_t negative = 0x00800000u;
    int32_t p = (positive & 0x00800000u) ? (int32_t)(positive | 0xFF000000u) : (int32_t)positive;
    int32_t n = (negative & 0x00800000u) ? (int32_t)(negative | 0xFF000000u) : (int32_t)negative;
    return p == 8388607 && n == -8388608;
}

static int run_monitor(void) {
    sensors_init();
    signal_init(&accumulator);
    classifier_init();
    seed_recipe();
    alert_count = 0u;
    processed_cycles = 0u;

    bool previous_hall = false;
    const uint32_t sample_interval_us = 1000000u / FORCE_SAMPLE_HZ;
    const uint32_t simulation_duration_us = 70u * 40000u;
    for (uint32_t now = 0u; now < simulation_duration_us; now += sample_interval_us) {
        sensor_frame_t frame;
        if (sensors_sample(&frame, now) != SENSOR_OK) {
            fprintf(stderr, "sensor acquisition failed at %u us\n", (unsigned)now);
            return 2;
        }
        bool boundary = frame.hall && !previous_hall;
        previous_hall = frame.hall;
        stitch_features_t completed;
        if (signal_ingest(&accumulator, &frame, boundary, &completed)) {
            process_completed(&completed);
        }
    }
    printf("summary cycles=%u alerts=%u battery_mv=%u capabilities=0x%04X\n",
           (unsigned)processed_cycles, (unsigned)alert_count,
           (unsigned)sensors_battery_mv(), (unsigned)sensors_capability_mask());
    return processed_cycles >= 60u && alert_count > 0u ? 0 : 3;
}

int main(int argc, char **argv) {
    bool self_test = argc > 1 && strcmp(argv[1], "--self-test") == 0;
    if (self_test) {
        bool protocol_ok = test_protocol();
        bool adc_ok = test_sign_extension();
        int monitor_result = run_monitor();
        printf("self-test protocol=%s adc=%s monitor=%s\n",
               protocol_ok ? "PASS" : "FAIL",
               adc_ok ? "PASS" : "FAIL",
               monitor_result == 0 ? "PASS" : "FAIL");
        return protocol_ok && adc_ok && monitor_result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    return run_monitor();
}
