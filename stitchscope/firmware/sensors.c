/* StitchScope sensor abstraction and deterministic host model
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "stitchscope.h"
#include "registers.h"
#include <limits.h>
#include <string.h>

static uint32_t rng_state = 0x51C0A11Du;
static uint16_t capability_mask;
static uint16_t battery_mv;
static int32_t force_offset;
static uint32_t sample_number;

static uint32_t xorshift32(void) {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static int32_t noise(int32_t amplitude) {
    uint32_t span = (uint32_t)(amplitude * 2 + 1);
    return (int32_t)(xorshift32() % span) - amplitude;
}

static int32_t triangle(uint32_t phase, uint32_t period, int32_t amplitude) {
    uint32_t half = period / 2u;
    uint32_t p = phase % period;
    int32_t rising = (p < half) ? (int32_t)p : (int32_t)(period - p);
    return (rising * amplitude * 2) / (int32_t)half - amplitude;
}

static int32_t pulse(uint32_t phase, uint32_t center, uint32_t width, int32_t height) {
    uint32_t distance = phase > center ? phase - center : center - phase;
    if (distance >= width) {
        return 0;
    }
    return (int32_t)(((uint64_t)(width - distance) * (uint32_t)height) / width);
}

void sensors_init(void) {
    capability_mask = 0x001Fu;
    battery_mv = 4075u;
    force_offset = 1200;
    sample_number = 0u;
    rng_state = 0x51C0A11Du;
}

uint16_t sensors_capability_mask(void) {
    return capability_mask;
}

uint16_t sensors_battery_mv(void) {
    return battery_mv;
}

sensor_status_t sensors_sample(sensor_frame_t *frame, uint32_t now_us) {
    if (frame == NULL) {
        return SENSOR_RANGE_ERROR;
    }
    memset(frame, 0, sizeof(*frame));
    frame->timestamp_us = now_us;

#ifdef STITCHSCOPE_TARGET_NRF5340
    /* Target integration replaces this body with EasyDMA transfers. The public
     * contract remains identical, which keeps signal processing host-testable. */
    (void)force_offset;
    return SENSOR_NOT_READY;
#else
    const uint32_t cycle_us = 40000u; /* 1500 stitches/minute. */
    const uint32_t phase = now_us % cycle_us;
    int32_t tension = 230000 + triangle(phase, cycle_us, 75000);
    tension += pulse(phase, 24500u, 4000u, 380000);

    /* Deliberately introduce realistic seeded faults for simulator validation. */
    uint32_t cycle = now_us / cycle_us;
    if (cycle >= 35u && cycle < 39u) {
        tension = 9000 + noise(2500); /* escaped/broken upper thread */
    }
    if (cycle >= 55u && cycle < 60u) {
        tension += pulse(phase, 27000u, 9000u, 650000); /* growing loop */
    }
    frame->force_un = force_offset + tension + noise(5000);

    frame->accel_mg[0] = (int16_t)noise(18);
    frame->accel_mg[1] = (int16_t)noise(22);
    frame->accel_mg[2] = (int16_t)(1000 + noise(15));
    int32_t impact = pulse(phase, 10500u, 900u, 2900);
    if (cycle == 47u) {
        impact += pulse(phase, 10500u, 500u, 12000);
    }
    if (impact > INT16_MAX) {
        impact = INT16_MAX;
    }
    frame->accel_mg[2] = (int16_t)(frame->accel_mg[2] + impact);

    uint32_t phase_q16 = (uint32_t)(((uint64_t)phase << 16) / cycle_us);
    frame->mag_ut[0] = (int16_t)(triangle(phase_q16, 65536u, 780) + noise(5));
    frame->mag_ut[1] = (int16_t)(triangle((phase_q16 + 16384u) & 65535u,
                                          65536u, 760) + noise(5));
    frame->mag_ut[2] = (int16_t)(210 + noise(4));
    frame->hall = phase < 1000u;

    uint16_t base_range = (uint16_t)(68u + (cycle % 7u));
    for (size_t i = 0; i < 9u; ++i) {
        uint32_t movement = (cycle >= 42u && cycle < 46u) ? 0u : (cycle * 2u);
        frame->tof_mm[i] = (uint16_t)(base_range + i + (movement % 13u));
        frame->tof_confidence[i] = (uint8_t)(185u + (xorshift32() % 55u));
    }
    frame->temperature_centi_c = (int16_t)(2340 + noise(8));
    frame->humidity_centi_pct = (uint16_t)(4720 + noise(15));

    ++sample_number;
    if ((sample_number % (FORCE_SAMPLE_HZ * 60u)) == 0u && battery_mv > 3400u) {
        --battery_mv;
    }
    return SENSOR_OK;
#endif
}
